#pragma once
#include "platform.h"

namespace Kenshin
{
	static const u32 InvalidIndex = 0xFFFFFFFF;

	typedef u32 ResourceHandle;

    struct BufferHandle {
        ResourceHandle                  index;
    }; // struct BufferHandle

    struct TextureHandle {
        ResourceHandle                  index;
    }; // struct TextureHandle

    struct ShaderStateHandle {
        ResourceHandle                  index;
    }; // struct ShaderStateHandle

    struct SamplerHandle {
        ResourceHandle                  index;
    }; // struct SamplerHandle

    struct DescriptorSetLayoutHandle {
        ResourceHandle                  index;
    }; // struct DescriptorSetLayoutHandle

    struct DescriptorSetHandle {
        ResourceHandle                  index;
    }; // struct DescriptorSetHandle

    struct PipelineHandle {
        ResourceHandle                  index;
    }; // struct PipelineHandle

    struct RenderPassHandle {
        ResourceHandle                  index;
    }; // struct RenderPassHandle

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

    static const u8                     MaxImageOutputs = 8;                // Maximum number of images/render_targets/fbo attachments usable.
    static const u8                     MaxDescriptorSetLayouts = 8;       // Maximum number of layouts in the pipeline.
    static const u8                     MaxShaderStages = 5;                // Maximum simultaneous shader stages. Applicable to all different type of pipelines.
    static const u8                     MaxDescriptorsPerSet = 16;         // Maximum list elements for both descriptor set layout and descriptor sets.
    static const u8                     MaxVertexStreams = 16;
    static const u8                     MaxVertexAttributes = 16;

    static const u32                    SubmitHeaderSentinel = 0xfefeb7ba;
    static const u32                    MaxResourceDeletions = 64;
}