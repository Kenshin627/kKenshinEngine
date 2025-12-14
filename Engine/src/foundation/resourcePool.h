#pragma once
#include <platform.h>
#include <memory.h>
//#include 

namespace Kenshin
{
	class ResourcePool
	{
	public:
		ResourcePool() = default;
		virtual ~ResourcePool() = default;
		void init(Allocator* allocator, u32 poolSize, u32 resourceSize);
		virtual void shutDown();
		u32 obtainResource();
		void releaseResource(u32 resourceHandle);
		void releaseAllResources();
		void* accessResource(u32  resourceHandle);
		const void* accessResource(u32 resourceHandle) const;
	protected:
		Allocator* mAllocator{ nullptr };
		u8*		   mMemory{ nullptr };
		u32		   mFreeIndicesHead{ 0 };
		u32		   mPoolSize{ 16 };
		u32		   mResourceSize{ 4 };
		u32*	   mFreeIndices{ nullptr };
		u32		   mUsedIndices{ 0 };
	};

	template<typename T>
	class ResourcePoolTyped : public ResourcePool
	{
	public:
		ResourcePoolTyped() = default;
		~ResourcePoolTyped() = default;
		void init(Allocator* allocator, u32 poolSize);
		virtual void shutDown() override;
		T* obtain();
		void release(T* resource);
		T* get(u32 resourceHandle);
		const T* get(u32 resourceHandle) const;
	};

	template<typename T>
	inline void ResourcePoolTyped<T>::init(Allocator* allocator, u32 poolSize)
	{
		ResourcePool::init(allocator, poolSize, sizeof(T));
	}

	template<typename T>
	inline void ResourcePoolTyped<T>::shutDown()
	{
		ResourcePool::shutDown();
	}

	template<typename T>
	inline T* ResourcePoolTyped<T>::obtain()
	{
		const u32 index = ResourcePool::obtainResource();
		if (index != InvalidIndex)
		{
			void* data = ResourcePool::accessResource(index);
			if (!data)
			{
				KS_CORE_ASSERT(false, "ResourcePoolTyped obtained null resource!");
				return nullptr;
			}	
			T* resource = get(index);
			resource->poolIndex = index;
			return resource;
		}
		KS_CORE_ASSERT(false, "ResourcePoolTyped out of resources!");
		return nullptr;
	}

	template<typename T>
	inline void ResourcePoolTyped<T>::release(T* resource)
	{
		if (!resource)
		{
			KS_CORE_ASSERT(false, "resource ptr is null!");
			return;
		}
		u32 resourceHandle = resource->poolIndex;
		ResourcePool::releaseResource(resourceHandle);
	}

	template<typename T>
	inline T* ResourcePoolTyped<T>::get(u32 resourceHandle)
	{
		KS_CORE_ASSERT(resourceHandle < mPoolSize, "Resource handle out of bounds!");
		void* resource = ResourcePool::accessResource(resourceHandle);
		return reinterpret_cast<T*>(resource);
	}

	template<typename T>
	inline const T* ResourcePoolTyped<T>::get(u32 resourceHandle) const
	{
		KS_CORE_ASSERT(resourceHandle < mPoolSize, "Resource handle out of bounds!");
		void* resource = ResourcePool::accessResource(resourceHandle);
		return reinterpret_cast<T*>(resource);
	}
}