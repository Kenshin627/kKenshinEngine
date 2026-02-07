#include "pch.h"
#include <spirv_reflect.h>
#include"shaderModule.h"
#include "shaderEffect.h"
#include "gpuDevice.h"

KENSHIN_BEGIN

void ShaderEffect::addShaderStage(ShaderModule* shader)
{
	if (shader)
	{
		mShaderModules.push_back(shader);
	}
}

void ShaderEffect::reflectDescriptorSetLayout(GPUDevice* device, const std::vector<ReflectionOverrides>& ovrrides)
{
	std::vector<DescriptorSetLayoutData> setLayouts;
	std::vector<VkPushConstantRange> pushConstantRanges;
	SpvReflectResult result = SpvReflectResult::SPV_REFLECT_RESULT_SUCCESS;
	for (auto& s : mShaderModules)
	{
		SpvReflectShaderModule module;
		result = spvReflectCreateShaderModule(s->code.size() * sizeof(u32), s->code.data(), &module);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			KS_CORE_ASSERT(false, "Shader reflection failed to create shader module for shader stage");
			continue;
		}
		
		//descriptorSetLayouts
		u32 currentStageSetCount = 0;
		result = spvReflectEnumerateDescriptorSets(&module, &currentStageSetCount, nullptr);
		if (result)
		{
			KS_CORE_ASSERT(false, "Shader reflection failed to enumerate descriptor sets for shader stage");
			continue;
		}
		std::vector<SpvReflectDescriptorSet*> sets(currentStageSetCount);
		sets.resize(currentStageSetCount);
		result = spvReflectEnumerateDescriptorSets(&module, &currentStageSetCount, sets.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			KS_CORE_ASSERT(false, "Shader reflection failed to enumerate descriptor sets for shader stage");
			continue;
		}
		for (u32 i = 0; i < currentStageSetCount; ++i)
			{
				SpvReflectDescriptorSet& reflectSet = *(sets[i]);
				u32 bindingCount = reflectSet.binding_count;

				DescriptorSetLayoutData layout{};
				layout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layout.createInfo.pNext = nullptr;
				layout.createInfo.bindingCount = bindingCount;
				layout.createInfo.flags = 0;
				layout.setNumber = reflectSet.set;	
				layout.bindings.resize(bindingCount);
				for (u32 j = 0; j < bindingCount; ++j)
				{
					VkDescriptorSetLayoutBinding& layoutBinding = layout.bindings[j];
					SpvReflectDescriptorBinding* reflectBinding = reflectSet.bindings[j];
					layoutBinding.binding = reflectBinding->binding;
					layoutBinding.descriptorCount =  1;
					layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectBinding->descriptor_type);
					layoutBinding.stageFlags = 0;					
					layoutBinding.pImmutableSamplers = nullptr;
					for (auto& override : ovrrides)
					{
						if (strcmp(reflectBinding->name, override.descriptorName) == 0)
						{
							layoutBinding.descriptorType = override.overrideType;
							break;
						}
					}

					for (u32 k = 0; k < reflectBinding->array.dims_count; ++k)
					{
						layoutBinding.descriptorCount *= reflectBinding->array.dims[k];
					}
					mReflectedBindings[reflectBinding->name] = { layout.setNumber, layoutBinding.binding, layoutBinding.descriptorType };					
				}
				layout.createInfo.pBindings = layout.bindings.data();
				setLayouts.push_back(layout);
			}

		//pushConstantRanges
		u32 pushConstantCount = 0;
		result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			KS_CORE_ASSERT(false, "Shader reflection failed to enumerate push constant blocks for shader stage");
			continue;
		}
		std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
		result = spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			KS_CORE_ASSERT(false, "Shader reflection failed to enumerate push constant blocks for shader stage");
			continue;
		}
		for (u32 i = 0; i < pushConstantCount; ++i)
		{
			SpvReflectBlockVariable& pushConstant = *(pushConstants[i]);
			VkPushConstantRange range{};
			range.stageFlags = s->stage;
			range.offset = pushConstant.offset;
			range.size = pushConstant.size;
			pushConstantRanges.push_back(range);
		}
	}

	//combine reflected set layouts with same set number, and create Vulkan descriptor set layouts.
	std::array<DescriptorSetLayoutData, 4> combinedSetLayouts;
	for (u8 i = 0; i < 4; ++i)
	{
		DescriptorSetLayoutData& combinedLayout = combinedSetLayouts[i];
		std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindingMap;
		for (auto& layout : setLayouts)
		{
			if (layout.setNumber == i) 
			{
				for (auto& binding : layout.bindings)
				{
					if (bindingMap.find(binding.binding) != bindingMap.cend())
					{
						bindingMap[binding.binding].stageFlags |= binding.stageFlags;
					}
					else
					{
						bindingMap[binding.binding] = binding;
					}
				}
			}
		}
		combinedLayout.bindings.reserve(bindingMap.size());
		for (auto& [k, v] : bindingMap)
		{
			combinedLayout.bindings.push_back(v);
		}
		combinedLayout.setNumber = i;
		combinedLayout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		combinedLayout.createInfo.pNext = nullptr;
		combinedLayout.createInfo.bindingCount = bindingMap.size();
		combinedLayout.createInfo.flags = 0;
		combinedLayout.createInfo.pBindings = combinedLayout.bindings.data();

		if (combinedLayout.bindings.size() > 0)
		{
			VkDescriptorSetLayout vkLayout;
			VK_CHECK(vkCreateDescriptorSetLayout(device->getDevice(), &combinedLayout.createInfo, device->getAllocCallbacks(), &vkLayout));
			mDescriptorSetLayout[i] = vkLayout;
		}
		else
		{
			mDescriptorSetLayout[i] = VK_NULL_HANDLE;
		}
	}

	//compacted descriptorSetLayouts
	std::vector<VkDescriptorSetLayout> compactedSetLayouts;
	u8 descriptorSetlayoutCounter = 0;
	for (u8 i = 0; i < 4; ++i)
	{
		if (mDescriptorSetLayout[i])
		{
			compactedSetLayouts.push_back(mDescriptorSetLayout[descriptorSetlayoutCounter++]);
		}
	}

	VkPipelineLayoutCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  .pNext = nullptr, .flags = 0 };
	pipelineInfo.setLayoutCount = 0;
	pipelineInfo.pSetLayouts = nullptr;
	pipelineInfo.pushConstantRangeCount = 0;
	pipelineInfo.pPushConstantRanges = nullptr;

	pipelineInfo.pushConstantRangeCount = static_cast<u32>(pushConstantRanges.size());
	pipelineInfo.pPushConstantRanges = pushConstantRanges.data();
	pipelineInfo.setLayoutCount = static_cast<u32>(compactedSetLayouts.size());
	pipelineInfo.pSetLayouts = compactedSetLayouts.data();
	VK_CHECK(vkCreatePipelineLayout(device->getDevice(), &pipelineInfo, device->getAllocCallbacks(), &mPipelineLayout));
}

void ShaderEffect::fillStage(std::vector<VkPipelineShaderStageCreateInfo>& outStageCreateInfo)
{
}

KENSHIN_END
