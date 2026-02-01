#pragma once
#include "platform.h"
#include <glm/glm.hpp>
#include "typeDefs.h"

KENSHIN_BEGIN

class SceneGraph;

struct IRenderable
{
	virtual ~IRenderable() = default;
	virtual void update(const glm::mat4& transform, SceneGraph* sceneGraph) = 0;
};

struct Node : public IRenderable
{
	Weak<Node>			   parent;
	std::vector<Ref<Node>> children;
	glm::mat4			   localMatrix{ 1.0 };
	glm::mat4			   worldMatrix{ 1.0 };
	cstring				   name{ nullptr };

	void updateTransform(const glm::mat4& transform);
	virtual void update(const glm::mat4& transform, SceneGraph* sceneGraph) override;
};

struct MeshNode : public Node
{
	Ref<MeshAsset> meshAsset;
	virtual void update(const glm::mat4& transform, SceneGraph* sceneGraph) override;
};

KENSHIN_END