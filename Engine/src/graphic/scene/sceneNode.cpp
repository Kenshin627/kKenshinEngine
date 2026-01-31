#include "pch.h"
#include "sceneNode.h"
#include "glTFLoader.h"

KENSHIN_BEGIN

void Node::updateTransform(const glm::mat4& transform)
{
	worldMatrix = transform * localMatrix;
	for (auto& node : children)
	{
		node->updateTransform(worldMatrix);
	}
}

void Node::draw(const glm::mat4& transform, DrawContext& context)
{
	for (auto& node : children)
	{
		node->draw(transform, context);
	}
}

void MeshNode::draw(const glm::mat4& transform, DrawContext & context)
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
		context.pushRenderObject(renderObject);
	}
	Node::draw(transform, context);
}

void DrawContext::pushRenderObject(const RenderObject& r)
{
	renderList.push_back(r);
}

KENSHIN_END

