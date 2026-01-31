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