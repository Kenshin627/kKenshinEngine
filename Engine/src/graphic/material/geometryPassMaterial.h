#pragma once
#include "platform.h"
#include "gpuResource.h"
#include "descriptorSet/descriptorSetWriter.h"

KENSHIN_BEGIN

class GPUDevice;

struct GeometryPassMaterial
{
	struct MaterialUniformBufferData
	{
		glm::vec4 colorFactor;
		glm::vec4 metalRoughness;
		//TODO: require these padding?
		glm::vec4 extra[14];
	};

	struct MaterialResource
	{
		TextureHandle albedo;
		SamplerHandle albedoSampler;
		TextureHandle metalRoughness;
		SamplerHandle metalRoughnessSampler;
		BufferHandle  uboData;
		u32			  uboOffset;
	};

	GeometryPassMaterial(GPUDevice* device);
	~GeometryPassMaterial() = default;
	void clearResource();
	void buildPipelines();
	Ref<PBRMaterial> buildMaterialInstance(MaterialPass matPass, const MaterialResource& matResource);

	DescriptorWriter		  mDescriptorSetWriter;
	GPUDevice* mDevice{ nullptr };
	PipelineHandle			  mOpaquePipeline{ InvalidIndex };
	PipelineHandle			  mTransparentPipeline{ InvalidIndex };
	DescriptorSetLayoutHandle mDescriptorSetlayout{ InvalidIndex };
};

KENSHIN_END