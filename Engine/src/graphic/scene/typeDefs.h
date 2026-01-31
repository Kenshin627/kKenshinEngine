#pragma once
#include "platform.h"
#include "material/glTFMetalRoughnessMaterial.h"
#include "array.h"
#include "commandBuffer.h"

KENSHIN_BEGIN

struct GeoSurface
{
	u32				 start{ 0 };
	u32				 cont{ 0 };
	PBRMaterial		 material;
};

struct MeshAsset
{
	cstring name{ nullptr };
	Buffer* vertexBuffer{ nullptr };
	Buffer* indexBuffer{ nullptr };
	Array<GeoSurface> surfaces;
};

struct RenderObject
{
	u32					firstIndex;
	u32					count;
	Buffer* indexBuffer;
	u64 				vertexBufferAddress;
	glm::mat4			modelMatrix;
	PBRMaterial			material;
	void draw(CommandBuffer* cmd)
	{

	}
};

struct DrawContext
{
	void pushRenderObject(const RenderObject& r);
	std::vector<RenderObject> renderList;
};

KENSHIN_END