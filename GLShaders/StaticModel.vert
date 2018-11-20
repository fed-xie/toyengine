#version 330 core

layout(location = 0) in vec3 vi_Position;
layout(location = 1) in vec3 vi_Normal;
layout(location = 2) in vec2 vi_Coord;

uniform mat4 MMat; //model matrix

layout(shared, row_major) uniform UCameraMatrices
{
	mat4 PVMat; //projection-view matrix
	mat4 PMat;  //projection matrix
	mat4 VMat;  //view matrix
};

out vec2 vo_Coord;

void main()
{
	mat4 PVMMat = PVMat * MMat;
	gl_Position = PVMMat * vec4(vi_Position, 1.0);
	vo_Coord = vi_Coord.st;
}
