#pragma once
#include <vector>
#include "platform.h"
#include "typeDefs.h"
#include "glTFLoader.h"

KENSHIN_BEGIN

class GPUDevice;
class Camera;
class DirectionLight;
struct DescriptorSetHandle;
struct CommandBuffer;

class SceneGraph
{
public:
	SceneGraph(GPUDevice* device);
	~SceneGraph() = default;
	bool loadGLTFScene(cstring filePath);
	bool updateScene(const glm::mat4& transform);
	bool addRenderObject(const RenderObject& renderObject);
	void draw(CommandBuffer* cmd);
private:
	Ref<Camera>					   mCamera;
	Ref<DirectionLight>			   mDirectionalLight;
	std::vector<RenderObject>      mRenderList;
	GPUDevice*					   mDevice;
	Scope<GLTFLoader>			   mGLTFLoader;
	DescriptorSetHandle            mGlobalDescriptorSet{ InvalidIndex };
};

KENSHIN_END