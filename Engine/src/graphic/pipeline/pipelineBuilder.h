#pragma once
#include<vulkan/vulkan.hpp>
#include "platform.h"

KENSHIN_BEGIN

struct GPUDevice;

class PipelineBuilder
{
public:
	PipelineBuilder(GPUDevice* device);
	virtual ~PipelineBuilder() = default;
	virtual VkPipeline build() = 0;
protected:
	GPUDevice*		 mDevice;
	VkPipelineLayout mPipelineLayout;
};

KENSHIN_END