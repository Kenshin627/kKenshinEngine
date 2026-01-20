#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "commandBuffer.h"
#include "array.h"
#include "gpuResource.h"

KENSHIN_BEGIN

class GPUDevice;
class Allocator;

struct CommandBufferServiceConfiguration
{
	GPUDevice* gpuDevice			    { nullptr };
	Allocator* systemAllocator			{ nullptr };
};

class CommandBufferService : public Service
{
public:
	virtual bool init(void* config = nullptr) override;
	virtual void shutdown() override;
	void resetCommandPool(u8 framIndex);
	CommandBuffer* getCommandBuffer(u8 frame, bool beginRecord);
	CommandBuffer* getCommandBufferInstant(u8 frame, bool beginRecord);
	u8 getCommandPoolIndex(u8 commandBufferIndex);
	KS_SERVICE_TYPE(CommandBufferService);
	constexpr static cstring typeName = "commandBuffer Service";
private:
	u8				       mThreadCount = MaxThreadCount;
	u8				       mCommandPoolCount = MaxInFlightFrames * MaxThreadCount;
	u8				       mCommandBuffersPerPool = CommandBufferCountPerPool;
	u8				       mCommandBufferCount = MaxInFlightFrames * MaxThreadCount * CommandBufferCountPerPool;
	GPUDevice*	           mDevice;
	Array<CommandBuffer>   mCommandBuffers;/*[mCommandBufferCount];*/
	Array<VkCommandPool>   mCommandPools;/*[mCommandPoolCount];*/
};

KENSHIN_END