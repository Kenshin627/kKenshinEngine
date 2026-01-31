#include "pch.h"
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>
#include "glTFLoader.h"
#include "gpuDevice.h"
#include "array.h"

KENSHIN_BEGIN

GLTFLoader::GLTFLoader(GPUDevice* device) :mDevice(device)
{
}

bool GLTFLoader::loadFromFile(cstring filename)
{
	std::vector<Ref<MeshAsset>>		meshes;
	std::vector<TextureHandle>	    images;
	std::vector<Ref<PBRMaterial>>	materials;
	std::vector<Ref<Node>>		    nodes;
	std::vector<SamplerHandle>	    samplers;

	auto expectedBuffer = fastgltf::GltfDataBuffer::FromPath(filename);
	fastgltf::Options options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;
	fastgltf::Parser parser;
	if (expectedBuffer.error() != fastgltf::Error::None)
	{
		KS_CORE_ERROR("fastgltf::GltfDataBuffer::FromPath succeeded");
		return false;
	}
	fastgltf::GltfType type = fastgltf::determineGltfFileType(expectedBuffer.get());
	fastgltf::Asset gltfDataBuffer;
	bool loadSuccess = true;
	if (type == fastgltf::GltfType::GLB)
	{
		auto expectedDataBuffer = parser.loadGltfBinary(expectedBuffer.get(), std::filesystem::path(filename).parent_path(), options, fastgltf::Category::All);
		if (expectedDataBuffer.error() != fastgltf::Error::None)
		{
			loadSuccess = false;
		}
		else
		{
			gltfDataBuffer = std::move(expectedDataBuffer.get());
		}
	}
	else
	{
		auto expectedDataBuffer = parser.loadGltfJson(expectedBuffer.get(), std::filesystem::path(filename).parent_path(), options, fastgltf::Category::All);
		if (expectedDataBuffer.error() != fastgltf::Error::None)
		{
			loadSuccess = false;
		}
		else
		{
			gltfDataBuffer = std::move(expectedDataBuffer.get());
		}
	}

	if (!loadSuccess)
	{
		KS_CORE_ERROR("GLTFLoader::loadFromFile failed");
		return false;
	}
	KS_CORE_INFO("GLTFLoader::loadFromFile succeeded");

	//samplers
	samplers.reserve(gltfDataBuffer.samplers.size());
	for (const auto& sampler : gltfDataBuffer.samplers)
	{
		SamplerCreation creation{};
		creation.setName(sampler.name.c_str())
				.setAddressModeUV(extractVkAddressMode(sampler.wrapS), extractVkAddressMode(sampler.wrapT))
				.setMinMagMip(extractVkFilter(sampler.minFilter), extractVkFilter(sampler.magFilter), extractVkMipmapFilter(sampler.minFilter))
				.setName(sampler.name.c_str());
		SamplerHandle samplerHandle = mDevice->createSampler(creation);
		samplers.push_back(samplerHandle);
	}

	//images
	images.reserve(gltfDataBuffer.images.size());
	for (auto& tex : gltfDataBuffer.images)
	{
		TextureHandle texHandle = loadImage(gltfDataBuffer, tex);
		if (texHandle.index == InvalidIndex)
		{
			texHandle = mDevice->mCheckboardTexture;
		}
		images.push_back(texHandle);
	}

	//materials
	materials.reserve(gltfDataBuffer.materials.size());
	sizet bufferSize = gltfDataBuffer.materials.size() * sizeof(GLTFMetalRoughnessMaterial::MaterialUniformBufferData);
	BufferCreation globalUniformBufferCreation{};
	globalUniformBufferCreation.reset()
							   .set(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Immutable, bufferSize)
							   .setName("GLTF Global Uniform Buffer")
							   .setPersistent(true);
	mGlobalUniformBuffer = mDevice->createBuffer(globalUniformBufferCreation);
	sizet materialIndex = 0;
	void* uboAddress = mDevice->accessBuffer(mGlobalUniformBuffer)->mappedData;
	GLTFMetalRoughnessMaterial::MaterialUniformBufferData* materialUBOMappedData = static_cast<GLTFMetalRoughnessMaterial::MaterialUniformBufferData*>(uboAddress);
	for (auto& material : gltfDataBuffer.materials)
	{
		GLTFMetalRoughnessMaterial::MaterialUniformBufferData bufferData{};
		bufferData.colorFactor.r = material.pbrData.baseColorFactor[0];
		bufferData.colorFactor.g = material.pbrData.baseColorFactor[1];
		bufferData.colorFactor.b = material.pbrData.baseColorFactor[2];
		bufferData.colorFactor.a = material.pbrData.baseColorFactor[3];
		bufferData.metalRoughness.r = material.pbrData.metallicFactor;
		bufferData.metalRoughness.g = material.pbrData.roughnessFactor;
		materialUBOMappedData[materialIndex] = bufferData;

		MaterialPass materialPass = material.alphaMode == fastgltf::AlphaMode::Opaque ? MaterialPass::Opaque : MaterialPass::Transparent;

		GLTFMetalRoughnessMaterial::MaterialResource resource{};
		resource.uboData = mGlobalUniformBuffer;
		resource.uboOffset = materialIndex * sizeof(GLTFMetalRoughnessMaterial::MaterialUniformBufferData);
		resource.albedo = mDevice->mWhiteTexture;
		resource.albedoSampler = mDevice->mLinearSampler;
		resource.metalRoughness = mDevice->mGrayTexture;
		resource.metalRoughnessSampler = mDevice->mNearestSampler;

		if (material.pbrData.baseColorTexture.has_value())
		{
			sizet textureIndex = material.pbrData.baseColorTexture.value().textureIndex;
			sizet img = gltfDataBuffer.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
			sizet sampler = gltfDataBuffer.textures[material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
			resource.albedo = images[img];
			resource.albedoSampler = samplers[sampler];
		}

		if (material.pbrData.metallicRoughnessTexture.has_value())
		{
			sizet textureIndex = material.pbrData.metallicRoughnessTexture.value().textureIndex;
			sizet img = gltfDataBuffer.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
			sizet sampler = gltfDataBuffer.textures[material.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();
			resource.metalRoughness = images[img];
			resource.metalRoughnessSampler = samplers[sampler];
		}

		auto pbrMaterial = mDevice->mGLTFMetalRoughnessMaterial->buildMaterialInstance(materialPass, resource);
		materials.push_back(pbrMaterial);
		++materialIndex;
	}

	//meshes
	size_t meshCount = gltfDataBuffer.meshes.size();
	meshes.reserve(meshCount);

	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	for (int i = 0; i < meshCount; ++i)
	{
		vertices.clear();
		indices.clear();
		auto& gltfMesh = gltfDataBuffer.meshes[i];
		Ref<MeshAsset> meshAsset = makeRef<MeshAsset>();
		meshAsset->name = gltfMesh.name.c_str();
		meshAsset->surfaces.init(mDevice->mSystemAllocator, gltfMesh.primitives.size());
		for (auto& primitive : gltfMesh.primitives)
		{
			GeoSurface surface;
			sizet initialVertexCount = vertices.size();

			//indices
			{
				fastgltf::Accessor& indicesAccessor = gltfDataBuffer.accessors[primitive.indicesAccessor.value()];
				surface.start = static_cast<u32>(indices.size());
				surface.cont = indicesAccessor.count;
				indices.reserve(indices.size() + indicesAccessor.count);
				fastgltf::iterateAccessor<u32>(gltfDataBuffer, indicesAccessor, [&](u32 value) {
					indices.push_back(value + initialVertexCount);
					});
			}

			//position
			{
				fastgltf::Accessor& positionAccessor = gltfDataBuffer.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + positionAccessor.count);
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfDataBuffer, positionAccessor, [&](glm::vec3 position, size_t index) {
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
					fastgltf::Accessor& normalAccessor = gltfDataBuffer.accessors[normalAttr->accessorIndex];
					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltfDataBuffer, normalAccessor, [&](glm::vec3 normal, size_t index) {
						vertices[initialVertexCount + index].normal = { normal.x, normal.y, normal.z };
						});
				}
			}

			//uvs
			{
				auto uvAttr = primitive.findAttribute("TEXCOORD_0");
				if (uvAttr != primitive.attributes.end())
				{
					fastgltf::Accessor& uvAccessor = gltfDataBuffer.accessors[uvAttr->accessorIndex];
					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltfDataBuffer, uvAccessor, [&](glm::vec2 uv, size_t index) {
						vertices[initialVertexCount + index].u = uv.x;
						vertices[initialVertexCount + index].v = uv.y;
						});
				}
			}
			if (primitive.materialIndex.has_value())
			{
				sizet matIndex = primitive.materialIndex.value();
				surface.material = *materials[matIndex].get();
			}
			else
			{
				surface.material = *mDevice->mDefaultPBRMaterial.get();
			}
			meshAsset->surfaces.pushBack(surface);
		}

		//createBuffers
		BufferCreation vertexBufferCreation{};
		vertexBufferCreation.reset()
			.set(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, ResourceUsageType::Immutable, static_cast<u32>(sizeof(Vertex) * vertices.size()))
			.setData(vertices.data())
			.setName(meshAsset->name);
		BufferHandle vboHandle = mDevice->createBuffer(vertexBufferCreation);
		Buffer* vertexBuffer = mDevice->accessBuffer(vboHandle);
		VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr };
		addressInfo.buffer = vertexBuffer->vkBuffer;
		vertexBuffer->deviceAddress = vkGetBufferDeviceAddress(mDevice->getDevice(), &addressInfo);

		meshAsset->vertexBuffer = vertexBuffer;

		BufferCreation indexBufferCreation{};
		indexBufferCreation.reset()
			.set(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ResourceUsageType::Immutable, static_cast<u32>(sizeof(u32) * indices.size()))
			.setData(indices.data())
			.setName(meshAsset->name);
		BufferHandle iboHandle = mDevice->createBuffer(indexBufferCreation);
		Buffer* indexBuffer = mDevice->accessBuffer(iboHandle);
		meshAsset->indexBuffer = indexBuffer;
		meshes.push_back(meshAsset);
	}
	//nodes
	nodes.reserve(gltfDataBuffer.nodes.size());
	for (const auto& n : gltfDataBuffer.nodes)
	{
		Ref<Node> node;
		if (n.meshIndex.has_value())
		{
			sizet meshIndex = n.meshIndex.value();
			node = makeRef<MeshNode>();
			static_cast<MeshNode*>(node.get())->meshAsset = meshes[meshIndex];
		}
		else
		{
			node = makeRef<Node>();
		}
		node->name = n.name.c_str();
		std::visit(fastgltf::visitor{
			[&](const fastgltf::math::fmat4x4& matrix) {
				memcpy(&node->localMatrix, matrix.data(), sizeof(matrix));
			},
			[&](const fastgltf::TRS& transform) {
			  glm::vec3 translation(transform.translation[0], transform.translation[1],
				  transform.translation[2]);
			  glm::quat rotate(transform.rotation[3], transform.rotation[0], transform.rotation[1],
				  transform.rotation[2]);
			  glm::vec3 scale(transform.scale[0], transform.scale[1], transform.scale[2]);

			  glm::mat4 tm = glm::translate(glm::mat4(1.f), translation);
			  glm::mat4 rm = glm::mat4_cast(rotate);
			  glm::mat4 sm = glm::scale(glm::mat4(1.f), scale);

			  node->localMatrix = tm * rm * sm;
			}
			}, n.transform);
		nodes.push_back(node);
	}

	//parent->children
	for (sizet i = 0; i < gltfDataBuffer.nodes.size(); ++i)
	{
		auto& n = gltfDataBuffer.nodes[i];
		auto& node = nodes[i];
		for (sizet childIndex : n.children)
		{
			nodes[childIndex]->parent = node;
			node->children.push_back(nodes[childIndex]);
		}
	}

	//topNodes
	for (sizet i = 0; i < nodes.size(); ++i)
	{
		auto& node = nodes[i];
		if (!node->parent.lock())
		{
			mTopNodes.insert({ node->name, node });
			node->updateTransform(glm::mat4(1.f));
		}
	}
}

void GLTFLoader::update(const glm::mat4& transform, SceneGraph* sceneGraph)
{
	for (auto& [name, node] : mTopNodes)
	{
		node->update(transform, sceneGraph);
	}
}

VkFilter GLTFLoader::extractVkFilter(std::optional<fastgltf::Filter> filter) const
{
	bool hasFilter = filter.has_value();
	VkFilter defaultFilter = VK_FILTER_LINEAR;
	if (hasFilter)
	{
		fastgltf::Filter filterValue = filter.value();
		switch (filterValue)
		{
		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapLinear:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_FILTER_LINEAR;
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::NearestMipMapNearest:
			return VK_FILTER_NEAREST;
		default:
			return defaultFilter;
		}
	}
	else
	{
		return defaultFilter;
	}	
}

VkSamplerMipmapMode GLTFLoader::extractVkMipmapFilter(std::optional<fastgltf::Filter> filter) const
{
	bool hasFilter = filter.has_value();
	if (hasFilter)
	{
		fastgltf::Filter filterValue = filter.value();
		switch (filterValue)
		{
			case fastgltf::Filter::LinearMipMapLinear:
			case fastgltf::Filter::NearestMipMapLinear:
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			case fastgltf::Filter::LinearMipMapNearest:
			case fastgltf::Filter::NearestMipMapNearest:
				return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			default:
				return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
		}
	}
	else
	{
		return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	}	
}

VkSamplerAddressMode GLTFLoader::extractVkAddressMode(fastgltf::Wrap wrap) const
{
	switch (wrap)
	{
	case fastgltf::Wrap::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case fastgltf::Wrap::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case fastgltf::Wrap::MirroredRepeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}
}

TextureHandle GLTFLoader::loadImage(fastgltf::Asset& asset, fastgltf::Image& image)
{
	int width, height, nrChannels;
	TextureHandle texture{ InvalidTexture };
	std::visit(
		fastgltf::visitor{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& filePath) {
				KS_CORE_ASSERT(filePath.fileByteOffset == 0, "We don't support offsets with stbi.");
				KS_CORE_ASSERT(filePath.uri.isLocalPath(), " We're only capable of loading");
				const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
				unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
				if (data) 
				{
					TextureCreation texCreation{};
					texCreation.setData(data)
							   .setFlags(1, 0)
							   .setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
							   .setName(image.name.c_str())
							   .setSize(static_cast<u16>(width), static_cast<u16>(height), 1)
							   .setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
					texture = mDevice->createTexture(texCreation);
					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::Vector& vector) {
				unsigned char* data = stbi_load_from_memory((unsigned char*)vector.bytes.data(), static_cast<int>(vector.bytes.size()),
					&width, &height, &nrChannels, 4);
					if (data) 
					{
						TextureCreation texCreation{};
						texCreation.setData(data)
								   .setFlags(1, 0)
								   .setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
								   .setName(image.name.c_str())
								   .setSize(static_cast<u16>(width), static_cast<u16>(height), 1)
								   .setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
						texture = mDevice->createTexture(texCreation);
			
						stbi_image_free(data);
					}
			},
			[&](fastgltf::sources::BufferView& view) {
				auto& bufferView = asset.bufferViews[view.bufferViewIndex];
				auto& buffer = asset.buffers[bufferView.bufferIndex];

				std::visit(fastgltf::visitor{ 
					[&](fastgltf::sources::URI& filePath) {
						auto bufferPath =  std::string(filePath.uri.path().begin(), filePath.uri.path().end());

						unsigned char* data = stbi_load(bufferPath.c_str(), &width, &height, &nrChannels, 4);
						if (data)
						{
							TextureCreation texCreation{};
							texCreation.setData(data)
								.setFlags(1, 0)
								.setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
								.setName(image.name.c_str())
								.setSize(static_cast<u16>(width), static_cast<u16>(height), 1)
								.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
							texture = mDevice->createTexture(texCreation);
							stbi_image_free(data);
						}
					},
					[&](fastgltf::sources::Vector& vector) {
						unsigned char* data = stbi_load_from_memory((unsigned char*)vector.bytes.data() + bufferView.byteOffset,
						static_cast<int>(bufferView.byteLength),
						&width, &height, &nrChannels, 4);
						if (data) 
						{
							TextureCreation texCreation{};
							texCreation.setData(data)
									   .setFlags(1, 0)
									   .setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
									   .setName(image.name.c_str())
									   .setSize(static_cast<u16>(width), static_cast<u16>(height), 1)
									   .setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
							texture = mDevice->createTexture(texCreation);
							stbi_image_free(data);
						}
					},
					[&](fastgltf::sources::Array& array) {
						
						unsigned char* data = stbi_load_from_memory((unsigned char*)array.bytes.data() + bufferView.byteOffset,
						static_cast<int>(bufferView.byteLength),
						&width, &height, &nrChannels, 4);
						if (data)
						{
							TextureCreation texCreation{};
							texCreation.setData(data)
									   .setFlags(1, 0)
									   .setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
									   .setName(image.name.c_str())
									   .setSize(static_cast<u16>(width), static_cast<u16>(height), 1)
									   .setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
							texture = mDevice->createTexture(texCreation);
							stbi_image_free(data);
						}
					},
					[](fastgltf::sources::BufferView& view) {
						KS_CORE_ASSERT(false, "Unsupported buffer data source type");
					},
					[](fastgltf::sources::CustomBuffer& view) {
						KS_CORE_ASSERT(false, "Unsupported buffer data source type");
					},
					[](fastgltf::sources::ByteView& view) {
						KS_CORE_ASSERT(false, "Unsupported buffer data source type");
					},
					[](auto& view) {
						KS_CORE_ASSERT(false, "Unsupported buffer data source type");
					}
				},
				buffer.data);
			},
		},
		image.data);

	return texture;
}

KENSHIN_END