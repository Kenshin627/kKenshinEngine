#version 460 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders/input.glsl"

layout(location = 0) in vec2 vCoord;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vPos;

layout(location = 0) out vec4 albedoAttachment;
layout(location = 1) out vec4 positionAttachment;
layout(location = 2) out vec4 normalAttachment;
layout(location = 3) out vec4 metalRoughnessAttachment;

void main()
{
	vec3 n = normalize(vNormal);
    vec4 metalRoughess = texture(metalRoughnessTexture, vCoord);
    vec4 albedo = texture(albedoTexture, vCoord);
	
	albedoAttachment = albedo;
	positionAttachment = vec4(vPos, 1.0);
	normalAttachment = vec4(n, 1.0);
	metalRoughnessAttachment = metalRoughess;
}

