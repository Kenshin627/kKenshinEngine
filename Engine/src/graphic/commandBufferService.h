#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "commandBuffer.h"
#include "array.h"

KENSHIN_BEGIN

class Allocator;
class GPUDevice;
struct CommandBufferServiceConfiguration
{
	u8		   frameCount				{ 3		  };
	u8		   threadCount				{ 4		  };
	u8		   commandBufferCountPerPool{ 4		  };
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
	Array<CommandBuffer> mCommandBuffers;
	Array<VkCommandPool> mCommandPools  ;
	u8					 mCommandPoolCount		{ 0		  };
	u8					 mCommandBuffersPerPool { 0		  };
	u8				     mThreadCount			{ 0		  };
	u8					 mCommandBufferCount	{ 0		  };
	GPUDevice*			 mDevice				{ nullptr };
};

KENSHIN_END