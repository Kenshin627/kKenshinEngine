#include "pch.h"
#include "memory.h"
#include "tlsf.h"

KENSHIN_BEGIN

void memoryCopy(void* dest, void* src, sizet size)
{
	memcpy(dest, src, size);
};

sizet memoryAlign(sizet size, sizet alignment)
{
	sizet alignMask = alignment - 1;
	return (size + alignMask) & ~alignMask;
};

//Heap Allocator///////////////////////////////////////////////////////////////////////////////////////////

void HeapAllocator::init(sizet s)
{
	mMemory = malloc(s);
	mTotalSize = s;
	mAllocatedSize = 0;
	mTlsfHandle = tlsf_create_with_pool(mMemory, s);
}

void HeapAllocator::shutdown()
{
	MemoryStatistics stats{ 0,  mTotalSize };
	void* pool = tlsf_get_pool(mTlsfHandle);
	tlsf_walk_pool(pool, [](void* ptr, size_t size, int used, void* user)
		{
			MemoryStatistics* stats = reinterpret_cast<MemoryStatistics*>(user);
			if (used)
			{
				stats->add(size);
				KS_CORE_WARN("Memory leak detected: {} bytes at {}", size, ptr);
			}
		}, (void*)& stats);
	if (stats.allocatedBytes)
	{
		KS_CORE_ERROR("HeapAllocator shutdown with {} bytes leaked in {} allocations!", stats.allocatedBytes, stats.allocatedCount);
	}
	else
	{
		KS_CORE_INFO("HeapAllocator shutdown cleanly with no memory leaks.");
	}

	KS_CORE_ASSERT(mAllocatedSize == 0, "Memory leak detected in HeapAllocator!");

	tlsf_destroy(mTlsfHandle);
	free(mMemory);
}

void* HeapAllocator::allocate(sizet size, sizet alignment)
{
	void* p = alignment <= 1 ? tlsf_malloc(mTlsfHandle, size) : tlsf_memalign(mTlsfHandle, alignment, size);
	KS_CORE_ASSERT(p, "HeapAllocator out of memory!");
	sizet blockSize = tlsf_block_size(p);
	mAllocatedSize += blockSize;
	return p;
}

void HeapAllocator::deallocate(void* p)
{
	KS_CORE_ASSERT(p, "Trying to deallocate a null pointer in HeapAllocator!");
	sizet blockSize = tlsf_block_size(p);
	mAllocatedSize -= blockSize;
	tlsf_free(mTlsfHandle, p);
}

//Linear Allocator///////////////////////////////////////////////////////////////////////////////////////////

void LinearAllocator::init(sizet s)
{
	mMemory = reinterpret_cast<u8*>(malloc(s));
	mTotalSize = s;
	mOffset = 0;
}

void LinearAllocator::shutdown()
{
	free(mMemory);
	mMemory = nullptr;
	mTotalSize = 0;
	mOffset = 0;
}

void* LinearAllocator::allocate(sizet size, sizet alignment)
{
	sizet alignedAllocatedSize = memoryAlign(mOffset, alignment);
	KS_CORE_ASSERT(alignedAllocatedSize < mTotalSize, "LinearAllocator, out of memory!");
	sizet newOffset = alignedAllocatedSize + size;
	KS_CORE_ASSERT(newOffset <= mTotalSize, "LinearAllocator, out of memory!");
	mOffset = newOffset;
	return mMemory + alignedAllocatedSize;
}

void LinearAllocator::deallocate(void* p)
{
	KS_CORE_ASSERT(false, "LinearAllocator does not support deallocation of individual pointers!");
}

void LinearAllocator::clear()
{
	mOffset = 0;
}

//Stack Allocator///////////////////////////////////////////////////////////////////////////////////////////

void StackAllocator::init(sizet s)
{
	mMemory = reinterpret_cast<u8*>(malloc(s));
	mTotalSize = s;
	mAllocatedSize = 0;
}

void StackAllocator::shutdown()
{
	if (mMemory)
	{
		free(mMemory);
		mMemory = nullptr;
		mTotalSize = 0;
		mAllocatedSize = 0;
	}
}

void* StackAllocator::allocate(sizet size, sizet alignment)
{
	sizet blockSize = memoryAlign(mAllocatedSize, alignment);
	KS_CORE_ASSERT(blockSize < mTotalSize, "StackAllocator out of memory!");
	sizet newSize = blockSize + size;
	KS_CORE_ASSERT(newSize <= mTotalSize, "StackAllocator out of memory!");
	mAllocatedSize = newSize;
	return mMemory + blockSize;
}

void StackAllocator::deallocate(void* p)
{
	KS_CORE_ASSERT(p, "p is nullptr!");
	const sizet ptrOffset = reinterpret_cast<u8*>(p) - mMemory;
	mAllocatedSize = ptrOffset;
}

void StackAllocator::clear()
{
	mAllocatedSize = 0;
}

sizet StackAllocator::getMarker()
{
	return mAllocatedSize;
}

void StackAllocator::freeToMarker(sizet marker)
{
	sizet diff = marker - mAllocatedSize;
	if (diff > 0)
	{
		mAllocatedSize = marker;
	}
}

//Memory Service///////////////////////////////////////////////////////////////////////////////////////////

static MemoryService sMemoryService;

static sizet sMemoryServiceMemorySize = kmega(32) + tlsf_size() + 8;

bool MemoryService::init(void* memoryServiceConfiguration)
{	
	KS_CORE_INFO("Initializing Memory Allocator Service.");
	sizet memorySize = sMemoryServiceMemorySize;
	if (memoryServiceConfiguration)
	{
		auto* config = reinterpret_cast<MemoryServiceConfiguration*>(memoryServiceConfiguration);
		if (config && config->maximum_dynamic_size)
		{
			memorySize = config->maximum_dynamic_size;
		}
	}
	mSystemAllocator.init(memorySize);
	return true;
	
	return false;
}

void MemoryService::shutdown()
{
	KS_CORE_INFO("Shutting down Memory Allocator Service.");
	mSystemAllocator.shutdown();
}

MemoryService* MemoryService::instance()
{
	return &sMemoryService;
}
KENSHIN_END