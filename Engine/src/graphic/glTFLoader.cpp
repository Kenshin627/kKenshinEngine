#include "pch.h"
#include "glTFLoader.h"
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include "gpuDevice.h"

KENSHIN_BEGIN

GLTFLoader::GLTFLoader(GPUDevice* device)
	:mDevice(device)
{
}

std::vector<MeshAsset> GLTFLoader::loadFromFile(cstring filename)
{
	std::vector<MeshAsset> meshes;

	auto expectedBuffer = fastgltf::GltfDataBuffer::FromPath(filename);
	fastgltf::Options options = fastgltf::Options::LoadExternalBuffers | 
								fastgltf::Options::LoadExternalImages  | 
								//fastgltf::Options::LoadGLBBuffers	   |
								fastgltf::Options::GenerateMeshIndices;
	fastgltf::Parser parser;
	if (expectedBuffer.error() != fastgltf::Error::None)
	{
		KS_CORE_ERROR("fastgltf::GltfDataBuffer::FromPath succeeded");
		return {};
	}
	auto expectedAsset = parser.loadGltfBinary(expectedBuffer.get(), std::filesystem::path(filename).parent_path(), options, fastgltf::Category::All);
	if (expectedAsset.error() != fastgltf::Error::None)
	{
		KS_CORE_ERROR("parser.loadGltfBinary succeeded");
		return {};
	}
	KS_CORE_INFO("GLTFLoader::loadFromFile succeeded");
	fastgltf::Asset& asset = expectedAsset.get();
	sizet meshCount = asset.meshes.size();
	meshes.reserve(meshCount);
	KS_CORE_INFO("Mesh count: {}", meshCount);

	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	for (int i = 0; i < meshCount; ++i)
	{
		vertices.clear();
		indices.clear();
		auto& gltfMesh = asset.meshes[i];
		MeshAsset meshAsset;
		meshAsset.name = gltfMesh.name.c_str();
		meshAsset.surfaces.init(mDevice->mSystemAllocator, gltfMesh.primitives.size());
		for (auto& primitive : gltfMesh.primitives)
		{
			GeoSurface surface;
			sizet initialVertexCount = vertices.size();

			//indices
			{
				fastgltf::Accessor& indicesAccessor = asset.accessors[primitive.indicesAccessor.value()];
				surface.start = static_cast<u32>(indices.size());
				surface.cont = indicesAccessor.count;				
				indices.reserve(indices.size() + indicesAccessor.count);
				fastgltf::iterateAccessor<u32>(asset, indicesAccessor, [&](u32 value) {
					indices.push_back(value + initialVertexCount);
				});
			}

			//position
			{
				fastgltf::Accessor& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + positionAccessor.count);
				fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, positionAccessor, [&](glm::vec3 position, size_t index) {
					Vertex vertex{};
					vertex.position = { position.x, position.y, position.z };
					vertex.normal = { 1, 0, 0 };
					vertex.u = 0;
					vertex.v = 0;
					vertices[initialVertexCount + index] = vertex;
				});
			}

			//normal
			{
				auto normalAttr = primitive.findAttribute("NORMAL");
				if (normalAttr != primitive.attributes.end())
				{
					fastgltf::Accessor& normalAccessor = asset.accessors[normalAttr->accessorIndex];
					fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, normalAccessor, [&](glm::vec3 normal, size_t index) {
						vertices[initialVertexCount + index].normal = { normal.x, normal.y, normal.z };
					});
				}
			}

			//uvs
			{
				auto uvAttr = primitive.findAttribute("TEXCOORD_0");
				if (uvAttr != primitive.attributes.end())
				{
					fastgltf::Accessor& uvAccessor = asset.accessors[uvAttr->accessorIndex];
					fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, uvAccessor, [&](glm::vec2 uv, size_t index) {
						vertices[initialVertexCount + index].u = uv.x;
						vertices[initialVertexCount + index].v = uv.y;
					});
				}
			}
			meshAsset.surfaces.pushBack(surface);
		}

		//createBuffers
		BufferCreation vertexBufferCreation{};
		vertexBufferCreation.reset()
			.set(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, ResourceUsageType::Immutable, static_cast<u32>(sizeof(Vertex) * vertices.size()))
			.setData(vertices.data())
			.setName(meshAsset.name);
		BufferHandle vboHandle = mDevice->createBuffer(vertexBufferCreation);
		Buffer* vertexBuffer = mDevice->accessBuffer(vboHandle);
		VkBufferDeviceAddressInfo addressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr };
		addressInfo.buffer = vertexBuffer->vkBuffer;
		vertexBuffer->mDeviceAddress = vkGetBufferDeviceAddress(mDevice->getDevice(), &addressInfo);

		meshAsset.vertexBuffer = vertexBuffer;

		BufferCreation indexBufferCreation{};
		indexBufferCreation.reset()
			.set(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ResourceUsageType::Immutable, static_cast<u32>(sizeof(u32) * indices.size()))
			.setData(indices.data())
			.setName(meshAsset.name);
		BufferHandle iboHandle = mDevice->createBuffer(indexBufferCreation);
		Buffer* indexBuffer = mDevice->accessBuffer(iboHandle);
		meshAsset.indexBuffer = indexBuffer;
		meshes.push_back(meshAsset);
	}
	return meshes;
}

KENSHIN_END