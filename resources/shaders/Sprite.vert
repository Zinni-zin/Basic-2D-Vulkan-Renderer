#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject 
{
	mat4 pv;
} ubo;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColour;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in int inTexIndex;

layout (location = 0) out vec4 fragColour;
layout (location = 1) out vec2 fragUV; 
layout (location = 2) out int texIndex;

void main() 
{
	gl_Position = ubo.pv * vec4(inPos, 1.0);

	fragColour = inColour;
	fragUV = inTexCoord;
	texIndex = inTexIndex;
}