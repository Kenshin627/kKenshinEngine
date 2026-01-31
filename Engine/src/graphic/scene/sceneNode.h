#pragma once
#include "platform.h"
#include <glm/glm.hpp>
#include "typeDefs.h"

KENSHIN_BEGIN

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