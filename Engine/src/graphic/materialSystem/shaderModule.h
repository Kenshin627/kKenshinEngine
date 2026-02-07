#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "platform.h"

KENSHIN_BEGIN

struct ShaderModule
{
	std::vector<u32>	  code;
	VkShaderModule		  shaderModule;
	VkShaderStageFlagBits stage;
};

KENSHIN_END