#include "pch.h"
#include "descriptorSetAllocator.h"
#include "gpuDevice.h"

KENSHIN_BEGIN

void DescriptorSetAllocator::init(GPUDevice* device)
{
	mDevice = device;
	mPoolSizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
	};
}

void DescriptorSetAllocator::destroyPools()
{
	for (VkDescriptorPool pool : mFreePools)
	{
		vkDestroyDescriptorPool(mDevice->getDevice(), pool, mDevice->getAllocCallbacks());
	}
	mFreePools.clear();
	for (VkDescriptorPool pool : mUsedPools)
	{
		vkDestroyDescriptorPool(mDevice->getDevice(), pool, mDevice->getAllocCallbacks());
	}
	mUsedPools.clear();
}

void DescriptorSetAllocator::resetPools()
{
	for (VkDescriptorPool pool : mUsedPools)
	{
		vkResetDescriptorPool(mDevice->getDevice(), pool, 0);
		mFreePools.push_back(pool);
	}
	mUsedPools.clear();
	mCurrentPool = VK_NULL_HANDLE;
}

bool DescriptorSetAllocator::allocateDescriptorSet(VkDescriptorSet* outSet, VkDescriptorSetLayout layout)
{
	if (mCurrentPool == VK_NULL_HANDLE)
	{
		mCurrentPool = getPool();
		mUsedPools.push_back(mCurrentPool);
	}
	VkDescriptorSetAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr };
	allocInfo.descriptorPool = mCurrentPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkResult result = vkAllocateDescriptorSets(mDevice->getDevice(), &allocInfo, outSet);
	switch (result)
	{
	case VK_SUCCESS:
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		mCurrentPool = getPool();
		mUsedPools.push_back(mCurrentPool);
		allocInfo.descriptorPool = mCurrentPool;
		result = vkAllocateDescriptorSets(mDevice->getDevice(), &allocInfo, outSet);
		if (result == VK_SUCCESS)
		{
			return true;
		}
		else
		{
			KS_CORE_ASSERT(false, "Failed to allocate descriptor set after getting new pool!");
			return false;
		}
	default:
		KS_CORE_ASSERT(false, "Failed to allocate descriptor set!");
		return false;
	}
}

VkDescriptorPool DescriptorSetAllocator::getPool()
{
	if (mFreePools.size() > 0)
	{
		VkDescriptorPool pool = mFreePools.back();
		mFreePools.pop_back();
		return pool;
	}
	else
	{
		return createPool(1000, 0);
	}
}

VkDescriptorPool DescriptorSetAllocator::createPool(u32 poolCount, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(mPoolSizes.size());
	for (auto& poolSize : mPoolSizes)
	{
		VkDescriptorPoolSize size{ .type = poolSize.type };
		size.descriptorCount = static_cast<u32>(poolSize.ratio * poolCount);
		sizes.push_back(size);
	}
	VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr,.flags = flags };
	poolInfo.maxSets = poolCount;
	poolInfo.poolSizeCount = static_cast<u32>(sizes.size());
	poolInfo.pPoolSizes = sizes.data();

	VkDescriptorPool pool = VK_NULL_HANDLE;
	VK_CHECK(vkCreateDescriptorPool(mDevice->getDevice(), &poolInfo, mDevice->getAllocCallbacks(), &pool));	
	return pool;
}

KENSHIN_END
