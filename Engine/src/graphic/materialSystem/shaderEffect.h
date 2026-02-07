#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include<unordered_map>
#include "platform.h"

KENSHIN_BEGIN

struct ShaderModule;
struct GPUDevice;

class ShaderEffect
{

public:
	struct  ReflectionOverrides
	{
		cstring descriptorName;
		VkDescriptorType overrideType;
	};

	struct ReflectedBinding
	{
		u32 set;
		u32 binding;
		VkDescriptorType type;
	};
	struct DescriptorSetLayoutData
	{
		u32								          setNumber;
		VkDescriptorSetLayoutCreateInfo           createInfo;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};
public:
	ShaderEffect() = default;
	~ShaderEffect() = default;
	void addShaderStage(ShaderModule* shader);
	void reflectDescriptorSetLayout(GPUDevice* device, const std::vector<ReflectionOverrides>& ovrrides);
	void fillStage(std::vector<VkPipelineShaderStageCreateInfo>& outStageCreateInfo);
private:
	VkPipelineLayout							  mPipelineLayout;
	std::array<VkDescriptorSetLayout, 4>    	  mDescriptorSetLayout;
	std::vector<ShaderModule*>					  mShaderModules;
	std::unordered_map<cstring, ReflectedBinding> mReflectedBindings;
};

KENSHIN_END