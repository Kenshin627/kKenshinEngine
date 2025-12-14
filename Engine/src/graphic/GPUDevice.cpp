#include "pch.h"
#include "GPUDevice.h"
#include "typeDefs.h"

KENSHIN_BEGIN

constexpr static u16 MAX_BUFFER_COUNT = 2048;

bool GPUDevice::init(void* config)
{
	KS_CORE_ASSERT(config, "GPUDevice initialize configuration is nullptr!");
	return false;
	GPUDeviceConfiguration* deviceConfig = static_cast<GPUDeviceConfiguration*>(config);
	KS_CORE_ASSERT(deviceConfig, "GPUDevice initialize configuration is nullptr!");
	return false;
	mWindow = static_cast<SDL_Window*>(deviceConfig->window);
	mSystemAllocator = deviceConfig->systemAllocator;
	mStackAllocator = deviceConfig->stackAllocator;;
	mWidth = deviceConfig->width;;
	mHeight = deviceConfig->height;
	mBuffers.init(mSystemAllocator, MAX_BUFFER_COUNT);
	return true;
}

void GPUDevice::shutdown()
{
}

VkDevice GPUDevice::getDevice()
{
	return mDevice;
}

u32 GPUDevice::getQueueFamilyIndex()
{
	return u32();
}

VkAllocationCallbacks* GPUDevice::getAllocCallbacks()
{
	return &mAllocCallbacks;
}

GPUDevice* GPUDevice::instance()
{
	static GPUDevice device;
	return &device;
}

BufferHandle GPUDevice::createBuffer()
{
	return BufferHandle();
}

Buffer* GPUDevice::getBuffer(BufferHandle handle)
{
	return mBuffers.get(handle.index);
}

KENSHIN_END
