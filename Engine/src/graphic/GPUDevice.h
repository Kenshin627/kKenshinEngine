#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "resourcePool.h"
#include "gpuResource.h"

struct SDL_Window;

KENSHIN_BEGIN

class Buffer;
struct GPUDeviceConfiguration
{
	u32 width;
	u32 height;
	Allocator* systemAllocator;
	Allocator* stackAllocator;
	void* window;
	VkAllocationCallbacks allocationCallbacks;
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
	VkDebugUtilsMessengerCreateInfoEXT buildDebugUtilsMessageCreateInfo();
	bool getQueuefamily(VkPhysicalDevice physicalDevice);
private:
	VkDevice			       mDevice;
	VkInstance				   mVkInstance;
	VkAllocationCallbacks      mAllocCallbacks;
	ResourcePoolTyped<Buffer>  mBuffers;
	Allocator*				   mSystemAllocator;
	Allocator*				   mStackAllocator;
	SDL_Window*				   mWindow;
	u32						   mWidth;
	u32						   mHeight;
	bool					   mDebugUtilsMessagePresent{ false };
	VkDebugUtilsMessengerEXT   mDebugMessage;
							   
	VkSurfaceKHR			   mSurface;
	VkPhysicalDeviceProperties mPhysicalDeviceProperties;
	u32						   mQueueFamilyIndex;
	VkPhysicalDevice		   mPhysicalDevice;
	u32						   minSSBOAlignment;
	u32						   minUBOAlignment;
};

KENSHIN_END