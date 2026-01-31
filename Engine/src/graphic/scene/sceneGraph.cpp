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
	mCamera->setViewMatrix(glm::lookAt({ 5, 3, 8 }, glm::vec3(0, 2, 0), glm::vec3(0, 1, 0)));
	mCamera->setProjectionMatrix(glm::perspective(glm::radians(45.0f), (float)mDevice->mSwapchainWidth / (float)mDevice->mSwapchainHeight, 0.01f, 100.0f));

	mDirectionalLight = makeRef<DirectionLight>();
	mDirectionalLight->setDirection({ 0.5, 0.5, 0.5, 1.0 });
	mDirectionalLight->setColor({ 1.0, 1.0, 1.0, 1.0 });

	//SceneData initialize
	DescriptorSetCreation globalDsCreation{};
	globalDsCreation.reset()
					.setLayout(mDevice->mGlobalDescriptorSetLayout)
					.setName("globalDescriptorSet");
	mGlobalDescriptorSet = mDevice->createDescriptorSet(globalDsCreation);


	SceneUniformBufferData sceneUbo{};
	sceneUbo.position = mCamera->getPosition();
	sceneUbo.viewMatrix = mCamera->getViewMatrix();
	sceneUbo.projectionMatrix = mCamera->getProjectionMatrix();
	sceneUbo.viewProjectionMatrix = mCamera->getViewProjectionMatrix();
	sceneUbo.direction = mDirectionalLight->getDirection();
	sceneUbo.color = mDirectionalLight->getColor();

	BufferCreation bufferCreation{};
	bufferCreation.reset()
				  .set(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Immutable, sizeof(SceneUniformBufferData))
				  .setData(&sceneUbo)
				  .setName("sceneUniformBuffer");
	BufferHandle sceneUBOHandle = mDevice->createBuffer(bufferCreation);
	UpdateDescriptorSetCreation updateGlobalDsCreation{};
	updateGlobalDsCreation.reset().buffer(sceneUBOHandle, 0);
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

DescriptorSetHandle SceneGraph::getGlobalDescriptorSet()
{
	return mGlobalDescriptorSet;
}

const std::vector<RenderObject>& SceneGraph::getRenderList()
{
	return mRenderList;
}

KENSHIN_END

