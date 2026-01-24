#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "gpuResource.h"
#include "typeDefs.h"

KENSHIN_BEGIN

class GPUDevice;
struct CommandBuffer
{
	////////////////////////////////////////////////////////////////////////////////////////////
	void init(GPUDevice* device, u8 bufferIndex);
	void reset();
	void beginRecord();
	void endRecord();
	void bindRenderPass(RenderPassHandle handle);
	void bindPipeline(PipelineHandle handle);
	void bindVertexBuffer(BufferHandle handle, u32 firstBindingPoint);
	void bindIndexBuffer(BufferHandle handle, VkIndexType indexType);
	void setViewport(Viewport* viewport);
	void setScissor(const Rect2DInt* rect);
	void draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
	void drawIndex(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);
	void drawIndirect(BufferHandle buffer, u64 offset, u32 drawCount, u32 stride);
	void drawIndexIndirect(BufferHandle buffer, u64 offset, u32 drawCount, u32 stride);
	void dispatch(const ComputeGroupSize& size);
	void dispatchIndirect(BufferHandle drawBuffer, u32 offset);
	void bindDescriptorSet(DescriptorSetHandle* handles, u32 numLists, u32* offsets, u32 numOffsets);
	void setClearColor(f32 r, f32 g, f32 b, f32 a);
	void setClearDepth(f32 clearDepth);
	void setClearStencil(u32 clearStencil);
	void fillBuffer(BufferHandle dstBuffer, u32 size, u32 offset, u32 data);
	void pushMarker(cstring name);
	void popMarker();
	void blitImage(VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
	void beginDynamicRendering(TextureHandle* colorAttachments, u32 numColorAttachments, VkRect2D range,  TextureHandle* depthAttachment = nullptr, TextureHandle* stencilAttachment = nullptr);
	void endDynamicRendering();
	void pushConstant(VkShaderStageFlags stage, u32 offset, u32 size, const void* data);
	////////////////////////////////////////////////////////////////////////////////////////////
	VkCommandBuffer mCommandBuffer;
	GPUDevice*		mDevice{ nullptr };
	u8				mBufferIndex{0};
	bool			mIsRecording{ false };
	VkClearValue	mclearValue[2];
	VkDescriptorSet mVkDescriptorSet[MaxDescriptorsPerSet];
	RenderPass*		mCurrentRenderPass{nullptr};
	Pipeline*		mCurrentPipeline{nullptr};
	bool		    mIsDynamicRendering{ false };
	////////////////////////////////////////////////////////////////////////////////////////////
};

KENSHIN_END