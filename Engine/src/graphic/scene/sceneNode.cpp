#include "pch.h"
#include "sceneNode.h"
#include "scene/sceneGraph.h"

KENSHIN_BEGIN

void Node::updateTransform(const glm::mat4& transform)
{
	worldMatrix = transform * localMatrix;
	for (auto& node : children)
	{
		node->updateTransform(worldMatrix);
	}
}

void Node::update(const glm::mat4& transform, SceneGraph* sceneGraph)
{
	for (auto& node : children)
	{
		node->update(transform, sceneGraph);
	}
}

void MeshNode::update(const glm::mat4& transform, SceneGraph* sceneGraph)
{

	sizet surfaceCount = meshAsset->surfaces.size();
	for (sizet i = 0; i < surfaceCount; ++i)
	{
		RenderObject renderObject;
		const GeoSurface& surface		 = meshAsset->surfaces[i];
		renderObject.firstIndex			 = surface.start;
		renderObject.count				 = surface.cont;
		renderObject.indexBuffer		 = meshAsset->indexBuffer;
		renderObject.modelMatrix		 = transform * worldMatrix;
		renderObject.material			 = surface.material;
		renderObject.vertexBufferAddress = meshAsset->vertexBuffer->deviceAddress;
		sceneGraph->addRenderObject(renderObject);
	}
	Node::update(transform, sceneGraph);
}

void DrawContext::pushRenderObject(const RenderObject& r)
{
	renderList.push_back(r);
}

KENSHIN_END

