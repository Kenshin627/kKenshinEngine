#include "pch.h"
#include "commandBufferService.h"
#include "GPUDevice.h"

KENSHIN_BEGIN

bool CommandBufferService::init(void* config)
{
	KS_CORE_ASSERT(config, "cmdService configuration is nullptr!");
	KS_CORE_INFO("Initializing CommandBuffer Service.");
	CommandBufferServiceConfiguration* cmdConfig = static_cast<CommandBufferServiceConfiguration*>(config);
	KS_CORE_ASSERT(cmdConfig, "cmdService configuration is nullptr!");
	mCommandPools.init(cmdConfig->systemAllocator, mCommandPoolCount, mCommandPoolCount);
	mCommandBuffers.init(cmdConfig->systemAllocator, mCommandBufferCount, mCommandBufferCount);
	mDevice = cmdConfig->gpuDevice;
	VkDevice vkDevice = mDevice->getDevice();
	for (size_t i = 0; i < mCommandPoolCount; i++)
	{
		VkCommandPoolCreateInfo info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = mDevice->getQueueFamilyIndex();
		VK_CHECK(vkCreateCommandPool(vkDevice, &info, mDevice->getAllocCallbacks(), &mCommandPools[i]));
	}

	for (size_t i = 0; i < mCommandBufferCount; i++)
	{
		VkCommandBufferAllocateInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandBufferCount = 1;
		info.commandPool = mCommandPools[getCommandPoolIndex(i)];
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK(vkAllocateCommandBuffers(vkDevice, &info, &mCommandBuffers[i].mCommandBuffer));
		mCommandBuffers[i].init(mDevice, i);
	}
}

void CommandBufferService::shutdown()
{
	for (size_t i = 0; i < mCommandPoolCount; i++)
	{
		vkDestroyCommandPool(mDevice->getDevice(), mCommandPools[i], mDevice->getAllocCallbacks());
	}
}

void CommandBufferService::resetCommandPool(u8 framIndex)
{
	for (size_t i = 0; i < mThreadCount; i++)
	{
		VK_CHECK(vkResetCommandPool(mDevice->getDevice(), mCommandPools[framIndex * mThreadCount + i], 0));
	}
}

CommandBuffer* CommandBufferService::getCommandBuffer(u8 frame, bool beginRecord)
{
	CommandBuffer* cmdBuffer = &mCommandBuffers[frame * mCommandBuffersPerPool];
	if (beginRecord)
	{
		cmdBuffer->reset();
		cmdBuffer->beginRecord();
	}
	return cmdBuffer;
}

CommandBuffer* CommandBufferService::getCommandBufferInstant(u8 frame, bool beginRecord)
{
	CommandBuffer* cmdBuffer = &mCommandBuffers[frame * mCommandBuffersPerPool + 1];
	if (beginRecord)
	{
		cmdBuffer->reset();
		cmdBuffer->beginRecord();
	}
	return cmdBuffer;
}

u8 CommandBufferService::getCommandPoolIndex(u8 commandBufferIndex)
{
	return commandBufferIndex / mCommandBuffersPerPool;
}

CommandBufferService* CommandBufferService::instance()
{
	static CommandBufferService sInstance;
	return &sInstance;
}

KENSHIN_END
