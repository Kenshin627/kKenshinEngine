#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "resourcePool.h"
KENSHIN_BEGIN

class Buffer;
struct SDL_Window;
struct GPUDeviceConfiguration
{
	u32 width;
	u32 height;
	Allocator* systemAllocator;
	Allocator* stackAllocator;
	void* window;
};

class GPUDevice: public Service
{
public:
	GPUDevice() = default;
	virtual ~GPUDevice() = default;
	virtual bool init(void* config = nullptr) override;
	virtual void shutdown() override;
	VkDevice getDevice();
	u32 getQueueFamilyIndex();
	VkAllocationCallbacks* getAllocCallbacks();
	BufferHandle createBuffer();
	Buffer* getBuffer(BufferHandle handle);
	KS_SERVICE_TYPE(GPUDevice);
	constexpr static cstring typeName = "GPU Device Service";
private:
	VkDevice			      mDevice;
	u32					      mQueueFamilyIndex;
	VkAllocationCallbacks     mAllocCallbacks;
	ResourcePoolTyped<Buffer> mBuffers;
	Allocator*				  mSystemAllocator;
	Allocator*				  mStackAllocator;
	SDL_Window*				  mWindow;
	u32						  mWidth;
	u32						  mHeight;
};

KENSHIN_END