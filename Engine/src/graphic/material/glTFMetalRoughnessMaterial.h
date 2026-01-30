#pragma once
#include "platform.h"
#include "gpuResource.h"
#include "descriptorSet/descriptorSetWriter.h"

KENSHIN_BEGIN

class GPUDevice;

enum class MaterialPass
{
	Opaque = 0,
	Transparent
};

struct PBRMaterial
{
	PipelineHandle		materialPipeline	 { InvalidIndex		    };
	MaterialPass		materialPass		 { MaterialPass::Opaque };
	DescriptorSetHandle materialDescriptorSet{ InvalidIndex			};
};

struct GLTFMetalRoughnessMaterial
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

	GLTFMetalRoughnessMaterial(GPUDevice* device);
	~GLTFMetalRoughnessMaterial() = default;
	void clearResource();
	void buildPipelines();
	PBRMaterial buildMaterialInstance(MaterialPass matPass, const MaterialResource& matResource);

	DescriptorWriter		  mDescriptorSetWriter;
	GPUDevice*				  mDevice			  { nullptr		 };
	PipelineHandle			  mOpaquePipeline	  { InvalidIndex };
	PipelineHandle			  mTransparentPipeline{ InvalidIndex };
	DescriptorSetLayoutHandle mDescriptorSetlayout{ InvalidIndex };
};

KENSHIN_END