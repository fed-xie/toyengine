#pragma once
#include "Types.h"
#include "Memory.h"
#include "IO.h"

namespace toy
{
	namespace io
	{
		class Model3D
		{
		public:
			struct Mesh
			{
				size_t vertexDataSize = 0; //vertex data size (bytes)
				size_t posSize = 0;
				size_t normalSize = 0;
				size_t coordSize = 0;
				size_t boneIndexSize = 0;
				size_t boneWeightSize = 0;

				/* vertex data pointers, point to data in *vertexData */
				float(*positionPtr)[3] = nullptr;
				float(*normalPtr)[3] = nullptr;
				float(*coordPtr)[8][3] = nullptr;
				u32(*boneIdx)[4] = nullptr; //index of boneOffset
				float(*boneWeight)[4] = nullptr;

				void *vertexData = nullptr;
				u32 *vertIndices = nullptr;

				float aabboxMin[3] = { -1, -1, -1 };
				float aabboxMax[3] = { 1, 1, 1 };

				u32 vertNum = 0;
				u32 indicesNum = 0;
				u32 materialIdx = 0xFFFFFFFF;
			};
			struct TexturePath
			{
				size_t strLen = 0;
				char* path = nullptr;
			};
			struct Material
			{
				enum TextureType
				{
					TEXTURE_UNKNOWN = 0,
					TEXTURE_DIFFUSE,
					TEXTURE_SPECULAR,
					TEXTURE_AMBIENT,
					TEXTURE_EMISSIVE,
					TEXTURE_NORMALS,
					TEXTURE_HEIGHT,
					TEXTURE_SHININESS,
					TEXTURE_OPACITY,
					TEXTURE_DISPLACEMENT,
					TEXTURE_LIGHTMAP,
					TEXTURE_REFLECTION,

					TEXTURE_TYPEMAX,
				};
				struct MatTex
				{
					uint texIndex; //index of texPaths
					TextureType type;
				};
				int texNum = 0; //number of texture in this material
				MatTex *textures;

				bool operator==(const Material& mat)
				{
					if (texNum != mat.texNum)
						return false;
					for (int i = 0; i != texNum; ++i)
					{
						if (textures[i].texIndex != mat.textures[i].texIndex
							|| textures[i].type != mat.textures[i].type)
							return false;
					}
					return true;
				}
			};
			struct Skeleton
			{
				struct SkeletalNode
				{
					float nodeMatrix[16]; //row-major matrix
					u32 boneIdx; //0xFFFFFFFFF if not exist
					u32 parentIdx; //0xFFFFFFFF if not exist
					u32 firstChildIdx; //0xFFFFFFFF if not exist
					u32 nextBroIdx; //0xFFFFFFFF if not exist
					char name[63];
					bool isMeshNode;
				};
				u32 nodeNum = 0;
				u32 boneNum = 0;
				SkeletalNode *nodes = nullptr;
				float* boneOffsets = nullptr; //row-major matrix, 4x4
			};
			struct SkeletalAnimation
			{
				struct NodeAnim
				{
					size_t dataSize = 0;
					void *keyData = nullptr;

					u32 nodeIndex;//index of skeleton.skeletonNodes

					u32 posKeyNum = 0;
					u32 rotKeyNum = 0;
					u32 scaleKeyNum = 0;

					/* pointers to keyData */
					double *positionKeyTimes = nullptr;//time points, not always begin with 0
					float *positionKeys = nullptr;//vec3

					double *rotationKeyTimes = nullptr;//time points, not always begin with 0
					float *rotationKeys = nullptr;//quaternion(w, x, y, z)

					double *scaleKeyTimes = nullptr;//time points, not always begin with 0
					float *scaleKeys = nullptr;//vec3
				};
				double ticksPerSecond = 0.0;
				double duration = 0.0;//duration of this animation in ticks
				u32 nodeAnimNum = 0;
				NodeAnim *nodeAnims = nullptr;
			};

			u32 meshNum = 0;
			u32 texNum = 0;
			u32 materialNum = 0;
			u32 animNum = 0;

			Skeleton skeleton;
			SkeletalAnimation *skelAnims = nullptr;//model animations

			/* pointers to the first element */
			Mesh *meshes = nullptr;
			TexturePath *texPaths = nullptr; //unique path
			Material *materials = nullptr; //unique material

			bool loadFromMemory(
				void* srcData, size_t srcSize, const char* typeHint, 
				DE_Allocator_NoLock &allocator);
		};
	}
}