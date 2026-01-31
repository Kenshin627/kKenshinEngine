#include "pch.h"
#include "sceneGraph.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gpuDevice.h"
#include "camera.h"
#include "directionLight.h"
#include "gpuResource.h"
//#include "glTFLoader.h"

KENSHIN_BEGIN

SceneGraph::SceneGraph(GPUDevice* device)
	:mDevice(device)
{
	mGLTFLoader = makeScope<GLTFLoader>(device);

	//TODO:default camera & light
	mCamera = makeRef<Camera>();
	mCamera->setPosition({ 5, 3, 8, 1 });
	mCamera->setCenter({ 0, 2, 0 });
	mCamera->setAspectRatio((float)mDevice->mSwapchainWidth / (float)mDevice->mSwapchainHeight);

	mDirectionalLight = makeRef<DirectionLight>();
	mDirectionalLight->setDirection({ 0.5, 0.5, 0.5, 1.0 });
	mDirectionalLight->setColor({ 1.0, 1.0, 1.0, 1.0 });

	//SceneData initialize
	DescriptorSetCreation globalDsCreation{};
	globalDsCreation.reset()
					.setLayout(mDevice->mGlobalDescriptorSetLayout)
					.setName("globalDescriptorSet");
	mGlobalDescriptorSet = mDevice->createDescriptorSet(globalDsCreation);

	BufferCreation bufferCreation{};
	bufferCreation.reset()
				  .set(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Immutable, sizeof(SceneUniformBufferData))
				  //.setData(&sceneUbo)
				  .setName("sceneUniformBuffer")
				  .setPersistent(true);
	mSceneUniformBuffer = mDevice->createBuffer(bufferCreation);
	UpdateDescriptorSetCreation updateGlobalDsCreation{};
	updateGlobalDsCreation.reset().buffer(mSceneUniformBuffer, 0);

	updateSceneUniformBuffer();
	mDevice->updateDescriptorSet(updateGlobalDsCreation, mGlobalDescriptorSet);
}

bool SceneGraph::loadGLTFScene(cstring filePath)
{
	return mGLTFLoader->loadFromFile(filePath);	
}

bool SceneGraph::updateScene(const glm::mat4& transform)
{
	mGLTFLoader->update(transform, this);
	return true;
}

bool SceneGraph::addRenderObject(const RenderObject& renderObject)
{
	mRenderList.push_back(renderObject);
	return true;
}

void SceneGraph::draw(CommandBuffer* cmd)
{
	sizet drawIndex = 0;
	//TODO: update scene UBO only neccessary
	updateSceneUniformBuffer();
	for (auto& drawItem : mRenderList)
	{
		cmd->bindPipeline(drawItem.material.materialPipeline);
		cmd->bindDescriptorSet(&mGlobalDescriptorSet, 1, nullptr, 0, 0);
		//Dynamic uniform buffer
		u32 dynamicOffset = static_cast<u32>(drawIndex * sizeof(Kenshin::GLTFMetalRoughnessMaterial::MaterialUniformBufferData));
		cmd->bindDescriptorSet(const_cast<Kenshin::DescriptorSetHandle*>(&drawItem.material.materialDescriptorSet), 1, &dynamicOffset, 1, 1);
		cmd->bindIndexBuffer(drawItem.indexBuffer->handle, VK_INDEX_TYPE_UINT32);
		Kenshin::RenderObjectPushConstant modelPushConstant
		{
			drawItem.modelMatrix,
			drawItem.vertexBufferAddress
		};
		cmd->pushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Kenshin::RenderObjectPushConstant), &modelPushConstant);
		cmd->drawIndex(drawItem.count, 1, drawItem.firstIndex, 0, 0);
		++drawIndex;
	}
}

void SceneGraph::updateSceneUniformBuffer()
{
	SceneUniformBufferData sceneUbo{};
	sceneUbo.position = mCamera->getPosition();
	sceneUbo.viewMatrix = mCamera->getViewMatrix();
	sceneUbo.projectionMatrix = mCamera->getProjectionMatrix();
	sceneUbo.viewProjectionMatrix = mCamera->getViewProjectionMatrix();
	sceneUbo.direction = mDirectionalLight->getDirection();
	sceneUbo.color = mDirectionalLight->getColor();
	Buffer* sceneUBO = mDevice->accessBuffer(mSceneUniformBuffer);
	memcpy(sceneUBO->mappedData, &sceneUbo, sizeof(SceneUniformBufferData));
}

KENSHIN_END

