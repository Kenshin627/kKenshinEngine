#include "pch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "GPUDevice.h"
#include "typeDefs.h"

KENSHIN_BEGIN

constexpr static u16 MAX_BUFFER_COUNT = 2048;

static const char* extensions[] =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static const char* layers[] =
{
	"VK_LAYER_KHRONOS_validation"
};

static VkBool32 vkDebugUtilsMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	//TODO
	return false;
}

bool GPUDevice::init(void* config)
{
	//0. assign initial Data from configuration
	KS_CORE_ASSERT(config, "GPUDevice initialize configuration is nullptr!");
	return false;
	GPUDeviceConfiguration* deviceConfig = static_cast<GPUDeviceConfiguration*>(config);
	KS_CORE_ASSERT(deviceConfig, "GPUDevice initialize configuration is nullptr!");
	return false;
	mWindow = static_cast<SDL_Window*>(deviceConfig->window);
	mSystemAllocator = deviceConfig->systemAllocator;
	mStackAllocator = deviceConfig->stackAllocator;
	mAllocCallbacks = deviceConfig->allocationCallbacks;
	mSwapchainWidth = deviceConfig->width;;
	mSwapchainHeight = deviceConfig->height;
	
	//1. vkinstance
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "kEngine";
	appInfo.pEngineName = "kEngine";
	appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);
	VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = buildDebugUtilsMessageCreateInfo();
	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.enabledExtensionCount = ArraySize(extensions);
	instanceInfo.ppEnabledExtensionNames = extensions;
	instanceInfo.enabledLayerCount = ArraySize(layers);
	instanceInfo.ppEnabledLayerNames = layers;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.pNext = &debugMessageInfo;
	VK_CHECK(vkCreateInstance(&instanceInfo, &mAllocCallbacks, &mVkInstance));
	u32 extensionCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	VkExtensionProperties* extensions = static_cast<VkExtensionProperties*>(kalloca(extensionCount * sizeof(VkExtensionProperties), mSystemAllocator));
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions));
	for (size_t i = 0; i < extensionCount; i++)
	{
		const VkExtensionProperties& extension = extensions[i];
		if (!strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		{
			mDebugUtilsMessagePresent = true;
			break;
		}
	}
	if (mDebugUtilsMessagePresent)
	{
		PFN_vkCreateDebugUtilsMessengerEXT createDebugFunction = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(mVkInstance, "vkCreateDebugUtilsMessengerEXT"));
		VkDebugUtilsMessengerCreateInfoEXT utilsMessageInfo = buildDebugUtilsMessageCreateInfo();
		createDebugFunction(mVkInstance, &utilsMessageInfo, &mAllocCallbacks, &mDebugMessage);
	}
	kfree(extensions, mSystemAllocator);

	//2. surface
	bool  createSurfaceRes = SDL_Vulkan_CreateSurface(mWindow, mVkInstance, &mAllocCallbacks, &mSurface);
	KS_CORE_ASSERT(createSurfaceRes, "sdl create vulkan surface failed!");
	return false;

	//3. physicalDevice
	u32 physicalDeviceCount;
	VkPhysicalDevice discreteGpu{ nullptr };
	VkPhysicalDevice integratedGpu{ nullptr };
	VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, nullptr));
	VkPhysicalDevice* physicalDevices = static_cast<VkPhysicalDevice*>(kalloca(physicalDeviceCount * sizeof(VkPhysicalDevice), mSystemAllocator));
	VK_CHECK(vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, physicalDevices));
	for (size_t i = 0; i < physicalDeviceCount; i++)
	{
		const VkPhysicalDevice& physicalDevice = physicalDevices[i];
		vkGetPhysicalDeviceProperties(physicalDevice, &mPhysicalDeviceProperties);
		if (mPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			if (getQueuefamily(physicalDevice))
			{
				discreteGpu = physicalDevice;
				break;
			}
		}
		else if(mPhysicalDeviceProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			if (getQueuefamily(physicalDevice))
			{
				integratedGpu = physicalDevice;
			}
		}
	}
	if (discreteGpu)
	{
		mPhysicalDevice = discreteGpu;
	}
	else if (integratedGpu)
	{
		mPhysicalDevice = integratedGpu;
	}
	else
	{
		KS_CORE_ERROR("no gpu supported!");
		return false;
	}
	kfree(physicalDevices, mSystemAllocator);
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceProperties);
	minSSBOAlignment = mPhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
	minUBOAlignment = mPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

	//4. device
	VkDeviceCreateInfo deviceInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	float priority[] = {1.0};
	VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.flags = 0;
	queueInfo.pQueuePriorities = priority;
	queueInfo.queueFamilyIndex = mQueueFamilyIndex;
	queueInfo.queueCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	cstring deviceExtensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;
	VkPhysicalDeviceFeatures2 feature2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &feature2);
	deviceInfo.pNext = &feature2;
	VK_CHECK(vkCreateDevice(mPhysicalDevice, &deviceInfo, &mAllocCallbacks, &mDevice));

	if (mDebugUtilsMessagePresent)
	{
		PFN_vkCmdBeginDebugUtilsLabelEXT mDebugUtilsBeginLabel =  (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(mVkInstance, "vkCmdBeginDebugUtilsLabelEXT");
		PFN_vkCmdEndDebugUtilsLabelEXT mDebugUtilsEndLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(mVkInstance, "vkCmdEndDebugUtilsLabelEXT");
		PFN_vkSetDebugUtilsObjectNameEXT mDebugUtilsSetObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(mVkInstance, "vkSetDebugUtilsObjectNameEXT");
	}

	//5. Queue
	vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);

	//6. swapChain
	u32 formatCount;
	bool isFoundFormat{ false };
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr));
	VkSurfaceFormatKHR* surfaceFormats = static_cast<VkSurfaceFormatKHR*>(kalloca(formatCount * sizeof(VkSurfaceFormatKHR), mSystemAllocator));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, surfaceFormats));
	VkFormat supportedFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	u32 supportedFormatCount = ArraySize(supportedFormats);
	VkColorSpaceKHR supportedColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	for (size_t i = 0; i < formatCount; ++i)
	{
		for (size_t j = 0; j < supportedFormatCount; ++j)
		{
			if (surfaceFormats[i].format == supportedFormats[j] && surfaceFormats[i].colorSpace == supportedColorSpace)
			{
				isFoundFormat = true;
				mSurfaceFormat = surfaceFormats[i];
				break;
			}
		}
		if (isFoundFormat)
		{
			break;
		}
	}

	if (!isFoundFormat)
	{
		mSurfaceFormat = surfaceFormats[0];
		KS_CORE_WARN("no surfaceFormat is found, check you device please!");
	}
	kfree(surfaceFormats, mSystemAllocator);

	//7. presentMode
	setPresentMode(mPresentMode);

	//8. swapchain
	createSwapchain();

	//9. vmaInit
	VmaAllocatorCreateInfo vmaInfo;
	vmaInfo.device = mDevice;
	vmaInfo.flags = 0;
	vmaInfo.instance = mVkInstance;
	vmaInfo.physicalDevice = mPhysicalDevice;
	vmaInfo.vulkanApiVersion = VK_MAKE_VERSION(1, 3, 0);
	vmaCreateAllocator(&vmaInfo, &mVmaAllocator);

	//10. buffers samplers inits
	mBuffers.init(mSystemAllocator, MAX_BUFFER_COUNT);
	//11. descriptorPool

	//12. semaphore / fence


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
	return mQueueFamilyIndex;
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
	//TODO
	return { 0 };
}

Buffer* GPUDevice::getBuffer(BufferHandle handle)
{
	return mBuffers.get(handle.index);
}

void GPUDevice::setPresentMode(PresentMode::Enum mode)
{
	bool isFoundPresentMode{ false };
	VkPresentModeKHR vkMode = toVkPresentMode(mode);
	u32 presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentModeCount, nullptr));
	VkPresentModeKHR* presentModes = static_cast<VkPresentModeKHR*>(kalloca(presentModeCount * sizeof(VkPresentModeKHR), mSystemAllocator));
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentModeCount, presentModes));
	for (size_t i = 0; i < presentModeCount; i++)
	{
		if (presentModes[i] == vkMode)
		{
			isFoundPresentMode = true;
			break;
		}
	}
	if (isFoundPresentMode)
	{
		mVkPresentMode = vkMode;
	}
	else
	{
		mVkPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		mPresentMode = PresentMode::Enum::Vsync;
	} 
	mSwapchainImageCount = 3;
	kfree(presentModes, mSystemAllocator);
}

void GPUDevice::createSwapchain()
{
	VkSurfaceCapabilitiesKHR capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &capabilities));
	if (capabilities.currentExtent.width == u32_max)
	{
		capabilities.currentExtent.width = std::clamp(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		capabilities.currentExtent.height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	mSwapchainWidth = capabilities.currentExtent.width;
	mSwapchainHeight = capabilities.currentExtent.height;
	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.clipped = true;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.flags = 0;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageColorSpace = mSurfaceFormat.colorSpace;
	swapchainInfo.imageExtent = capabilities.currentExtent;
	swapchainInfo.imageFormat = mSurfaceFormat.format;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainInfo.minImageCount = mSwapchainImageCount;
	swapchainInfo.pQueueFamilyIndices = &mQueueFamilyIndex;
	swapchainInfo.queueFamilyIndexCount = 1;
	swapchainInfo.presentMode = mVkPresentMode;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.surface = mSurface;
	VK_CHECK(vkCreateSwapchainKHR(mDevice, &swapchainInfo, &mAllocCallbacks, &mSwapChain));

	mSwapchainImages.init(mSystemAllocator, mSwapchainImageCount, mSwapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(mDevice, mSwapChain, &mSwapchainImageCount, mSwapchainImages.data()));

	mSwapchainImageViews.init(mSystemAllocator, mSwapchainImageCount, mSwapchainImageCount);
	for (size_t i = 0; i < mSwapchainImageCount; i++)
	{
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A;
		viewInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B;
		viewInfo.flags = 0;
		viewInfo.format = mSurfaceFormat.format;
		viewInfo.image = mSwapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkCreateImageView(mDevice, &viewInfo, &mAllocCallbacks, &mSwapchainImageViews[i]);
	}
}

VkDebugUtilsMessengerCreateInfoEXT GPUDevice::buildDebugUtilsMessageCreateInfo()
{
	VkDebugUtilsMessengerCreateInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	info.flags = 0;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	info.pfnUserCallback = vkDebugUtilsMessageCallback;	
	return info;
}

bool GPUDevice::getQueuefamily(VkPhysicalDevice physicalDevice)
{
	u32 queuefamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuefamilyCount, nullptr);
	VkQueueFamilyProperties* queuefamilies = static_cast<VkQueueFamilyProperties*>(kalloca(queuefamilyCount * sizeof(VkQueueFamilyProperties), mSystemAllocator));
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuefamilyCount, queuefamilies);
	for (size_t i = 0; i < queuefamilyCount; i++)
	{
		const VkQueueFamilyProperties& queueFamily = queuefamilies[i];
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
		{
			mQueueFamilyIndex = i;
			return true;
		}
	}
	kfree(queuefamilies, mSystemAllocator);
	return false;
}

VkPresentModeKHR GPUDevice::toVkPresentMode(PresentMode::Enum mode)
{
	switch(mode)
	{
	case PresentMode::Enum::Immediate:
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	case PresentMode::Enum::Vsync:
		return VK_PRESENT_MODE_FIFO_KHR;
	case PresentMode::Enum::VsyncFast:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	case PresentMode::Enum::VsyncRelax:
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	default:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	}
}

KENSHIN_END
