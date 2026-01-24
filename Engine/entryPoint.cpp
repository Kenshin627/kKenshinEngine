#include "pch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "timer.h"
#include "window.h"
#include "gpuDevice.h"
#include "resourceManager.h"
#include "renderer.h"

int main()
{
	int width = 1920;
	int height = 1080;
	Kenshin::Log::Init();
	KS_CORE_INFO("Eegine Initializing...");
	KS_CORE_INFO("Initializing Memory Service...");
	Kenshin::MemoryService::instance()->init();
	KS_CORE_INFO("Initializing Timer Service...");
	Kenshin::Timer::instance()->init();	
	KS_CORE_INFO("Initializing Allocators...");
	Kenshin::HeapAllocator* allocator = &Kenshin::MemoryService::instance()->mSystemAllocator;
	Kenshin::StackAllocator stackAllocator;
	stackAllocator.init(kmega(8));
	KS_CORE_INFO("Initializing Window...");
	Kenshin::WindowConfiguration winConfig{ allocator, width, height, "KenshinEngine" };
	Kenshin::Window& window = *Kenshin::Window::instance();
	window.init(&winConfig);

	//TODO:input handle

	KS_CORE_INFO("Initializing GPUDevice...");
	Kenshin::GPUDeviceConfiguration gpuConfig{};
	//gpuConfig.allocationCallbacks;
	gpuConfig.debug = false;
	gpuConfig.enableGpuTimeQueries = false;
	gpuConfig.gpuTimeQueriesPerFrame = 32;
	gpuConfig.width = width;
	gpuConfig.height = height;
	gpuConfig.systemAllocator = allocator;
	gpuConfig.stackAllocator = &stackAllocator;
	gpuConfig.window = Kenshin::Window::instance()->mWindow;
	Kenshin::GPUDevice* gpu = Kenshin::GPUDevice::instance();
	gpu->init(&gpuConfig);
	KS_CORE_INFO("Initializing ResourceManager...");
	Kenshin::ResourceManager rm;
	rm.init(allocator, nullptr);

	//TODO:?
	//Kenshin::GPUProfiler gpu_profiler;
	//gpu_profiler.init(allocator, 100);
	KS_CORE_INFO("Initializing Renderer...");
	Kenshin::Renderer* renderer = Kenshin::Renderer::instance();
	Kenshin::RendererCreation rendererCreation{};
	rendererCreation.gpu = gpu;
	rendererCreation.allocator = allocator;
	renderer->init(&rendererCreation);
	renderer->setLoaders(&rm);

	//testCode

	Kenshin::Vertex vertices[4] = {
		{ glm::vec3(-0.5f, -0.5f, 0.0f), 0, glm::vec2(0.0, 0.0) },
		{ glm::vec3( 0.5f, -0.5f, 0.0f), 0, glm::vec2(1.0, 0.0) },
		{ glm::vec3( 0.5f,  0.5f, 0.0f), 0, glm::vec2(1.0, 1.0) },
		{ glm::vec3(-0.5f,  0.5f, 0.0f), 0, glm::vec2(0.0, 1.0) }
	};

	u32 indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	//Kenshin::BufferCreation vbo{};
	//vbo.reset()
	//   .setData(vertices)
	//   .set(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Kenshin::ResourceUsageType::Immutable, sizeof(vertices))
	//   .setName("Quad VBO");
	//Kenshin::BufferHandle vertexHandle = gpu->createBuffer(vbo);
	//
	//Kenshin::BufferCreation ibo{};
	//ibo.reset()
	//	.setData(indices)
	//	.set(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Kenshin::ResourceUsageType::Immutable, sizeof(indices))
	//	.setName("Quad IBO");
	//Kenshin::BufferHandle indexHandle = gpu->createBuffer(ibo);
	Kenshin::BufferCreation vboCreation{};
	vboCreation.reset()
		.setName("Geometry VertexBuffer")
		.set
		(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		Kenshin::ResourceUsageType::Immutable, sizeof(vertices))
		.setData(vertices);
	Kenshin::BufferHandle vertexHandle = gpu->createBuffer(vboCreation);
	Kenshin::Buffer* vbo = gpu->accessBuffer(vertexHandle);
	VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr };	
	addressInfo.buffer = vbo->vkBuffer;
	vbo->mDeviceAddress = vkGetBufferDeviceAddress(gpu->getDevice(), &addressInfo);

	Kenshin::BufferCreation iboCreation{};
	iboCreation.reset()
		.setName("Geometry IndexBuffer")
		.set
		(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ,
			Kenshin::ResourceUsageType::Immutable, sizeof(indices))
		.setData(indices);
	Kenshin::BufferHandle indexHandle = gpu->createBuffer(iboCreation);


	Kenshin::DescriptorSetCreation mGraphicDescriptorSetCreation{};
	mGraphicDescriptorSetCreation.reset()
		.setLayout(gpu->mDefaultGraphicDescriptorSetLayout)
		.setName("DefaultGraphicDescriptorSet")
		.texture(gpu->mDefaultTexture, 0)
		.buffer(vertexHandle, 1);
	gpu->mDefaultGraphicDescriptorSet = gpu->createDescriptorSet(mGraphicDescriptorSetCreation);

	//RenderLoop
    while (!window.mIsQuit)
    {
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_EVENT_QUIT:
					window.mIsQuit = true;
					break;
				case SDL_EVENT_WINDOW_MINIMIZED:
					window.mIsMinimized = true;
					break;
				case SDL_EVENT_WINDOW_RESIZED:
					window.mWidth = static_cast<u32>(e.window.data1); //new width
					window.mHeight = static_cast<u32>(e.window.data2); //new height
					window.mIsMinimized = false;
					gpu->mResized = true;
					break;
			default:
				break;
			}
			
			//input->new_frame();
			//window.handle_os_messages();

			if (gpu->mResized)
			{
				//renderer->resize_swapchain( window.width, window.height );
				//on_resize( window.width, window.height );
				renderer->resizeSwapchain();
			}

			if (!window.mIsMinimized)
			{
				renderer->beginFrame();
			}

			if (!window.mIsMinimized)
			{				
				Kenshin::CommandBuffer* cmd = renderer->getCommandBuffer(Kenshin::QueueType::Graphics, true);
				cmd->pushMarker("Frame");
				
				//compute pipeline
				cmd->bindPipeline(gpu->mDefaultComputePipeline);
				cmd->bindDescriptorSet(&gpu->mDefaultComputeDescriptorSet, 1, nullptr, 0);
				Kenshin::Texture* drawingImage = gpu->accessTexture(gpu->mDrawingImage);
				VkImage currentPresnetImage = gpu->mVkSwapchainImages[gpu->mVkImageIndex];
				gpu->transitionImageLayout(cmd->mCommandBuffer, drawingImage->vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, false);
				cmd->dispatch({ gpu->mSwapchainWidth / 16u ,gpu->mSwapchainHeight / 16u,1 });											 
				gpu->transitionImageLayout(cmd->mCommandBuffer, drawingImage->vkImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);

				//graphic pipeline
				cmd->beginDynamicRendering(&drawingImage->handle, 1, { {0,0}, { drawingImage->width, drawingImage->height } });
				cmd->bindPipeline(gpu->mDefaultGraphicPipeline);
				Kenshin::Viewport vp{};
				vp.rect.x = 0;
				vp.rect.y = 0;
				vp.rect.width = drawingImage->width;
				vp.rect.height = drawingImage->height;
				vp.minDepth = 0;
				vp.maxDepth = 1.0f;
				cmd->setViewport(&vp);

				Kenshin::Rect2DInt scissor{};
				scissor.x = 0;
				scissor.y = 0;
				scissor.width = drawingImage->width;
				scissor.height = drawingImage->height;
				cmd->setScissor(&scissor);

				glm::vec4 color = { 1, 1, 1, 1 };
				cmd->bindDescriptorSet(&gpu->mDefaultGraphicDescriptorSet, 1, nullptr, 0);
				cmd->pushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4), &color);
				//BDA
				cmd->pushConstant(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec4), sizeof(u64), &vbo->mDeviceAddress);
				//cmd->bindVertexBuffer(vertexHandle, 0);
				cmd->bindIndexBuffer(indexHandle, VK_INDEX_TYPE_UINT32);
				cmd->drawIndex(6, 1, 0, 0, 0);
				cmd->endDynamicRendering();

				gpu->transitionImageLayout(cmd->mCommandBuffer, drawingImage->vkImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, false);
				gpu->transitionImageLayout(cmd->mCommandBuffer, currentPresnetImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);
				cmd->blitImage(drawingImage->vkImage, currentPresnetImage, { drawingImage->width, drawingImage->height }, { gpu->mSwapchainWidth, gpu->mSwapchainHeight });
				gpu->transitionImageLayout(cmd->mCommandBuffer, currentPresnetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);

				cmd->popMarker();

				//gpu_profiler.update(gpu);

				// Send commands to GPU
				renderer->queueCommandBuffer(cmd);
				renderer->endFrame();

			}
			/*else
			{
				ImGui::Render();
			}*/
		}
    }
	return 0;
}