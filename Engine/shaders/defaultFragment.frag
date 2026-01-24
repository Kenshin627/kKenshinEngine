#version 460 core

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
	vec4 color;
} pc;	

void main()
{
	fragColor = pc.color;
}

