#include "pch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "GPUDevice.h"
#include"gpuTimestampManager.h"
#include "typeDefs.h"
#include "process.h"
#include "file.h"
#include <stb_image.h>

#if defined (_MSC_VER)
#pragma warning (disable: 4127)
#pragma warning (disable: 4189)
#pragma warning (disable: 4191)
#pragma warning (disable: 4296)
#pragma warning (disable: 4324)
#pragma warning (disable: 4355)
#pragma warning (disable: 4365)
#pragma warning (disable: 4625)
#pragma warning (disable: 4626)
#pragma warning (disable: 4668)
#pragma warning (disable: 5026)
#pragma warning (disable: 5027)
#endif // _MSC_VER

KENSHIN_BEGIN

static cstring instanceExtensions[] =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	"VK_KHR_win32_surface",
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

static cstring deviceExtensions[] = 
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME 
};

static const char* layers[] =
{
	"VK_LAYER_KHRONOS_validation"
};

static VkBool32 vkDebugUtilsMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
	void* pUserData)
{
	// 按级别打印日志（ERROR/WARNING/INFO/VERBOSE）
	std::string severity;
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severity = "[ERROR]";
		KS_CORE_ERROR("Vulkan Validation Layer: {0} {1}", severity.c_str(), pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severity = "[WARNING]";
		KS_CORE_WARN("Vulkan Validation Layer: {0} {1}", severity.c_str(), pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		severity = "[INFO]";
		KS_CORE_INFO("Vulkan Validation Layer: {0} {1}", severity.c_str(), pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		severity = "[VERBOSE]";
		KS_CORE_TRACE("Vulkan Validation Layer: {0} {1}", severity.c_str(), pCallbackData->pMessage);
		break;
	default:
		severity = "[UNKNOWN]";
	}
	
	return false;
}

// helper method
bool isEndOfLine(char c) 
{
	bool result = ((c == '\n') || (c == '\r'));
	return(result);
}

void GPUDevice::fillWriteDescriptorSets(GPUDevice& gpu, const DesciptorSetLayout* descriptor_set_layout, VkDescriptorSet vk_descriptor_set,
	VkWriteDescriptorSet* descriptor_write, VkDescriptorBufferInfo* buffer_info, VkDescriptorImageInfo* image_info,
	VkSampler vk_default_sampler, u32& num_resources, const ResourceHandle* resources, const SamplerHandle* samplers, const u16* bindings) {

	u32 used_resources = 0;
	for (u32 r = 0; r < num_resources; r++) 
	{
		// Binding array contains the index into the resource layout binding to retrieve
		// the correct binding informations.
		u32 layout_binding_index = bindings[r];
		const DescriptorBinding& binding = descriptor_set_layout->bindings[layout_binding_index];
		u32 i = used_resources;
		++used_resources;

		descriptor_write[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptor_write[i].dstSet = vk_descriptor_set;
		// Use binding array to get final binding point.		
		descriptor_write[i].dstBinding = binding.bindingPoint;
		descriptor_write[i].dstArrayElement = 0;
		descriptor_write[i].descriptorCount = 1;

		switch (binding.type) 
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			{
				descriptor_write[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				TextureHandle texture_handle = { resources[r] };
				Texture* texture_data = gpu.accessTexture(texture_handle);

				// Find proper sampler.
				// TODO: improve. Remove the single texture interface ?
				image_info[i].sampler = vk_default_sampler;
				if (texture_data->sampler) {
					image_info[i].sampler = texture_data->sampler->vkSampler;
				}
				// TODO: else ?
				if (samplers[r].index != InvalidIndex) 
				{
					Sampler* sampler = gpu.accessSampler({ samplers[r] });
					image_info[i].sampler = sampler->vkSampler;
				}
				image_info[i].imageLayout = TextureFormat::hasDepthOrStencil(texture_data->vkFormat) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image_info[i].imageView = texture_data->vkImageView;
				descriptor_write[i].pImageInfo = &image_info[i];
				break;
			}

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				descriptor_write[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				TextureHandle texture_handle = { resources[r] };
				Texture* texture_data = gpu.accessTexture(texture_handle);
				image_info[i].sampler = nullptr;
				image_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				image_info[i].imageView = texture_data->vkImageView;
				descriptor_write[i].pImageInfo = &image_info[i];
				break;
			}

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				BufferHandle buffer_handle = { resources[r] };
				Buffer* buffer = gpu.accessBuffer(buffer_handle);

				descriptor_write[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptor_write[i].descriptorType = buffer->usage == ResourceUsageType::Dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				// Bind parent buffer if present, used for dynamic resources.
				if (buffer->parentBufferHandle.index != InvalidIndex) 
				{
					Buffer* parent_buffer = gpu.accessBuffer(buffer->parentBufferHandle);
					buffer_info[i].buffer = parent_buffer->vkBuffer;
				}
				else {
					buffer_info[i].buffer = buffer->vkBuffer;
				}

				buffer_info[i].offset = 0;
				buffer_info[i].range = buffer->size;

				descriptor_write[i].pBufferInfo = &buffer_info[i];

				break;
			}

			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				BufferHandle buffer_handle = { resources[r] };
				Buffer* buffer = gpu.accessBuffer(buffer_handle);

				descriptor_write[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				// Bind parent buffer if present, used for dynamic resources.
				if (buffer->parentBufferHandle.index != InvalidIndex) 
				{
					Buffer* parent_buffer = gpu.accessBuffer(buffer->parentBufferHandle);
					buffer_info[i].buffer = parent_buffer->vkBuffer;
				}
				else 
				{
					buffer_info[i].buffer = buffer->vkBuffer;
				}
				buffer_info[i].offset = 0;
				buffer_info[i].range = buffer->size;
				descriptor_write[i].pBufferInfo = &buffer_info[i];
				break;
			}

			default:
			{
				KS_CORE_ASSERT(false, "Resource type %d not supported in descriptor set creation!\n", binding.name);
				break;
			}
		}
	}

	num_resources = used_resources;
}


void GPUDevice::createSwapchainPass(const RenderPassCreation& creation, RenderPass* renderPass)
{
	// Color attachment
	VkAttachmentDescription colorAttachment = {};
	Texture* drawImage = accessTexture(mDrawingImage);
	colorAttachment.format = drawImage->vkFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	VkAttachmentDescription depthAttachment{};
	Texture* depthTexture = accessTexture(mDepthTexture);
	depthAttachment.format = depthTexture->vkFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(mVkDevice, &renderPassInfo, mVkAllocationCallbacks, &renderPass->vkRenderPass));

	setResourceName(VK_OBJECT_TYPE_RENDER_PASS, (u64)renderPass->vkRenderPass, creation.name);

	// Create framebuffer into the device.
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = renderPass->vkRenderPass;
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.width  = drawImage->width;
	framebufferInfo.height = drawImage->height;
	framebufferInfo.layers = 1;

	VkImageView framebufferAttachments[2];
	framebufferAttachments[0] = drawImage->vkImageView;
	framebufferAttachments[1] = depthTexture->vkImageView;
	framebufferInfo.pAttachments = framebufferAttachments;
	vkCreateFramebuffer(mVkDevice, &framebufferInfo, nullptr, &mVkSwapchainFramebuffers);
	setResourceName(VK_OBJECT_TYPE_FRAMEBUFFER, (u64)mVkSwapchainFramebuffers, creation.name);
	renderPass->width = drawImage->width;
	renderPass->height = drawImage->height;
}

void GPUDevice::transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, bool isDepth) 
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else {
		//hy_assertm( false, "Unsupported layout transition!\n" );
	}
	//TODO Use sycronization2
	vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

bool GPUDevice::init(void* config)
{
	//0. assign initial Data from configuration
	KS_CORE_ASSERT(config, "GPUDevice initialize configuration is nullptr!");
	GPUDeviceConfiguration* deviceConfig = static_cast<GPUDeviceConfiguration*>(config);
	KS_CORE_ASSERT(deviceConfig, "GPUDevice initialize configuration is nullptr!");
	mWindow = static_cast<SDL_Window*>(deviceConfig->window);
	mSystemAllocator = deviceConfig->systemAllocator;
	mStackAllocator = deviceConfig->stackAllocator;
	mVkAllocationCallbacks = deviceConfig->allocationCallbacks;
	mSwapchainWidth = deviceConfig->width;;
	mSwapchainHeight = deviceConfig->height;
	mStringBuffer.init(1024 * 1024, mSystemAllocator);
	//1. vkInstance
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "kEngine";
	appInfo.pEngineName = "kEngine";
	appInfo.apiVersion = VK_MAKE_VERSION(1, 4, 0);
	VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = buildDebugUtilsMessageCreateInfo();
	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.enabledExtensionCount = ArraySize(instanceExtensions);
	instanceInfo.ppEnabledExtensionNames = instanceExtensions;
	instanceInfo.enabledLayerCount = ArraySize(layers);
	instanceInfo.ppEnabledLayerNames = layers;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.pNext = &debugMessageInfo;
	VK_CHECK(vkCreateInstance(&instanceInfo, mVkAllocationCallbacks, &mVkInstance));
	u32 extensionCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	VkExtensionProperties* extensions = static_cast<VkExtensionProperties*>(kalloca(extensionCount * sizeof(VkExtensionProperties), mSystemAllocator));
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions));
	for (size_t i = 0; i < extensionCount; i++)
	{
		const VkExtensionProperties& extension = extensions[i];
		if (!strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		{
			mdebugUtilsExtensionPresent = true;
			break;
		}
	}
	if (mdebugUtilsExtensionPresent)
	{
		PFN_vkCreateDebugUtilsMessengerEXT createDebugFunction = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(mVkInstance, "vkCreateDebugUtilsMessengerEXT"));
		VkDebugUtilsMessengerCreateInfoEXT utilsMessageInfo = buildDebugUtilsMessageCreateInfo();
		createDebugFunction(mVkInstance, &utilsMessageInfo, mVkAllocationCallbacks, &mVkDebugUtilsMessenger);
	}
	kfree(extensions, mSystemAllocator);

	//2. surface
	bool createSurfaceRes = SDL_Vulkan_CreateSurface(mWindow, mVkInstance, mVkAllocationCallbacks, &mVkWindowSurface);
	if (!createSurfaceRes)
	{
		KS_CORE_ERROR("sdl create vulkan surface error：{0}\n", SDL_GetError());
		KS_CORE_ASSERT(createSurfaceRes, "sdl create vulkan surface failed!");
		return false;
	}

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
		vkGetPhysicalDeviceProperties(physicalDevice, &mVkPhysicalDeviceProperties);
		if (mVkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			if (getQueuefamily(physicalDevice))
			{
				discreteGpu = physicalDevice;
				break;
			}
		}
		else if(mVkPhysicalDeviceProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			if (getQueuefamily(physicalDevice))
			{
				integratedGpu = physicalDevice;
			}
		}
	}
	if (discreteGpu)
	{
		mVkPhysicalDevice = discreteGpu;
	}
	else if (integratedGpu)
	{
		mVkPhysicalDevice = integratedGpu;
	}
	else
	{
		KS_CORE_ERROR("no gpu supported!");
		return false;
	}
	kfree(physicalDevices, mSystemAllocator);
	vkGetPhysicalDeviceProperties(mVkPhysicalDevice, &mVkPhysicalDeviceProperties);
	mMinSSBOAlignment = mVkPhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
	mMinUBOAlignment  = mVkPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

	//4. device
	VkDeviceCreateInfo deviceInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	float priority[] = {1.0};
	VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.flags = 0;
	queueInfo.pQueuePriorities = priority;
	queueInfo.queueFamilyIndex = mVkQueueFamilyIndex;
	queueInfo.queueCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;	
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.enabledExtensionCount = ArraySize(deviceExtensions);
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;
	VkPhysicalDeviceFeatures2 feature2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	dynamicRenderingFeature.dynamicRendering = VK_TRUE;	
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
	bufferDeviceAddressFeature.bufferDeviceAddress = VK_TRUE;
	dynamicRenderingFeature.pNext = &bufferDeviceAddressFeature;
	feature2.pNext = &dynamicRenderingFeature;
	vkGetPhysicalDeviceFeatures2(mVkPhysicalDevice, &feature2);
	deviceInfo.pNext = &feature2;
	VK_CHECK(vkCreateDevice(mVkPhysicalDevice, &deviceInfo, mVkAllocationCallbacks, &mVkDevice));

	if (mdebugUtilsExtensionPresent)
	{
		mDebugUtilsBeginLabel =  (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(mVkInstance, "vkCmdBeginDebugUtilsLabelEXT");
		mDebugUtilsEndLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(mVkInstance, "vkCmdEndDebugUtilsLabelEXT");
		mDebugUtilsSetObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(mVkInstance, "vkSetDebugUtilsObjectNameEXT");
	}

	//5. Queue
	vkGetDeviceQueue(mVkDevice, mVkQueueFamilyIndex, 0, &mVkQueue);

	//6. swapChain
	u32 formatCount;
	bool isFoundFormat{ false };
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mVkPhysicalDevice, mVkWindowSurface, &formatCount, nullptr));
	VkSurfaceFormatKHR* surfaceFormats = static_cast<VkSurfaceFormatKHR*>(kalloca(formatCount * sizeof(VkSurfaceFormatKHR), mSystemAllocator));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mVkPhysicalDevice, mVkWindowSurface, &formatCount, surfaceFormats));
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
				mVkSurfaceFormat = surfaceFormats[i];
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
		mVkSurfaceFormat = surfaceFormats[0];
		KS_CORE_WARN("no surfaceFormat is found, check you device please!");
	}
	kfree(surfaceFormats, mSystemAllocator);

	mSwapchainOutput.reset();
	mSwapchainOutput.color(mVkSurfaceFormat.format);
	//7. presentMode
	setPresentMode(mPresentMode);

	//8. swapchain
	createSwapchain();

	//9. vmaInit
	VmaAllocatorCreateInfo vmaInfo{};
	vmaInfo.device			 = mVkDevice;
	vmaInfo.flags			 = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaInfo.instance		 = mVkInstance;
	vmaInfo.physicalDevice   = mVkPhysicalDevice;
	vmaInfo.vulkanApiVersion = VK_MAKE_VERSION(1, 4, 0);
	vmaCreateAllocator(&vmaInfo, &mVmaAllocator);

	//10. descriptorPool
	VkDescriptorPoolSize poolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxGlobelPoolElements},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, MaxGlobelPoolElements },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MaxGlobelPoolElements }
	};
	VkDescriptorPoolCreateInfo descriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolInfo.maxSets = MaxGlobelPoolElements * ArraySize(poolSizes);
	descriptorPoolInfo.poolSizeCount = ArraySize(poolSizes);
	descriptorPoolInfo.pPoolSizes = poolSizes;
	VK_CHECK(vkCreateDescriptorPool(mVkDevice, &descriptorPoolInfo, mVkAllocationCallbacks, &mVkDescriptorPool));
	
	//TODO 11. Query Create timestamp query pool used for GPU timings.
	//VkQueryPoolCreateInfo vqpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0, VK_QUERY_TYPE_TIMESTAMP, creation.gpu_time_queries_per_frame * 2u * k_max_frames, 0 };
	//vkCreateQueryPool(vulkan_device, &vqpci, vulkan_allocation_callbacks, &vulkan_timestamp_query_pool);
	
	//12. resource pools
	mBuffers.init(mSystemAllocator, 4096, sizeof(Buffer));
	mTextures.init(mSystemAllocator, 512, sizeof(Texture));
	mRenderPasses.init(mSystemAllocator, 256, sizeof(RenderPass));
	mDescriptorSetLayouts.init(mSystemAllocator, 128, sizeof(DesciptorSetLayout));
	mPipelines.init(mSystemAllocator, 128, sizeof(Pipeline));
	mShaders.init(mSystemAllocator, 128, sizeof(ShaderState));
	mDescriptorSets.init(mSystemAllocator, 256, sizeof(DesciptorSet));
	mSamplers.init(mSystemAllocator, 32, sizeof(Sampler));

	//13. semaphore / fence
	VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (size_t i = 0; i < MaxInFlightFrames; ++i)
	{		
		vkCreateSemaphore(mVkDevice, &semaphore_info, mVkAllocationCallbacks, &mVkImageAcquiredSemaphore[i]);
		VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(mVkDevice, &fenceInfo, mVkAllocationCallbacks, &mVkCommandBufferExecutedFence[i]);
	}

	for (size_t i = 0; i < mVkSwapchainImageCount; ++i)
	{
		vkCreateSemaphore(mVkDevice, &semaphore_info, mVkAllocationCallbacks, &mVkRenderCompleteSemaphore[i]);
	}

	u8* memory = kallocm(sizeof(GPUTimestampManager) + sizeof(CommandBuffer*) * 128, mSystemAllocator);
	mGpuTimestampManager = (GPUTimestampManager*)(memory);
	mGpuTimestampManager->init(mSystemAllocator, deviceConfig->gpuTimeQueriesPerFrame, MaxInFlightFrames);

	CommandBufferServiceConfiguration cmdBufferServiceConfig{};
	cmdBufferServiceConfig.gpuDevice = this;
	cmdBufferServiceConfig.systemAllocator = this->mSystemAllocator;
	mCommandbufferManager.init(&cmdBufferServiceConfig);

	// Allocate queued command buffers array
	mQueuedCommandBuffers = (CommandBuffer**)(mGpuTimestampManager + 1);
	CommandBuffer** correctly_allocated_buffer = (CommandBuffer**)(memory + sizeof(GPUTimestampManager));
	KS_CORE_ASSERT(mQueuedCommandBuffers == correctly_allocated_buffer, "Wrong calculations for queued command buffers arrays. Should be {0}, but it is {1}.");

	mVkImageIndex = 0;
	mCurrentFrame = 1;
	mPreviousFrame = 0;
	mAbsoluteFrame = 0;
	mTimeStampsEnabled = false;

	mResourceDeletionQueue.init(mSystemAllocator, 16);
	mdDescriptorSetUpdatesQueue.init(mSystemAllocator, 16);

	//14. Init primitive resources
	SamplerCreation sc{};
	sc.setAddressModeUVW(
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
		.setMinMagMip(
		VK_FILTER_LINEAR, 
		VK_FILTER_LINEAR, 
		VK_SAMPLER_MIPMAP_MODE_LINEAR)
		.setName("Default Sampler");
	mDefaultSampler = createSampler(sc);

	BufferCreation fullscreenVBOCreation = { VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ResourceUsageType::Immutable, 0, nullptr, "FullScreen VBO" };
	mFullscreenVertexBuffer = createBuffer(fullscreenVBOCreation);
	
	mSwapchainOutput.depth(VK_FORMAT_D32_SFLOAT);

	//RenderPassCreation swapchainPassCreation = {};
	//swapchainPassCreation.setType(RenderPassType::Swapchain).setName("Swapchain");
	//swapchainPassCreation.setOperations(RenderPassOperation::Clear, RenderPassOperation::Clear, RenderPassOperation::Clear);
	//mSwapchainPass = createRenderPass(swapchainPassCreation);

	TextureCreation dummyTextureCreation = { nullptr, 1, 1, 1, 1, 0, VK_FORMAT_R8_UINT, TextureType::Texture2D };
	mDummyTexture = createTexture(dummyTextureCreation);

	BufferCreation dummyConstantBufferCreation = { VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ResourceUsageType::Immutable, 16, nullptr, "Dummy constant Buffer" };
	mDummyConstantBuffer = createBuffer(dummyConstantBufferCreation);

	// Get binaries path
	char* vulkan_env = mStringBuffer.reserve(512);
	ExpandEnvironmentStringsA("%VULKAN_SDK%", vulkan_env, 512);
	char* compiler_path = mStringBuffer.append_use_f("%s\\Bin\\", vulkan_env);

	strcpy(mVkBinariesPath, compiler_path);
	mStringBuffer.clear();

	// Dynamic buffer handling
	// TODO:
	BufferCreation dynamicBufferCreation;
	dynamicBufferCreation.set(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT  | 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ResourceUsageType::Immutable, DynamicBufferPerFrameSize * MaxInFlightFrames)
		.setName("Dynamic_Persistent_Buffer");
	mDynamicBuffer = createBuffer(dynamicBufferCreation);

	MapBufferParameters cbMap = { mDynamicBuffer, 0, 0 };
	mDynamicMappedMemory = (u8*)mapBuffer(cbMap);

	mRenderPassCache.init(mSystemAllocator, 16);

	TextureCreation drawingImageCreation = {};
	drawingImageCreation.setFlags(1, TextureFlags::ComputeMask)
					    .setFormatType(VK_FORMAT_R32G32B32A32_SFLOAT, TextureType::Texture2D)
					    .setName("Drawing Image")
					    .setSize(mSwapchainWidth, mSwapchainHeight, 1)
					    .setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	mDrawingImage = createTexture(drawingImageCreation);

	TextureCreation depthTextureCreation{};
	depthTextureCreation.setFlags(1, 0)
						.setFormatType(VK_FORMAT_D32_SFLOAT, TextureType::Texture2D)
						.setName("Depth Texture")
						.setSize(mSwapchainWidth, mSwapchainHeight, 1)
						.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	mDepthTexture = createTexture(depthTextureCreation);

	mDefaultTexture = createTexture("images/al.png","defaultDifTex");
	createPipelines();
	return true;
}

void GPUDevice::shutdown()
{
	vkDeviceWaitIdle(mVkDevice);
	mCommandbufferManager.shutdown();
	for (size_t i = 0; i < MaxInFlightFrames; i++) 
	{
		vkDestroySemaphore(mVkDevice, mVkRenderCompleteSemaphore[i], mVkAllocationCallbacks);
		vkDestroySemaphore(mVkDevice, mVkImageAcquiredSemaphore[i], mVkAllocationCallbacks);
		vkDestroyFence(mVkDevice, mVkCommandBufferExecutedFence[i], mVkAllocationCallbacks);
	}

	mGpuTimestampManager->shutdown();

	MapBufferParameters cbMap = { mDynamicBuffer, 0, 0 };
	unmapBuffer(cbMap);

	// Memory: this contains allocations for gpu timestamp memory, queued command buffers and render frames.
	kfree(mGpuTimestampManager, mSystemAllocator);

	destroyTexture(mDepthTexture);
	destroyBuffer(mFullscreenVertexBuffer);
	destroyBuffer(mDynamicBuffer);
	destroyRenderPass(mSwapchainPass);
	destroyTexture(mDummyTexture);
	destroyBuffer(mDummyConstantBuffer);
	destroySampler(mDefaultSampler);

	// Destroy all pending resources.
	for (u32 i = 0; i < mResourceDeletionQueue.size(); i++) 
	{
		ResourceUpdate& resourceDeletion = mResourceDeletionQueue[i];
		// Skip just freed resources.
		if (resourceDeletion.currentFrame == -1)
		{
			continue;
		}

		switch (resourceDeletion.type) 
		{
			case ResourceDeletionType::Buffer:
			{
				destroyBufferInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::Pipeline:
			{
				destroyPipelineInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::RenderPass:
			{
				destroyRenderPassInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::DescriptorSet:
			{
				destroyDescriptorSetInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::DescriptorSetLayout:
			{
				destroyDescriptorSetLayoutInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::Sampler:
			{
				destroySamplerInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::ShaderState:
			{
				destroyShaderStateInstant(resourceDeletion.handle);
				break;
			}

			case ResourceDeletionType::Texture:
			{
				destroyTextureInstant(resourceDeletion.handle);
				break;
			}
		}
	}

	// Destroy render passes from the cache.
	FlatHashMapIterator it = mRenderPassCache.iterator_begin();
	while (it.is_valid()) 
	{
		VkRenderPass vk_render_pass = mRenderPassCache.get(it);
		vkDestroyRenderPass(mVkDevice, vk_render_pass, mVkAllocationCallbacks);
		mRenderPassCache.iterator_advance(it);
	}
	mRenderPassCache.shutdown();

	RenderPass* vkSwapchainPass = accessRenderPass(mSwapchainPass);
	vkDestroyRenderPass(mVkDevice, vkSwapchainPass->vkRenderPass, mVkAllocationCallbacks);
	destroySwapchain();
	vkDestroySurfaceKHR(mVkInstance, mVkWindowSurface, mVkAllocationCallbacks);
	vmaDestroyAllocator(mVmaAllocator);
	mResourceDeletionQueue.shutdown();
	mdDescriptorSetUpdatesQueue.shutdown();
	mPipelines.shutdown();
	mBuffers.shutdown();
	mShaders.shutdown();
	mTextures.shutdown();
	mSamplers.shutdown();
	mDescriptorSetLayouts.shutdown();
	mDescriptorSets.shutdown();
	mRenderPasses.shutdown();

	if (mdebugUtilsExtensionPresent)
	{
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mVkInstance, "vkDestroyDebugUtilsMessengerEXT");
		vkDestroyDebugUtilsMessengerEXT(mVkInstance, mVkDebugUtilsMessenger, mVkAllocationCallbacks);
	}
	
	vkDestroyDescriptorPool(mVkDevice, mVkDescriptorPool, mVkAllocationCallbacks);
	//TODO
	//vkDestroyQueryPool(mVkDevice, vulkan_timestamp_query_pool, mVkAllocationCallbacks);
	vkDestroyDevice(mVkDevice, mVkAllocationCallbacks);
	vkDestroyInstance(mVkInstance, mVkAllocationCallbacks);

	mStringBuffer.shutdown();

	KS_CORE_INFO("Gpu Device shutdown");
}

VkDevice GPUDevice::getDevice()
{
	return mVkDevice;
}

u32 GPUDevice::getQueueFamilyIndex()
{
	return mVkQueueFamilyIndex;
}

VkAllocationCallbacks* GPUDevice::getAllocCallbacks()
{
	return mVkAllocationCallbacks;
}

GPUDevice* GPUDevice::instance()
{
	static GPUDevice device;
	return &device;
}

void GPUDevice::setPresentMode(PresentMode::Enum mode)
{
	bool isFoundPresentMode{ false };
	VkPresentModeKHR vkMode = toVkPresentMode(mode);
	u32 presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mVkPhysicalDevice, mVkWindowSurface, &presentModeCount, nullptr));
	VkPresentModeKHR* presentModes = static_cast<VkPresentModeKHR*>(kalloca(presentModeCount * sizeof(VkPresentModeKHR), mSystemAllocator));
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mVkPhysicalDevice, mVkWindowSurface, &presentModeCount, presentModes));
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
	mVkSwapchainImageCount = 3;
	kfree(presentModes, mSystemAllocator);
}

void GPUDevice::createSwapchain()
{
	VkSurfaceCapabilitiesKHR capabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mVkPhysicalDevice, mVkWindowSurface, &capabilities));
	if (capabilities.currentExtent.width == u32_max)
	{
		capabilities.currentExtent.width = std::clamp(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		capabilities.currentExtent.height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	mSwapchainWidth  = static_cast<u16>(capabilities.currentExtent.width);
	mSwapchainHeight = static_cast<u16>(capabilities.currentExtent.height);
	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.clipped = true;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.flags = 0;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageColorSpace = mVkSurfaceFormat.colorSpace;
	swapchainInfo.imageExtent = capabilities.currentExtent;
	swapchainInfo.imageFormat = mVkSurfaceFormat.format;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	//TODO: minImage??
	swapchainInfo.minImageCount = mVkSwapchainImageCount;
	swapchainInfo.pQueueFamilyIndices = &mVkQueueFamilyIndex;
	swapchainInfo.queueFamilyIndexCount = 1;
	swapchainInfo.presentMode = mVkPresentMode;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.surface = mVkWindowSurface;
	VK_CHECK(vkCreateSwapchainKHR(mVkDevice, &swapchainInfo, mVkAllocationCallbacks, &mVkSwapchain));
	VK_CHECK(vkGetSwapchainImagesKHR(mVkDevice, mVkSwapchain, &mVkSwapchainImageCount, nullptr));
	VK_CHECK(vkGetSwapchainImagesKHR(mVkDevice, mVkSwapchain, &mVkSwapchainImageCount, mVkSwapchainImages));

	for (size_t i = 0; i < mVkSwapchainImageCount; i++)
	{
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A;
		viewInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B;
		viewInfo.flags		  = 0;
		viewInfo.format		  = mVkSurfaceFormat.format;
		viewInfo.image		  = mVkSwapchainImages[i];
		viewInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkCreateImageView(mVkDevice, &viewInfo, mVkAllocationCallbacks, &mVkSwapchainImageViews[i]);
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
	for (size_t i = 0; i < queuefamilyCount; ++i)
	{
		const VkQueueFamilyProperties& queueFamily = queuefamilies[i];
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
		{
			mVkQueueFamilyIndex = i;
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

VkShaderModuleCreateInfo GPUDevice::compileShader(cstring code, u32 code_size, VkShaderStageFlagBits stage, cstring name) 
{
	VkShaderModuleCreateInfo shaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

	// Compile from glsl to SpirV.
	// TODO: detect if input is HLSL.
	const char* tempFilename = "temp.shader";

	// Write current shader to file.
	FILE* tempShaderFile;
	fopen_s(&tempShaderFile, tempFilename, "w");
	fwrite(code, code_size, 1, tempShaderFile);
	fclose(tempShaderFile);

	sizet currentMarker = mStackAllocator->getMarker();
	StringBuffer tempStringBuffer;
	tempStringBuffer.init(kkilo(1), mStackAllocator);

	// Add uppercase define as STAGE_NAME
	char* stageDefine = tempStringBuffer.append_use_f("%s_%s", toStageDefines(stage), name);
	sizet stageDefineLength = strlen(stageDefine);
	for (u32 i = 0; i < stageDefineLength; ++i)
	{
		stageDefine[i] = toupper(stageDefine[i]);
	}
	// Compile to SPV
#if defined(_MSC_VER)
	char* glslCompilerPath = tempStringBuffer.append_use_f("%sglslangValidator.exe", mVkBinariesPath);
	char* finalSpirvFilename = tempStringBuffer.append_use("shader_final.spv");
	// TODO: add optional debug information in shaders (option -g).
	char* arguments = tempStringBuffer.append_use_f("glslangValidator.exe %s -V --target-env vulkan1.4 -o %s -S %s --D %s --D %s", tempFilename, finalSpirvFilename, toCompilerExtension(stage), stageDefine, toStageDefines(stage));
#else
	char* glsl_compiler_path = temp_string_buffer.append_use_f("%sglslangValidator", vulkan_binaries_path);
	char* final_spirv_filename = temp_string_buffer.append_use("shader_final.spv");
	char* arguments = temp_string_buffer.append_use_f("%s -V --target-env vulkan1.2 -o %s -S %s --D %s --D %s", temp_filename, final_spirv_filename, to_compiler_extension(stage), stage_define, to_stage_defines(stage));
#endif
	processExecute(".", glslCompilerPath, arguments, "");

	bool optimize_shaders = false;

	if (optimize_shaders) {
		// TODO: add optional optimization stage
		//"spirv-opt -O input -o output
		char* spirv_optimizer_path = tempStringBuffer.append_use_f("%sspirv-opt.exe", mVkBinariesPath);
		char* optimized_spirv_filename = tempStringBuffer.append_use_f("shader_opt.spv");
		char* spirv_opt_arguments = tempStringBuffer.append_use_f("spirv-opt.exe -O --preserve-bindings %s -o %s", finalSpirvFilename, optimized_spirv_filename);

		processExecute(".", spirv_optimizer_path, spirv_opt_arguments, "");

		// Read back SPV file.
		shaderCreateInfo.pCode = reinterpret_cast<const u32*>(readBinaryFile(optimized_spirv_filename, mStackAllocator, &shaderCreateInfo.codeSize));

		fileDelete(optimized_spirv_filename);
	}
	else {
		// Read back SPV file.
		shaderCreateInfo.pCode = reinterpret_cast<const u32*>(readBinaryFile(finalSpirvFilename, mStackAllocator, &shaderCreateInfo.codeSize));
	}

	// Handling compilation error
	if (shaderCreateInfo.pCode == nullptr) 
	{
		dumpShaderCode(tempStringBuffer, code, stage, name);
	}

	// Temporary files cleanup
	fileDelete(tempFilename);
	fileDelete(finalSpirvFilename);
	return shaderCreateInfo;
}

void GPUDevice::resize()
{
	if (mResized)
	{
		resizeSwapchain();
		resizeDrawingImage();
		mResized = false;
	}
}

void GPUDevice::setResize()
{
	mResized = true;
}

void GPUDevice::dumpShaderCode(StringBuffer& tempStringBuffer, cstring code, VkShaderStageFlagBits stage, cstring name) 
{
	cstring stageDefines = toStageDefines(stage);
	KS_CORE_ERROR("Error in creation of shader %s, stage %s. Writing shader:\n", name, stageDefines);
	cstring currentCode = code;
	u32 lineIndex = 1;
	while (currentCode) 
	{
		cstring endOfLine = currentCode;
		if (!endOfLine || *endOfLine == 0) 
		{
			break;
		}
		while (!isEndOfLine(*endOfLine)) 
		{
			++endOfLine;
		}
		if (*endOfLine == '\r') 
		{
			++endOfLine;
		}
		if (*endOfLine == '\n') 
		{
			++endOfLine;
		}

		tempStringBuffer.clear();
		char* line = tempStringBuffer.append_use_substring(currentCode, 0, (endOfLine - currentCode));
		KS_CORE_INFO("%u: %s", lineIndex++, line);
		currentCode = endOfLine;
	}
}

ShaderStateHandle GPUDevice::createShaderState(const ShaderStateCreation& creation) 
{
	ShaderStateHandle handle = { InvalidIndex };

	if (creation.stages_count == 0 || creation.stages == nullptr) 
	{
		KS_CORE_ERROR("Shader %s does not contain shader stages.\n", creation.name);
		return handle;
	}

	handle.index = mShaders.obtainResource();
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	// For each shader stage, compile them individually.
	u32 compiledShaders = 0;

	ShaderState* shaderState = accessShaderState(handle);
	shaderState->graphicsPipeline = true;
	shaderState->activeShaders = 0;

	sizet currentMarker = mStackAllocator->getMarker();

	for (compiledShaders = 0; compiledShaders < creation.stages_count; ++compiledShaders) 
	{
		const ShaderStage& stage = creation.stages[compiledShaders];

		// Gives priority to compute: if any is present (and it should not be) then it is not a graphics pipeline.
		if (stage.type == VK_SHADER_STAGE_COMPUTE_BIT) 
		{
			shaderState->graphicsPipeline = false;
		}

		VkShaderModuleCreateInfo shaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

		if (creation.spv_input) 
		{
			shaderModuleCreateInfo.codeSize = stage.code_size;
			shaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(stage.code);
		}
		else 
		{
			shaderModuleCreateInfo = compileShader(stage.code, stage.code_size, stage.type, creation.name);
		}

		// Compile shader module
		VkPipelineShaderStageCreateInfo& shaderStageInfo = shaderState->shaderStageInfo[compiledShaders];
		memset(&shaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.pName = "main";
		shaderStageInfo.stage = stage.type;

		if (vkCreateShaderModule(mVkDevice, &shaderModuleCreateInfo, mVkAllocationCallbacks, &shaderState->shaderStageInfo[compiledShaders].module) != VK_SUCCESS)
		{
			KS_CORE_ASSERT(false, "Failed to create shader module.");
			break;
		}

		// Not needed anymore - temp allocator freed at the end.
		//if ( compiled ) {
		//    rfree( ( void* )createInfo.pCode, allocator );
		//}

		setResourceName(VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderState->shaderStageInfo[compiledShaders].module, creation.name);
	}
	// Not needed anymore - temp allocator freed at the end.
	//name_buffer.shutdown();
	mStackAllocator->freeToMarker(currentMarker);

	bool creationFailed = compiledShaders != creation.stages_count;
	if (!creationFailed) 
	{
		shaderState->activeShaders = compiledShaders;
		shaderState->name = creation.name;
	}

	if (creationFailed) 
	{
		destroyShaderState(handle);
		handle.index = InvalidIndex;

		// Dump shader code
		KS_CORE_ERROR("Error in creation of shader %s. Dumping all shader informations.\n", creation.name);
		for (compiledShaders = 0; compiledShaders < creation.stages_count; ++compiledShaders)
		{
			//const ShaderStage& stage = creation.stages[compiledShaders];
			//ERROR("%u:\n%s\n", stage.type, stage.code);KS_CORE_
			KS_CORE_ASSERT(false, "Failed to create shader state.");
		}
	}
	return handle;
}

PipelineHandle GPUDevice::createPipeline(const PipelineCreation& creation) 
{
	PipelineHandle handle = { mPipelines.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	ShaderStateHandle shaderState = createShaderState(creation.shaders);
	if (shaderState.index == InvalidIndex)
	{
		mPipelines.releaseResource(handle.index);
		handle.index = InvalidIndex;
		return handle;
	}

	Pipeline* pipeline = accessPipeline(handle);
	pipeline->shaderState = shaderState;
	VkDescriptorSetLayout vk_layouts[MaxDescriptorSetLayouts];

	// Create VkPipelineLayout
	for (u32 i = 0; i < creation.numActiveDescriptorSetLayouts; ++i) 
	{
		pipeline->descriptorSetLayout[i] = accessDescriptorSetLayout(creation.descriptorSetLayout[i]);
		pipeline->descriptorSetLayoutHandle[i] = creation.descriptorSetLayout[i];
		vk_layouts[i] = pipeline->descriptorSetLayout[i]->vkDescriptorSetLayout;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	//TODO: pushConstants
	pipelineLayoutInfo.pSetLayouts = vk_layouts;
	pipelineLayoutInfo.setLayoutCount = creation.numActiveDescriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = creation.numPushConstantRanges;
	pipelineLayoutInfo.pPushConstantRanges = creation.pushConstantRanges;
	VkPipelineLayout pipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(mVkDevice, &pipelineLayoutInfo, mVkAllocationCallbacks, &pipelineLayout));
	// Cache pipeline layout
	pipeline->vkPipelineLayout = pipelineLayout;
	pipeline->numActiveDescriptorSetLayouts = creation.numActiveDescriptorSetLayouts;

	// Create full pipeline
	ShaderState* shaderStateData = accessShaderState(shaderState);
	if (shaderStateData->graphicsPipeline)
	{
		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.pStages    = shaderStateData->shaderStageInfo;
		pipelineInfo.stageCount = shaderStateData->activeShaders;
		pipelineInfo.layout		= pipelineLayout;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkVertexInputAttributeDescription vertexAttributes[MaxVertexAttributes];
		if (creation.vertexInput.numVertexAttributes) 
		{
			for (u32 i = 0; i < creation.vertexInput.numVertexAttributes; ++i)
			{
				const VertexAttribute& vertexAttribute = creation.vertexInput.vertexAttributes[i];
				vertexAttributes[i] = { vertexAttribute.location, vertexAttribute.binding, toVkVertexFormat(vertexAttribute.format), vertexAttribute.offset };
			}

			vertexInputInfo.vertexAttributeDescriptionCount = creation.vertexInput.numVertexAttributes;
			vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;
		}
		else 
		{
			vertexInputInfo.vertexAttributeDescriptionCount = 0;
			vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		}

		VkVertexInputBindingDescription vertexBindings[MaxVertexStreams];
		if (creation.vertexInput.numVertexStreams) 
		{
			for (u32 i = 0; i < creation.vertexInput.numVertexStreams; ++i) 
			{
				const VertexStream& vertexStream = creation.vertexInput.vertexStreams[i];
				VkVertexInputRate vertex_rate = vertexStream.inputRate == VertexInputRate::PerVertex ? VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX : VkVertexInputRate::VK_VERTEX_INPUT_RATE_INSTANCE;
				vertexBindings[i] = { vertexStream.binding, vertexStream.stride, vertex_rate };
			}
			vertexInputInfo.pVertexBindingDescriptions = vertexBindings;
			vertexInputInfo.vertexBindingDescriptionCount = creation.vertexInput.numVertexStreams;
		}
		else 
		{
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.pVertexBindingDescriptions = nullptr;
		}

		pipelineInfo.pVertexInputState = &vertexInputInfo;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		pipelineInfo.pInputAssemblyState = &inputAssembly;

		//// Color Blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment[MaxImageOutputs];

		if (creation.blendState.activeStates) 
		{
			for (size_t i = 0; i < creation.blendState.activeStates; ++i) 
			{
				const BlendState& blendState = creation.blendState.blendStates[i];

				colorBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment[i].blendEnable = blendState.blendEnabled ? VK_TRUE : VK_FALSE;
				colorBlendAttachment[i].srcColorBlendFactor = blendState.sourceColor;
				colorBlendAttachment[i].dstColorBlendFactor = blendState.destinationColor;
				colorBlendAttachment[i].colorBlendOp = blendState.colorOperation;

				if (blendState.separateBlend) 
				{
					colorBlendAttachment[i].srcAlphaBlendFactor = blendState.sourceAlpha;
					colorBlendAttachment[i].dstAlphaBlendFactor = blendState.destinationAlpha;
					colorBlendAttachment[i].alphaBlendOp = blendState.alphaOperation;
				}
				else 
				{
					colorBlendAttachment[i].srcAlphaBlendFactor = blendState.sourceColor;
					colorBlendAttachment[i].dstAlphaBlendFactor = blendState.destinationColor;
					colorBlendAttachment[i].alphaBlendOp = blendState.colorOperation;
				}
			}
		}
		else 
		{
			// Default non blended state
			colorBlendAttachment[0] = {};
			colorBlendAttachment[0].blendEnable = VK_FALSE;
			colorBlendAttachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = creation.blendState.activeStates ? creation.blendState.activeStates : 1; // Always have 1 blend defined.
		colorBlending.pAttachments = colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		pipelineInfo.pColorBlendState = &colorBlending;

		//// Depth Stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		depthStencil.depthWriteEnable = creation.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;
		depthStencil.stencilTestEnable = creation.depthStencil.stencilEnable ? VK_TRUE : VK_FALSE;
		depthStencil.depthTestEnable = creation.depthStencil.depthEnable ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = creation.depthStencil.depthComparison;
		if (creation.depthStencil.stencilEnable) 
		{
			//TODO: add stencil
		}

		pipelineInfo.pDepthStencilState = &depthStencil;

		//// Multisample
		VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		pipelineInfo.pMultisampleState = &multisampling;

		//// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = creation.rasterization.cull_mode;
		rasterizer.frontFace = creation.rasterization.front;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		pipelineInfo.pRasterizationState = &rasterizer;

		//// Tessellation
		pipelineInfo.pTessellationState;


		//// Viewport state
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)mSwapchainWidth;
		viewport.height = (float)mSwapchainHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { mSwapchainWidth, mSwapchainHeight };

		VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		pipelineInfo.pViewportState = &viewportState;

		//dynamic rendering
		VkPipelineRenderingCreateInfo pipelineRenderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		pipelineRenderingInfo.viewMask = 0x0;
		pipelineRenderingInfo.colorAttachmentCount = creation.numColorAttachments;
		pipelineRenderingInfo.pColorAttachmentFormats = creation.colorAttachmentFormats;
		pipelineRenderingInfo.depthAttachmentFormat = creation.depthFormat;
		pipelineRenderingInfo.stencilAttachmentFormat = creation.stencilFormat;
		pipelineRenderingInfo.pNext = nullptr;
		pipelineInfo.pNext = &pipelineRenderingInfo;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;

		//// Dynamic states
		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicStateInfo.dynamicStateCount = ArraySize(dynamicStates);
		dynamicStateInfo.pDynamicStates = dynamicStates;

		pipelineInfo.pDynamicState = &dynamicStateInfo;

		VK_CHECK(vkCreateGraphicsPipelines(mVkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, mVkAllocationCallbacks, &pipeline->vkPipeline));

		pipeline->vkBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
	}
	else 
	{
		VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

		pipelineInfo.stage = shaderStateData->shaderStageInfo[0];
		pipelineInfo.layout = pipelineLayout;

		VK_CHECK(vkCreateComputePipelines(mVkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, mVkAllocationCallbacks, &pipeline->vkPipeline));

		pipeline->vkBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
	}

	return handle;
}

BufferHandle GPUDevice::createBuffer(const BufferCreation& creation) 
{
	BufferHandle handle = { mBuffers.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	Buffer* buffer = accessBuffer(handle);

	buffer->name = creation.mName;
	buffer->size = creation.mSize;
	buffer->typeFlags = creation.mTypeFlags;
	buffer->usage = creation.mUsage;
	buffer->handle = handle;
	buffer->globelBufferOffset = 0;
	buffer->parentBufferHandle = { InvalidIndex };

	// Cache and calculate if dynamic buffer can be used.
	static const VkBufferUsageFlags dynamicBufferMask = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	const bool useGlobalBuffer = (creation.mTypeFlags & dynamicBufferMask) != 0;
	if (creation.mUsage == ResourceUsageType::Dynamic && useGlobalBuffer)
	{
		buffer->parentBufferHandle = mDynamicBuffer;
		return handle;
	}

	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | creation.mTypeFlags;
	bufferInfo.size = creation.mSize > 0 ? creation.mSize : 1;       // 0 sized creations are not permitted.

	VmaAllocationCreateInfo memoryInfo{};
	memoryInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
	memoryInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VmaAllocationInfo allocationInfo{};
	VK_CHECK(vmaCreateBuffer(mVmaAllocator, &bufferInfo, &memoryInfo, &buffer->vkBuffer, &buffer->vmaAllocation, &allocationInfo));

	setResourceName(VK_OBJECT_TYPE_BUFFER, (u64)buffer->vkBuffer, creation.mName);

	buffer->vkDeviceMemory = allocationInfo.deviceMemory;

	if (creation.mInitialData) 
	{
		void* data;
		vmaMapMemory(mVmaAllocator, buffer->vmaAllocation, &data);
		memcpy(data, creation.mInitialData, (size_t)creation.mSize);
		vmaUnmapMemory(mVmaAllocator, buffer->vmaAllocation);
	}

	// TODO
	//if ( persistent )
	//{
	//    mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	//}
	return handle;
}

TextureHandle GPUDevice::createTexture(const TextureCreation& creation)
{
	u32 resourceIndex = mTextures.obtainResource();
	TextureHandle handle = { resourceIndex };
	if (resourceIndex == InvalidIndex)
	{
		return handle;
	}
	Texture* texture = accessTexture(handle);
	createTexture(creation, handle, texture);

	if (creation.mInitialData) 
	{
		// Create stating buffer
		VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		//TODO: support different formats
		u32 imageSize = creation.mWidth * creation.mHeight * 4;
		bufferInfo.size = imageSize;

		VmaAllocationCreateInfo memoryInfo{};
		memoryInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
		memoryInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		VmaAllocationInfo allocationInfo{};
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VK_CHECK(vmaCreateBuffer(mVmaAllocator, &bufferInfo, &memoryInfo, &stagingBuffer, &stagingAllocation, &allocationInfo));

		// Copy buffer_data
		void* destinationData;
		vmaMapMemory(mVmaAllocator, stagingAllocation, &destinationData);
		memcpy(destinationData, creation.mInitialData, static_cast<size_t>(imageSize));
		vmaUnmapMemory(mVmaAllocator, stagingAllocation);

		// Execute command buffer
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		CommandBuffer* cmdBuffer = getInstantCommandBuffer();
		vkBeginCommandBuffer(cmdBuffer->mCommandBuffer, &beginInfo);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { creation.mWidth, creation.mHeight, creation.mDepth };

		// Transition
		transitionImageLayout(cmdBuffer->mCommandBuffer, texture->vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);
		// Copy
		vkCmdCopyBufferToImage(cmdBuffer->mCommandBuffer, stagingBuffer, texture->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		// Transition
		transitionImageLayout(cmdBuffer->mCommandBuffer, texture->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
		vkEndCommandBuffer(cmdBuffer->mCommandBuffer);
		// Submit command buffer
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer->mCommandBuffer;
		vkQueueSubmit(mVkQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(mVkQueue);
		vmaDestroyBuffer(mVmaAllocator, stagingBuffer, stagingAllocation);
		// TODO: free command buffer
		vkResetCommandBuffer(cmdBuffer->mCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		texture->vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	return handle;
}

TextureHandle GPUDevice::createTexture(cstring filename, cstring textureName)
{
	if (filename) 
	{
		int comp, width, height;
		stbi_set_flip_vertically_on_load(true);
		uint8_t* imageData = stbi_load(filename, &width, &height, &comp, 4);
		if (!imageData)
		{
			KS_CORE_ERROR("texture {0} data is null!", filename);
			return InvalidTexture;;
		}
		TextureCreation creation{};
		creation.setData(imageData)
				.setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
				.setFlags(1, 0)
				.setSize((u16)width, (u16)height, 1)
				.setName(textureName)
				.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);

		TextureHandle newTexture = createTexture(creation);
		free(imageData);
		return newTexture;
	}
	return InvalidTexture;
}

SamplerHandle GPUDevice::createSampler(const SamplerCreation& creation) 
{
	SamplerHandle handle = { mSamplers.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	Sampler* sampler = accessSampler(handle);

	sampler->addressModeU = creation.addressModeU;
	sampler->addressModeV = creation.addressModeV;
	sampler->addressModeW = creation.addressModeW;
	sampler->minFilter = creation.minFilter;
	sampler->magFilter = creation.magFilter;
	sampler->mipFilter = creation.mipFilter;
	sampler->name = creation.name;

	VkSamplerCreateInfo create_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	create_info.addressModeU = creation.addressModeU;
	create_info.addressModeV = creation.addressModeV;
	create_info.addressModeW = creation.addressModeW;
	create_info.minFilter    = creation.minFilter;
	create_info.magFilter    = creation.magFilter;
	create_info.mipmapMode   = creation.mipFilter;
	create_info.anisotropyEnable = 0;
	create_info.compareEnable = 0;
	create_info.unnormalizedCoordinates = 0;
	create_info.borderColor = VkBorderColor::VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	// TODO:
	/*float                   mipLodBias;
	float                   maxAnisotropy;
	VkCompareOp             compareOp;
	float                   minLod;
	float                   maxLod;
	VkBorderColor           borderColor;
	VkBool32                unnormalizedCoordinates;*/

	VK_CHECK(vkCreateSampler(mVkDevice, &create_info, mVkAllocationCallbacks, &sampler->vkSampler));

	setResourceName(VK_OBJECT_TYPE_SAMPLER, (u64)sampler->vkSampler, creation.name);

	return handle;
}

DescriptorSetLayoutHandle GPUDevice::createDescriptorSetLayout(const DescriptorSetLayoutCreation& creation) 
{
	DescriptorSetLayoutHandle handle = { mDescriptorSetLayouts.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	DesciptorSetLayout* descriptorSetLayout = accessDescriptorSetLayout(handle);

	// TODO: add support for multiple sets.
	// Create flattened binding list
	descriptorSetLayout->numBindings = (u16)creation.numBindings;
	u8* memory = kallocm((sizeof(VkDescriptorSetLayoutBinding) + sizeof(DescriptorBinding)) * creation.numBindings, mSystemAllocator);
	descriptorSetLayout->bindings = (DescriptorBinding*)memory;
	descriptorSetLayout->vkBinding = (VkDescriptorSetLayoutBinding*)(memory + sizeof(DescriptorBinding) * creation.numBindings);
	descriptorSetLayout->handle = handle;
	descriptorSetLayout->setIndex = u16(creation.setIndex);

	u32 usedBindings = 0;
	for (u32 i = 0; i < creation.numBindings; ++i) 
	{
		DescriptorBinding& binding = descriptorSetLayout->bindings[i];
		const DescriptorSetLayoutCreation::Binding& inputBinding = creation.bindings[i];
		binding.bindingPoint = inputBinding.bindingPoint == u16_max ? (u16)i : inputBinding.bindingPoint;
		binding.count = 1;
		binding.type = inputBinding.type;
		binding.name = inputBinding.name;

		VkDescriptorSetLayoutBinding& vkBinding = descriptorSetLayout->vkBinding[usedBindings];
		++usedBindings;

		vkBinding.binding = binding.bindingPoint;
		vkBinding.descriptorType = binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : binding.type;
		vkBinding.descriptorCount = 1;

		// TODO:
		vkBinding.stageFlags = VK_SHADER_STAGE_ALL;
		vkBinding.pImmutableSamplers = nullptr;
	}

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutInfo.bindingCount = usedBindings;// creation.num_bindings;
	descriptorSetLayoutInfo.pBindings = descriptorSetLayout->vkBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(mVkDevice, &descriptorSetLayoutInfo, mVkAllocationCallbacks, &descriptorSetLayout->vkDescriptorSetLayout));
	return handle;
}

DescriptorSetHandle GPUDevice::createDescriptorSet(const DescriptorSetCreation& creation) 
{
	DescriptorSetHandle handle = { mDescriptorSets.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	DesciptorSet* descriptorSet = accessDescriptorSet(handle);
	const DesciptorSetLayout* descriptorSetLayout = accessDescriptorSetLayout(creation.mLayout);

	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mVkDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout->vkDescriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(mVkDevice, &allocInfo, &descriptorSet->vkDescriptorSet));
	// Cache data
	u8* memory = kallocm((sizeof(ResourceHandle) + sizeof(SamplerHandle) + sizeof(u16)) * creation.mNumResources, mSystemAllocator);
	descriptorSet->resources = (ResourceHandle*)memory;
	descriptorSet->samplers = (SamplerHandle*)(memory + sizeof(ResourceHandle) * creation.mNumResources);
	descriptorSet->bindings = (u16*)(memory + (sizeof(ResourceHandle) + sizeof(SamplerHandle)) * creation.mNumResources);
	descriptorSet->numResources = creation.mNumResources;
	descriptorSet->layout = descriptorSetLayout;

	// Update descriptor set
	VkWriteDescriptorSet   descriptorWrite[MaxDescriptorsPerSet];
	VkDescriptorBufferInfo bufferInfo[MaxDescriptorsPerSet];
	VkDescriptorImageInfo  imageInfo[MaxDescriptorsPerSet];

	Sampler* defaultSampler = accessSampler(mDefaultSampler);

	u32 numResources = creation.mNumResources;
	fillWriteDescriptorSets(*this, descriptorSetLayout, descriptorSet->vkDescriptorSet, descriptorWrite, bufferInfo, imageInfo, defaultSampler->vkSampler,
		numResources, creation.mResources, creation.mSamplers, creation.mBindings);

	// Cache resources
	for (u32 i = 0; i < creation.mNumResources; ++i) 
	{
		descriptorSet->resources[i] = creation.mResources[i];
		descriptorSet->samplers[i] = creation.mSamplers[i];
		descriptorSet->bindings[i] = creation.mBindings[i];
	}

	vkUpdateDescriptorSets(mVkDevice, numResources, descriptorWrite, 0, nullptr);

	return handle;
}

void GPUDevice::createFramebuffer(RenderPass* renderPass, const TextureHandle* outputTextures, u32 numRenderTargets, TextureHandle depthStencilTexture) 
{
	// Create framebuffer
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = renderPass->vkRenderPass;
	framebufferInfo.width = renderPass->width;
	framebufferInfo.height = renderPass->height;
	framebufferInfo.layers = 1;

	//max colorAttachments + depthStencilAttachment
	VkImageView framebufferAttachments[MaxImageOutputs + 1]{};
	u32 activeAttachments = 0;
	for (; activeAttachments < numRenderTargets; ++activeAttachments)
	{
		Texture* texture = accessTexture(outputTextures[activeAttachments]);
		framebufferAttachments[activeAttachments] = texture->vkImageView;
	}

	if (depthStencilTexture.index != InvalidIndex) 
	{
		Texture* depthTexture = accessTexture(depthStencilTexture);
		framebufferAttachments[activeAttachments++] = depthTexture->vkImageView;
	}
	framebufferInfo.pAttachments = framebufferAttachments;
	framebufferInfo.attachmentCount = activeAttachments;

	VK_CHECK(vkCreateFramebuffer(mVkDevice, &framebufferInfo, mVkAllocationCallbacks, &renderPass->vkFrameBuffer));
	setResourceName(VK_OBJECT_TYPE_FRAMEBUFFER, (u64)renderPass->vkFrameBuffer, renderPass->name);
}

void GPUDevice::createPipelines()
{
	//compute pipeline
	DescriptorSetLayoutCreation descriptorSetlayoutCreation{};
	DescriptorSetLayoutCreation::Binding binding;
	binding.count = 1;
	binding.name = "drawingImage";
	binding.bindingPoint = 0;
	binding.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorSetlayoutCreation
		.reset()
		.addBinding(binding)
		.setName("DefaultComputeDescriptorSetLayout")
		.setSetIndex(0);
	FileResult computeShader = readTextFile("shaders/defaultCompute.comp", mSystemAllocator);
	mDefaultComputeDescriptorSetLayout = createDescriptorSetLayout(descriptorSetlayoutCreation);
	PipelineCreation pipelineCreation;
	pipelineCreation.addDescriptorSetLayout(mDefaultComputeDescriptorSetLayout);
	pipelineCreation.shaders
					.addStage(computeShader.data, computeShader.size, VK_SHADER_STAGE_COMPUTE_BIT)
					.setSpvInput(false)
					.setName("DefaultComputeShader");
	mDefaultComputePipeline = createPipeline(pipelineCreation);

	DescriptorSetCreation computeDsCreation{};
	computeDsCreation.reset()
					 .setLayout(mDefaultComputeDescriptorSetLayout)
					 .setName("DefaultComputeDescriptorSet")
					 .texture(mDrawingImage, 0);
	mDefaultComputeDescriptorSet = createDescriptorSet(computeDsCreation);

	//graphic pipeline
	DescriptorSetLayoutCreation graphicDescriptorSetlayoutCreation{};
	graphicDescriptorSetlayoutCreation.reset()
									  .setName("DefaultGraphicDescriptorSetLayout")
									  .setSetIndex(0)
									  .addBinding({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, "diffuseTexture" }) //binding = 0
									  .addBinding({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, "Geometry VertexBuffer" }); //binding = 1
	
	FileResult vertexShader   = readTextFile("shaders/defaultVertex.vert", mSystemAllocator);
	FileResult fragmentShader = readTextFile("shaders/defaultFragment.frag", mSystemAllocator);
	Texture* depthTexture	  = accessTexture(mDepthTexture);
	Texture* colorTexture	  = accessTexture(mDrawingImage);

	mDefaultGraphicDescriptorSetLayout = createDescriptorSetLayout(graphicDescriptorSetlayoutCreation);
	PipelineCreation defaultGraphicPipelineCreation{};
	defaultGraphicPipelineCreation.addDescriptorSetLayout(mDefaultGraphicDescriptorSetLayout)
								  .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4))
							      .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec4), 8)
								  .setName("DefaultGraphicPipeline")
								  .addColorAttachmentFormat(colorTexture->vkFormat)
								  .setDepthFormat(depthTexture->vkFormat)
								  .setStencilFormat(VK_FORMAT_UNDEFINED)
								  .shaders
								  .addStage(vertexShader.data, vertexShader.size, VK_SHADER_STAGE_VERTEX_BIT)
								  .addStage(fragmentShader.data, fragmentShader.size, VK_SHADER_STAGE_FRAGMENT_BIT)
								  .setSpvInput(false)
								  .setName("defaultGraphicShader");

	VertexInputCreation vertexInputCreation{};
	vertexInputCreation.reset()
					   .addVertexAttribute({ 0, 0, 0, VertexComponentFormat::Float3 })
					   .addVertexAttribute({ 1, 0 ,sizeof(glm::vec3) , VertexComponentFormat::Float2 })
					   .addVertexStream({ 0, sizeof(Vertex), VertexInputRate::PerVertex });
	//defaultGraphicPipelineCreation.vertexInput = vertexInputCreation;
	mDefaultGraphicPipeline = createPipeline(defaultGraphicPipelineCreation);
}

VkRenderPass GPUDevice::createRenderPass(const RenderPassOutput& output, cstring name) 
{
	VkAttachmentDescription colorAttachments[MaxImageOutputs] = {};
	VkAttachmentReference colorAttachmentsRef[MaxImageOutputs] = {};

	VkAttachmentLoadOp colorOp, depthOp, stencilOp;
	VkImageLayout colorInitialLayout, depthInitialLayout;

	switch (output.colorOperation) 
	{
		case RenderPassOperation::Load:
			colorOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			colorInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		case RenderPassOperation::Clear:
			colorOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		default:
			colorOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
	}

	switch (output.depthOperation) 
	{
		case RenderPassOperation::Load:
			depthOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case RenderPassOperation::Clear:
			depthOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		default:
			depthOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;
	}

	switch (output.stencilOperation) 
	{
		case RenderPassOperation::Load:
			stencilOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		case RenderPassOperation::Clear:
			stencilOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		default:
			stencilOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
	}

	// Color attachments
	u32 c = 0;
	for (; c < output.numColorFormats; ++c) 
	{
		VkAttachmentDescription& colorAttachment = colorAttachments[c];
		colorAttachment.format = output.colorFormats[c];
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = colorOp;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = stencilOp;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = colorInitialLayout;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& colorAttachmentRef = colorAttachmentsRef[c];
		colorAttachmentRef.attachment = c;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// Depth attachment
	VkAttachmentDescription depthAttachment{};
	VkAttachmentReference depthAttachmentRef{};

	if (output.depthStencilFormat != VK_FORMAT_UNDEFINED) 
	{
		depthAttachment.format = output.depthStencilFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = depthOp;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = stencilOp;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = depthInitialLayout;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depthAttachmentRef.attachment = c;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// Create subpass.
	// TODO: for now is just a simple subpass, evolve API.
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// Calculate active attachments for the subpass
	VkAttachmentDescription attachments[MaxImageOutputs + 1]{};
	u32 activeAttachments = 0;
	for (; activeAttachments < output.numColorFormats; ++activeAttachments)
	{
		attachments[activeAttachments] = colorAttachments[activeAttachments];
	}
	subpass.colorAttachmentCount = activeAttachments ? activeAttachments - 1 : 0;
	subpass.pColorAttachments = colorAttachmentsRef;

	subpass.pDepthStencilAttachment = nullptr;

	u32 depth_stencil_count = 0;
	if (output.depthStencilFormat != VK_FORMAT_UNDEFINED) 
	{
		attachments[subpass.colorAttachmentCount] = depthAttachment;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		depth_stencil_count = 1;
	}

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

	renderPassInfo.attachmentCount = (activeAttachments ? activeAttachments - 1 : 0) + depth_stencil_count;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// Create external subpass dependencies
	//VkSubpassDependency external_dependencies[ 16 ];
	//u32 num_external_dependencies = 0;

	VkRenderPass vkRenderPass;
	VK_CHECK(vkCreateRenderPass(mVkDevice, &renderPassInfo, mVkAllocationCallbacks, &vkRenderPass));
	setResourceName(VK_OBJECT_TYPE_RENDER_PASS, (u64)vkRenderPass, name);
	return vkRenderPass;
}

RenderPassOutput GPUDevice::fillRendePassOutput(const RenderPassCreation& creation)
{
	RenderPassOutput output;
	output.reset();
	for (u32 i = 0; i < creation.numRenderTargets; ++i) 
	{
		Texture* texture = accessTexture(creation.outputTextures[i]);
		output.color(texture->vkFormat);
	}
	if (creation.depthStencilTexture.index != InvalidIndex) 
	{
		Texture* depthTexture = accessTexture(creation.depthStencilTexture);
		output.depth(depthTexture->vkFormat);
	}

	output.colorOperation = creation.colorOperation;
	output.depthOperation = creation.depthOperation;
	output.stencilOperation = creation.stencilOperation;
	return output;
}

RenderPassHandle GPUDevice::createRenderPass(const RenderPassCreation& creation) 
{
	RenderPassHandle handle = { mRenderPasses.obtainResource() };
	if (handle.index == InvalidIndex) 
	{
		return handle;
	}

	RenderPass* renderPass = accessRenderPass(handle);
	renderPass->type = creation.type;
	renderPass->numRenderTargets = (u8)creation.numRenderTargets;
	renderPass->dispatchX = 0;
	renderPass->dispatchY = 0;
	renderPass->dispatchZ = 0;
	renderPass->name = creation.name;
	renderPass->vkFrameBuffer = nullptr;
	renderPass->vkRenderPass = nullptr;
	renderPass->scaleX = creation.scaleX;
	renderPass->scaleY = creation.scaleY;
	renderPass->resize = creation.resize;

	// Cache texture handles
	u32 c = 0;
	for (; c < creation.numRenderTargets; ++c) 
	{
		Texture* texture = accessTexture(creation.outputTextures[c]);

		renderPass->width = texture->width;
		renderPass->height = texture->height;

		// Cache texture handles
		renderPass->outputTextures[c] = creation.outputTextures[c];
	}

	renderPass->outputDepth = creation.depthStencilTexture;

	switch (creation.type) 
	{
		case RenderPassType::Swapchain:
		{
			createSwapchainPass(creation, renderPass);
			break;
		}

		case RenderPassType::Compute:
		{
			break;
		}

		case RenderPassType::Geometry:
		{
			renderPass->output = fillRendePassOutput(creation);
			renderPass->vkRenderPass = getVulkanRenderPass(renderPass->output, creation.name);
			createFramebuffer(renderPass, creation.outputTextures, creation.numRenderTargets, creation.depthStencilTexture);
			break;
		}
	}
	return handle;
}

// Resource Destruction /////////////////////////////////////////////////////////

void GPUDevice::destroyBuffer(BufferHandle buffer) 
{
	if (buffer.index < mBuffers.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::Buffer, buffer.index, mCurrentFrame });
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid Buffer %u\n", buffer.index);
	}
}

void GPUDevice::destroyTexture(TextureHandle texture) {
	if (texture.index < mTextures.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::Texture, texture.index, mCurrentFrame });
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid Texture %u\n", texture.index);
	}
}

void GPUDevice::destroyPipeline(PipelineHandle pipeline) {
	if (pipeline.index < mPipelines.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::Pipeline, pipeline.index, mCurrentFrame });
		// Shader state creation is handled internally when creating a pipeline, thus add this to track correctly.
		destroyShaderState(accessPipeline(pipeline)->shaderState);
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid Pipeline %u\n", pipeline.index);
	}
}

void GPUDevice::destroySampler(SamplerHandle sampler) 
{
	if (sampler.index < mSamplers.mPoolSize)
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::Sampler, sampler.index, mCurrentFrame });
	}
	else
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid Sampler %u\n", sampler.index);
	}
}

void GPUDevice::destroyDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayout) 
{
	if (descriptorSetLayout.index < mDescriptorSetLayouts.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::DescriptorSetLayout, descriptorSetLayout.index, mCurrentFrame });
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid DescriptorSetLayout %u\n", descriptorSetLayout.index);
	}
}

void GPUDevice::destroyDescriptorSet(DescriptorSetHandle descriptorSet) 
{
	if (descriptorSet.index < mDescriptorSets.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::DescriptorSet, descriptorSet.index, mCurrentFrame });
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to free invalid DescriptorSet %u\n", descriptorSet.index);
	}
}

void GPUDevice::destroyRenderPass(RenderPassHandle render_pass) 
{
	if (render_pass.index < mRenderPasses.mPoolSize) {
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::RenderPass, render_pass.index, mCurrentFrame });
	}
	else {
		KS_CORE_ERROR("Graphics error: trying to free invalid RenderPass %u\n", render_pass.index);
	}
}

void GPUDevice::destroyShaderState(ShaderStateHandle shader) 
{
	if (shader.index < mShaders.mPoolSize) 
	{
		mResourceDeletionQueue.pushBack({ ResourceDeletionType::ShaderState, shader.index, mCurrentFrame });
	}
	else {
		KS_CORE_ERROR("Graphics error: trying to free invalid Shader %u\n", shader.index);
	}
}

// Real destruction methods - the other enqueue only the resources.
void GPUDevice::destroyBufferInstant(ResourceHandle bufferHandle) 
{
	Buffer* buffer = static_cast<Buffer*>(mBuffers.accessResource(bufferHandle));

	if (buffer && buffer->parentBufferHandle.index == InvalidBuffer.index) 
	{
		vmaDestroyBuffer(mVmaAllocator, buffer->vkBuffer, buffer->vmaAllocation);
	}
	mBuffers.releaseResource(bufferHandle);
}

void GPUDevice::destroyTextureInstant(ResourceHandle textureHandle) 
{
	Texture* texture = static_cast<Texture*>(mTextures.accessResource(textureHandle));
	if (texture) 
	{
		vkDestroyImageView(mVkDevice, texture->vkImageView, mVkAllocationCallbacks);
		vmaDestroyImage(mVmaAllocator, texture->vkImage, texture->vmaAllocation);
	}
	mTextures.releaseResource(textureHandle);
}

void GPUDevice::destroyPipelineInstant(ResourceHandle pipelineHandle) 
{
	Pipeline* pipeline = static_cast<Pipeline*>(mPipelines.accessResource(pipelineHandle));
	if (pipeline) 
	{
		vkDestroyPipeline(mVkDevice, pipeline->vkPipeline, mVkAllocationCallbacks);
		vkDestroyPipelineLayout(mVkDevice, pipeline->vkPipelineLayout, mVkAllocationCallbacks);
	}
	mPipelines.releaseResource(pipelineHandle);
}

void GPUDevice::destroySamplerInstant(ResourceHandle samplerHandle) 
{
	Sampler* sampler = static_cast<Sampler*>(mSamplers.accessResource(samplerHandle));
	if (sampler) 
	{
		vkDestroySampler(mVkDevice, sampler->vkSampler, mVkAllocationCallbacks);
	}
	mSamplers.releaseResource(samplerHandle);
}

void GPUDevice::destroyDescriptorSetLayoutInstant(ResourceHandle descriptorSetLayoutHandle) 
{
	DesciptorSetLayout* descriptorSetLayout = static_cast<DesciptorSetLayout*>(mDescriptorSetLayouts.accessResource(descriptorSetLayoutHandle));
	if (descriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(mVkDevice, descriptorSetLayout->vkDescriptorSetLayout, mVkAllocationCallbacks);
		// This contains also vk_binding allocation.
		kfree(descriptorSetLayout->bindings, mSystemAllocator);
	}
	mDescriptorSetLayouts.releaseResource(descriptorSetLayoutHandle);
}

void GPUDevice::destroyDescriptorSetInstant(ResourceHandle descriptorSetHandle) 
{
	DesciptorSet* descriptorSet = static_cast<DesciptorSet*>(mDescriptorSets.accessResource(descriptorSetHandle));
	if (descriptorSet)
	{
		// Contains the allocation for all the resources, binding and samplers arrays.
		kfree(descriptorSet->resources, mSystemAllocator);
		// This is freed with the DescriptorSet pool.
		//vkFreeDescriptorSets
	}
	mDescriptorSets.releaseResource(descriptorSetHandle);
}

void GPUDevice::destroyRenderPassInstant(ResourceHandle renderPassHandle) 
{
	RenderPass* renderPass = static_cast<RenderPass*>(mRenderPasses.accessResource(renderPassHandle));
	if (renderPass) 
	{
		if (renderPass->numRenderTargets)
		{
			vkDestroyFramebuffer(mVkDevice, renderPass->vkFrameBuffer, mVkAllocationCallbacks);
		}

		// NOTE: this is now destroyed with the render pass cache, to avoid double deletes.
		//vkDestroyRenderPass( vulkan_device, v_render_pass->vk_render_pass, vulkan_allocation_callbacks );
	}
	mRenderPasses.releaseResource(renderPassHandle);
}

void GPUDevice::destroyShaderStateInstant(ResourceHandle shader) 
{
	ShaderState* shaderState = static_cast<ShaderState*>(mShaders.accessResource(shader));
	if (shaderState) 
	{
		for (size_t i = 0; i < shaderState->activeShaders; ++i) 
		{
			vkDestroyShaderModule(mVkDevice, shaderState->shaderStageInfo[i].module, mVkAllocationCallbacks);
		}
	}
	mShaders.releaseResource(shader);
}

void GPUDevice::destroySwapchain()
{
	for (size_t i = 0; i < mVkSwapchainImageCount; ++i)
	{
		vkDestroyImageView(mVkDevice, mVkSwapchainImageViews[i], mVkAllocationCallbacks);
	}
	vkDestroyFramebuffer(mVkDevice, mVkSwapchainFramebuffers, mVkAllocationCallbacks);
	vkDestroySwapchainKHR(mVkDevice, mVkSwapchain, mVkAllocationCallbacks);
}

VkRenderPass GPUDevice::getVulkanRenderPass(const RenderPassOutput& output, cstring name)
{
	// Hash the memory output and find a compatible VkRenderPass.
   // In current form RenderPassOutput should track everything needed, including load operations.
	u64 hashedMemory = Kenshin::hash_bytes((void*)&output, sizeof(RenderPassOutput));
	VkRenderPass vkRenderPass = mRenderPassCache.get(hashedMemory);
	if (vkRenderPass)
	{
		return vkRenderPass;
	}
	vkRenderPass = createRenderPass(output, name);
	mRenderPassCache.insert(hashedMemory, vkRenderPass);

	return vkRenderPass;
}

void GPUDevice::setResourceName(VkObjectType type, u64 handle, const char* name) 
{
	if (!mdebugUtilsExtensionPresent)
	{
		return;
	}
	VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	nameInfo.objectType = type;
	nameInfo.objectHandle = handle;
	nameInfo.pObjectName = name;
	mDebugUtilsSetObjectName(mVkDevice, &nameInfo);
}

void GPUDevice::pushMarker(VkCommandBuffer cmdBuffer, cstring name) 
{
	VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	label.pLabelName = name;
	label.color[0] = 1.0f;
	label.color[1] = 1.0f;
	label.color[2] = 1.0f;
	label.color[3] = 1.0f;
	mDebugUtilsBeginLabel(cmdBuffer, &label);
}

void GPUDevice::popMarker(VkCommandBuffer cmdBuffer) 
{
	mDebugUtilsEndLabel(cmdBuffer);
}

void GPUDevice::resizeTexture(Texture* texture, Texture* textureDelete, u16 width, u16 height, u16 depth)
{
	// Cache handles to be delayed destroyed
	textureDelete->vkImageView = texture->vkImageView;
	textureDelete->vkImage = texture->vkImage;
	textureDelete->vmaAllocation = texture->vmaAllocation;

	// Re-create image in place.
	TextureCreation tc;
	tc.setFlags(texture->mipmaps, texture->flags)
	  .setFormatType(texture->vkFormat, texture->type)
      .setName(texture->name)
      .setSize(width, height, depth)
	  .setUsage(texture->vkUsage);
	createTexture(tc, texture->handle, texture);
}

void GPUDevice::createTexture(const TextureCreation& creation, TextureHandle handle, Texture* texture)
{
	texture->width = creation.mWidth;
	texture->height = creation.mHeight;
	texture->depth = creation.mDepth;
	texture->mipmaps = creation.mMipmaps;
	texture->type = creation.mType;
	texture->name = creation.mName;
	texture->vkFormat = creation.mFormat;
	texture->sampler = nullptr;
	texture->flags = creation.mFlags;
	texture->handle = handle;
	texture->vkUsage = creation.mUsage;

	//// Create the image
	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.format = texture->vkFormat;
	imageInfo.flags = 0;
	imageInfo.imageType = toVkImageType(creation.mType);
	imageInfo.extent.width = creation.mWidth;
	imageInfo.extent.height = creation.mHeight;
	imageInfo.extent.depth = creation.mDepth;
	imageInfo.mipLevels = creation.mMipmaps;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = creation.mUsage;

	const bool isRenderTarget = (creation.mFlags & TextureFlags::RenderTargetMask) == TextureFlags::RenderTargetMask;
	const bool isComputeUsed = (creation.mFlags & TextureFlags::ComputeMask) == TextureFlags::ComputeMask;

	// Default to always readable from shader.
	imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	imageInfo.usage |= isComputeUsed ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

	if (TextureFormat::hasDepthOrStencil(creation.mFormat)) 
	{
		// Depth/Stencil textures are normally textures you render into.
		imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	}
	else 
	{
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // TODO
		imageInfo.usage |= isRenderTarget ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
	}

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(mVmaAllocator, &imageInfo, &memory_info, &texture->vkImage, &texture->vmaAllocation, nullptr));

	setResourceName(VK_OBJECT_TYPE_IMAGE, (u64)texture->vkImage, creation.mName);

	//// Create the image view
	VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	info.image = texture->vkImage;
	info.viewType = toVkImageViewType(creation.mType);
	info.format = imageInfo.format;

	if (TextureFormat::hasDepthOrStencil(creation.mFormat)) 
	{
		info.subresourceRange.aspectMask = TextureFormat::hasDepth(creation.mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
		// TODO:gs
		//info.subresourceRange.aspectMask |= TextureFormat::has_stencil( creation.format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
	}
	else 
	{
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	info.subresourceRange.levelCount = 1;
	info.subresourceRange.layerCount = 1;
	VK_CHECK(vkCreateImageView(mVkDevice, &info, mVkAllocationCallbacks, &texture->vkImageView));

	setResourceName(VK_OBJECT_TYPE_IMAGE_VIEW, (u64)texture->vkImageView, creation.mName);

	texture->vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void GPUDevice::resizeSwapchain() 
{
	vkDeviceWaitIdle(mVkDevice);
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mVkPhysicalDevice, mVkWindowSurface, &surfaceCapabilities);
	VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;

	if (swapchainExtent.width == 0 || swapchainExtent.height == 0) 
	{
		return;
	}

	RenderPass* swapchainPass = accessRenderPass(mSwapchainPass);
	vkDestroyRenderPass(mVkDevice, swapchainPass->vkRenderPass, mVkAllocationCallbacks);

	destroySwapchain();
	vkDestroySurfaceKHR(mVkInstance, mVkWindowSurface, mVkAllocationCallbacks);

	if (!SDL_Vulkan_CreateSurface(mWindow, mVkInstance, mVkAllocationCallbacks, &mVkWindowSurface))
	{
		KS_CORE_ERROR("Failed to create Vulkan surface.\n");
	}
	createSwapchain();
	RenderPassCreation swapchainPassCreation = {};
	swapchainPassCreation.setType(RenderPassType::Swapchain).setName("Swapchain");
	createSwapchainPass(swapchainPassCreation, swapchainPass);
	vkDeviceWaitIdle(mVkDevice);
}

void GPUDevice::resizeDrawingImage()
{
	destroyDescriptorSet(mDefaultComputeDescriptorSet);

	TextureHandle colorTextureToDeleteHandle = { mTextures.obtainResource() };
	Texture* colorTextureToDelete = accessTexture(colorTextureToDeleteHandle);
	Texture* textureToUpdate = accessTexture(mDrawingImage);
	colorTextureToDelete->handle = colorTextureToDeleteHandle;
	resizeTexture(textureToUpdate, colorTextureToDelete, mSwapchainWidth, mSwapchainHeight, 1);
	mDrawingImage = textureToUpdate->handle;
	destroyTexture(colorTextureToDeleteHandle);

	TextureHandle depthTextureToDeleteHandle = { mTextures.obtainResource() };
	Texture*depthTextureToDelete = accessTexture(depthTextureToDeleteHandle);
	depthTextureToDelete->handle = depthTextureToDeleteHandle;
	Texture* depthTexture = accessTexture(mDepthTexture);
	resizeTexture(depthTexture, depthTextureToDelete, mSwapchainWidth, mSwapchainHeight, 1);
	mDepthTexture = depthTexture->handle;
	destroyTexture(depthTextureToDeleteHandle);

	DescriptorSetCreation computeDsCreation{};
	computeDsCreation.reset()
		.setLayout(mDefaultComputeDescriptorSetLayout)
		.setName("DefaultComputeDescriptorSet")
		.texture(mDrawingImage, 0);
	mDefaultComputeDescriptorSet = createDescriptorSet(computeDsCreation);
}

void GPUDevice::updateDescriptorSet(DescriptorSetHandle descriptorSet) 
{
	if (descriptorSet.index < mDescriptorSets.mPoolSize) 
	{
		DescriptorSetUpdate newUpdate = { descriptorSet, mCurrentFrame };
		mdDescriptorSetUpdatesQueue.pushBack(newUpdate);
	}
	else 
	{
		KS_CORE_ERROR("Graphics error: trying to update invalid DescriptorSet %u\n", descriptorSet.index);
	}
}

void GPUDevice::updateDescriptorSetInstant(const DescriptorSetUpdate& update) 
{
	// Use a dummy descriptor set to delete the vulkan descriptor set handle
	DescriptorSetHandle descriptorSetDeleteHandle = { mDescriptorSets.obtainResource() };
	DesciptorSet* descriptorSetDelete = accessDescriptorSet(descriptorSetDeleteHandle);

	DesciptorSet* descriptorSet = accessDescriptorSet(update.descriptorSetHandle);
	const DesciptorSetLayout* descriptorSetLayout = descriptorSet->layout;

	descriptorSetDelete->vkDescriptorSet = descriptorSet->vkDescriptorSet;
	descriptorSetDelete->bindings = nullptr;
	descriptorSetDelete->resources = nullptr;
	descriptorSetDelete->samplers = nullptr;
	descriptorSetDelete->numResources = 0;

	destroyDescriptorSet(descriptorSetDeleteHandle);

	// Allocate the new descriptor set and update its content.
	VkWriteDescriptorSet descriptorWrite[8];
	VkDescriptorBufferInfo bufferInfo[8];
	VkDescriptorImageInfo imageInfo[8];

	Sampler* defaultSampler = accessSampler(mDefaultSampler);

	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mVkDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout->vkDescriptorSetLayout;
	vkAllocateDescriptorSets(mVkDevice, &allocInfo, &descriptorSet->vkDescriptorSet);

	u32 numResources = descriptorSetLayout->numBindings;
	fillWriteDescriptorSets(*this, descriptorSetLayout, descriptorSet->vkDescriptorSet, descriptorWrite, bufferInfo, imageInfo, defaultSampler->vkSampler,
		numResources, descriptorSet->resources, descriptorSet->samplers, descriptorSet	->bindings);

	vkUpdateDescriptorSets(mVkDevice, numResources, descriptorWrite, 0, nullptr);
}

void GPUDevice::resizeOutputTextures(RenderPassHandle rendePassHandle, u32 width, u32 height) 
{
	// For each texture, create a temporary pooled texture and cache the handles to delete.
	// This is because we substitute just the Vulkan texture when resizing so that
	// external users don't need to update the handle.
	RenderPass* renderPass = accessRenderPass(rendePassHandle);
	if (renderPass) 
	{
		// No need to resize!
		if (!renderPass->resize) 
		{
			return;
		}

		// Calculate new width and height based on render pass sizing informations.
		u16 newWidth = (u16)(width * renderPass->scaleX);
		u16 newHeight = (u16)(height * renderPass->scaleY);

		// Resize textures if needed
		const u32 rts = renderPass->numRenderTargets;
		for (u32 i = 0; i < rts; ++i) 
		{
			TextureHandle textureHandle = renderPass->outputTextures[i];
			Texture* texture = accessTexture(textureHandle);

			if (texture->width == newWidth && texture->height == newHeight) 
			{
				continue;
			}

			// Queue deletion of texture by creating a temporary one
			TextureHandle textureToDeleteHandle = { mTextures.obtainResource() };
			Texture* textureToDelete = accessTexture(textureToDeleteHandle);
			// Update handle so it can be used to update bindless to dummy texture.
			textureToDelete->handle = textureToDeleteHandle;
			resizeTexture(texture, textureToDelete, newWidth, newHeight, 1);
			destroyTexture(textureToDeleteHandle);
		}

		if (renderPass->outputDepth.index != InvalidIndex)
		{
			Texture* texture = accessTexture(renderPass->outputDepth);

			if (texture->width != newWidth || texture->height != newHeight)
			{
				// Queue deletion of texture by creating a temporary one
				TextureHandle textureToDeleteHandle = { mTextures.obtainResource() };
				Texture* textureToDelete = accessTexture(textureToDeleteHandle);
				// Update handle so it can be used to update bindless to dummy texture.
				textureToDelete->handle = textureToDeleteHandle;
				resizeTexture(texture, textureToDelete, newWidth, newHeight, 1);
				destroyTexture(textureToDeleteHandle);
			}
		}

		// Again: create temporary resource to use the standard deferred deletion mechanism.
		RenderPassHandle render_pass_to_destroy = { mRenderPasses.obtainResource() };
		RenderPass* vk_render_pass_to_destroy = accessRenderPass(render_pass_to_destroy);

		vk_render_pass_to_destroy->vkFrameBuffer = renderPass->vkFrameBuffer;
		// This is checked in the destroy method to proceed with frame buffer destruction.
		vk_render_pass_to_destroy->numRenderTargets = 1;
		// Set this to 0 so deletion won't be performed.
		vk_render_pass_to_destroy->vkRenderPass = 0;

		destroyRenderPass(render_pass_to_destroy);

		// Update render pass size
		renderPass->width = newWidth;
		renderPass->height = newHeight;

		// Recreate framebuffer if present (mainly for dispatch only passes)
		if (renderPass->vkFrameBuffer) {
			createFramebuffer(renderPass, renderPass->outputTextures, renderPass->numRenderTargets, renderPass->outputDepth);
		}
	}
}

void GPUDevice::fillBarrier(RenderPassHandle renderPassHandle, ExecutionBarrier& outBarrier) 
{
	RenderPass* renderPass = accessRenderPass(renderPassHandle);
	outBarrier.numImageBarriers = 0;
	if (renderPass) 
	{
		const u32 rts = renderPass->numRenderTargets;
		for (u32 i = 0; i < rts; ++i) 
		{
			outBarrier.imageBarriers[outBarrier.numImageBarriers++].texture = renderPass->outputTextures[i];
		}

		if (renderPass->outputDepth.index != InvalidIndex) 
		{
			outBarrier.imageBarriers[outBarrier.numImageBarriers++].texture = renderPass->outputDepth;
		}
	}
}

void GPUDevice::newFrame() 
{
	// Fence wait and reset
	VkFence* renderComplateFence = &mVkCommandBufferExecutedFence[mCurrentFrame];
	if (vkGetFenceStatus(mVkDevice, *renderComplateFence) != VK_SUCCESS) 
	{
		vkWaitForFences(mVkDevice, 1, renderComplateFence, VK_TRUE, UINT64_MAX);
	}
	vkResetFences(mVkDevice, 1, renderComplateFence);
	VkResult result = vkAcquireNextImageKHR(mVkDevice, mVkSwapchain, UINT64_MAX, mVkImageAcquiredSemaphore[mCurrentFrame], VK_NULL_HANDLE, &mVkImageIndex);
	//TODO:other result?
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		mResized = true;
		return;
	}

	mCommandbufferManager.resetCommandPool(mCurrentFrame);
	// Dynamic memory update
	const u32 usedSize = mDynamicAllocatedSize - (mDynamicPerFrameSize * mPreviousFrame);
	mDynamicMaxPerFrameSize = std::max(usedSize, mDynamicMaxPerFrameSize);
	mDynamicAllocatedSize = mDynamicPerFrameSize * mCurrentFrame;

	// Descriptor Set Updates
	if (mdDescriptorSetUpdatesQueue.size()) 
	{
		for (i32 i = mdDescriptorSetUpdatesQueue.size() - 1; i >= 0; --i)
		{
			DescriptorSetUpdate& update = mdDescriptorSetUpdatesQueue[i];
			//if ( update.frame_issued == current_frame )
			{
				updateDescriptorSetInstant(update);
				update.frameIssued = u32_max;
				mdDescriptorSetUpdatesQueue.deleteSwap(i);
			}
		}
	}
}

void GPUDevice::present() 
{
	VkFence*	 renderCompleteFence     = &mVkCommandBufferExecutedFence[mCurrentFrame];
	VkSemaphore* renderCompleteSemaphore = &mVkRenderCompleteSemaphore[mVkImageIndex];

	// Copy all commands
	VkCommandBuffer enqueuedcmdBuffers[128];
	for (u32 i = 0; i < mNumQueuedCommandBuffers; ++i) 
	{
		CommandBuffer* cmdBuffer = mQueuedCommandBuffers[i];
		enqueuedcmdBuffers[i] = cmdBuffer->mCommandBuffer;
		// NOTE: why it was needing current_pipeline to be setup ?
		if (cmdBuffer->mIsRecording && cmdBuffer->mCurrentRenderPass && (cmdBuffer->mCurrentRenderPass->type != RenderPassType::Compute))
		{
			vkCmdEndRenderPass(cmdBuffer->mCommandBuffer);
		}
		vkEndCommandBuffer(cmdBuffer->mCommandBuffer);
	}

	// Submit command buffers
	VkSemaphore waitSemaphores[]      = { mVkImageAcquiredSemaphore[mCurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo           = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount     = 1;
	submitInfo.pWaitSemaphores        = waitSemaphores;
	submitInfo.pWaitDstStageMask      = waitStages;
	submitInfo.commandBufferCount     = mNumQueuedCommandBuffers;
	submitInfo.pCommandBuffers        = enqueuedcmdBuffers;
	submitInfo.signalSemaphoreCount   = 1;
	submitInfo.pSignalSemaphores      = renderCompleteSemaphore;

	VK_CHECK(vkQueueSubmit(mVkQueue, 1, &submitInfo, *renderCompleteFence));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = renderCompleteSemaphore;

	VkSwapchainKHR swapChains[]    = { mVkSwapchain };
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapChains;
	presentInfo.pImageIndices      = &mVkImageIndex;
	presentInfo.pResults           = nullptr; // Optional
	VkResult result				   = vkQueuePresentKHR(mVkQueue, &presentInfo);
	mNumQueuedCommandBuffers	   = 0;

	// GPU Timestamp resolve
	if (mTimeStampsEnabled) 
	{
		if (mGpuTimestampManager->hasValidQueries()) 
		{
			// Query GPU for all timestamps.
			const u32 query_offset = (mCurrentFrame * mGpuTimestampManager->queriesPerFrame) * 2;
			const u32 query_count = mGpuTimestampManager->currentQuery * 2;
			VK_CHECK(vkGetQueryPoolResults(mVkDevice, mVkTimestampQueryPool, query_offset, query_count,
				sizeof(u64) * query_count * 2, &mGpuTimestampManager->timestampsData[query_offset],
				sizeof(mGpuTimestampManager->timestampsData[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

			// Calculate and cache the elapsed time
			for (u32 i = 0; i < mGpuTimestampManager->currentQuery; ++i)
			{
				u32 index				= (mCurrentFrame * mGpuTimestampManager->queriesPerFrame) + i;
				GPUTimestamp& timestamp = mGpuTimestampManager->timestamps[index];
				double start            = (double)mGpuTimestampManager->timestampsData[(index * 2)];
				double end              = (double)mGpuTimestampManager->timestampsData[(index * 2) + 1];
				double range            = end - start;
				double elapsedTime      = range * mGpuTimestampFrequency;

				timestamp.elapsedMs     = elapsedTime;
				timestamp.frameIndex    = mAbsoluteFrame;
			}
		}
		else if (mGpuTimestampManager->currentQuery)
		{
			KS_CORE_WARN("Asymmetrical GPU queries, missing pop of some markers!\n");
		}
		mGpuTimestampManager->reset();
		mGpuTimestampReset = true;
	}
	else 
	{
		mGpuTimestampReset = false;
	}


	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
	{
		//resize();
		//frameCountersAdvance();
		mResized = true;
		return;
	}

	// This is called inside resize_swapchain as well to correctly work.
	frameCountersAdvance();

	// Resource deletion using reverse iteration and swap with last element.
	if (mResourceDeletionQueue.size() > 0)
	{
		for (i32 i = mResourceDeletionQueue.size() - 1; i >= 0; --i)
		{
			ResourceUpdate& resourceDeletion = mResourceDeletionQueue[i];

			if (resourceDeletion.currentFrame == mCurrentFrame)
			{
				switch (resourceDeletion.type) 
				{
					case ResourceDeletionType::Buffer:
					{
						destroyBufferInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::Pipeline:
					{
						destroyPipelineInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::RenderPass:
					{
						destroyRenderPassInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::DescriptorSet:
					{
						destroyDescriptorSetInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::DescriptorSetLayout:
					{
						destroyDescriptorSetLayoutInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::Sampler:
					{
						destroySamplerInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::ShaderState:
					{
						destroyShaderStateInstant(resourceDeletion.handle);
						break;
					}

					case ResourceDeletionType::Texture:
					{
						destroyTextureInstant(resourceDeletion.handle);
						break;
					}
				}

				// Mark resource as free
				resourceDeletion.currentFrame = u32_max;

				// Swap element
				mResourceDeletionQueue.deleteSwap(i);
			}
		}
	}
}

void GPUDevice::linkTextureSampler(TextureHandle textureHandle, SamplerHandle samplerHandle) 
{
	Texture* texture = accessTexture(textureHandle);
	Sampler* sampler = accessSampler(samplerHandle);
	texture->sampler = sampler;
}

void GPUDevice::frameCountersAdvance() 
{
	mPreviousFrame = mCurrentFrame;
	mCurrentFrame = (mCurrentFrame + 1) % mVkSwapchainImageCount;
	++mAbsoluteFrame;
}

void GPUDevice::queueCommandBuffer(CommandBuffer* cmdBuffer) 
{
	mQueuedCommandBuffers[mNumQueuedCommandBuffers++] = cmdBuffer;
}

CommandBuffer* GPUDevice::getCommandBuffer(QueueType::Enum type, bool begin) 
{
	CommandBuffer* cmdBuffer = mCommandbufferManager.getCommandBuffer(mCurrentFrame, begin);
	// The first commandbuffer issued in the frame is used to reset the timestamp queries used.
	if (mGpuTimestampReset && begin) 
	{
		// These are currently indices!
		//TODO?
		//vkCmdResetQueryPool(
		//	cmdBuffer->mCommandBuffer, 
		//	mVkTimestampQueryPool, 
		//	mCurrentFrame * mGpuTimestampManager->queriesPerFrame * 2,
		//	mGpuTimestampManager->queriesPerFrame
		//);
		mGpuTimestampReset = false;
	}

	return cmdBuffer;
}

CommandBuffer* GPUDevice::getInstantCommandBuffer() 
{
	CommandBuffer* cmdBuffer = mCommandbufferManager.getCommandBufferInstant(mCurrentFrame, false);
	return cmdBuffer;
}

void GPUDevice::queryBuffer(BufferHandle buffer, BufferDescription& outDescription) 
{
	if (buffer.index != InvalidIndex) 
	{
		const Buffer* bufferData = accessBuffer(buffer);
		outDescription.name = bufferData->name;
		outDescription.size = bufferData->size;
		outDescription.typeFlags = bufferData->typeFlags;
		outDescription.usage = bufferData->usage;
		outDescription.parentHandle = bufferData->parentBufferHandle;
		outDescription.nativeHandle = (void*)&bufferData->vkBuffer;
	}
}

void GPUDevice::queryTexture(TextureHandle textureHandle, TextureDescription& outDescription) 
{
	if (textureHandle.index != InvalidIndex)
	{
		const Texture* texture        = accessTexture(textureHandle);
		outDescription.width          = texture->width;
		outDescription.height         = texture->height;
		outDescription.depth          = texture->depth;
		outDescription.format         = texture->vkFormat;
		outDescription.mipmaps        = texture->mipmaps;
		outDescription.type           = texture->type;
		outDescription.renderTarget  = (texture->flags & TextureFlags::RenderTargetMask) == TextureFlags::RenderTargetMask;
		outDescription.computeAccess = (texture->flags & TextureFlags::ComputeMask) == TextureFlags::ComputeMask;
		outDescription.nativeHandle  = (void*)&texture->vkImage;
		outDescription.name			  = texture->name;
	}
}

void GPUDevice::queryPipeline(PipelineHandle pipelineHandle, PipelineDescription& outDescription) 
{
	if (pipelineHandle.index != InvalidIndex) 
	{
		const Pipeline* pipeline = accessPipeline(pipelineHandle);
		outDescription.shader = pipeline->shaderState;
	}
}

void GPUDevice::querySampler(SamplerHandle samplerHandle, SamplerDescription& outDescription) 
{
	if (samplerHandle.index != InvalidIndex)
	{
		const Sampler* sampler = accessSampler(samplerHandle);

		outDescription.addressModeU = sampler->addressModeU;
		outDescription.addressModeV = sampler->addressModeV;
		outDescription.addressModeW = sampler->addressModeW;
		outDescription.minFilter    = sampler->minFilter;
		outDescription.magFilter    = sampler->magFilter;
		outDescription.mipFilter    = sampler->mipFilter;
		outDescription.name		    = sampler->name;
	}
}

void GPUDevice::queryDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle, DescriptorSetLayoutDescription& outDescription) 
{
	if (descriptorSetLayoutHandle.index != InvalidIndex) 
	{
		const DesciptorSetLayout* descriptorSetLayout = accessDescriptorSetLayout(descriptorSetLayoutHandle);
		const u32 numBindings = descriptorSetLayout->numBindings;
		for (size_t i = 0; i < numBindings; i++) 
		{
			outDescription.bindings[i].name = descriptorSetLayout->bindings[i].name;
			outDescription.bindings[i].type = descriptorSetLayout->bindings[i].type;
		}
		outDescription.numActiveBindings = descriptorSetLayout->numBindings;
	}
}

void GPUDevice::queryDescriptorSet(DescriptorSetHandle descriptor_set, DesciptorSetDescription& out_description) {
	if (descriptor_set.index != InvalidIndex) {
		const DesciptorSet* descriptorSetData = accessDescriptorSet(descriptor_set);

		out_description.num_active_resources = descriptorSetData->numResources;
		for (u32 i = 0; i < out_description.num_active_resources; ++i) {
			//out_description.resources[ i ].data = descriptorSetData->resources[ i ].data;
		}
	}
}

const RenderPassOutput& GPUDevice::getRenderPassOutput(RenderPassHandle render_pass) const {
	const RenderPass* vulkan_render_pass = accessRenderPass(render_pass);
	return vulkan_render_pass->output;
}

void* GPUDevice::mapBuffer(const MapBufferParameters& parameters) 
{
	if (parameters.buffer.index == InvalidIndex)
	{
		return nullptr;
	}

	Buffer* buffer = accessBuffer(parameters.buffer);

	if (buffer->parentBufferHandle.index == mDynamicBuffer.index) 
	{
		buffer->globelBufferOffset = mDynamicAllocatedSize;
		return dynamicAllocate(parameters.size == 0 ? buffer->size : parameters.size);
	}
	void* data;
	vmaMapMemory(mVmaAllocator, buffer->vmaAllocation, &data);
	return data;
}

void GPUDevice::unmapBuffer(const MapBufferParameters& parameters) 
{
	if (parameters.buffer.index == InvalidIndex)
	{
		return;
	}

	Buffer* buffer = accessBuffer(parameters.buffer);
	if (buffer->parentBufferHandle.index == mDynamicBuffer.index)
	{
		return;
	}
	vmaUnmapMemory(mVmaAllocator, buffer->vmaAllocation);
}

void* GPUDevice::dynamicAllocate(u32 size) 
{
	void* mappedMemory = mDynamicMappedMemory + mDynamicAllocatedSize;
	//TODO:SSBO?
	mDynamicAllocatedSize += (u32)Kenshin::memoryAlign(size, mMinUBOAlignment);
	return mappedMemory;
}

void GPUDevice::setBufferGlobalOffset(BufferHandle bufferHandle, u32 offset) 
{
	if (bufferHandle.index == InvalidIndex)
	{
		return;
	}
	Buffer* buffer = accessBuffer(bufferHandle);
	buffer->globelBufferOffset = offset;
}

u32 GPUDevice::getGpuTimestamps(GPUTimestamp* outTimestamps) 
{
	return mGpuTimestampManager->resolve(mPreviousFrame, outTimestamps);
}

void GPUDevice::pushGpuTimestamp(CommandBuffer* cmdBuffer, const char* name) 
{
	if (!mTimeStampsEnabled)
	{
		return;
	}
	u32 queryIndex = mGpuTimestampManager->push(mCurrentFrame, name);
	vkCmdWriteTimestamp(cmdBuffer->mCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, mVkTimestampQueryPool, queryIndex);
}

void GPUDevice::popGpuTimestamp(CommandBuffer* cmdBuffer) 
{
	if (!mTimeStampsEnabled)
	{
		return;
	}

	u32 queryIndex = mGpuTimestampManager->pop(mCurrentFrame);
	vkCmdWriteTimestamp(cmdBuffer->mCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, mVkTimestampQueryPool, queryIndex);
}

BufferHandle GPUDevice::getFullscreenVBO() const 
{
	return mFullscreenVertexBuffer;
}

RenderPassHandle GPUDevice::getSwapchainPass() const 
{
	return mSwapchainPass;
}

TextureHandle GPUDevice::getDummyTexture() const 
{
	return mDummyTexture;
}

BufferHandle GPUDevice::getDummyConstantBuffer() const 
{
	return mDummyConstantBuffer;
}

void GPUDevice::resize(u16 width, u16 height) 
{
	mSwapchainWidth = width;
	mSwapchainHeight = height;
	mResized = true;
}

const ShaderState* GPUDevice::accessShaderState(ShaderStateHandle shaderHandle) const
{
	return static_cast<const ShaderState*>(mShaders.accessResource(shaderHandle.index));
}

ShaderState* GPUDevice::accessShaderState(ShaderStateHandle shaderHandle) 
{
	return const_cast<ShaderState*>(static_cast<const GPUDevice*>(this)->accessShaderState(shaderHandle));
}

const Texture* GPUDevice::accessTexture(TextureHandle textureHandle) const
{
	return static_cast<const Texture*>(mTextures.accessResource(textureHandle.index));
}

Texture* GPUDevice::accessTexture(TextureHandle textureHandle) 
{
	return const_cast<Texture*>(static_cast<const GPUDevice*>(this)->accessTexture(textureHandle));
}

const Buffer* GPUDevice::accessBuffer(BufferHandle bufferHandle) const 
{
	return static_cast<const Buffer*>(mBuffers.accessResource(bufferHandle.index));
}

Buffer* GPUDevice::accessBuffer(BufferHandle bufferHandle) 
{
	return const_cast<Buffer*>(static_cast<const GPUDevice*>(this)->accessBuffer(bufferHandle));
}

const Pipeline* GPUDevice::accessPipeline(PipelineHandle pipelineHandle) const 
{
	return static_cast<const Pipeline*>(mPipelines.accessResource(pipelineHandle.index));
}

Pipeline* GPUDevice::accessPipeline(PipelineHandle pipeline) 
{
	return const_cast<Pipeline*>(static_cast<const GPUDevice*>(this)->accessPipeline(pipeline));
}

const Sampler* GPUDevice::accessSampler(SamplerHandle samplerHandle) const 
{
	return static_cast<const Sampler*>(mSamplers.accessResource(samplerHandle.index));
}

Sampler* GPUDevice::accessSampler(SamplerHandle samplerHandle) 
{
	return const_cast<Sampler*>(static_cast<const GPUDevice*>(this)->accessSampler(samplerHandle));
}

const DesciptorSetLayout* GPUDevice::accessDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle) const 
{
	return static_cast<const DesciptorSetLayout*>(mDescriptorSetLayouts.accessResource(descriptorSetLayoutHandle.index));
}

DesciptorSetLayout* GPUDevice::accessDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle) 
{
	return const_cast<DesciptorSetLayout*>(static_cast<const GPUDevice*>(this)->accessDescriptorSetLayout(descriptorSetLayoutHandle));
}

const DesciptorSet* GPUDevice::accessDescriptorSet(DescriptorSetHandle descriptorSetHandle) const
{
	return static_cast<const DesciptorSet*>(mDescriptorSets.accessResource(descriptorSetHandle.index));
}

DesciptorSet* GPUDevice::accessDescriptorSet(DescriptorSetHandle descriptorSetHandle) 
{
	return const_cast<DesciptorSet*>(static_cast<const GPUDevice*>(this)->accessDescriptorSet(descriptorSetHandle));
}

const RenderPass* GPUDevice::accessRenderPass(RenderPassHandle renderPassHandle) const 
{
	return static_cast<const RenderPass*>(mRenderPasses.accessResource(renderPassHandle.index));
}

RenderPass* GPUDevice::accessRenderPass(RenderPassHandle renderPassHandle) 
{
	return const_cast<RenderPass*>(static_cast<const GPUDevice*>(this)->accessRenderPass(renderPassHandle));
}

KENSHIN_END
