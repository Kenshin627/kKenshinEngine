#pragma once
#include <vulkan/vulkan.h>
#include "platform.h"
#include "service.h"
#include "resourcePool.h"
#include "gpuResource.h"
#include "typeDefs.h"
#include "array.h"
#include "commandBufferService.h"
#include "stringBuffer.h"
#include "hash_map.hpp"

struct SDL_Window;

KENSHIN_BEGIN

struct CommandBuffer;
struct GPUTimestampManager;
struct GPUTimestamp;
class  Buffer;

struct GPUDeviceConfiguration
{
	u32                    width;
	u32                    height;
	Allocator*             systemAllocator;
    StackAllocator*        stackAllocator;
	void*                  window;
    VkAllocationCallbacks* allocationCallbacks{nullptr};
    u16                    gpuTimeQueriesPerFrame = 32;
    bool                   enableGpuTimeQueries = false;
    bool                   debug = false;
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

    // Creation/Destruction of resources /////////////////////////////////
    BufferHandle createBuffer(const BufferCreation& creation);
    TextureHandle createTexture(const TextureCreation& creation);
    void createTexture(const TextureCreation& creation, TextureHandle handle, Texture* texture);
    PipelineHandle createPipeline(const PipelineCreation& creation);
    SamplerHandle createSampler(const SamplerCreation& creation);
    DescriptorSetLayoutHandle createDescriptorSetLayout(const DescriptorSetLayoutCreation& creation);
    DescriptorSetHandle createDescriptorSet(const DescriptorSetCreation& creation);
    RenderPassHandle createRenderPass(const RenderPassCreation& creation);
    ShaderStateHandle createShaderState(const ShaderStateCreation& creation);

    void resizeTexture(Texture* texture, Texture* textureDelete, u16 width, u16 height, u16 depth);

    void destroyBuffer(BufferHandle buffer);
    void destroyTexture(TextureHandle texture);
    void destroyPipeline(PipelineHandle pipeline);
    void destroySampler(SamplerHandle sampler);
    void destroyDescriptorSetLayout(DescriptorSetLayoutHandle layout);
    void destroyDescriptorSet(DescriptorSetHandle set);
    void destroyRenderPass(RenderPassHandle render_pass);
    void destroyShaderState(ShaderStateHandle shader);

    ShaderState* accessShaderState(ShaderStateHandle shader);
    const ShaderState* accessShaderState(ShaderStateHandle shader) const;
    Texture* accessTexture(TextureHandle texture);
    const Texture* accessTexture(TextureHandle texture) const;
    Buffer* accessBuffer(BufferHandle buffer);
    const Buffer* accessBuffer(BufferHandle buffer) const;
    Pipeline* accessPipeline(PipelineHandle pipeline);
    const Pipeline* accessPipeline(PipelineHandle pipeline) const;
    Sampler* accessSampler(SamplerHandle sampler);
    const Sampler* accessSampler(SamplerHandle sampler) const;
    DesciptorSetLayout* accessDescriptorSetLayout(DescriptorSetLayoutHandle layout);
    const DesciptorSetLayout* accessDescriptorSetLayout(DescriptorSetLayoutHandle layout) const;
    DesciptorSet* accessDescriptorSet(DescriptorSetHandle set);
    const DesciptorSet* accessDescriptorSet(DescriptorSetHandle set) const;
    RenderPass* accessRenderPass(RenderPassHandle renderPass);
    const RenderPass* accessRenderPass(RenderPassHandle renderPass) const;

    // Query Description /////////////////////////////////////////////////
    void queryBuffer(BufferHandle buffer, BufferDescription& out_description);
    void queryTexture(TextureHandle texture, TextureDescription& out_description);
    void queryPipeline(PipelineHandle pipeline, PipelineDescription& out_description);
    void querySampler(SamplerHandle sampler, SamplerDescription& out_description);
    void queryDescriptorSetLayout(DescriptorSetLayoutHandle layout, DescriptorSetLayoutDescription& out_description);
    void queryDescriptorSet(DescriptorSetHandle set, DesciptorSetDescription& out_description);
    void queryShaderState(ShaderStateHandle shader, ShaderStateDescription& out_description);

    const RenderPassOutput& getRenderPassOutput(RenderPassHandle render_pass) const;

    // Update/Reload resources ///////////////////////////////////////////
    void resizeOutputTextures(RenderPassHandle rendePassHandle, u32 width, u32 height);
    void updateDescriptorSet(DescriptorSetHandle set);

    // Misc //////////////////////////////////////////////////////////////
    void linkTextureSampler(TextureHandle texture, SamplerHandle sampler);   // TODO: for now specify a sampler for a texture or use the default one.
         
    void setPresentMode(PresentMode::Enum mode);
         
    void frameCountersAdvance();
         
    bool getFamilyQueue(VkPhysicalDevice physical_device);

    VkShaderModuleCreateInfo compileShader(cstring code, u32 code_size, VkShaderStageFlagBits stage, cstring name);
    
    void resize();
    void setResize();

    // Swapchain //////////////////////////////////////////////////////////
    void createSwapchain();
    void destroySwapchain();
    void resizeSwapchain();
    void resizeDrawingImage();

    // Map/Unmap /////////////////////////////////////////////////////////
    void* mapBuffer(const MapBufferParameters& parameters);
    void  unmapBuffer(const MapBufferParameters& parameters);

    void* dynamicAllocate(u32 size);

    void setBufferGlobalOffset(BufferHandle buffer, u32 offset);

    // Command Buffers ///////////////////////////////////////////////////
    CommandBuffer* getCommandBuffer(QueueType::Enum type, bool begin);
    CommandBuffer* getInstantCommandBuffer();

    void queueCommandBuffer(CommandBuffer* cmdBuffer);          // Queue command buffer that will not be executed until present is called.

    // Rendering /////////////////////////////////////////////////////////
    void newFrame();
    void present();
    void resize(u16 width, u16 height);
    void fillBarrier(RenderPassHandle render_pass, ExecutionBarrier& outBarrier);
    BufferHandle getFullscreenVBO() const;
    // Returns what is considered the final pass that writes to the swapchain.
    RenderPassHandle getSwapchainPass() const;                  
    TextureHandle getDummyTexture() const;
    BufferHandle getDummyConstantBuffer() const;
    const RenderPassOutput& getSwapchainOutput() const { return mSwapchainOutput; }
    VkRenderPass getVulkanRenderPass(const RenderPassOutput& output, cstring name);

    // Names and markers /////////////////////////////////////////////////
    void setResourceName(VkObjectType object_type, uint64_t handle, const char* name);
    void pushMarker(VkCommandBuffer command_buffer, cstring name);
    void popMarker(VkCommandBuffer command_buffer);

    // GPU Timings ///////////////////////////////////////////////////////
    void setGpuTimestampsEnable(bool value) { mTimeStampsEnabled = value; }         
    u32  getGpuTimestamps(GPUTimestamp* out_timestamps);
    void pushGpuTimestamp(CommandBuffer* command_buffer, const char* name);
    void popGpuTimestamp(CommandBuffer* command_buffer);

    // Instant methods ///////////////////////////////////////////////////
    void destroyBufferInstant(ResourceHandle buffer);
    void destroyTextureInstant(ResourceHandle texture);
    void destroyPipelineInstant(ResourceHandle pipeline);
    void destroySamplerInstant(ResourceHandle sampler);
    void destroyDescriptorSetLayoutInstant(ResourceHandle layout);
    void destroyDescriptorSetInstant(ResourceHandle set);
    void destroyRenderPassInstant(ResourceHandle render_pass);
    void destroyShaderStateInstant(ResourceHandle shader);         
    void updateDescriptorSetInstant(const DescriptorSetUpdate& update);

    VkRenderPass createRenderPass(const RenderPassOutput& output, cstring name);
    RenderPassOutput fillRendePassOutput(const RenderPassCreation& creation);
    void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, bool isDepth);
	KS_SERVICE_TYPE(GPUDevice);
	constexpr static cstring typeName = "GPU Device Service";
private:
	VkDebugUtilsMessengerCreateInfoEXT buildDebugUtilsMessageCreateInfo();
	bool getQueuefamily(VkPhysicalDevice physicalDevice);
	VkPresentModeKHR toVkPresentMode(PresentMode::Enum mode);
    void dumpShaderCode(StringBuffer& tempStringBuffer, cstring code, VkShaderStageFlagBits stage, cstring name);
    void fillWriteDescriptorSets(GPUDevice& gpu, const DesciptorSetLayout* descriptor_set_layout, VkDescriptorSet vk_descriptor_set,
    VkWriteDescriptorSet* descriptor_write, VkDescriptorBufferInfo* buffer_info, VkDescriptorImageInfo* image_info,
    VkSampler vk_default_sampler, u32& num_resources, const ResourceHandle* resources, const SamplerHandle* samplers, const u16* bindings);
    void createSwapchainPass(const RenderPassCreation& creation, RenderPass* renderPass);
    void createFramebuffer(RenderPass* renderPass, const TextureHandle* outputTextures, u32 numRenderTargets, TextureHandle depthStencilTexture);
public:
    ResourcePool                     mBuffers;
    ResourcePool                     mTextures;
    ResourcePool                     mPipelines;
    ResourcePool                     mSamplers;
    ResourcePool                     mDescriptorSetLayouts;
    ResourcePool                     mDescriptorSets;
    ResourcePool                     mRenderPasses;
    ResourcePool                     mCommandBuffers;
    ResourcePool                     mShaders;
                                     
    // Primitive resources           
    BufferHandle                     mFullscreenVertexBuffer;
    RenderPassHandle                 mSwapchainPass;
    SamplerHandle                    mDefaultSampler;
    // Dummy resources               
    TextureHandle                    mDummyTexture;
    BufferHandle                     mDummyConstantBuffer;
                                     
    RenderPassOutput                 mSwapchainOutput;
                                     
    StringBuffer                     mStringBuffer;
                                     
    Allocator*                       mSystemAllocator;
    StackAllocator*                  mStackAllocator;
                                     
    u32                              mDynamicMaxPerFrameSize;
    BufferHandle                     mDynamicBuffer;
    u8*                              mDynamicMappedMemory;
    u32                              mDynamicAllocatedSize;
    u32                              mDynamicPerFrameSize;
                                     
    CommandBuffer**                  mQueuedCommandBuffers = nullptr;
    u32                              mNumAllocatedCommandBuffers = 0;
    u32                              mNumQueuedCommandBuffers = 0;
                                     
    PresentMode::Enum                mPresentMode = PresentMode::VsyncFast;
    u32                              mCurrentFrame;
    u32                              mPreviousFrame;
    u32                              mAbsoluteFrame;
                                     
    u16                              mSwapchainWidth = 1;
    u16                              mSwapchainHeight = 1;
                                     
    GPUTimestampManager*             mGpuTimestampManager = nullptr;
                                     
    bool                             mBindlessSupported = false;
    bool                             mTimeStampsEnabled = false;
    bool                             mResized = false;
    bool                             mVerticalSync = false;
                                     
    VkAllocationCallbacks*           mVkAllocationCallbacks{nullptr};
    VkInstance                       mVkInstance;
    VkPhysicalDevice                 mVkPhysicalDevice;
    VkPhysicalDeviceProperties       mVkPhysicalDeviceProperties;
    VkDevice                         mVkDevice;
    VkQueue                          mVkQueue;
    uint32_t                         mVkQueueFamilyIndex;
    VkDescriptorPool                 mVkDescriptorPool;
                                     
    // Swapchain                     
    SDL_Window*                      mWindow;
    VkImage                          mVkSwapchainImages[MaxSwapchainImages];
    VkImageView                      mVkSwapchainImageViews[MaxSwapchainImages];
    VkFramebuffer                    mVkSwapchainFramebuffers[MaxSwapchainImages];
                                     
    VkQueryPool                      mVkTimestampQueryPool;
    // Per frame synchronization     
    //Note: renderCompleteSemaphore allocation base swapchainImageCount.
    VkSemaphore                      mVkRenderCompleteSemaphore[MaxSwapchainImages];
    VkSemaphore                      mVkImageAcquiredSemaphore[MaxInFlightFrames];
    VkFence                          mVkCommandBufferExecutedFence[MaxInFlightFrames];
    TextureHandle                    mDepthTexture;

    // Windows specific
    VkSurfaceKHR                     mVkWindowSurface;
    VkSurfaceFormatKHR               mVkSurfaceFormat;
    VkPresentModeKHR                 mVkPresentMode;
    VkSwapchainKHR                   mVkSwapchain;
    u32                              mVkSwapchainImageCount;
    VkDebugReportCallbackEXT         mVkDebugCallback;
    VkDebugUtilsMessengerEXT         mVkDebugUtilsMessenger;
    PFN_vkCmdBeginDebugUtilsLabelEXT mDebugUtilsBeginLabel;
    PFN_vkCmdEndDebugUtilsLabelEXT   mDebugUtilsEndLabel;
    PFN_vkSetDebugUtilsObjectNameEXT mDebugUtilsSetObjectName;
    u32                              mVkImageIndex;
    VmaAllocator                     mVmaAllocator;

    // These are dynamic - so that workload can be handled correctly.
    Array<ResourceUpdate>            mResourceDeletionQueue;
    Array<DescriptorSetUpdate>       mdDescriptorSetUpdatesQueue;
    f32                              mGpuTimestampFrequency;
    bool                             mGpuTimestampReset = true;
    bool                             mdebugUtilsExtensionPresent = false;
    char                             mVkBinariesPath[512];
    u64								 mMinSSBOAlignment;
    u64								 mMinUBOAlignment;
    CommandBufferService             mCommandbufferManager;
    FlatHashMap<u64, VkRenderPass>   mRenderPassCache;
    TextureHandle                    mDrawingImage;
    PipelineHandle                   mDefaultComputePipeline{ InvalidIndex };
    DescriptorSetLayoutHandle        mDefaultComputeDescriptorSetLayout{ InvalidIndex };
    DescriptorSetHandle              mDefaultComputeDescriptorSet{ InvalidIndex };
};

KENSHIN_END