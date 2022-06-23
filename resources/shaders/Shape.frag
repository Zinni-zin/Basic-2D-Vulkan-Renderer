#version 450 core

layout (location = 0) out vec4 outColour;

layout (location = 0) in vec4 fragColour;
layout (location = 1) in vec2 localPos;
layout (location = 2) in float circleThickness;
layout (location = 3) in float circleFade;
layout (location = 4) in float shapeType;

// enum class ShapeType { RECTANGLE = 0, CIRCLE = 1 };

void main()
{	
	vec4 colour = fragColour;

	if(shapeType == 1.0)
	{
		float distance = 1.0 - length(localPos);
		float circle = smoothstep(0.0, circleFade, distance);
		circle *= smoothstep(circleThickness + circleFade, circleThickness, distance);

		if(circle == 0.0)
			discard;

		colour.a *= circle;
	}

	outColour = colour;
}
