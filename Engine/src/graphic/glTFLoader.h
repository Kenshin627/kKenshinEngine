#pragma once
#include "platform.h"
#include "gpuResource.h"
#include <glm/glm.hpp>
KENSHIN_BEGIN

struct GeoSurface
{
	u32 start{ 0 };
	u32 cont{ 0 };
};

struct MeshAsset
{
	cstring name{ nullptr };
	Buffer* vertexBuffer{ nullptr };
	Buffer* indexBuffer{ nullptr };
	Array<GeoSurface> surfaces;
};

class GPUDevice;

class GLTFLoader
{
public:
	GLTFLoader(GPUDevice* device);
	virtual ~GLTFLoader() = default;
	std::vector<MeshAsset> loadFromFile(cstring filename);
private:
	GPUDevice* mDevice{ nullptr };
};

KENSHIN_END