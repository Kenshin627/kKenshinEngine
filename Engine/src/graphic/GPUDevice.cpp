#include "pch.h"
#include "GPUDevice.h"

KENSHIN_BEGIN

bool GPUDevice::init(void* configuration)
{
	return true;
}

void GPUDevice::shutdown()
{
}

VkDevice GPUDevice::getDevice()
{
	return VkDevice();
}

u32 GPUDevice::getQueueFamilyIndex()
{
	return u32();
}

VkAllocationCallbacks* GPUDevice::getAllocCallbacks() const
{
	return nullptr;
}

KENSHIN_END
