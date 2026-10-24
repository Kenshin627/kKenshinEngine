#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "platform.h"
#include "materialSystem/shaderModule.h"
#include "gpuResource.h"

KENSHIN_BEGIN
struct GPUDevice;
GRAHPIC_UTILS_BEGIN

struct  ReflectionOverrides
{
	cstring descriptorName;
	VkDescriptorType overrideType;
};

struct DescriptorSetLayoutData
{
	u32								          setNumber;
	VkDescriptorSetLayoutCreateInfo           createInfo;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

PipelineHandle reflectDescriptorSetLayout(GPUDevice* device, const std::vector<Kenshin::ShaderModule*>& shaderModule, const std::vector<ReflectionOverrides>& ovrrides);

GRAPHIC_UTILS_END
KENSHIN_END