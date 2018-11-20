#version 330 core

uniform mat4 PVMMat; // Projection-View-Model matrix

layout(location = 0) in vec3 vertInPos;
layout(location = 1) in vec3 vertInNormal;
layout(location = 2) in vec2 vertInTexCoord;

out vec2 vertOutTexCoord;

void main()
{
	gl_Position = PVMMat * vec4(vertInPos, 1.0);
	vertOutTexCoord = vertInTexCoord;
}