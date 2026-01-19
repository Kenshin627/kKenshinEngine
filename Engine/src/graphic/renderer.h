#pragma once

#include "gpuDevice.h"
#include "gpuResource.h"
#include "resourceManager.h"

KENSHIN_BEGIN

struct Renderer;
struct CommandBuffer;

struct BufferResource : public Kenshin::Resource 
{
    BufferHandle                    handle;
    u32                             poolIndex;
    BufferDescription               desc;
    static constexpr cstring        type = "KENSHIN_BUFFER_TYPE";
    static u64                      type_hash;
}; 


struct TextureResource : public Kenshin::Resource 
{
    TextureHandle                   handle;
    u32                             poolIndex;
    TextureDescription              desc;
    static constexpr cstring        type = "KENSHIN_TEXTURE_TYPE";
    static u64                      type_hash;
}; 

struct SamplerResource : public Kenshin::Resource 
{
    SamplerHandle                   handle;
    u32                             poolIndex;
    SamplerDescription              desc;
    static constexpr cstring        type = "KENSHIN_SAMPLER_TYPE";
    static u64                      type_hash;
};

struct ResourceCache 
{
    void init(Allocator* allocator);
    void shutdown(Renderer* renderer);
    FlatHashMap<u64, TextureResource*> textures;
    FlatHashMap<u64, BufferResource*>  buffers;
    FlatHashMap<u64, SamplerResource*> samplers;

};

struct RendererCreation 
{
    Kenshin::GPUDevice* gpu;
    Allocator*          allocator;

}; 

//Renderer////////////////////////////////////////////////////////////////////
struct Renderer : public Service 
{
    KS_SERVICE_TYPE(Renderer);
    void init(const RendererCreation& creation);
    void shutdown();         
    void setLoaders(Kenshin::ResourceManager* manager);         
    void beginFrame();
    void endFrame();         
    void resizeSwapchain(u32 width, u32 height);
    f32 aspectRatio() const;
    BufferResource* createBuffer(const BufferCreation& creation);
    BufferResource* createBuffer(VkBufferUsageFlags type, ResourceUsageType::Enum usage, u32 size, void* data, cstring name);
    TextureResource* createTexture(const TextureCreation& creation);
    TextureResource* createTexture(cstring name, cstring filename);
    SamplerResource* createSampler(const SamplerCreation& creation);
    void destroyBuffer(BufferResource* buffer);
    void destroyTexture(TextureResource* texture);
    void destroySampler(SamplerResource* sampler);
    void* mapBuffer(BufferResource* buffer, u32 offset = 0, u32 size = 0);
    void  unmapBuffer(BufferResource* buffer);

    CommandBuffer* getCommandBuffer(QueueType::Enum type, bool begin) { return gpu->getCommandBuffer(type, begin); }
    void queueCommandBuffer(Kenshin::CommandBuffer* commands) { gpu->queueCommandBuffer(commands); }

    ResourcePoolTyped<TextureResource>  textures;
    ResourcePoolTyped<BufferResource>   buffers;
    ResourcePoolTyped<SamplerResource>  samplers;
    ResourceCache                       resourceCache;
    Kenshin::GPUDevice*                 gpu;
    u16                                 width;
    u16                                 height;
    static constexpr cstring            typeName = "Renderer Service";

};

KENSHIN_END
