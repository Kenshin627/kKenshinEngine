#pragma once
#include "platform.h"
#include <glm/glm.hpp>
#include <typeDefs.h>

KENSHIN_BEGIN

//TODO:remove
struct Vertex
{
    glm::vec3 position;
    float u;
    glm::vec3 normal;
    float v;
};

struct SceneUniformBufferData
{   
    glm::vec4 direction;
    glm::vec4 color;
    glm::vec4 position;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;
};

struct RenderObjectPushConstant
{
    glm::mat4 modelMatrix;
    u64 bufferDeviceAddress;
};

static const u32 InvalidIndex = 0xFFFFFFFF;

typedef u32 ResourceHandle;

struct BufferHandle 
{
    ResourceHandle                  index;
}; 

struct TextureHandle 
{
    ResourceHandle                  index;
}; 

struct ShaderStateHandle 
{
    ResourceHandle                  index;
}; 

struct SamplerHandle 
{
    ResourceHandle                  index;
}; 

struct DescriptorSetLayoutHandle 
{
    ResourceHandle                  index;
}; 

struct DescriptorSetHandle 
{
    ResourceHandle                  index;
};

struct PipelineHandle 
{
    ResourceHandle                  index;
};

struct RenderPassHandle 
{
    ResourceHandle                  index;
}; 

// Invalid handles
static BufferHandle                 InvalidBuffer   { InvalidIndex };
static TextureHandle                InvalidTexture  { InvalidIndex };
static ShaderStateHandle            InvalidShader   { InvalidIndex };
static SamplerHandle                InvalidSampler  { InvalidIndex };
static DescriptorSetLayoutHandle    InvalidLayout   { InvalidIndex };
static DescriptorSetHandle          InvalidSet      { InvalidIndex };
static PipelineHandle               InvalidPipeline { InvalidIndex };
static RenderPassHandle             InvalidPass     { InvalidIndex };



// Consts /////////////////////////////////////////////////////////////////
static constexpr u8                     MaxImageOutputs = 8;                // Maximum number of images/render_targets/fbo attachments usable.
static constexpr u8                     MaxDescriptorSetLayouts = 8;       // Maximum number of layouts in the pipeline.
static constexpr u8                     MaxShaderStages = 5;                // Maximum simultaneous shader stages. Applicable to all different type of pipelines.
static constexpr u8                     MaxDescriptorsPerSet = 16;         // Maximum list elements for both descriptor set layout and descriptor sets.
static constexpr u8                     MaxVertexStreams = 16;
static constexpr u8                     MaxVertexAttributes = 16;
static constexpr u8                     MaxPushConstantRanges = 16;

static constexpr u32                    SubmitHeaderSentinel = 0xfefeb7ba;
static constexpr u32                    MaxResourceDeletions = 64;

static constexpr u32                    MaxInFlightFrames = 3;
static constexpr u32                    MaxSwapchainImages = 3;
static constexpr u32                    DynamicBufferPerFrameSize = 1024 * 1024 * 10;
static constexpr u16                    MaxGlobelPoolElements = 128;

static constexpr u8		                MaxThreadCount			  = 1;
static constexpr u8		                CommandBufferCountPerPool = 4;

// Resource creation structs //////////////////////////////////////////////

struct Rect2D 
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
}; 

struct Rect2DInt 
{
    i16 x = 0;
    i16 y = 0;
    u16 width = 0;
    u16 height = 0;
};

struct Viewport 
{
    Rect2D    rect;
    f32       minDepth = 0.0f;
    f32       maxDepth = 0.0f;
};  

struct ViewportState 
{
    u32 num_viewports = 0;
    u32 num_scissors = 0;
    Viewport* viewport = nullptr;
    Rect2DInt* scissors = nullptr;
}; 

struct StencilOperationState 
{
    VkStencilOp fail = VK_STENCIL_OP_KEEP;
    VkStencilOp pass = VK_STENCIL_OP_KEEP;
    VkStencilOp depth_fail = VK_STENCIL_OP_KEEP;
    VkCompareOp compare = VK_COMPARE_OP_ALWAYS;
    u32         compare_mask = 0xff;
    u32         write_mask = 0xff;
    u32         reference = 0xff;

};

struct DepthStencilCreation 
{
    StencilOperationState           front;
    StencilOperationState           back;
    VkCompareOp                     depthComparison = VK_COMPARE_OP_LESS;
    u8                              depthEnable : 1;
    u8                              depthWriteEnable : 1;
    u8                              stencilEnable : 1;
    u8                              pad : 5;
    DepthStencilCreation() : depthEnable(0), depthWriteEnable(0), stencilEnable(0) { }
    DepthStencilCreation& setDepth(bool depthWrite, VkCompareOp comparisonTest);

}; 

struct BlendState 
{
    VkBlendFactor                   sourceColor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor                   destinationColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp                       colorOperation = VK_BLEND_OP_ADD;
    VkBlendFactor                   sourceAlpha = VK_BLEND_FACTOR_ONE;
    VkBlendFactor                   destinationAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp                       alphaOperation = VK_BLEND_OP_ADD;
    ColorWriteEnabled::Mask         colorWriteMask = ColorWriteEnabled::All_mask;
    u8                              blendEnabled : 1;
    u8                              separateBlend : 1;
    u8                              pad : 6;

    BlendState() : blendEnabled(0), separateBlend(0) 
    {
    }

    BlendState& setColor(VkBlendFactor source_color, VkBlendFactor destination_color, VkBlendOp color_operation);
    BlendState& setAlpha(VkBlendFactor source_color, VkBlendFactor destination_color, VkBlendOp color_operation);
    BlendState& setColorWriteMask(ColorWriteEnabled::Mask value);

};

struct BlendStateCreation 
{
    BlendState                      blendStates[MaxImageOutputs];
    u32                             activeStates = 0;
    BlendStateCreation& reset();
    BlendState& addBlendState();
}; 


struct RasterizationCreation 
{
    VkCullModeFlagBits              cull_mode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace                     front = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    FillMode::Enum                  fill = FillMode::Solid;
}; 

struct BufferCreation 
{
    VkBufferUsageFlags              mTypeFlags = 0;
    ResourceUsageType::Enum         mUsage = ResourceUsageType::Immutable;
    u32                             mSize = 0;
    void*                           mInitialData{ nullptr };
    const char*                     mName       { nullptr };
    bool                            mPersistent{ false };
    BufferCreation& reset();
    BufferCreation& set(VkBufferUsageFlags flags, ResourceUsageType::Enum usage, u32 size);
    BufferCreation& setData(void* data);
    BufferCreation& setName(const char* name);
	BufferCreation& setPersistent(bool persistent);
}; 

struct TextureCreation 
{
    void*                           mInitialData{ nullptr };
    u16                             mWidth   = 1;
    u16                             mHeight  = 1;
    u16                             mDepth   = 1;
    u8                              mMipmaps = 1;
    u8                              mFlags   = 0;    // TextureFlags bitmasks
    VkFormat                        mFormat  = VK_FORMAT_UNDEFINED;
    TextureType::Enum               mType    = TextureType::Texture2D;
    const char*                     mName{ nullptr };
    VkImageUsageFlags               mUsage{ 0 };
    TextureCreation& setSize(u16 width, u16 height, u16 depth);
    TextureCreation& setFlags(u8 mipmaps, u8 flags);
    TextureCreation& setFormatType(VkFormat format, TextureType::Enum type);
    TextureCreation& setName(const char* name);
    TextureCreation& setData(void* data);
    TextureCreation& setUsage(VkImageUsageFlags bits);
}; 


struct SamplerCreation
{
    VkFilter                        minFilter = VK_FILTER_NEAREST;
    VkFilter                        magFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode             mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode            addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    const char* name = nullptr;
    SamplerCreation& setMinMagMip(VkFilter min, VkFilter mag, VkSamplerMipmapMode mip);
    SamplerCreation& setAddressModeU(VkSamplerAddressMode u);
    SamplerCreation& setAddressModeUV(VkSamplerAddressMode u, VkSamplerAddressMode v);
    SamplerCreation& setAddressModeUVW(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
    SamplerCreation& setName(const char* name);
};

struct ShaderStage 
{
    const char* code = nullptr;
    u32                             code_size = 0;
    VkShaderStageFlagBits           type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
};

struct ShaderStateCreation 
{
    ShaderStage                     stages[MaxShaderStages];
    const char* name = nullptr;
    u32                             stages_count = 0;
    u32                             spv_input = 0;
    // Building helpers
    ShaderStateCreation& reset();
    ShaderStateCreation& setName(const char* name);
    ShaderStateCreation& addStage(const char* code, u32 code_size, VkShaderStageFlagBits type);
    ShaderStateCreation& setSpvInput(bool value);

};

struct DescriptorSetLayoutCreation 
{
    struct Binding 
    {
        VkDescriptorType            type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        u16                         bindingPoint = 0;
        u16                         count = 0;
        cstring                     name = nullptr;  // Comes from external memory.
    };

    Binding                         bindings[MaxDescriptorsPerSet];
    u32                             numBindings = 0;
    u32                             setIndex = 0;
    cstring                         name = nullptr;
    DescriptorSetLayoutCreation& reset();
    DescriptorSetLayoutCreation& addBinding(const Binding& binding);
    DescriptorSetLayoutCreation& setName(cstring name);
    DescriptorSetLayoutCreation& setSetIndex(u32 index);
}; 

struct DescriptorSetCreation 
{    
    DescriptorSetLayoutHandle       mLayout;   
    cstring                         mName { nullptr };
    DescriptorSetCreation& reset();
    DescriptorSetCreation& setLayout(DescriptorSetLayoutHandle layout);    
    DescriptorSetCreation& setName(cstring name);
}; 

struct UpdateDescriptorSetCreation
{
    ResourceHandle                  mResources[MaxDescriptorsPerSet];
    SamplerHandle                   mSamplers[MaxDescriptorsPerSet];
    u16                             mBindings[MaxDescriptorsPerSet];
    u32                             mNumResources = 0;
    UpdateDescriptorSetCreation& reset();
    UpdateDescriptorSetCreation& texture(TextureHandle texture, u16 binding);
    UpdateDescriptorSetCreation& buffer(BufferHandle buffer, u16 binding);
    UpdateDescriptorSetCreation& textureSampler(TextureHandle texture, SamplerHandle sampler, u16 binding);   // TODO: separate samplers from textures
};

struct DescriptorSetUpdate 
{
    DescriptorSetHandle             descriptorSetHandle;
    u32                             frameIssued = 0;
}; 

struct VertexAttribute 
{
    u16                             location = 0;
    u16                             binding = 0;
    u32                             offset = 0;
    VertexComponentFormat::Enum     format = VertexComponentFormat::Count;

}; 

struct VertexStream 
{
    u16                             binding = 0;
    u16                             stride = 0;
    VertexInputRate::Enum           inputRate = VertexInputRate::Count;
}; 


struct VertexInputCreation 
{
    u32                             numVertexStreams = 0;
    u32                             numVertexAttributes = 0;
    VertexStream                    vertexStreams[MaxVertexStreams];
    VertexAttribute                 vertexAttributes[MaxVertexAttributes];
    VertexInputCreation& reset();
    VertexInputCreation& addVertexStream(const VertexStream& stream);
    VertexInputCreation& addVertexAttribute(const VertexAttribute& attribute);
};

struct RenderPassOutput 
{
    VkFormat                        colorFormats[MaxImageOutputs];
    VkFormat                        depthStencilFormat{ VK_FORMAT_UNDEFINED };
    u32                             numColorFormats{ 0 };
    RenderPassOperation::Enum       colorOperation = RenderPassOperation::DontCare;
    RenderPassOperation::Enum       depthOperation = RenderPassOperation::DontCare;
    RenderPassOperation::Enum       stencilOperation = RenderPassOperation::DontCare;
    RenderPassOutput& reset();
    RenderPassOutput& color(VkFormat format);
    RenderPassOutput& depth(VkFormat format);
    RenderPassOutput& setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);
};

struct RenderPassCreation 
{
    u16                             numRenderTargets = 0;
    RenderPassType::Enum            type = RenderPassType::Geometry;
    TextureHandle                   outputTextures[MaxImageOutputs];
    TextureHandle                   depthStencilTexture;
    f32                             scaleX = 1.f;
    f32                             scaleY = 1.f;
    u8                              resize = 1;
    RenderPassOperation::Enum       colorOperation = RenderPassOperation::DontCare;
    RenderPassOperation::Enum       depthOperation = RenderPassOperation::DontCare;
    RenderPassOperation::Enum       stencilOperation = RenderPassOperation::DontCare;
    const char*                     name = nullptr;
    RenderPassCreation& reset();
    RenderPassCreation& addRenderTexture(TextureHandle texture);
    RenderPassCreation& setScaling(f32 scaleX, f32 scaleY, u8 resize);
    RenderPassCreation& setDepthStencilTexture(TextureHandle texture);
    RenderPassCreation& setName(const char* name);
    RenderPassCreation& setType(RenderPassType::Enum type);
    RenderPassCreation& setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);
}; 

struct RenderingAttachmentCreation
{
    VkImageView         iamgeView;
	VkImageLayout       imageLayout;
    VkAttachmentLoadOp  loadOp;
	VkAttachmentStoreOp storeOp;
    VkClearValue        clearValue;
};

struct PipelineCreation 
{
    RasterizationCreation           rasterization;
    DepthStencilCreation            depthStencil;
    BlendStateCreation              blendState;
    VertexInputCreation             vertexInput;
    ShaderStateCreation             shaders;
    RenderPassOutput                renderPass;
    DescriptorSetLayoutHandle       descriptorSetLayout[MaxDescriptorSetLayouts];
    const ViewportState*            viewport = nullptr;
    u32                             numActiveDescriptorSetLayouts = 0;
    const char*                     name = nullptr;
    u32                             numColorAttachments = 0;
	VkFormat						colorAttachmentFormats[MaxImageOutputs];
    VkFormat						depthFormat = VK_FORMAT_UNDEFINED;
    VkFormat                        stencilFormat = VK_FORMAT_UNDEFINED;
    u32                             numPushConstantRanges = 0;
    VkPushConstantRange             pushConstantRanges[MaxPushConstantRanges];
    PipelineCreation& addDescriptorSetLayout(DescriptorSetLayoutHandle handle);
    PipelineCreation& setName(const cstring n);
	PipelineCreation& addPushConstantRange(VkShaderStageFlags stageFlags, u32 offset, u32 size);
	PipelineCreation& addColorAttachmentFormat(VkFormat format);
	PipelineCreation& setDepthFormat(VkFormat format);
	PipelineCreation& setStencilFormat(VkFormat format);
    RenderPassOutput& renderPassOutput();

}; 

namespace TextureFormat 
{
    inline bool isDepthStencil(VkFormat value) 
    {
        return value == VK_FORMAT_D16_UNORM_S8_UINT || 
               value == VK_FORMAT_D24_UNORM_S8_UINT || 
               value == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    inline bool isDepthOnly(VkFormat value) 
    {
        return value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_D32_SFLOAT;
    }

    inline bool isStencilOnly(VkFormat value) 
    {
        return value == VK_FORMAT_S8_UINT;
    }

    inline bool hasDepth(VkFormat value) 
    {
        return (value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_S8_UINT) || 
               (value >= VK_FORMAT_D16_UNORM_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT);
    }

    inline bool hasStencil(VkFormat value) 
    {
        return value >= VK_FORMAT_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    inline bool hasDepthOrStencil(VkFormat value) 
    {
        return value >= VK_FORMAT_D16_UNORM && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
}

struct ResourceData 
{
    void* data = nullptr;
};

struct ResourceBinding 
{
    u16                             type = 0;    // ResourceType
    u16                             start = 0;
    u16                             count = 0;
    u16                             set = 0;
    const char* name = nullptr;
};


// API-agnostic descriptions //////////////////////////////////////////////
struct ShaderStateDescription 
{
    void* native_handle = nullptr;
    cstring                         name = nullptr;
}; 

struct BufferDescription 
{
    void*                           nativeHandle = nullptr;
    cstring                         name = nullptr;
    VkBufferUsageFlags              typeFlags = 0;
    ResourceUsageType::Enum         usage = ResourceUsageType::Immutable;
    u32                             size = 0;
    BufferHandle                    parentHandle;
};

//
//
struct TextureDescription 
{
    void*                           nativeHandle = nullptr;
    cstring                         name = nullptr;
    u16                             width = 1;
    u16                             height = 1;
    u16                             depth = 1;
    u8                              mipmaps = 1;
    u8                              renderTarget = 0;
    u8                              computeAccess = 0;
    VkFormat                        format = VK_FORMAT_UNDEFINED;
    TextureType::Enum               type = TextureType::Texture2D;
}; 

struct SamplerDescription 
{
    cstring                         name         = nullptr;
    VkFilter                        minFilter    = VK_FILTER_NEAREST;
    VkFilter                        magFilter    = VK_FILTER_NEAREST;
    VkSamplerMipmapMode             mipFilter    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode            addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

struct DescriptorSetLayoutDescription 
{
    ResourceBinding                 bindings[MaxDescriptorsPerSet];
    u32                             numActiveBindings = 0;
}; 

struct DesciptorSetDescription 
{
    ResourceData                    resources[MaxDescriptorsPerSet];
    u32                             num_active_resources = 0;
}; 

struct PipelineDescription 
{
    ShaderStateHandle               shader;
};

struct MapBufferParameters 
{
    BufferHandle                    buffer;
    u32                             offset = 0;
    u32                             size = 0;
}; 

// Synchronization ////////////////////////////////////////////////////////
struct ImageBarrier 
{
    TextureHandle                   texture;
}; 

struct MemoryBarrier {
    BufferHandle                    buffer;
}; 

struct ExecutionBarrier 
{
    PipelineStage::Enum             sourcePipelineStage;
    PipelineStage::Enum             destinationPipelineStage;
    u32                             newBarrierExperimental = u32_max;
    u32                             loadOperation = 0;
    u32                             numImageBarriers;
    u32                             numMemoryBarriers;
    ImageBarrier                    imageBarriers[8];
    MemoryBarrier                   memoryBarriers[8];

    ExecutionBarrier& reset();
    ExecutionBarrier& set(PipelineStage::Enum source, PipelineStage::Enum destination);
    ExecutionBarrier& addImageBarrier(const ImageBarrier& imageBarrier);
    ExecutionBarrier& addMemoryBarrier(const MemoryBarrier& memoryBarrier);
}; 

struct ResourceUpdate
{
    ResourceDeletionType::Enum      type;
    ResourceHandle                  handle;
    u32                             currentFrame;
};

struct Buffer 
{
    VkBuffer                        vkBuffer;
    VmaAllocation                   vmaAllocation;
    VkDeviceMemory                  vkDeviceMemory;
    VkDeviceSize                    vkDeviceSize;
    VkBufferUsageFlags              typeFlags = 0;
    ResourceUsageType::Enum         usage = ResourceUsageType::Immutable;
    u32                             size = 0;
    u32                             globelBufferOffset = 0;    // Offset into global constant, if dynamic
    BufferHandle                    handle;
    BufferHandle                    parentBufferHandle;
    const char*                     name = nullptr;
	VkDeviceAddress                 deviceAddress = 0;
	void*                           mappedData = nullptr;
}; 

struct Sampler 
{
    VkSampler                       vkSampler;
    VkFilter                        minFilter = VK_FILTER_NEAREST;
    VkFilter                        magFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode             mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode            addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode            addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    const char* name = nullptr;
}; 

struct Texture 
{
    VkImage                         vkImage;
    VkImageView                     vkImageView;
    VkImageUsageFlags               vkUsage;
    VkFormat                        vkFormat;
    VkImageLayout                   vkImageLayout;
    VmaAllocation                   vmaAllocation;
    u16                             width = 1;
    u16                             height = 1;
    u16                             depth = 1;
    u8                              mipmaps = 1;
    u8                              flags = 0;
    TextureHandle                   handle;
    TextureType::Enum               type = TextureType::Texture2D;
    Sampler* sampler = nullptr;
    const char* name = nullptr;
}; 

struct ShaderState 
{
    VkPipelineShaderStageCreateInfo shaderStageInfo[MaxShaderStages];
    const char*                     name = nullptr;
    u32                             activeShaders = 0;
    bool                            graphicsPipeline = false;
};

struct DescriptorBinding 
{
    VkDescriptorType                type;
    u16                             bindingPoint = 0;
    u16                             count = 0;
    u16                             set = 0;
    const char* name = nullptr;
};

struct DesciptorSetLayout 
{
    VkDescriptorSetLayout           vkDescriptorSetLayout;
    VkDescriptorSetLayoutBinding*   vkBinding = nullptr;
    DescriptorBinding*              bindings = nullptr;
    u16                             numBindings = 0;
    u16                             setIndex = 0;
    DescriptorSetLayoutHandle       handle;
}; 

struct DesciptorSet 
{
    VkDescriptorSet                 vkDescriptorSet;
    ResourceHandle*                 resources    { nullptr };
    SamplerHandle*                  samplers     { nullptr };
    u16*                            bindings     { nullptr };
    const DesciptorSetLayout*       layout       { nullptr };
    u32                             numResources { 0       };
}; 

struct Pipeline 
{
    VkPipeline                      vkPipeline;
    VkPipelineLayout                vkPipelineLayout;
    VkPipelineBindPoint             vkBindPoint;
    ShaderStateHandle               shaderState;
    const DesciptorSetLayout*       descriptorSetLayout[MaxDescriptorSetLayouts];
    DescriptorSetLayoutHandle       descriptorSetLayoutHandle[MaxDescriptorSetLayouts];
    u32                             numActiveDescriptorSetLayouts = 0;
    DepthStencilCreation            depthStencil;
    BlendStateCreation              blendState;
    RasterizationCreation           rasterization;
    PipelineHandle                  handle;
    bool                            graphicsPipeline = true;
}; 

struct RenderPass 
{
    VkRenderPass                    vkRenderPass;
    VkFramebuffer                   vkFrameBuffer;
    RenderPassOutput                output;
    TextureHandle                   outputTextures[MaxImageOutputs];
    TextureHandle                   outputDepth;
    RenderPassType::Enum            type;
    f32                             scaleX = 1.f;
    f32                             scaleY = 1.f;
    u16                             width = 0;
    u16                             height = 0;
    u16                             dispatchX = 0;
    u16                             dispatchY = 0;
    u16                             dispatchZ = 0;
    u8                              resize = 0;
    u8                              numRenderTargets = 0;
    const char* name = nullptr;
}; 


static cstring toCompilerExtension(VkShaderStageFlagBits value) 
{
    switch (value) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return "vert";
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return "frag";
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return "comp";
    default:
        return "";
    }
}

static cstring toStageDefines(VkShaderStageFlagBits value) 
{
    switch (value) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return "VERTEX";
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return "FRAGMENT";
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return "COMPUTE";
    default:
        return "";
    }
}

static VkImageType toVkImageType(TextureType::Enum type) 
{
    static VkImageType s_vk_target[TextureType::Count] = { VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D };
    return s_vk_target[type];
}


static VkImageViewType toVkImageViewType(TextureType::Enum type) 
{
    static VkImageViewType s_vk_data[] = { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY };
    return s_vk_data[type];
}

//
//
static VkFormat toVkVertexFormat(VertexComponentFormat::Enum value) 
{
    // Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2, Uint4, Count
    static VkFormat s_vk_vertex_formats[VertexComponentFormat::Count] = 
    { 
        VK_FORMAT_R32_SFLOAT, 
        VK_FORMAT_R32G32_SFLOAT, 
        VK_FORMAT_R32G32B32_SFLOAT, 
        VK_FORMAT_R32G32B32A32_SFLOAT, 
        /*MAT4 TODO*/
        VK_FORMAT_R32G32B32A32_SFLOAT,                                                                          
        VK_FORMAT_R8_SINT, 
        VK_FORMAT_R8G8B8A8_SNORM, 
        VK_FORMAT_R8_UINT, 
        VK_FORMAT_R8G8B8A8_UINT, 
        VK_FORMAT_R16G16_SINT, 
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16B16A16_SINT, 
        VK_FORMAT_R16G16B16A16_SNORM, 
        VK_FORMAT_R32_UINT, 
        VK_FORMAT_R32G32_UINT, 
        VK_FORMAT_R32G32B32A32_UINT 
    };

    return s_vk_vertex_formats[value];
}

static VkPipelineStageFlags toVkPipelineStage(PipelineStage::Enum value) 
{
    static VkPipelineStageFlags s_vk_values[] = 
    { 
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
        VK_PIPELINE_STAGE_TRANSFER_BIT 
    };
    return s_vk_values[value];
}

//
//
static VkAccessFlags utilToVkAccessFlags(ResourceState state) 
{
    VkAccessFlags ret = 0;
    if (state & RESOURCE_STATE_COPY_SOURCE) {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (state & RESOURCE_STATE_COPY_DEST) {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {
        ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (state & RESOURCE_STATE_INDEX_BUFFER) {
        ret |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (state & RESOURCE_STATE_UNORDERED_ACCESS) {
        ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_INDIRECT_ARGUMENT) {
        ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if (state & RESOURCE_STATE_RENDER_TARGET) {
        ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (state & RESOURCE_STATE_DEPTH_WRITE) {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (state & RESOURCE_STATE_SHADER_RESOURCE) {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (state & RESOURCE_STATE_PRESENT) {
        ret |= VK_ACCESS_MEMORY_READ_BIT;
    }
#ifdef ENABLE_RAYTRACING
    if (state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    }
#endif

    return ret;
}

static VkImageLayout utilToVkImageLayout(ResourceState usage) 
{
    if (usage & RESOURCE_STATE_COPY_SOURCE)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (usage & RESOURCE_STATE_COPY_DEST)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if (usage & RESOURCE_STATE_RENDER_TARGET)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (usage & RESOURCE_STATE_DEPTH_WRITE)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (usage & RESOURCE_STATE_DEPTH_READ)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    if (usage & RESOURCE_STATE_UNORDERED_ACCESS)
        return VK_IMAGE_LAYOUT_GENERAL;

    if (usage & RESOURCE_STATE_SHADER_RESOURCE)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (usage & RESOURCE_STATE_PRESENT)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (usage == RESOURCE_STATE_COMMON)
        return VK_IMAGE_LAYOUT_GENERAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

// Determines pipeline stages involved for given accesses
static VkPipelineStageFlags utilDeterminePipelineStageFlags(VkAccessFlags accessFlags, QueueType::Enum queueType) 
{
    VkPipelineStageFlags flags = 0;

    switch (queueType) {
    case QueueType::Graphics:
    {
        if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

        if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0) {
            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            /*if ( pRenderer->pActiveGpuSettings->mGeometryShaderSupported ) {
                flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
            }
            if ( pRenderer->pActiveGpuSettings->mTessellationSupported ) {
                flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
            }*/
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
#ifdef ENABLE_RAYTRACING
            if (pRenderer->mVulkan.mRaytracingExtension) {
                flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
            }
#endif
        }

        if ((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        break;
    }
    case QueueType::Compute:
    {
        if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 ||
            (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
            (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
            (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        break;
    }
    case QueueType::CopyTransfer: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    default: break;
    }

    // Compatible with both compute and graphics queues
    if ((accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) != 0)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    if ((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if ((accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if (flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
}
KENSHIN_END