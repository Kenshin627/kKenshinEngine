#version 460 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders/input.glsl"

#define PI 3.14159265359

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vCoord;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vPos;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
	vec3 n = normalize(vNormal);
	vec3 l = normalize(sceneData.light.direction.xyz);
    vec3 v = normalize(sceneData.camera.position.xyz - vPos);
	vec3 h = normalize(l + v);

    vec4 metalRoughess = texture(metalRoughnessTexture, vCoord);
    float metal = metalRoughess.r;
    float roughness = metalRoughess.g;
    vec3 albedo = texture(albedoTexture, vCoord).rgb;

    vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metal);

	float NDF = DistributionGGX(n, h, roughness);   
	float G   = GeometrySmith(n, v, l, roughness);      
	vec3  F   = fresnelSchlick(max(dot(h, v), 0.0), F0);
	   
	vec3 numerator    = NDF * G * F; 
	float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
	vec3 specular = numerator / denominator;

	vec3 Ks = F;
	vec3 Kd = vec3(1.0) - Ks;
	Kd *= (1.0 - metal);
	float NdotL = max(dot(n, l), 0.0);

    //TODO SSAO
    float ambientOcclusion = 1.0f;

	vec3 ao = vec3(0.3) * sceneData.light.color.rgb * albedo * ambientOcclusion;
	fragColor.rgb = (Kd * albedo / PI + specular) * NdotL * sceneData.light.color.rgb + ao;
	fragColor.a = 1.0;
}

