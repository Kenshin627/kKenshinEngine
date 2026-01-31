#version 460 core

struct Camera
{
    vec4 position;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
};

struct DirectionLight
{
    vec4 direction;
    vec4 color;
};

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 vCoord;
layout(location = 1) in vec3 vNormal;

layout(set = 0, binding = 0) uniform SceneData
{
	DirectionLight light;
	Camera camera;
} sceneData;

layout(set = 1, binding = 0) uniform MaterialData
{
	vec4 albeoFactor;
	vec4 metalRoughnessFactor;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
							 
layout(set = 1, binding = 2) uniform sampler2D metalRoughnessTexture;


void main()
{
	vec3 n = normalize(vNormal);
	vec3 l = normalize(sceneData.light.direction.xyz);
	float diffuse = max(dot(n, l), 0);
	vec3 texColor = texture(albedoTexture, vCoord).rgb;
	//fragColor = vec4(diffuse * texColor *  sceneData.light.color.rgb, 1.0);
	n = (n + 1.0) * 0.5;
	fragColor = vec4(n, 1.0);
}

