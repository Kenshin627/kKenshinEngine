#include "pch.h"
#include <fstream>
#include "shaderCache.h"
#include "gpuDevice.h"
#include "kassert.h"
#include "shaderModule.h"

KENSHIN_BEGIN

ShaderCache::ShaderCache(GPUDevice* device)
	:mDevice(device)
{
}

ShaderModule* ShaderCache::getShaderModule(cstring shaderPath, VkShaderStageFlagBits stage, bool isSpirV)
{
	auto iter = mShaderModules.find(shaderPath);
	if (iter != mShaderModules.cend())
	{
		return &iter->second;
	}
	else
	{
		ShaderModule module;
		bool loaded = loadShaderCode(&module, shaderPath, stage, isSpirV);
		if (!loaded)
		{
			KS_CORE_ASSERT("Failed to load shader code from path: {}", shaderPath);
			return nullptr;
		}
		else
		{
			mShaderModules[shaderPath] = module;
			return &mShaderModules[shaderPath];
		}
		
	}
}

bool ShaderCache::loadShaderCode(ShaderModule* outModule, cstring shaderPath, VkShaderStageFlagBits stage, bool isSpirV)
{
	VkShaderModuleCreateInfo info{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .pNext = nullptr };
	info.flags = 0;
	outModule->stage = stage;
	std::vector<u32> buffer;
	if (isSpirV)
	{
		bool readResult = readBinaryFile(shaderPath, buffer);
		if (!readResult)
		{
			KS_CORE_ASSERT(false, "read binaryFile failed!");
		}
		info.codeSize = buffer.size() * sizeof(u32);
		info.pCode = buffer.data();
		VK_CHECK(vkCreateShaderModule(mDevice->getDevice(), &info, mDevice->getAllocCallbacks(), &outModule->shaderModule));
		return true;
	}
	else
	{
		FileResult shaderString = readTextFile(shaderPath, mDevice->mSystemAllocator);
		if (!shaderString.data)
		{
			KS_CORE_ASSERT("fail to read shader file: {}", shaderPath);
			return false;
		}
		// Compile from glsl to SpirV.
		// TODO: detect if input is HLSL.
		const char* tempFilename = "temp.shader";

		// Write current shader to file.
		FILE* tempShaderFile;
		fopen_s(&tempShaderFile, tempFilename, "w");
		fwrite(shaderString.data, shaderString.size, 1, tempShaderFile);
		fclose(tempShaderFile);

		sizet currentMarker = mDevice->mStackAllocator->getMarker();
		StringBuffer tempStringBuffer;
		tempStringBuffer.init(kkilo(1), mDevice->mStackAllocator);

		// Add uppercase define as STAGE_NAME
		char* stageDefine = tempStringBuffer.append_use_f("%s_%s", toStageDefines(stage), "");
		sizet stageDefineLength = strlen(stageDefine);
		for (u32 i = 0; i < stageDefineLength; ++i)
		{
			stageDefine[i] = toupper(stageDefine[i]);
		}
		// Compile to SPV
#if defined(_MSC_VER)
		char* glslCompilerPath = tempStringBuffer.append_use_f("%sglslangValidator.exe", mDevice->mVkBinariesPath);
		char* finalSpirvFilename = tempStringBuffer.append_use("shader_final.spv");
		// TODO: add optional debug information in shaders (option -g).
		char* arguments = tempStringBuffer.append_use_f("glslangValidator.exe %s -V --target-env vulkan1.4 -o %s -S %s --D %s --D %s", tempFilename, finalSpirvFilename, toCompilerExtension(stage), stageDefine, toStageDefines(stage));
#else
		char* glsl_compiler_path = temp_string_buffer.append_use_f("%sglslangValidator", vulkan_binaries_path);
		char* final_spirv_filename = temp_string_buffer.append_use("shader_final.spv");
		char* arguments = temp_string_buffer.append_use_f("%s -V --target-env vulkan1.2 -o %s -S %s --D %s --D %s", temp_filename, final_spirv_filename, to_compiler_extension(stage), stage_define, to_stage_defines(stage));
#endif
		processExecute(".", glslCompilerPath, arguments, "");

		bool optimize_shaders = false;

		if (optimize_shaders) {
			// TODO: add optional optimization stage
			//"spirv-opt -O input -o output
			char* spirv_optimizer_path = tempStringBuffer.append_use_f("%sspirv-opt.exe", mDevice->mVkBinariesPath);
			char* optimized_spirv_filename = tempStringBuffer.append_use_f("shader_opt.spv");
			char* spirv_opt_arguments = tempStringBuffer.append_use_f("spirv-opt.exe -O --preserve-bindings %s -o %s", finalSpirvFilename, optimized_spirv_filename);

			processExecute(".", spirv_optimizer_path, spirv_opt_arguments, "");

			// Read back SPV file.
			bool readResult = readBinaryFile(optimized_spirv_filename, buffer);
			if (!readResult)
			{
				KS_CORE_ASSERT(false, "read binaryFile failed!");
			}

			info.codeSize = buffer.size() * sizeof(u32);
			info.pCode = buffer.data();

			fileDelete(optimized_spirv_filename);
		}
		else {
			// Read back SPV file.
			//info.pCode = reinterpret_cast<const u32*>(readBinaryFile(finalSpirvFilename, mDevice->mStackAllocator, &info.codeSize));
			// Read back SPV file.
			bool readResult = readBinaryFile(finalSpirvFilename, buffer);
			if (!readResult)
			{
				KS_CORE_ASSERT(false, "read binaryFile failed!");
			}

			info.codeSize = buffer.size() * sizeof(u32);
			info.pCode = buffer.data();

			
		}

		// Handling compilation error
		if (info.pCode == nullptr)
		{
			mDevice->dumpShaderCode(tempStringBuffer, shaderString.data, stage, "");
			KS_CORE_ASSERT(false, "shader code is nullptr!");
			return false;
		}
		
		
		VK_CHECK(vkCreateShaderModule(mDevice->getDevice(), &info, mDevice->getAllocCallbacks(), &outModule->shaderModule));

		outModule->code = std::move(buffer);
		fileDelete(tempFilename);
		fileDelete(finalSpirvFilename);
		
	}
}

bool ShaderCache::readBinaryFile(cstring filePath, std::vector<u32>& outBuffer)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		KS_CORE_ASSERT("Failed to open shader file: {}", filePath);
		return false;
	}
	sizet fileSize = static_cast<sizet>(file.tellg());
	file.seekg(0);
	outBuffer.clear();
	outBuffer.resize(fileSize / sizeof(u32));

	file.read(reinterpret_cast<char*>(outBuffer.data()), fileSize);
	file.close();
	return true;
}

KENSHIN_END
