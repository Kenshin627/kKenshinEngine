#include "pch.h"
#include "renderer.h"
#include "platform.h"
#include "commandBuffer.h"
#include "gpuDevice.h"
#include "gpuResource.h"
#include <stb_image.h>

KENSHIN_BEGIN


struct TextureLoader : public ResourceLoader 
{
    Resource* get(cstring name) override;
    Resource* get(u64 hashed_name) override;
    Resource* unload(cstring name) override;
    Resource* createFromFile(cstring name, cstring filename, ResourceManager* resource_manager) override;
    Renderer* renderer{ nullptr };
}; 

struct BufferLoader : public ResourceLoader 
{
    Resource* get(cstring name) override;
    Resource* get(u64 hashed_name) override;
    Resource* unload(cstring name) override;
    Renderer* renderer{ nullptr };
}; 

struct SamplerLoader : public ResourceLoader 
{
    Resource* get(cstring name) override;
    Resource* get(u64 hashed_name) override;
    Resource* unload(cstring name) override;
    Renderer* renderer{ nullptr };
}; 

static TextureHandle createTextureFromFile(GPUDevice& gpu, cstring filename, cstring name) 
{
    if (filename) 
    {
        int comp, width, height;
        uint8_t* imageData = stbi_load(filename, &width, &height, &comp, 4);
        if (!imageData)
        {
            KS_CORE_ERROR("Error loading texture %s", filename);
            return InvalidTexture;
        }

        TextureCreation creation;
        creation
            .setData(imageData)
            .setFormatType(VK_FORMAT_R8G8B8A8_UNORM, TextureType::Texture2D)
            .setFlags(1, 0)
            .setSize((u16)width, (u16)height, 1)
            .setName(name);

        TextureHandle newTexture = gpu.createTexture(creation);
        //TODO?
        free(imageData);
        return newTexture;
    }
    return InvalidTexture;
}


// Renderer /////////////////////////////////////////////////////////////////////
u64 TextureResource::typeHash = 0;
u64 BufferResource::typeHash  = 0;
u64 SamplerResource::typeHash = 0;

static TextureLoader sTextureLoader;
static BufferLoader sBufferLoader;
static SamplerLoader sSamplerLoader;



Renderer* Renderer::instance() 
{
    static Renderer sRenderer;
    return &sRenderer;
}

bool Renderer::init(void* configuration)
{
	RendererCreation& creation = *(RendererCreation*)configuration; 
    KS_CORE_INFO("Renderer init\n");
    mGPU    = creation.gpu;
    mWidth  = mGPU->mSwapchainWidth;
    mHeight = mGPU->mSwapchainHeight;

    mTextures.init(creation.allocator, 512);
    mBuffers.init(creation.allocator, 4096);
    mSamplers.init(creation.allocator, 128);
    mResourceCache.init(creation.allocator);

    // Init resource hashes
    TextureResource::typeHash = hash_calculate(TextureResource::type);
    BufferResource::typeHash  = hash_calculate(BufferResource::type);
    SamplerResource::typeHash = hash_calculate(SamplerResource::type);

    sTextureLoader.renderer = this;
    sBufferLoader.renderer  = this;
    sSamplerLoader.renderer = this;
    return true;
}

void Renderer::shutdown() 
{
    mResourceCache.shutdown(this);
    mTextures.shutdown();
    mBuffers.shutdown();
    mSamplers.shutdown();
    KS_CORE_INFO("Renderer shutdown\n");
    mGPU->shutdown();
}

void Renderer::setLoaders(ResourceManager* manager) 
{
    manager->setLoader(TextureResource::type, &sTextureLoader);
    manager->setLoader(BufferResource::type, &sBufferLoader);
    manager->setLoader(SamplerResource::type, &sSamplerLoader);
}

void Renderer::beginFrame() 
{
    mGPU->newFrame();
}

void Renderer::endFrame() 
{
    mGPU->present();
}

void Renderer::resizeSwapchain() 
{
    mGPU->resize();
}

f32 Renderer::aspectRatio() const 
{
    return mGPU->mSwapchainWidth * 1.f / mGPU->mSwapchainHeight;
}

BufferResource* Renderer::createBuffer(const BufferCreation& creation) 
{
    BufferResource* buffer = mBuffers.obtain();
    if (buffer) 
    {
        BufferHandle handle = mGPU->createBuffer(creation);
        buffer->handle = handle;
        buffer->name = creation.name;
        mGPU->queryBuffer(handle, buffer->desc);
        if (creation.name != nullptr) 
        {
            mResourceCache.buffers.insert(hash_calculate(creation.name), buffer);
        }
        buffer->references = 1;
        return buffer;
    }
    return nullptr;
}

BufferResource* Renderer::createBuffer(VkBufferUsageFlags type, ResourceUsageType::Enum usage, u32 size, void* data, cstring name) 
{
    BufferCreation creation{ type, usage, size, data, name };
    return createBuffer(creation);
}

TextureResource* Renderer::createTexture(const TextureCreation& creation)
{
    TextureResource* texture = mTextures.obtain();
    if (texture) {
        TextureHandle handle = mGPU->createTexture(creation);
        texture->handle = handle;
        texture->name = creation.mName;
        mGPU->queryTexture(handle, texture->desc);

        if (creation.mName != nullptr)
        {
            mResourceCache.textures.insert(hash_calculate(creation.mName), texture);
        }
        texture->references = 1;
        return texture;
    }
    return nullptr;
}

TextureResource* Renderer::createTexture(cstring name, cstring filename) 
{
    TextureResource* texture = mTextures.obtain();
    if (texture) 
    {
        TextureHandle handle = createTextureFromFile(*mGPU, filename, name);
        texture->handle = handle;
        mGPU->queryTexture(handle, texture->desc);
        texture->references = 1;
        texture->name = name;
        mResourceCache.textures.insert(hash_calculate(name), texture);
        return texture;
    }
    return nullptr;
}

SamplerResource* Renderer::createSampler(const SamplerCreation& creation) 
{
    SamplerResource* sampler = mSamplers.obtain();
    if (sampler) 
    {
        SamplerHandle handle = mGPU->createSampler(creation);
        sampler->handle = handle;
        sampler->name = creation.name;
        mGPU->querySampler(handle, sampler->desc);
        if (creation.name != nullptr) 
        {
            mResourceCache.samplers.insert(hash_calculate(creation.name), sampler);
        }
        sampler->references = 1;
        return sampler;
    }
    return nullptr;
}

void Renderer::destroyBuffer(BufferResource* buffer) 
{
    if (!buffer) 
    {
        return;
    }

    buffer->remove_reference();
    if (buffer->references) 
    {
        return;
    }

    mResourceCache.buffers.remove(hash_calculate(buffer->desc.name));
    mGPU->destroyBuffer(buffer->handle);
    mBuffers.release(buffer);
}

void Renderer::destroyTexture(TextureResource* texture) 
{
    if (!texture) 
    {
        return;
    }
    texture->remove_reference();
    if (texture->references) 
    {
        return;
    }
    mResourceCache.textures.remove(hash_calculate(texture->desc.name));
    mGPU->destroyTexture(texture->handle);
    mTextures.release(texture);
}

void Renderer::destroySampler(SamplerResource* sampler) 
{
    if (!sampler) 
    {
        return;
    }
    sampler->remove_reference();
    if (sampler->references) 
    {
        return;
    }
    mResourceCache.samplers.remove(hash_calculate(sampler->desc.name));
    mGPU->destroySampler(sampler->handle);
    mSamplers.release(sampler);
}

void* Renderer::mapBuffer(BufferResource* buffer, u32 offset, u32 size) 
{

    MapBufferParameters mapParams = { buffer->handle, offset, size };
    return mGPU->mapBuffer(mapParams);
}

void Renderer::unmapBuffer(BufferResource* buffer) 
{
    if (buffer->desc.parentHandle.index == InvalidIndex) 
    {
        MapBufferParameters mapParams = { buffer->handle, 0, 0 };
        mGPU->unmapBuffer(mapParams);
    }
}

CommandBuffer* Renderer::getCommandBuffer(QueueType::Enum type, bool begin)
{
    return mGPU->getCommandBuffer(type, begin);
}

void Renderer::queueCommandBuffer(Kenshin::CommandBuffer* commands)
{
    mGPU->queueCommandBuffer(commands);
}

// Resource Loaders ///////////////////////////////////////////////////////

// Texture Loader /////////////////////////////////////////////////////////
Resource* TextureLoader::get(cstring name) 
{
    const u64 hashedName = hash_calculate(name);
    return renderer->mResourceCache.textures.get(hashedName);
}

Resource* TextureLoader::get(u64 hashedName) 
{
    return renderer->mResourceCache.textures.get(hashedName);
}

Resource* TextureLoader::unload(cstring name) 
{
    const u64 hashed_name = hash_calculate(name);
    TextureResource* texture = renderer->mResourceCache.textures.get(hashed_name);
    if (texture) 
    {
        renderer->destroyTexture(texture);
    }
    return nullptr;
}

Resource* TextureLoader::createFromFile(cstring name, cstring filename, ResourceManager* resource_manager) 
{
    return renderer->createTexture(name, filename);
}

// BufferLoader //////////////////////////////////////////////////////////
Resource* BufferLoader::get(cstring name) 
{
    const u64 hashed_name = hash_calculate(name);
    return renderer->mResourceCache.buffers.get(hashed_name);
}

Resource* BufferLoader::get(u64 hashed_name) 
{
    return renderer->mResourceCache.buffers.get(hashed_name);
}

Resource* BufferLoader::unload(cstring name) {
    const u64 hashed_name = hash_calculate(name);
    BufferResource* buffer = renderer->mResourceCache.buffers.get(hashed_name);
    if (buffer) 
    {
        renderer->destroyBuffer(buffer);
    }

    return nullptr;
}

// SamplerLoader /////////////////////////////////////////////////////////
Resource* SamplerLoader::get(cstring name) 
{
    const u64 hashedName = hash_calculate(name);
    return renderer->mResourceCache.samplers.get(hashedName);
}

Resource* SamplerLoader::get(u64 hashedName) 
{
    return renderer->mResourceCache.samplers.get(hashedName);
}

Resource* SamplerLoader::unload(cstring name) 
{
    const u64 hashedName = hash_calculate(name);
    SamplerResource* sampler = renderer->mResourceCache.samplers.get(hashedName);
    if (sampler) 
    {
        renderer->destroySampler(sampler);
    }
    return nullptr;
}

// ResourceCache
void ResourceCache::init(Allocator* allocator) 
{
    // Init resources caching
    textures.init(allocator, 16);
    buffers.init(allocator, 16);
    samplers.init(allocator, 16);
}

void ResourceCache::shutdown(Renderer* renderer) 
{
    FlatHashMapIterator it = textures.iterator_begin();
    while (it.is_valid()) 
    {
        TextureResource* texture = textures.get(it);
        renderer->destroyTexture(texture);
        textures.iterator_advance(it);
    }
    it = buffers.iterator_begin();
    while (it.is_valid()) 
    {
        BufferResource* buffer = buffers.get(it);
        renderer->destroyBuffer(buffer);
        buffers.iterator_advance(it);
    }

    it = samplers.iterator_begin();

    while (it.is_valid()) 
    {
        SamplerResource* sampler = samplers.get(it);
        renderer->destroySampler(sampler);
        samplers.iterator_advance(it);
    }

    textures.shutdown();
    buffers.shutdown();
    samplers.shutdown();
}

KENSHIN_END