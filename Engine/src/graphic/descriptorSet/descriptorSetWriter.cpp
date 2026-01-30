#include "pch.h"
#include <vulkan/vulkan.h>
#include "descriptorSetWriter.h"
#include "gpuDevice.h"

KENSHIN_BEGIN

DescriptorWriter::DescriptorWriter(GPUDevice* device)
	:mDevice(device)
{
}

void DescriptorWriter::addImage(u32 bindingPoint, TextureHandle textureHandle, SamplerHandle samplerHandle, VkImageLayout layout, VkDescriptorType type)
{
	Texture* tex = mDevice->accessTexture(textureHandle);
	Sampler* samp = mDevice->accessSampler(samplerHandle);

	VkDescriptorImageInfo& imageInfo = mImages.emplace_back(VkDescriptorImageInfo{
		.sampler = samp->vkSampler,
		.imageView = tex->vkImageView,
		.imageLayout = layout
	});

	VkWriteDescriptorSet writer{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr };
	writer.descriptorCount = 1;
	writer.descriptorType  = type;
	writer.dstArrayElement = 0;
	writer.dstBinding	   = bindingPoint;
	writer.dstSet		   = 0;
	writer.pImageInfo      = &imageInfo;
	writer.pBufferInfo	   = nullptr;
	mWriters.push_back(writer);
}

void DescriptorWriter::addBuffer(u32 bindingPoint,  BufferHandle bufferHandle, u64 offset, u64 size, VkDescriptorType type)
{
	Buffer* buffer = mDevice->accessBuffer(bufferHandle);
	VkDescriptorBufferInfo& bufferInfo = mBuffers.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer->vkBuffer,
		.offset = offset,
		.range = size
	});

	VkWriteDescriptorSet writer{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr };
	writer.descriptorCount = 1;
	writer.descriptorType  = type;
	writer.dstArrayElement = 0;
	writer.dstBinding	   = bindingPoint;
	writer.dstSet		   = 0;
	writer.pBufferInfo	   = &bufferInfo;
	writer.pImageInfo	   = nullptr;
	mWriters.push_back(writer);
}

void DescriptorWriter::writeDescriptorSet(DescriptorSetHandle dsHandle)
{
	DesciptorSet* ds = mDevice->accessDescriptorSet(dsHandle);
	for (auto& w : mWriters)
	{
		w.dstSet = ds->vkDescriptorSet;
	}
	vkUpdateDescriptorSets(mDevice->getDevice(), mWriters.size(), mWriters.data(), 0, nullptr);
}

void DescriptorWriter::clear()
{
	mImages.clear();
	mBuffers.clear();
	mWriters.clear();
}

KENSHIN_END
