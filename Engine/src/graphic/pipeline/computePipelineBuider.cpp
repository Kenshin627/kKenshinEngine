#include "pch.h"
#include "computePipelineBuider.h"

KENSHIN_BEGIN

ComputePipelineBuider::ComputePipelineBuider(GPUDevice* device)
	:PipelineBuilder(device)
{
}

ComputePipelineBuider& ComputePipelineBuider::addStage()
{
	//TODO
	return *this;
}

VkPipeline ComputePipelineBuider::build()
{
	return VkPipeline();
}

KENSHIN_END