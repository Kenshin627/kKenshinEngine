#include "pch.h"
#include "resourcePool.h"

namespace Kenshin
{
	void ResourcePool::init(Allocator* allocator, u32 poolSize, u32 resourceSize)
	{
		mAllocator = allocator;
		mPoolSize = static_cast<u32>(poolSize);
		mResourceSize = static_cast<u32>(resourceSize);
		mUsedIndices = 0;
		mFreeIndicesHead = 0;
		sizet allocatedSize = mPoolSize * (mResourceSize + sizeof(u32));
		mMemory = static_cast<u8*>(mAllocator->allocate(allocatedSize, 1));
		memset(mMemory, 0, allocatedSize);
		mFreeIndices = reinterpret_cast<u32*>(mMemory + mPoolSize * mResourceSize);
		for (size_t i = 0; i < mPoolSize; i++)
		{
			mFreeIndices[i] = i;
		}
	}

	void ResourcePool::shutdown()
	{
		mPoolSize = 0;
		mResourceSize = 0;
		mUsedIndices = 0;
		mFreeIndicesHead = 0;
		mFreeIndices = nullptr;
		if (mMemory)
		{
			mAllocator->deallocate(mMemory);
			mMemory = nullptr;
		}
	}

	u32 ResourcePool::obtainResource()
	{
		if (mFreeIndicesHead < mPoolSize)
		{
			++mUsedIndices;
			return mFreeIndices[mFreeIndicesHead++];
		}
		else
		{
			KS_CORE_ASSERT(false, "ResourcePool out of resources!");
			return InvalidIndex;
		}
	}

	void ResourcePool::releaseResource(u32 resourceHandle)
	{
		KS_CORE_ASSERT(resourceHandle < mPoolSize, "Resource handle out of bounds!");
		mFreeIndices[--mFreeIndicesHead] = resourceHandle;
		--mUsedIndices;
	}

	void ResourcePool::releaseAllResources()
	{
		mUsedIndices = 0;
		mFreeIndicesHead = 0;
		for (size_t i = 0; i < mPoolSize; i++)
		{
			mFreeIndices[i] = i;
		}
	}

	void* ResourcePool::accessResource(u32 resourceHandle)
	{
		KS_CORE_ASSERT(resourceHandle < mPoolSize, "Resource handle out of bounds!");
		return &mMemory[resourceHandle * mResourceSize];
	}

	const void* ResourcePool::accessResource(u32 resourceHandle) const
	{
		if (resourceHandle == InvalidIndex)
		{
			return nullptr;
		}
		KS_CORE_ASSERT(resourceHandle < mPoolSize, "Resource handle out of bounds!");
		return &mMemory[resourceHandle * mResourceSize];
	}
}
