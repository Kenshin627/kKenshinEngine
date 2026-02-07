#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "platform.h"

KENSHIN_BEGIN

struct ShaderModule;
struct GPUDevice;

class ShaderCache
{
public:
	explicit ShaderCache(GPUDevice* device);
	~ShaderCache() = default;
	ShaderModule* getShaderModule(cstring shaderPath, VkShaderStageFlagBits stage, bool isSpirV = false);
	bool loadShaderCode(ShaderModule* outModule, cstring shaderPath, VkShaderStageFlagBits stage, bool isSpirV = false);
	bool readBinaryFile(cstring filePath, std::vector<u32>& outBuffer);
private:
	std::unordered_map<cstring, ShaderModule> mShaderModules;
	GPUDevice* mDevice;
};

KENSHIN_END