#pragma once
#include "platform.h"
#include "pipelineBuilder.h"

KENSHIN_BEGIN

class ComputePipelineBuider :public PipelineBuilder
{
public:
	ComputePipelineBuider(GPUDevice* device);
	~ComputePipelineBuider() = default;
	ComputePipelineBuider& addStage();
	virtual VkPipeline build() override;
};

KENSHIN_END