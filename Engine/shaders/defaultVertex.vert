#version 460 core
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2: enable

layout(buffer_reference, std430, buffer_reference_align = 8) buffer VertexBufferRef
{
	vec3 pos;
	float u;
	vec3 normal;
	float v;
};

layout(std430, push_constant) uniform VertexBufferAddress
{
	layout(offset = 16)	 mat4 viewMatrix;
	layout(offset = 80)	 mat4 projectionMatrix;
	layout(offset = 144) VertexBufferRef vertexBuffer; 
} sceneData;


layout(location = 0) out vec2 vCoord;
layout(location = 1) out vec3 vNormal;
void main()
{
	vCoord.s = sceneData.vertexBuffer[gl_VertexIndex].u;
	vCoord.t = sceneData.vertexBuffer[gl_VertexIndex].v;
	vNormal = sceneData.vertexBuffer[gl_VertexIndex].normal;
	gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * vec4(sceneData.vertexBuffer[gl_VertexIndex].pos, 1.0);	
}