#pragma once
#include "platform.h"
#include "gpuResource.h"
#include <glm/glm.hpp>
#include "material/glTFMetalRoughnessMaterial.h"
#include <fastgltf/types.hpp>
#include "scene/sceneNode.h"

KENSHIN_BEGIN

struct GeoSurface
{
	u32				 start	 { 0 };
	u32				 cont	 { 0 };
	PBRMaterial		 material;
};

struct MeshAsset
{
	cstring name{ nullptr };
	Buffer* vertexBuffer{ nullptr };
	Buffer* indexBuffer{ nullptr };
	Array<GeoSurface> surfaces;
};

class GPUDevice;

class GLTFLoader : public IRenderable
{
public:
	GLTFLoader(GPUDevice* device);
	virtual ~GLTFLoader() = default;
	void loadFromFile(cstring filename);
	virtual void draw(const glm::mat4& transform, DrawContext& context) override;
private:
	VkFilter extractVkFilter(std::optional<fastgltf::Filter> filter) const;
	VkSamplerMipmapMode extractVkMipmapFilter(std::optional<fastgltf::Filter> filter) const;
	VkSamplerAddressMode extractVkAddressMode(fastgltf::Wrap wrap) const;
private:
	GPUDevice*									  mDevice{ nullptr };
	std::unordered_map<cstring, TextureHandle>	  mTextures;
	std::unordered_map<cstring, SamplerHandle>    mSamplers;
	std::unordered_map<cstring, Ref<PBRMaterial>> mMaterials;
	std::unordered_map<cstring, Ref<Node>>		  mNodes;
	std::unordered_map<cstring, Ref<MeshAsset>>   mMeshes;
	std::unordered_map<cstring, Ref<Node>>		  mTopNodes;
	BufferHandle								  mGlobalUniformBuffer{ InvalidIndex };
};

KENSHIN_END