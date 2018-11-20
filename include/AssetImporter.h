#pragma once
#include "Types.h"
#include "IRenderDriver.h"
#include "Memory.h"

namespace Assimp
{
	class Importer;
}

namespace toy
{
	enum TextureType
	{
		TEXTYPE_UNKNOWN = 0,
		TEXTYPE_DIFFUSE = 1,
		TEXTYPE_SPECULAR = 2,
		TEXTYPE_AMBIENT = 3,
		TEXTYPE_EMISSIVE = 4,
		TEXTYPE_HEIGHT = 5,
		TEXTYPE_NORMALS = 6,
		TEXTYPE_SHININESS = 7,
		TEXTYPE_OPACITY = 8,
		TEXTYPE_DISPLACEMENT = 9,
		TEXTYPE_LIGHTMAP = 10,
		TEXTYPE_REFLECTION = 11,

		TEXTYPE_TYPEMAX = 12,
	};

	struct AssetPath
	{
		u64 hash;
		u16 strLen;
		char path[1024 - 8 - 2];
	};

	struct AssetMaterial
	{
		struct MatTex
		{
			uint texIndex; //index of AssetPath
			TextureType type;
		};

		MatTex textures[8];
		u8 texNum = 0; //number of texture in this material

		bool operator==(const AssetMaterial& mat)
		{
			if (texNum != mat.texNum)
				return false;
			for (int i = 0; i != texNum; ++i)
			{
				if (textures[i].texIndex != mat.textures[i].texIndex || 
					textures[i].type != mat.textures[i].type)
					return false;
			}
			return true;
		}
	};

	struct AssetModel
	{
		struct Mesh
		{
			uint vertNum;
			uint triNum;

			//Vertex data
			fvec3* pos;
			fvec3* normal;
			fvec2* coord;
			uvec4* boneIndices;
			fvec4* boneWeights;

			uvec3* triIndices;

			float aabboxMin[3] = { -1.0f, -1.0f, -1.0f };
			float aabboxMax[3] = { 1.0f, 1.0f, 1.0f };
		};

		struct Skeleton
		{
			struct Node
			{
				fmat4 nodeMatrix;
				u16 boneIdx; //(u16)-1 if not exist
				u16 parentIdx; //(u16)-1 if not exist
				u16 firstChildIdx; //(u16)-1 if not exist
				u16 nextBroIdx; //(u16)-1 if not exist
				char name[63];
				u8 isMeshNode;
			};
			u16 nodeNum = 0;
			u16 boneNum = 0;
			Node *nodes = nullptr;
			fmat4* boneOffsets = nullptr;
		};

		struct SkeletalAnimation
		{
			struct NodeAnim
			{
				u32 posKeyNum = 0;
				u32 rotKeyNum = 0;
				u32 scaleKeyNum = 0;

				u32 lastTransKey = 0;
				u32 lastScaleKey = 0;
				u32 lastRotKey = 0;

				/* pointers to keyData */
				double *positionKeyTimes = nullptr;//time points, not always begin with 0
				fvec3 *positionKeys = nullptr;

				double *rotationKeyTimes = nullptr;//time points, not always begin with 0
				fquat *rotationKeys = nullptr;//quaternion(w, x, y, z)

				double *scaleKeyTimes = nullptr;//time points, not always begin with 0
				fvec3 *scaleKeys = nullptr;

				u16 nodeIndex;//index of skeleton.skeletonNodes

				void calcInterpolatedRotation(double time, fquat* out);
				void calcInterpolatedTranslation(double time, fvec3 *out);
				void calcInterpolatedScaling(double time, fvec3 *out);
			};
			double ticksPerSecond = 0.0;
			double duration = 0.0;//duration of this animation in ticks
			u32 nodeAnimNum = 0;
			NodeAnim *nodeAnims = nullptr;
		};

		u32 meshNum = 0;
		u32 animNum = 0;

		Skeleton* skeleton = nullptr;
		SkeletalAnimation *skelAnims = nullptr;//model animations

		/* pointers to the first element */
		Mesh *meshes = nullptr;
	};

	class AssetImporter
	{
	public:
		void init(IRenderDriver* rdr);
		void destroy();

		rd::TBO loadImage2D(const void* fileData, size_t fileSize, const char* suffix);

		//return number of meshes, dataSize is a rough size of model data
		uint openModel3D(
			const void* fileData, size_t fileSize, const char* suffix,
			size_t *dataSize = nullptr);

		//store skeletal data in the left of the allocator memory, mesh data in right side
		//AssetModel* will be kept until next call
		AssetModel* loadModel(DE_Allocator_NoLock* allocator);

		bool getTexPathsAndMaterials(
			uint &pathNum, AssetPath* &paths,
			uint &matNum, AssetMaterial* &mats,
			u8* materialIndices,
			DE_Allocator_NoLock *allocator);

		//release space allocated for this model, only need to call for last model
		void closeModel3D();

		void calcSkelAnimMatrices(
			AssetModel::Skeleton* skel,//skaleton related to anims
			AssetModel::SkeletalAnimation* anims,//animation related to skel
			uint animIndex,
			bool repeat,
			double time,
			fmat4* nodeMatrices,//temp space for calc, at least skel->nodeNum in length
			fmat4* out);//output, at least skel->boneNum in length

	private:
		AssetModel m_model;
		IRenderDriver* m_rdr;
		Assimp::Importer* m_aImporter;
	};
}