#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "platform.h"

KENSHIN_BEGIN

struct GPUDevice;

struct PoolSize
{
	VkDescriptorType type;
	float			 ratio;
};

class DescriptorSetAllocator
{
public:
	DescriptorSetAllocator() = default;
	~DescriptorSetAllocator() = default;
	void init(GPUDevice* device);
	void destroyPools();
	void resetPools();
	bool allocateDescriptorSet(VkDescriptorSet* outSet, VkDescriptorSetLayout layout);
private:
	VkDescriptorPool getPool();
	VkDescriptorPool createPool(u32 poolCount, VkDescriptorPoolCreateFlags flags = 0);
private:
	std::vector<PoolSize>		  mPoolSizes;
	VkDescriptorPool			  mCurrentPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorPool> mUsedPools;
	std::vector<VkDescriptorPool> mFreePools;
	GPUDevice*					  mDevice{ nullptr };
};

KENSHIN_END