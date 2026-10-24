#include "pch.h"

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

void ShaderEffect::fillStage(std::vector<VkPipelineShaderStageCreateInfo>& outStageCreateInfo)
{
}

KENSHIN_END
