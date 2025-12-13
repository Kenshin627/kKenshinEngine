#include "pch.h"
#include "commandBuffer.h"

KENSHIN_BEGIN

void CommandBuffer::init(GPUDevice* device, u8 bufferIndex)
{
	mDevice = device;
	mBufferIndex = bufferIndex;
}

void CommandBuffer::reset()
{
	mIsRecording = false;
}

void CommandBuffer::beginRecord()
{
	VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mCommandBuffer, &info));
}

void CommandBuffer::endRecord()
{
	VK_CHECK(vkEndCommandBuffer(mCommandBuffer));
}

KENSHIN_END
