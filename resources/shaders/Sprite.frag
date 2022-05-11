#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform sampler samp[];
layout(set = 0, binding = 2) uniform texture2D textures[];

layout(location = 0) out vec4 outColour;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in flat int texIndex; 

void main()
{
	vec4 text = texture(sampler2D(textures[texIndex], samp[texIndex]), fragUV);
	outColour = text * fragColour;
}
