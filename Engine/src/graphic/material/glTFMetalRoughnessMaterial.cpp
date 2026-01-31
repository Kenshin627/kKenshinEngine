#include "pch.h"
#include "glTFMetalRoughnessMaterial.h"
#include "gpuDevice.h"

KENSHIN_BEGIN

GLTFMetalRoughnessMaterial::GLTFMetalRoughnessMaterial(GPUDevice* device)
	:mDevice(device),
	 mDescriptorSetWriter(device)
{
}

void GLTFMetalRoughnessMaterial::clearResource()
{

}

void GLTFMetalRoughnessMaterial::buildPipelines()
{
	FileResult vertexShader = readTextFile("shaders/defaultVertex.vert", mDevice->mSystemAllocator);
	FileResult fragmentShader = readTextFile("shaders/defaultFragment.frag", mDevice->mSystemAllocator);
	Texture* depthTexture = mDevice->accessTexture(mDevice->mDepthTexture);
	Texture* colorTexture = mDevice->accessTexture(mDevice->mDrawingImage);

	DescriptorSetLayoutHandle globalSetLayout = mDevice->mGlobalDescriptorSetLayout;
	DescriptorSetLayoutCreation materialSetlayoutCreation{};
	materialSetlayoutCreation.reset()
							 .setSetIndex(1)
							 .setName("materialDescriptorSetlayout")
							 .addBinding({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "materialUniformBuffer" })
							 .addBinding({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, "albedoTexture" })
							 .addBinding({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, "metalRoughnessTexture" });
	mDescriptorSetlayout = mDevice->createDescriptorSetLayout(materialSetlayoutCreation);

	DepthStencilCreation depthCreation{};
	depthCreation.setDepth(true, VK_COMPARE_OP_LESS);

	PipelineCreation pbrPipelineCreation{};
	pbrPipelineCreation.addDescriptorSetLayout(globalSetLayout)
					   .addDescriptorSetLayout(mDescriptorSetlayout)
					   .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MaterialUniformBufferData))
					   .setName("PBRMaterialPipeline")
					   .addColorAttachmentFormat(colorTexture->vkFormat)
					   .setDepthFormat(depthTexture->vkFormat)
					   .setStencilFormat(VK_FORMAT_UNDEFINED)
					   .shaders
					   .addStage(vertexShader.data, static_cast<u32>(vertexShader.size), VK_SHADER_STAGE_VERTEX_BIT)
					   .addStage(fragmentShader.data, static_cast<u32>(fragmentShader.size), VK_SHADER_STAGE_FRAGMENT_BIT)
					   .setSpvInput(false)
					   .setName("defaultGraphicShader");
	
	pbrPipelineCreation.depthStencil = depthCreation;
	mOpaquePipeline = mDevice->createPipeline(pbrPipelineCreation);


	//enable blend & disable depthWrite
	BlendStateCreation blendStateCreation{};
	blendStateCreation
		.addBlendState()
		.setColor(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD);
	pbrPipelineCreation.blendState = blendStateCreation;

	depthCreation.setDepth(false, VK_COMPARE_OP_LESS);
	pbrPipelineCreation.depthStencil = depthCreation;
	mTransparentPipeline = mDevice->createPipeline(pbrPipelineCreation);
}

Ref<PBRMaterial> GLTFMetalRoughnessMaterial::buildMaterialInstance(MaterialPass matPass, const MaterialResource& matResource)
{
	auto mat = makeRef<PBRMaterial>();
	mat->materialPass = matPass;
	if (mat->materialPass == MaterialPass::Opaque)
	{
		mat->materialPipeline = mOpaquePipeline;
	}
	else
	{
		mat->materialPipeline = mTransparentPipeline;
	}
	DescriptorSetCreation dsCreation{};
	dsCreation.reset()
			  .setName("pbrMaterialDescriptorSet")
			  .setLayout(mDescriptorSetlayout);

	DescriptorSetHandle dsHandle = mDevice->createDescriptorSet(dsCreation);
	mat->materialDescriptorSet = dsHandle;
	
	//mDescriptorSetWriter.clear();
	//mDescriptorSetWriter.addBuffer(0, matResource.uboData, matResource.uboOffset, sizeof(MaterialUniformBufferData), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	//mDescriptorSetWriter.addImage(1, matResource.albedo, matResource.albedoSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	//mDescriptorSetWriter.addImage(2, matResource.metalRoughness, matResource.metalRoughnessSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	//mDescriptorSetWriter.writeDescriptorSet(dsHandle);
	//mDescriptorSetWriter.clear();

	UpdateDescriptorSetCreation updateDsCreation{};
	updateDsCreation.reset()
					.buffer(matResource.uboData, 0)
					.textureSampler(matResource.albedo, matResource.albedoSampler, 1)
					.textureSampler(matResource.metalRoughness, matResource.metalRoughnessSampler, 2);
	mDevice->updateDescriptorSet(updateDsCreation, dsHandle);
	return mat;
}

KENSHIN_END
