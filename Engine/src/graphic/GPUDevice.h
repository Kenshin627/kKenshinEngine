#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
KENSHIN_BEGIN

class GPUDevice: public Service
{
public:
	GPUDevice() = default;
	virtual ~GPUDevice() = default;
	virtual bool init(void* configuration = nullptr) override;
	virtual void shutdown() override;
	VkDevice getDevice();
	u32 getQueueFamilyIndex();
	VkAllocationCallbacks* getAllocCallbacks() const;
	KS_SERVICE_TYPE(GPUDevice);
	constexpr static cstring typeName = "GPU Device Service";
private:
	VkDevice mDevice;
	u32 mQueueFamilyIndex;
	VkAllocationCallbacks mAllocCallbacks;
};

KENSHIN_END