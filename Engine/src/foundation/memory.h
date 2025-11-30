#pragma once
#include "service.h"

KENSHIN_BEGIN

void memoryCopy(void* dest, void* src, sizet size);

sizet memoryAlign(sizet size, sizet alignment);

struct MemoryStatistics
{
	sizet allocatedBytes = 0;
	sizet totalBytes = 0;
	sizet allocatedCount = 0;
	void add(sizet a)
	{
		if (a)
		{
			allocatedBytes += a;
			++allocatedCount;
		}
	}
};

class Allocator
{
public:
	Allocator() {};
	virtual ~Allocator() {}
	virtual void init(sizet s) = 0;
	virtual void shutdown() = 0;
	virtual void* allocate(sizet size, sizet alignment) = 0;
	virtual void deallocate(void* p) = 0;
};

class HeapAllocator : public Allocator
{
public:
	virtual void init(sizet s) override;
	virtual void shutdown() override;
	virtual void* allocate(sizet size, sizet alignment) override;
	virtual void deallocate(void* p) override;
private:
	void* mTlsfHandle{ nullptr };
	void* mMemory{ nullptr };
	sizet mTotalSize{ 0 };
	sizet mAllocatedSize{ 0 };
};

class LinearAllocator : public Allocator
{
	public:
	virtual void init(sizet s) override;
	virtual void shutdown() override;
	virtual void* allocate(sizet size, sizet alignment) override;
	virtual void deallocate(void* p) override;
	void clear();
private:
	u8*   mMemory		{ nullptr };
	sizet mTotalSize	{ 0		  };
	sizet mOffset		{ 0		  };
};

class StackAllocator : public Allocator
{
	public:
	virtual void init(sizet s) override;
	virtual void shutdown() override;
	virtual void* allocate(sizet size, sizet alignment) override;
	virtual void deallocate(void* p) override;
	void clear();
	sizet getMarker();
	void freeToMarker(sizet marker);
private:
	u8*   mMemory			{ nullptr };
	sizet mTotalSize		{ 0		  };
	sizet mAllocatedSize	{ 0		  };
};

//memory Allocator Macro /////////////////////////
#define kalloca(size, allocator) ((allocator)->allocate(size, 1))
#define kallocm(size, allocator) ((u8*)(allocator)->allocate(size, 1))
#define kalloct(type, allocator) ((type*)(allocator)->allocate(sizeof(type), 1))
#define kallocaa(size, alignment, allocator) ((allocator)->allocate(size, alignment))
#define kfree(ptr, allocator) ((allocator)->deallocate(ptr))

#define kkilo(size) (size * 1024)
#define kmega(size) (size * 1024 * 1024)
#define kgiga(size) (size * 1024 * 1024 * 1024)
//MemoryService////////////////////////////////////////////////////////////////////////////////////
struct MemoryServiceConfiguration {

	sizet  maximum_dynamic_size = kmega(32);

}; // struct MemoryServiceConfiguration

class MemoryService : public Service
{
public:
	virtual bool init(void* memoryServiceConfiguration = nullptr) override;
	virtual void shutdown() override;
	KS_SERVICE_TYPE(MemoryService)
private:
	HeapAllocator mSystemAllocator;
	LinearAllocator mScratchAllocator;
};
KENSHIN_END