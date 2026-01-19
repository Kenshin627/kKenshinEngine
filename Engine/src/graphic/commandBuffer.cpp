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
	//if ( !is_recording )
	{
		mIsRecording = true;
		RenderPass* renderPass = mDevice->accessRenderPass(handle);

		// Begin/End render pass are valid only for graphics render passes.
		if (mCurrentRenderPass && (mCurrentRenderPass->type != RenderPassType::Compute) && (renderPass != mCurrentRenderPass)) 
		{
			vkCmdEndRenderPass(mCommandBuffer);
		}

		if (renderPass != mCurrentRenderPass && (renderPass->type != RenderPassType::Compute)) 
		{
			VkRenderPassBeginInfo renderPassBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			renderPassBegin.framebuffer = renderPass->type == RenderPassType::Swapchain ? mDevice->mVkSwapchainFramebuffers[mDevice->mVkImageIndex] : renderPass->vkFrameBuffer;
			renderPassBegin.renderPass = renderPass->vkRenderPass;
			renderPassBegin.renderArea.offset = { 0, 0 };
			renderPassBegin.renderArea.extent = { renderPass->width, renderPass->height };
			// TODO: this breaks.
			renderPassBegin.clearValueCount = 2;// render_pass->output.color_operation ? 2 : 0;
			renderPassBegin.pClearValues = mclearValue;
			vkCmdBeginRenderPass(mCommandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		}
		mCurrentRenderPass = renderPass;
	}
}

void CommandBuffer::bindPipeline(PipelineHandle pipelineHandle)
{
	Pipeline* pipeline = mDevice->accessPipeline(pipelineHandle);
	vkCmdBindPipeline(mCommandBuffer, pipeline->vkBindPoint, pipeline->vkPipeline);
	mCurrentPipeline = pipeline;
}

void CommandBuffer::bindVertexBuffer(BufferHandle handle, u32 firstBindingPoint)
{
	Buffer* buffer = mDevice->accessBuffer(handle);
	KS_CORE_ASSERT(buffer, "bindBuffer index handle is nullptr!");
	VkDeviceSize offsets[1] = { 0 };
	if (buffer->parentBufferHandle.index != InvalidIndex)
	{
		offsets[0] = buffer->globelBufferOffset;
		buffer = mDevice->accessBuffer(buffer->parentBufferHandle);
	}
	vkCmdBindVertexBuffers(mCommandBuffer, firstBindingPoint, 1, &buffer->vkBuffer, offsets);
}

void CommandBuffer::bindIndexBuffer(BufferHandle handle, VkIndexType indexType)
{
	Buffer* indexBuffer = mDevice->accessBuffer(handle);
	KS_CORE_ASSERT(indexBuffer, "bindBuffer index handle is nullptr!");
	VkDeviceSize bufferOffset{ 0 };
	if (indexBuffer->parentBufferHandle.index != InvalidIndex)
	{
		bufferOffset = indexBuffer->globelBufferOffset;
		indexBuffer = mDevice->accessBuffer(indexBuffer->parentBufferHandle);
	}
	vkCmdBindIndexBuffer(mCommandBuffer, indexBuffer->vkBuffer, bufferOffset, indexType);
}

void CommandBuffer::setViewport(Viewport* viewport)
{
	KS_CORE_ASSERT(viewport, "viewport is nullptr!");
	VkViewport vp;
	vp.x = viewport->rect.x;
	vp.y = viewport->rect.height - viewport->rect.y;
	vp.width = viewport->rect.width;
	vp.height = -viewport->rect.height;
	vp.minDepth = viewport->minDepth;
	vp.maxDepth = viewport->maxDepth;
	
	vkCmdSetViewport(mCommandBuffer, 0, 1, &vp);
}

void CommandBuffer::setScissor(const Rect2DInt* rect)
{
	KS_CORE_ASSERT(rect, "scissor is nullptr");
	VkRect2D vk_scissor;

	vk_scissor.offset.x = rect->x;
	vk_scissor.offset.y = rect->y;
	vk_scissor.extent.width = rect->width;
	vk_scissor.extent.height = rect->height;
	vkCmdSetScissor(mCommandBuffer, 0, 1, &vk_scissor);
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
	Buffer* drawBuffer = mDevice->accessBuffer(buffer);
	KS_CORE_ASSERT(drawBuffer, "drawBuffer is nullptr");
	vkCmdDrawIndirect(mCommandBuffer, drawBuffer->vkBuffer, { offset }, drawCount, stride);
}

void CommandBuffer::drawIndexIndirect(BufferHandle buffer, u64 offset, u32 drawCount, u32 stride)
{
	Buffer* drawBuffer = mDevice->accessBuffer(buffer);
	KS_CORE_ASSERT(drawBuffer, "drawBuffer is nullptr");
	vkCmdDrawIndexedIndirect(mCommandBuffer, drawBuffer->vkBuffer, { offset }, drawCount, stride);
}

void CommandBuffer::dispatch(const ComputeGroupSize& size)
{
	vkCmdDispatch(mCommandBuffer, size.xSize, size.ySize, size.zSize);
}

void CommandBuffer::dispatchIndirect(BufferHandle drawBuffer, u32 offset)
{
	Buffer* drawBuff = mDevice->accessBuffer(drawBuffer);
	KS_CORE_ASSERT(drawBuff, "drawBuffer is nullptr");
	vkCmdDispatchIndirect(mCommandBuffer, drawBuff->vkBuffer, { offset });
}

void CommandBuffer::bindDescriptorSet(DescriptorSetHandle* handles, u32 numLists, u32* offsets, u32 numOffsets)
{
	// TODO:
	u32 offsetsCache[8];
	numOffsets = 0;
	for (u32 i = 0; i < numLists; ++i)
	{
		DesciptorSet* descriptorSet = mDevice->accessDescriptorSet(handles[i]);
		mVkDescriptorSet[i] = descriptorSet->vkDescriptorSet;

		const DesciptorSetLayout* descriptorSetLayout = descriptorSet->layout;
		for (u32 j = 0; j < descriptorSetLayout->numBindings; ++j)
		{
			const DescriptorBinding& rb = descriptorSetLayout->bindings[j];
			//TODO:? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
			if (rb.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) 
			{
				const u32 index = descriptorSet->bindings[j];
				ResourceHandle bufferHandle = descriptorSet->resources[index];
				Buffer* buffer = mDevice->accessBuffer({ bufferHandle });
				offsetsCache[numOffsets++] = buffer->globelBufferOffset;
			}
		}
	}
	const u32 firstSet = 0;
	vkCmdBindDescriptorSets(mCommandBuffer, mCurrentPipeline->vkBindPoint, mCurrentPipeline->vkPipelineLayout, firstSet, numLists, mVkDescriptorSet, numOffsets, offsetsCache);
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
	Buffer* clearBuff = mDevice->accessBuffer(dstBuffer);
	KS_CORE_ASSERT(clearBuff, "drawBuffer is nullptr");
	vkCmdFillBuffer(mCommandBuffer, clearBuff->vkBuffer, { offset }, { size }, data);
}

void CommandBuffer::pushMarker(cstring name)
{
	mDevice->pushGpuTimestamp(this, name);
	if (!mDevice->mdebugUtilsExtensionPresent)
	{
		return;
	}
	mDevice->pushMarker(mCommandBuffer, name);
}

void CommandBuffer::popMarker()
{
	mDevice->popGpuTimestamp(this);
	if (!mDevice->mdebugUtilsExtensionPresent)
	{
		return;
	}
	mDevice->popMarker(mCommandBuffer);
}

KENSHIN_END
