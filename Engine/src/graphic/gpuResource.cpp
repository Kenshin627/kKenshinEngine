#include "pch.h"
#include "gpuResource.h"

KENSHIN_BEGIN

//depth stencil////////////////////////////////////////
DepthStencilCreation& DepthStencilCreation::setDepth(bool depthWrite, VkCompareOp comparisonTest) 
{
    depthEnable = 1;
    depthWriteEnable = depthWrite;
    depthComparison = comparisonTest;
    return *this;
}

//BlendState////////////////////////////////////////
BlendState& BlendState::setColor(VkBlendFactor source, VkBlendFactor destination, VkBlendOp operation) {
    sourceColor = source;
    destinationColor = destination;
    colorOperation = operation;
    blendEnabled = 1;
    return *this;
}

BlendState& BlendState::setAlpha(VkBlendFactor source, VkBlendFactor destination, VkBlendOp operation) {
    separateBlend = 1;
    sourceAlpha = source;
    destinationAlpha = destination;
    alphaOperation = operation;
    return *this;
}

BlendState& BlendState::setColorWriteMask(ColorWriteEnabled::Mask value) 
{
    colorWriteMask = value;
    return *this;
}

//BlendStateCreation //////////////////////////////////////
BlendStateCreation& BlendStateCreation::reset() 
{
    activeStates = 0;
    return *this;
}

BlendState& BlendStateCreation::addBlendState() 
{
    return blendStates[activeStates++];
}

//BufferCreation //////////////////////////////////////////
BufferCreation& BufferCreation::reset() 
{
    size = 0;
    initialData = nullptr;
    return *this;
}

BufferCreation& BufferCreation::set(VkBufferUsageFlags flags, ResourceUsageType::Enum usage, u32 size) 
{
    typeFlags = flags;
    usage = usage;
    size = size;
    return *this;
}

BufferCreation& BufferCreation::setData(void* data) 
{
    initialData = data;
    return *this;
}

BufferCreation& BufferCreation::setName(const char* name) 
{
    name = name;
    return *this;
}

//TextureCreation /////////////////////////////////////////
TextureCreation& TextureCreation::setSize(u16 width, u16 height, u16 depth) 
{
    mWidth = width;
    mHeight = height;
    mDepth = depth;
    return *this;
}

TextureCreation& TextureCreation::setFlags(u8 mipmaps, u8 flags) 
{
    mMipmaps = mipmaps;
    mFlags = flags;
    return *this;
}

TextureCreation& TextureCreation::setFormatType(VkFormat vFormat, TextureType::Enum vType)
{
    mFormat = vFormat;
    mType   = vType;
    return *this;
}

TextureCreation& TextureCreation::setName(const char* name) 
{
    mName = name;
    return *this;
}

TextureCreation& TextureCreation::setData(void* data)
{
    mInitialData = data;
    return *this;
}

TextureCreation& TextureCreation::setUsage(VkImageUsageFlags bits)
{
    mUsage = bits;
    return *this;
}

//SamplerCreation /////////////////////////////////////////
SamplerCreation& SamplerCreation::setMinMagMip(VkFilter min, VkFilter mag, VkSamplerMipmapMode mip) 
{
    minFilter = min;
    magFilter = mag;
    mipFilter = mip;
    return *this;
}

SamplerCreation& SamplerCreation::setAddressModeU(VkSamplerAddressMode u) 
{
    addressModeU = u;
    return *this;
}

SamplerCreation& SamplerCreation::setAddressModeUV(VkSamplerAddressMode u, VkSamplerAddressMode v) 
{
    addressModeU = u;
    addressModeV = v;
    return *this;
}

SamplerCreation& SamplerCreation::setAddressModeUVW(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w) 
{
    addressModeU = u;
    addressModeV = v;
    addressModeW = w;
    return *this;
}

SamplerCreation& SamplerCreation::setName(const char* name) 
{
    name = name;
    return *this;
}

// ShaderStateCreation /////////////////////////////////////
ShaderStateCreation& ShaderStateCreation::reset() 
{
    stages_count = 0;
    return *this;
}

ShaderStateCreation& ShaderStateCreation::setName(const char* name) 
{
    name = name;
    return *this;
}

ShaderStateCreation& ShaderStateCreation::addStage(const char* code, u32 code_size, VkShaderStageFlagBits type) 
{
    stages[stages_count].code = code;
    stages[stages_count].code_size = code_size;
    stages[stages_count].type = type;
    ++stages_count;
    return *this;
}

ShaderStateCreation& ShaderStateCreation::setSpvInput(bool value) 
{
    spv_input = value;
    return *this;
}

// DescriptorSetLayoutCreation ////////////////////////////////////////////
DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::reset() 
{
    numBindings = 0;
    setIndex = 0;
    return *this;
}

DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::addBinding(const Binding& binding) 
{
    bindings[numBindings] = binding;
    ++numBindings;
    return *this;
}

DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::setName(cstring name) 
{
    name = name;
    return *this;
}

DescriptorSetLayoutCreation& DescriptorSetLayoutCreation::setSetIndex(u32 index) 
{
    setIndex = index;
    return *this;
}

// DescriptorSetCreation //////////////////////////////////////////////////
DescriptorSetCreation& DescriptorSetCreation::reset() {
    numResources = 0;
    return *this;
}

DescriptorSetCreation& DescriptorSetCreation::setLayout(DescriptorSetLayoutHandle layout) 
{
    layout = layout;
    return *this;
}

DescriptorSetCreation& DescriptorSetCreation::texture(TextureHandle texture, u16 binding) 
{
    // Set a default sampler
    samplers[numResources]  = InvalidSampler;
    bindings[numResources]  = binding;
    resources[numResources] = texture.index;
    ++numResources;
    return *this;
}

DescriptorSetCreation& DescriptorSetCreation::buffer(BufferHandle buffer, u16 binding) 
{
    samplers[numResources]  = InvalidSampler;
    bindings[numResources]  = binding;
    resources[numResources] = buffer.index;
    ++numResources;
    return *this;
}

DescriptorSetCreation& DescriptorSetCreation::textureSampler(TextureHandle texture, SamplerHandle sampler, u16 binding) 
{
    bindings[numResources]  = binding;
    resources[numResources] = texture.index;
    samplers[numResources]  = sampler;
    ++numResources;
    return *this;
}

DescriptorSetCreation& DescriptorSetCreation::setName(cstring name) 
{
    name = name;
    return *this;
}

//VertexInputCreation /////////////////////////////////////
VertexInputCreation& VertexInputCreation::reset() 
{
    numVertexStreams = numVertexAttributes = 0;
    return *this;
}

VertexInputCreation& VertexInputCreation::addVertexStream(const VertexStream& stream) 
{
    vertexStreams[numVertexStreams] = stream;
    ++numVertexStreams;
    return *this;
}

VertexInputCreation& VertexInputCreation::addVertexAttribute(const VertexAttribute& attribute) 
{
    vertexAttributes[numVertexAttributes] = attribute;
    ++numVertexAttributes;
    return *this;
}

//RenderPassOutput ////////////////////////////////////////
RenderPassOutput& RenderPassOutput::reset() 
{
    numColorFormats = 0;
    for (u32 i = 0; i < MaxImageOutputs; ++i) 
    {
        colorFormats[i] = VK_FORMAT_UNDEFINED;
    }
    depthStencilFormat = VK_FORMAT_UNDEFINED;
    colorOperation = depthOperation = stencilOperation = RenderPassOperation::DontCare;
    return *this;
}

RenderPassOutput& RenderPassOutput::color(VkFormat format) 
{
    colorFormats[numColorFormats] = format;
    ++numColorFormats;
    return *this;
}

RenderPassOutput& RenderPassOutput::depth(VkFormat format) 
{
    depthStencilFormat = format;
    return *this;
}

RenderPassOutput& RenderPassOutput::setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil) 
{
    colorOperation = color;
    depthOperation = depth;
    stencilOperation = stencil;
    return *this;
}

//PipelineCreation ////////////////////////////////////////
PipelineCreation& PipelineCreation::addDescriptorSetLayout(DescriptorSetLayoutHandle handle) 
{
    descriptorSetLayout[numActiveDescriptorSetLayouts] = handle;
    ++numActiveDescriptorSetLayouts;
    return *this;
}

RenderPassOutput& PipelineCreation::renderPassOutput() 
{
    return renderPass;
}

//RenderPassCreation //////////////////////////////////////
RenderPassCreation& RenderPassCreation::reset() 
{
    numRenderTargets = 0;
    depthStencilTexture = InvalidTexture;
    resize = 0;
    scaleX = 1.f;
    scaleY = 1.f;
    colorOperation = depthOperation = stencilOperation = RenderPassOperation::DontCare;
    return *this;
}

RenderPassCreation& RenderPassCreation::addRenderTexture(TextureHandle texture) 
{
    outputTextures[numRenderTargets] = texture;
    ++numRenderTargets;
    return *this;
}

RenderPassCreation& RenderPassCreation::setScaling(f32 scaleX, f32 scaleY, u8 resize) 
{
    this->scaleX = scaleX;
    this->scaleY = scaleY;
    this->resize = resize;
    return *this;
}

RenderPassCreation& RenderPassCreation::setDepthStencilTexture(TextureHandle texture) 
{
    depthStencilTexture = texture;
    return *this;
}

RenderPassCreation& RenderPassCreation::setName(const char* n) 
{
    name = n;
    return *this;
}

RenderPassCreation& RenderPassCreation::setType(RenderPassType::Enum t) 
{
    type = t;
    return *this;
}

RenderPassCreation& RenderPassCreation::setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil) 
{
    colorOperation = color;
    depthOperation = depth;
    stencilOperation = stencil;
    return *this;
}

// ExecutionBarrier ////////////////////////////////////////
ExecutionBarrier& ExecutionBarrier::reset() 
{
    numImageBarriers = numMemoryBarriers = 0;
    sourcePipelineStage = PipelineStage::DrawIndirect;
    destinationPipelineStage = PipelineStage::DrawIndirect;
    return *this;
}

ExecutionBarrier& ExecutionBarrier::set(PipelineStage::Enum source, PipelineStage::Enum destination) {
    sourcePipelineStage = source;
    destinationPipelineStage = destination;
    return *this;
}

ExecutionBarrier& ExecutionBarrier::addImageBarrier(const ImageBarrier& imageBarrier) 
{
    imageBarriers[numImageBarriers] = imageBarrier;
    ++numImageBarriers;
    return *this;
}

ExecutionBarrier& ExecutionBarrier::addMemoryBarrier(const MemoryBarrier& memoryBarrier) 
{
    memoryBarriers[numMemoryBarriers] = memoryBarrier;
    ++numMemoryBarriers;
    return *this;
}

KENSHIN_END