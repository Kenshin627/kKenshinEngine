#pragma once
#include "platform.h"
#include <deque>
#include <vector>

struct VkDescriptorImageInfo;
struct VkDescriptorBufferInfo;
struct VkWriteDescriptorSet;
enum   VkImageLayout;
enum   VkDescriptorType;

KENSHIN_BEGIN

class GPUDevice;

class DescriptorWriter
{
public:
	DescriptorWriter(GPUDevice* device);
	~DescriptorWriter() = default;
	void addImage(u32 bindingPoint, TextureHandle textureHandle, SamplerHandle samplerHandle, VkImageLayout imageLayout, VkDescriptorType type);
	void addBuffer(u32 bindingPoint, BufferHandle bufferHandle, u64 offset, u64 size, VkDescriptorType type);
	void writeDescriptorSet(DescriptorSetHandle dsHandle);
	void clear();
private:
	std::deque<VkDescriptorImageInfo>  mImages;
	std::deque<VkDescriptorBufferInfo> mBuffers;
	std::vector<VkWriteDescriptorSet>  mWriters;
	GPUDevice* mDevice{ nullptr };
};

KENSHIN_END