#version 460 core

layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 vCoord;

layout(push_constant) uniform PushConstants {
	vec4 color;
} pc;	

void main()
{
	vec4 texColor = texture(textureSampler, vCoord);
	fragColor = pc.color * texColor;
}

