#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "platform.h"

KENSHIN_BEGIN

struct GPUDevice;
class DescriptorSetLayoutCache;
class DescriptorSetAllocator;

class DescriptorBuilder
{
public:
	DescriptorBuilder() = default;
	~DescriptorBuilder() = default;
	static DescriptorBuilder begin(GPUDevice* device, DescriptorSetLayoutCache* cache, DescriptorSetAllocator* allocator);
	DescriptorBuilder& bindImage(u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkDescriptorImageInfo* imageInfo);
	DescriptorBuilder& bindBuffer(u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkDescriptorBufferInfo* bufferInfo);
	bool build(VkDescriptorSet& set, VkDescriptorSetLayout& setlayout);
	bool build(VkDescriptorSet& set);
private:
	GPUDevice*								  mDevice;
	std::vector<VkWriteDescriptorSet>		  mWrites;
	std::vector<VkDescriptorSetLayoutBinding> mBindings;
	DescriptorSetLayoutCache*				  mCache;
	DescriptorSetAllocator*					  mAllocator;
};

KENSHIN_END