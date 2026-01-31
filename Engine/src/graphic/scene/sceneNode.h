#pragma once
#include "platform.h"
#include <glm/glm.hpp>
#include "gpuResource.h"
#include "material/glTFMetalRoughnessMaterial.h"

KENSHIN_BEGIN

struct MeshAsset;

struct RenderObject
{
	u32					firstIndex;
	u32					count;
	Buffer*				indexBuffer;
	Buffer*				vertexBuffer;
	glm::mat4			modelMatrix;
	PBRMaterial			material;
};

struct DrawContext
{
	void pushRenderObject(const RenderObject& r);
	std::vector<RenderObject> renderList;
};

struct IRenderable
{
	virtual void draw(const glm::mat4& transform, DrawContext& context) = 0;
};

struct Node : public IRenderable
{
	Weak<Node>			   parent;
	std::vector<Ref<Node>> children;
	glm::mat4			   localMatrix;
	glm::mat4			   worldMatrix;
	cstring				   name{ nullptr };

	void updateTransform(const glm::mat4& transform);
	virtual void draw(const glm::mat4& transform, DrawContext& context) override;
};

struct MeshNode : public Node
{
	Ref<MeshAsset> meshAsset;
	virtual void draw(const glm::mat4& transform, DrawContext& context) override;
};

KENSHIN_END