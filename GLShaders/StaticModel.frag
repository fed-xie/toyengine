#version 330 core

in vec2 vo_Coord;

uniform sampler2D diffuseTex;

out vec4 fo_Color;

void main()
{
	fo_Color = texture(diffuseTex, vo_Coord);
}