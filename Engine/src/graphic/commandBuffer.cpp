#include "pch.h"
#include "commandBuffer.h"
#include "GPUDevice.h"

KENSHIN_BEGIN

void CommandBuffer::init(GPUDevice* device, u8 bufferIndex)
{
	mDevice = device;
	mBufferIndex = bufferIndex;
	mCurrentRenderPass = nullptr;
	mCurrentPipeline = nullptr;
}

void CommandBuffer::reset()
{
	mIsRecording = false;
	mCurrentRenderPass = nullptr;
	mCurrentPipeline = nullptr;
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
			renderPassBegin.framebuffer = renderPass->type == RenderPassType::Swapchain ? mDevice->mVkSwapchainFramebuffers: renderPass->vkFrameBuffer;
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

void CommandBuffer::blitImage(VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };
	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(mCommandBuffer, &blitInfo);
}

void CommandBuffer::beginDynamicRendering(TextureHandle* colorAttachments, u32 numColorAttachments, VkRect2D range, TextureHandle* depthAttachment, TextureHandle* stencilAttachment)
{
	VkRenderingInfo renderInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .pNext = nullptr };
	Array< VkRenderingAttachmentInfo> colorAttachmentInfos;
	colorAttachmentInfos.init(mDevice->mSystemAllocator, numColorAttachments);
	for (size_t i = 0; i < numColorAttachments; ++i)
	{
		Texture* colorAttachment = mDevice->accessTexture(colorAttachments[i]);
		VkRenderingAttachmentInfo attachInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr };
		attachInfo.clearValue = mclearValue[0];
		attachInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachInfo.imageView = colorAttachment->vkImageView;
		//TODO: LOAD_OP_CLEAR
		attachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfos.pushBack(attachInfo);
	}
	renderInfo.colorAttachmentCount = numColorAttachments;
	renderInfo.pColorAttachments = colorAttachmentInfos.data();

	if (depthAttachment)
	{
		Texture* depthAtt = mDevice->accessTexture(*depthAttachment);
		VkRenderingAttachmentInfo depthAttachInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr };
		depthAttachInfo.clearValue = mclearValue[1];
		depthAttachInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthAttachInfo.imageView = depthAtt->vkImageView;
		depthAttachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderInfo.pDepthAttachment = &depthAttachInfo;
	}

	if(stencilAttachment)
	{
		Texture* stencilAtt = mDevice->accessTexture(*stencilAttachment);
		VkRenderingAttachmentInfo stencilAttachInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr };
		stencilAttachInfo.clearValue = mclearValue[1];
		stencilAttachInfo.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
		stencilAttachInfo.imageView = stencilAtt->vkImageView;
		//TODO:??
		stencilAttachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		stencilAttachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderInfo.pStencilAttachment = &stencilAttachInfo;
	}
	renderInfo.renderArea = range;
	renderInfo.layerCount = 1;
	mIsDynamicRendering = true;
	vkCmdBeginRendering(mCommandBuffer, &renderInfo);
}

void CommandBuffer::endDynamicRendering()
{
	if (mIsDynamicRendering)
	{
		vkCmdEndRendering(mCommandBuffer);
		mIsDynamicRendering = false;
	}
}

void CommandBuffer::pushConstant(VkShaderStageFlags stage, u32 offset, u32 size, const void* data)
{
	VkPushConstantsInfo info{ .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO, .pNext = nullptr };
	info.layout = mCurrentPipeline->vkPipelineLayout;
	info.offset = offset;
	info.pValues = data;
	info.size = size;
	info.stageFlags = stage;
	//vkCmdPushConstants2(mCommandBuffer, &info);
	vkCmdPushConstants(mCommandBuffer, mCurrentPipeline->vkPipelineLayout, stage, offset, size, data);
}

KENSHIN_END
