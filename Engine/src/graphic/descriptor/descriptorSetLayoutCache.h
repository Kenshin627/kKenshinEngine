#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "platform.h"

KENSHIN_BEGIN

struct GPUDevice;

class DescriptorSetLayoutCache
{
public:
	DescriptorSetLayoutCache() = default;
	~DescriptorSetLayoutCache() = default;
	void shutDown();
	void init(GPUDevice* device);
	VkDescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& creation);
public:
	struct DescriptorSetLayoutCreateInfo
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bool operator==(const DescriptorSetLayoutCreateInfo& other) const;
		sizet hash() const;
	};
private:
	struct DescriptorSetLayoutCreateInfoHash
	{
		sizet operator()(const DescriptorSetLayoutCreateInfo& k) const
		{
			return k.hash();
		}
	};
private:
	GPUDevice* mDevice{ nullptr };
	std::unordered_map<DescriptorSetLayoutCreateInfo, VkDescriptorSetLayout, DescriptorSetLayoutCreateInfoHash> mSetLayoutCache;
};

KENSHIN_END