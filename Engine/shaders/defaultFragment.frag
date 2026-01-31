#version 460 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders/input.glsl"

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vCoord;
layout(location = 1) in vec3 vNormal;

void main()
{
	vec3 n = normalize(vNormal);
	vec3 l = normalize(sceneData.light.direction.xyz);
	float diffuse = max(dot(n, l), 0);
	vec3 texColor = texture(albedoTexture, vCoord).rgb;
	fragColor = vec4(diffuse * texColor *  sceneData.light.color.rgb, 1.0);
}

