#version 150
#extension GL_ARB_shading_language_420pack : require
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 Projection;
uniform mat4 Modelview;

out vec2 frag_uv;

void main()
{
	frag_uv = texCoord;
    vec4 Position = vec4(position, 1.0);
    gl_Position = Projection * Modelview * Position;
}
