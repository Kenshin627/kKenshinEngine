#include "pch.h"
#include "memory.h"
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
        //ZoneScoped;
        // New frame
        if (!window.mIsMinimized) 
        {
            renderer->beginFrame();
        }
        //input->new_frame();
        //window.handle_os_messages();

        if (window.mIsResized) 
        {
            //renderer->resize_swapchain( window.width, window.height );
            //on_resize( window.width, window.height );
            renderer->resizeSwapchain(window.mWidth, window.mHeight);
            window.mIsResized = false;
        }
        if (!window.mIsMinimized) 
        {
            Kenshin::CommandBuffer* cmd = renderer->getCommandBuffer(Kenshin::QueueType::Graphics, true);
            cmd->pushMarker("Frame");

            cmd->setClearColor(0.3f, 0.9f, 0.3f, 1.0f);
            cmd->setClearDepth(1.0);
			cmd->setClearStencil(0);
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
	return 0;
}