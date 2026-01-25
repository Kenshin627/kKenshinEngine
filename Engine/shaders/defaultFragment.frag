#version 460 core

layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 vCoord;
layout(location = 1) in vec3 vNormal;

layout(push_constant) uniform DirectionLight {
	vec3 direction;
} directionLight;	

void main()
{
	vec3 n = normalize(vNormal);
	vec3 l = normalize(directionLight.direction);
	float diffuse = max(dot(n, l), 0);
	vec4 texColor = texture(textureSampler, vCoord);
	fragColor = vec4(diffuse * vec3(1, 1, 1), 1.0);
}

