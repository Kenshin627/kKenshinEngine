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

				cmd->setClearColor(0.3f, 0.9f, 0.3f, 1.0f);
				cmd->setClearDepth(1.0);
				cmd->setClearStencil(0);
				cmd->bindPipeline(gpu->mDefaultComputePipeline);
				cmd->bindDescriptorSet(&gpu->mDefaultComputeDescriptorSet, 1, nullptr, 0);
				Kenshin::Texture* drawingImage = gpu->accessTexture(gpu->mDrawingImage);
				VkImage currentPresnetImage = gpu->mVkSwapchainImages[gpu->mVkImageIndex];
				gpu->transitionImageLayout(cmd->mCommandBuffer, drawingImage->vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, false);
				cmd->dispatch({ gpu->mSwapchainWidth / 16u ,gpu->mSwapchainHeight / 16u,1 });											 
				gpu->transitionImageLayout(cmd->mCommandBuffer, drawingImage->vkImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, false);
				gpu->transitionImageLayout(cmd->mCommandBuffer, currentPresnetImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);
				cmd->blitImage(drawingImage->vkImage, currentPresnetImage, { drawingImage->width, drawingImage->height }, { gpu->mSwapchainWidth, gpu->mSwapchainHeight });
				gpu->transitionImageLayout(cmd->mCommandBuffer, currentPresnetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);
				/*cmd->bindPass(gpu.getSwapchainPass());
				cmd->bindPipeline(cube_pipeline);
				cmd->setScissor(nullptr);
				cmd->setViewport(nullptr);*/

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