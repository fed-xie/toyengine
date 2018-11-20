#version 420 core

in vec2 vertOutTexCoord;

layout(location = 0) out vec4 fragOutColor;

uniform sampler2D textData;
uniform uint charRGBA;

void main()
{
	vec4 textColor = unpackUnorm4x8(charRGBA).rgba;
	textColor.a *= texture(textData, vertOutTexCoord).r;
	fragOutColor = textColor;
}