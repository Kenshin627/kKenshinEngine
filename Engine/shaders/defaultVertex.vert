#version 460 core
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable

#extension GL_GOOGLE_include_directive : enable

#include "shaders/input.glsl"

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

layout(location = 0) out vec2 vCoord;
layout(location = 1) out vec3 vNormal;

void main()
{
	vCoord.s = objectData.vertexBuffer[gl_VertexIndex].u;
	vCoord.t = objectData.vertexBuffer[gl_VertexIndex].v;
	vNormal  = normalize(objectData.vertexBuffer[gl_VertexIndex].normal);
	gl_Position = sceneData.camera.viewProjectionMatrix * objectData.modelMatrix * vec4(objectData.vertexBuffer[gl_VertexIndex].pos, 1.0);	
}