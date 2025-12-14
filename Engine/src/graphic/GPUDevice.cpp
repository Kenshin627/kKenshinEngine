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
	mWidth = deviceConfig->width;;
	mHeight = deviceConfig->height;
	mBuffers.init(mSystemAllocator, MAX_BUFFER_COUNT);

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
	feature2.features.
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
}

Buffer* GPUDevice::getBuffer(BufferHandle handle)
{
	return mBuffers.get(handle.index);
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

KENSHIN_END
