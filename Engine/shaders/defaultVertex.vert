#version 460 core
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2: enable

layout(buffer_reference, std430, buffer_reference_align = 8) buffer VertexBufferRef
{
	vec3 pos;
	vec2 coords;
};

layout(std430, push_constant) uniform VertexBufferAddress
{
	layout(offset = 16) VertexBufferRef vertexBuffer; 
} dba;

layout(location = 0) out vec2 vCoord;
void main()
{
	vCoord = dba.vertexBuffer[gl_VertexIndex].coords;
	gl_Position = vec4(dba.vertexBuffer[gl_VertexIndex].pos, 1.0);	
}