#version 460 core
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2: enable

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

layout(buffer_reference, std430, buffer_reference_align = 8) buffer VertexBufferRef
{
	vec3 pos;
	float u;
	vec3 normal;
	float v;
};

layout(std430, push_constant) uniform ObjectData
{
	layout(offset = 0)	 mat4 modelMatrix;
	layout(offset = 64)	 VertexBufferRef vertexBuffer; 
} objectData;

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


layout(location = 0) out vec2 vCoord;
layout(location = 1) out vec3 vNormal;

void main()
{
	vCoord.s = objectData.vertexBuffer[gl_VertexIndex].u;
	vCoord.t = objectData.vertexBuffer[gl_VertexIndex].v;
	vNormal = objectData.vertexBuffer[gl_VertexIndex].normal;
	gl_Position = sceneData.camera.viewProjectionMatrix * vec4(objectData.vertexBuffer[gl_VertexIndex].pos, 1.0);	
}