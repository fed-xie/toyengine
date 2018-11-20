#version 330 core

layout(location = 0) in vec3 vi_position;
layout(location = 1) in vec3 vi_normal;
layout(location = 2) in vec2 vi_coordination;
layout(location = 3) in uvec4 vi_boneIdx;
layout(location = 4) in vec4 vi_boneWeight;

uniform mat4 MMat; //model matrix

layout(shared, row_major) uniform;
uniform UCameraMatrices
{
	mat4 PVMat; //projection-view matrix
	mat4 PMat;  //projection matrix
	mat4 VMat;  //view matrix
};
uniform UBoneMatrices
{
	mat4 boneMatrices[512];
};

//out vec3 vo_normal;
out vec2 vo_Coord; //must same to the name in fragment shader

void main()
{
	mat4 PVMMat = PVMat * MMat;
	mat4 poseMat = vi_boneWeight.x * boneMatrices[vi_boneIdx.x];
	poseMat += vi_boneWeight.y * boneMatrices[vi_boneIdx.y];
	poseMat += vi_boneWeight.z * boneMatrices[vi_boneIdx.z];
	poseMat += vi_boneWeight.w * boneMatrices[vi_boneIdx.w];
	
	float weightSum = round(vi_boneWeight.x + vi_boneWeight.y + vi_boneWeight.z + vi_boneWeight.w);
	poseMat = (1.0 - weightSum) * mat4(1.0) + weightSum * poseMat; //for some vertices not bind to skeleton, weightSum is 0
	gl_Position = PVMMat * poseMat * vec4(vi_position, 1.0);
	vo_Coord = vi_coordination.st;
}