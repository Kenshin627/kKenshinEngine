#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "resourcePool.h"
#include "gpuResource.h"
#include "typeDefs.h"
#include "array.h"

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
	void setPresentMode(PresentMode::Enum mode);
	void createSwapchain();
	KS_SERVICE_TYPE(GPUDevice);
	constexpr static cstring typeName = "GPU Device Service";
private:
	VkDebugUtilsMessengerCreateInfoEXT buildDebugUtilsMessageCreateInfo();
	bool getQueuefamily(VkPhysicalDevice physicalDevice);
	VkPresentModeKHR toVkPresentMode(PresentMode::Enum mode);
private:
	VkDevice			       mDevice;
	VkInstance				   mVkInstance;
	VkAllocationCallbacks      mAllocCallbacks;
	ResourcePoolTyped<Buffer>  mBuffers;
	Allocator*				   mSystemAllocator;
	Allocator*				   mStackAllocator;
	SDL_Window*				   mWindow;

	//DebugUtils
	bool							 mDebugUtilsMessagePresent{ false };
	VkDebugUtilsMessengerEXT		 mDebugMessage;
	PFN_vkCmdBeginDebugUtilsLabelEXT mDebugUtilsBeginLabel;
	PFN_vkCmdEndDebugUtilsLabelEXT	 mDebugUtilsEndLabel;
	PFN_vkSetDebugUtilsObjectNameEXT mDebugUtilsSetObjectName;
					
	//present
	VkSurfaceKHR					 mSurface;
	VkSurfaceFormatKHR				 mSurfaceFormat;
	VkPresentModeKHR				 mVkPresentMode;
	PresentMode::Enum				 mPresentMode{ PresentMode::Enum::VsyncFast }; 

	//swapchain
	VkSwapchainKHR					 mSwapChain;
	Array<VkImage>					 mSwapchainImages;
	Array<VkImageView>				 mSwapchainImageViews;
	u32							     mSwapchainWidth;
	u32							     mSwapchainHeight;
	u32								 mSwapchainImageCount;

	VkPhysicalDeviceProperties		 mPhysicalDeviceProperties;
	u32								 mQueueFamilyIndex;
	VkQueue							 mQueue;
	VkPhysicalDevice				 mPhysicalDevice;
	u32								 minSSBOAlignment;
	u32								 minUBOAlignment;

	//vma
	VmaAllocator					 mVmaAllocator;
};

KENSHIN_END