#pragma once
#include <glm/glm.hpp>
#include <fastgltf/types.hpp>
#include "platform.h"
#include "gpuResource.h"
#include "scene/sceneNode.h"

KENSHIN_BEGIN

class GPUDevice;
class SceneGraph;

class GLTFLoader : public IRenderable
{
public:
	GLTFLoader(GPUDevice* device);
	virtual ~GLTFLoader() = default;
	Ref<Node> loadFromFile(cstring filename);
	virtual void update(const glm::mat4& transform, SceneGraph* sceneGraph) override;
private:
	VkFilter extractVkFilter(std::optional<fastgltf::Filter> filter) const;
	VkSamplerMipmapMode extractVkMipmapFilter(std::optional<fastgltf::Filter> filter) const;
	VkSamplerAddressMode extractVkAddressMode(fastgltf::Wrap wrap) const;
	TextureHandle loadImage(fastgltf::Asset& asset, fastgltf::Image& image);
private:
	SceneGraph*      							  mSceneGraph;
	GPUDevice*									  mDevice{ nullptr };
	//std::unordered_map<cstring, Ref<Node>>		  mTopNodes;
	BufferHandle								  mGlobalUniformBuffer{ InvalidIndex };
};

KENSHIN_END