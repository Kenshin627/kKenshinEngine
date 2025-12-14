#include "pch.h"
#include "commandBuffer.h"
#include "GPUDevice.h"

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
	mIsRecording = true;
}

void CommandBuffer::endRecord()
{
	VK_CHECK(vkEndCommandBuffer(mCommandBuffer));
	mIsRecording = false;
}

void CommandBuffer::bindRenderPass(RenderPassHandle handle)
{
}

void CommandBuffer::bindPipeline(PipelineHandle handle)
{
	//vkCmdBindPipeline()
}

void CommandBuffer::bindVertexBuffer(BufferHandle handle, u32 firstBinding)
{
	Buffer* buf = mDevice->getBuffer(handle);
	KS_CORE_ASSERT(buf, "bindBuffer index handle is nullptr!");
	VkDeviceSize offsets[1] = { 0 };
	if (buf->mParentHandle.index != InvalidIndex)
	{
		buf = mDevice->getBuffer(buf->mParentHandle);
		offsets[0] = buf->offset;
	}
	vkCmdBindVertexBuffers(mCommandBuffer, firstBinding, 1, &buf->vkBuffer, offsets);
}

void CommandBuffer::bindIndexBuffer(BufferHandle handle, VkIndexType indexType)
{
	Buffer* indexBuffer = mDevice->getBuffer(handle);
	KS_CORE_ASSERT(indexBuffer, "bindBuffer index handle is nullptr!");
	VkDeviceSize bufferOffset{ 0 };
	if (indexBuffer->mParentHandle.index != InvalidIndex)
	{
		indexBuffer = mDevice->getBuffer(indexBuffer->mParentHandle);
		bufferOffset = indexBuffer->offset;
	}
	vkCmdBindIndexBuffer(mCommandBuffer, indexBuffer->vkBuffer, bufferOffset, indexType);
}

void CommandBuffer::setViewport(Viewport* viewport)
{
	KS_CORE_ASSERT(viewport, "viewport is nullptr!");
	VkViewport vp;
	vp.x = viewport->x;
	vp.y = viewport->height - viewport->y;
	vp.width = viewport->width;
	vp.height = -viewport->height;
	vp.minDepth = viewport->minDepth;
	vp.maxDepth = viewport->maxDepth;
	
	vkCmdSetViewport(mCommandBuffer, 0, 1, &vp);
}

void CommandBuffer::setScissor(Scissor* scissor)
{
	KS_CORE_ASSERT(scissor, "scissor is nullptr");
	VkRect2D rect;
	rect.offset.x = scissor->offsetX;
	rect.offset.y = scissor->offsetY;
	rect.extent.width = scissor->width;
	rect.extent.height = scissor->height;
	vkCmdSetScissor(mCommandBuffer, 0, 1, &rect);
}

void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
	vkCmdDraw(mCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndex(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
	vkCmdDrawIndexed(mCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::drawIndirect(BufferHandle buffer, u64 offset, u32 drawCount, u32 stride)
{
	Buffer* drawBuffer = mDevice->getBuffer(buffer);
	KS_CORE_ASSERT(drawBuffer, "drawBuffer is nullptr");
	vkCmdDrawIndirect(mCommandBuffer, drawBuffer->vkBuffer, { offset }, drawCount, stride);
}

void CommandBuffer::drawIndexIndirect(BufferHandle buffer, u64 offset, u32 drawCount, u32 stride)
{
	Buffer* drawBuffer = mDevice->getBuffer(buffer);
	KS_CORE_ASSERT(drawBuffer, "drawBuffer is nullptr");
	vkCmdDrawIndexedIndirect(mCommandBuffer, drawBuffer->vkBuffer, { offset }, drawCount, stride);
}

void CommandBuffer::dispatch(const ComputeGroupSize& size)
{
	vkCmdDispatch(mCommandBuffer, size.xSize, size.ySize, size.zSize);
}

void CommandBuffer::dispatchIndirect(BufferHandle drawBuffer, u32 offset)
{
	Buffer* drawBuff = mDevice->getBuffer(drawBuffer);
	KS_CORE_ASSERT(drawBuff, "drawBuffer is nullptr");
	vkCmdDispatchIndirect(mCommandBuffer, drawBuff->vkBuffer, { offset });
}

void CommandBuffer::bindDescriptorSet()
{
}

void CommandBuffer::setClearColor(f32 r, f32 g, f32 b, f32 a)
{
	mclearValue[0].color = { r, g, b, a };
}

void CommandBuffer::setClearDepth(f32 clearDepth)
{
	mclearValue[1].depthStencil.depth = clearDepth;
}

void CommandBuffer::setClearStencil(u32 clearStencil)
{
	mclearValue[1].depthStencil.stencil = clearStencil;
}

void CommandBuffer::fillBuffer(BufferHandle dstBuffer, u32 size, u32 offset, u32 data)
{
	Buffer* clearBuff = mDevice->getBuffer(dstBuffer);
	KS_CORE_ASSERT(clearBuff, "drawBuffer is nullptr");
	vkCmdFillBuffer(mCommandBuffer, clearBuff->vkBuffer, { offset }, { size }, data);
}

void CommandBuffer::pushMarker()
{
}

void CommandBuffer::popMarker()
{
}

KENSHIN_END
