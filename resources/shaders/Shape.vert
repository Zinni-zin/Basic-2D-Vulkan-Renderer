#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject 
{
	mat4 pv;
} ubo;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColour;
layout (location = 2) in vec2 inLocalPos;
layout (location = 3) in float inCircleThickness;
layout (location = 4) in float inCircleFade;
layout (location = 5) in float inShapeType;

layout (location = 0) out vec4 fragColour;
layout (location = 1) out vec2 localPos;
layout (location = 2) out float circleThickness;
layout (location = 3) out float circleFade;
layout (location = 4) out float shapeType;

void main() 
{
	gl_Position = ubo.pv * vec4(inPos, 1.0);

	fragColour = inColour;
	localPos = inLocalPos;
	shapeType = inShapeType;
	circleThickness = inCircleThickness;
	circleFade = inCircleFade;
}