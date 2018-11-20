#include "include/AssetImporter.h"
#include "include/IO.h"
#include "include/Mathlib.h"

#include "libs/include/assimp/Importer.hpp"
#include "libs/include/assimp/scene.h"
#include "libs/include/assimp/postprocess.h"

#pragma comment(lib, "libs/Assimp/assimp-vc140-mt.lib")

#include "libs/include/IL/il.h"
#include "libs/include/IL/ilu.h"
#include "libs/include/IL/ilut.h"

#pragma comment(lib, "libs/DevIL/DevIL.lib")
#pragma comment(lib, "libs/DevIL/ILU.lib")
#pragma comment(lib, "libs/DevIL/ILUT.lib")

namespace toy
{
	void AssetModel::SkeletalAnimation::NodeAnim::calcInterpolatedRotation(
		double time, fquat* out)
	{
		uint frame = rotationKeyTimes[lastRotKey] < time ? ++lastRotKey : 0;
		for (; frame < rotKeyNum; ++frame)
		{
			if (rotationKeyTimes[frame] > time)
				break;
		}
		frame %= rotKeyNum;

		if (0 != frame)
		{
			double deltaT = rotationKeyTimes[frame] - rotationKeyTimes[frame - 1];
			float factor = (time - rotationKeyTimes[frame - 1]) / deltaT;
			assert(factor >= 0.0f && factor <= 1.0f);
			*out = slerp(rotationKeys[frame - 1], rotationKeys[frame], factor);
			out->normalize();
			lastRotKey = --frame;
		}
		else
		{
			*out = rotationKeys[0];
			lastRotKey = 0;
		}
	}

	void AssetModel::SkeletalAnimation::NodeAnim::calcInterpolatedTranslation(
		double time, fvec3 *out)
	{
		uint frame = positionKeyTimes[lastTransKey] < time ? ++lastTransKey : 0;
		for (; frame < posKeyNum; ++frame)
		{
			if (positionKeyTimes[frame] > time)
				break;
		}
		frame %= posKeyNum;
		if (0 != frame)
		{
			double deltaT = positionKeyTimes[frame] - positionKeyTimes[frame - 1];
			float factor = (time - positionKeyTimes[frame - 1]) / deltaT;
			assert(factor >= 0.0f && factor <= 1.0f);
			*out = positionKeys[frame - 1] * factor + positionKeys[frame] * (1.0f - factor);
			lastTransKey = --frame;
		}
		else
		{
			*out = positionKeys[0];
			lastTransKey = 0;
		}
	}

	void AssetModel::SkeletalAnimation::NodeAnim::calcInterpolatedScaling(
		double time, fvec3 *out)
	{
		uint frame = scaleKeyTimes[lastScaleKey] < time ? ++lastScaleKey : 0;
		for (; frame < scaleKeyNum; ++frame)
		{
			if (scaleKeyTimes[frame] > time)
				break;
		}
		frame %= scaleKeyNum;
		if (0 != frame)
		{
			*out = scaleKeys[frame];
			//double deltaT = scaleKeyTimes[scaleIdx] - scaleKeyTimes[scaleIdx - 1];
			//float factor = (time - scaleKeyTimes[scaleIdx - 1]) / deltaT;
			//assert(factor >= 0.0f && factor <= 1.0f);
			//*out = scaleKeys[scaleIdx - 1] * factor + scaleKeys[scaleIdx] * (1.0f - factor);
			lastScaleKey = --frame;
		}
		else
		{
			*out = scaleKeys[0];
			lastScaleKey = 0;
		}
	}

	static aiTextureType aTexType[TEXTYPE_TYPEMAX] = {};

	void AssetImporter::init(IRenderDriver* rdr)
	{
		ilInit();
		//iluInit();
		//if(rd::RDT_OPENGL == rdr->getType())
		//{
		//	ilutRenderer(ILUT_OPENGL);
		//	ilutInit();
		//	ilutEnable(ILUT_GL_USE_S3TC);
		//}
		//else
		//{
		//	assert(0 && "importer not supported this driver yet");
		//}

		m_aImporter = new Assimp::Importer;

		m_rdr = rdr;
		
		aTexType[TEXTYPE_UNKNOWN] = aiTextureType_NONE;
		aTexType[TEXTYPE_DIFFUSE] = aiTextureType_DIFFUSE;
		aTexType[TEXTYPE_SPECULAR] = aiTextureType_SPECULAR;
		aTexType[TEXTYPE_AMBIENT] = aiTextureType_AMBIENT;
		aTexType[TEXTYPE_EMISSIVE] = aiTextureType_EMISSIVE;
		aTexType[TEXTYPE_HEIGHT] = aiTextureType_HEIGHT;
		aTexType[TEXTYPE_NORMALS] = aiTextureType_NORMALS;
		aTexType[TEXTYPE_SHININESS] = aiTextureType_SHININESS;
		aTexType[TEXTYPE_OPACITY] = aiTextureType_OPACITY;
		aTexType[TEXTYPE_DISPLACEMENT] = aiTextureType_DISPLACEMENT;
		aTexType[TEXTYPE_LIGHTMAP] = aiTextureType_LIGHTMAP;
		aTexType[TEXTYPE_REFLECTION] = aiTextureType_REFLECTION;
	}

	void AssetImporter::destroy()
	{
		delete m_aImporter;
		m_aImporter = nullptr;
	}

	struct DevILTypeHint
	{
		const char* suffix;
		u32 typeHint;
	};
	const static DevILTypeHint devILTypeHints[] = 
	{
		{"dds", IL_DDS},
		{"png", IL_PNG},
		{"tga", IL_TGA},
		{"jpg", IL_JPG},
		{"jpeg", IL_JPG},
	};
	static inline ILenum getDevILTypeHint(const char* suffix)
	{
		for (auto& devILType : devILTypeHints)
		{
			if (0 == str::striCmp(suffix, devILType.suffix))
				return devILType.typeHint;
		}
		return IL_TYPE_UNKNOWN;
	}
	rd::TBO AssetImporter::loadImage2D(
		const void* fileData, size_t fileSize, const char* suffix)
	{
		ILenum typeHint = getDevILTypeHint(suffix);
		ILuint handle;
		ilGenImages(1, &handle);
		ilBindImage(handle);
		if (!ilLoadL(typeHint, fileData, fileSize))
		{
			ilDeleteImage(handle);
			return rd::TBO(0);
		}
		rd::RawImage2D rawImg2D;
		rawImg2D.width = ilGetInteger(IL_IMAGE_WIDTH);
		rawImg2D.height = ilGetInteger(IL_IMAGE_HEIGHT);
		rawImg2D.glColorFmt = ilGetInteger(IL_IMAGE_FORMAT);
		rawImg2D.glDataType = ilGetInteger(IL_IMAGE_TYPE);
		//bitDepth = 8;// ilGetInteger(IL_IMAGE_DEPTH);
		rawImg2D.data = ilGetData();
		rawImg2D.size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);

		auto tbo = m_rdr->loadNewTexture(&rawImg2D);

		ilDeleteImage(handle);
		return tbo;
	}

	static void AddNodeWeight(unsigned int& iScene, const aiNode* pcNode)
	{
		iScene += sizeof(aiNode);
		iScene += sizeof(unsigned int) * pcNode->mNumMeshes;
		iScene += sizeof(void*) * pcNode->mNumChildren;

		for (unsigned int i = 0; i < pcNode->mNumChildren; ++i) {
			AddNodeWeight(iScene, pcNode->mChildren[i]);
		}
	}
	static void GetMemoryRequirements(aiMemoryInfo& in, const aiScene* mScene)
	{
		in = aiMemoryInfo();
		in.total = sizeof(aiScene);

		// add all meshes
		for (unsigned int i = 0; i < mScene->mNumMeshes; ++i)
		{
			in.meshes += sizeof(aiMesh);
			if (mScene->mMeshes[i]->HasPositions()) {
				in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;
			}
			if (mScene->mMeshes[i]->HasNormals()) {
				in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;
			}
			if (mScene->mMeshes[i]->HasTangentsAndBitangents()) {
				in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices * 2;
			}
			for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a) {
				if (mScene->mMeshes[i]->HasVertexColors(a)) {
					in.meshes += sizeof(aiColor4D) * mScene->mMeshes[i]->mNumVertices;
				}
				else break;
			}
			for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
				if (mScene->mMeshes[i]->HasTextureCoords(a)) {
					in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;
				}
				else break;
			}
			if (mScene->mMeshes[i]->HasBones()) {
				in.meshes += sizeof(void*) * mScene->mMeshes[i]->mNumBones;
				for (unsigned int p = 0; p < mScene->mMeshes[i]->mNumBones; ++p) {
					in.meshes += sizeof(aiBone);
					in.meshes += mScene->mMeshes[i]->mBones[p]->mNumWeights * sizeof(aiVertexWeight);
				}
			}
			in.meshes += (sizeof(aiFace) + 3 * sizeof(unsigned int))*mScene->mMeshes[i]->mNumFaces;
		}
		in.total += in.meshes;

		// add all embedded textures
		for (unsigned int i = 0; i < mScene->mNumTextures; ++i) {
			const aiTexture* pc = mScene->mTextures[i];
			in.textures += sizeof(aiTexture);
			if (pc->mHeight) {
				in.textures += 4 * pc->mHeight * pc->mWidth;
			}
			else in.textures += pc->mWidth;
		}
		in.total += in.textures;

		// add all animations
		for (unsigned int i = 0; i < mScene->mNumAnimations; ++i) {
			const aiAnimation* pc = mScene->mAnimations[i];
			in.animations += sizeof(aiAnimation);

			// add all bone anims
			for (unsigned int a = 0; a < pc->mNumChannels; ++a) {
				const aiNodeAnim* pc2 = pc->mChannels[a];//i in src, wait for fixing
				in.animations += sizeof(aiNodeAnim);
				in.animations += pc2->mNumPositionKeys * sizeof(aiVectorKey);
				in.animations += pc2->mNumScalingKeys * sizeof(aiVectorKey);
				in.animations += pc2->mNumRotationKeys * sizeof(aiQuatKey);
			}
		}
		in.total += in.animations;

		// add all cameras and all lights
		in.total += in.cameras = sizeof(aiCamera) *  mScene->mNumCameras;
		in.total += in.lights = sizeof(aiLight)  *  mScene->mNumLights;

		// add all nodes
		AddNodeWeight(in.nodes, mScene->mRootNode);
		in.total += in.nodes;

		// add all materials
		for (unsigned int i = 0; i < mScene->mNumMaterials; ++i) {
			const aiMaterial* pc = mScene->mMaterials[i];
			in.materials += sizeof(aiMaterial);
			in.materials += pc->mNumAllocated * sizeof(void*);

			for (unsigned int a = 0; a < pc->mNumProperties; ++a) {
				in.materials += pc->mProperties[a]->mDataLength;
			}
		}
		in.total += in.materials;
	}

	uint AssetImporter::openModel3D(
		const void* fileData, size_t fileSize, const char* suffix,
		size_t *dataSize)
	{
		const aiScene* aScene = m_aImporter->ReadFileFromMemory(
			fileData,
			fileSize,
			aiProcess_Triangulate | aiProcess_LimitBoneWeights |
			aiProcess_ImproveCacheLocality | aiProcess_FlipUVs,
			suffix);
		if (!aScene ||
			aScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
			!aScene->mRootNode)
		{
			LOG_EXT_WARNING("%s\n", m_aImporter->GetErrorString());
			return 0;
		}

		aiMemoryInfo in;
		GetMemoryRequirements(in, aScene);//bug in aiImporter::GetMemoryRequirements()
		if (dataSize)
			*dataSize = in.total;
		
		return aScene->mNumMeshes;
	}

#define ALLOC_L(v, a, t, n) \
	v = a->allocL<t>(n);\
	if(!v) return false;
#define ALLOC_AL(v, a, t, n) \
	v = a->allocAlignedL<t>(n);\
	if(!v) return false;
#define ALLOC_R(v, a, t, n) \
	v = a->allocR<t>(n);\
	if(!v) return false;
#define NEW_L(t, v, a, n) t* ALLOC_L(v, a, t, n)
#define NEW_R(t, v, a, n) t* ALLOC_R(v, a, t, n)
#define NEW_AL(t, v, a, n) t* ALLOC_AL(v, a, t, n)

	static void calcMeshAABBox(
		const aiMesh *aMesh,
		AssetModel::Mesh *mesh)
	{
		mesh->aabboxMin[0] = aMesh->mVertices[0].x;
		mesh->aabboxMin[1] = aMesh->mVertices[0].y;
		mesh->aabboxMin[2] = aMesh->mVertices[0].z;
		mesh->aabboxMax[0] = aMesh->mVertices[0].x;
		mesh->aabboxMax[1] = aMesh->mVertices[0].y;
		mesh->aabboxMax[2] = aMesh->mVertices[0].z;
		for (uint i = 1; i != aMesh->mNumVertices; ++i)
		{
			if (mesh->aabboxMin[0] > aMesh->mVertices[i].x)
				mesh->aabboxMin[0] = aMesh->mVertices[i].x;
			if (mesh->aabboxMin[1] > aMesh->mVertices[i].y)
				mesh->aabboxMin[1] = aMesh->mVertices[i].y;
			if (mesh->aabboxMin[2] > aMesh->mVertices[i].z)
				mesh->aabboxMin[2] = aMesh->mVertices[i].z;
			if (mesh->aabboxMax[0] < aMesh->mVertices[i].x)
				mesh->aabboxMax[0] = aMesh->mVertices[i].x;
			if (mesh->aabboxMax[1] < aMesh->mVertices[i].y)
				mesh->aabboxMax[1] = aMesh->mVertices[i].y;
			if (mesh->aabboxMax[2] < aMesh->mVertices[i].z)
				mesh->aabboxMax[2] = aMesh->mVertices[i].z;
		}
	}

	static bool loadMesh(
		const aiMesh* aMesh,
		AssetModel::Mesh* mesh,
		DE_Allocator_NoLock* allocator)
	{
		auto coordSize = aMesh->mNumVertices * sizeof(fvec2);
		auto triSize = aMesh->mNumFaces * sizeof(uvec3);
		auto boneIndicesSize = aMesh->mNumVertices * sizeof(uvec4);
		auto boneWeightSize = aMesh->mNumVertices * sizeof(fvec4);
		auto meshSize = coordSize + triSize + boneIndicesSize + boneWeightSize;

		NEW_R(c8, tempBuf, allocator, meshSize);
		uint tempOffset = 0;

		mesh->pos = reinterpret_cast<fvec3*>(aMesh->mVertices);
		mesh->normal = reinterpret_cast<fvec3*>(aMesh->mNormals);
		mesh->coord = nullptr;
		if (aMesh->HasTextureCoords(0))
		{
			mesh->coord = reinterpret_cast<fvec2*>(tempBuf + tempOffset);
			tempOffset += coordSize;
			for (uint vert = 0; vert != aMesh->mNumVertices; ++vert)
			{
				mesh->coord[vert].X = aMesh->mTextureCoords[0][vert].x;
				mesh->coord[vert].Y = aMesh->mTextureCoords[0][vert].y;
			}
		}
		mesh->vertNum = aMesh->mNumVertices;

		mesh->triIndices = nullptr;
		if (aMesh->HasFaces())
		{
			mesh->triIndices = reinterpret_cast<uvec3*>(tempBuf + tempOffset);
			tempOffset += triSize;
			for (uint tri = 0; tri != aMesh->mNumFaces; ++tri)
			{
				//assert(3 == aMesh->mFaces[tri].mNumIndices);
				mesh->triIndices[tri].X = aMesh->mFaces[tri].mIndices[0];
				mesh->triIndices[tri].Y = aMesh->mFaces[tri].mIndices[1];
				mesh->triIndices[tri].Z = aMesh->mFaces[tri].mIndices[2];
			}
		}
		mesh->triNum = aMesh->mNumFaces;

		mesh->boneIndices = nullptr;
		mesh->boneWeights = nullptr;
		if (aMesh->HasBones())
		{
			mesh->boneIndices = reinterpret_cast<uvec4*>(tempBuf + tempOffset);
			tempOffset += boneIndicesSize;
			mesh->boneWeights = reinterpret_cast<fvec4*>(tempBuf + tempOffset);
			tempOffset += boneWeightSize;
			memset(mesh->boneIndices, 0, boneIndicesSize + boneWeightSize);
			for (uint i = 0; i != aMesh->mNumBones; ++i)
			{
				auto &bone = aMesh->mBones[i];
				for (uint j = 0; j != bone->mNumWeights; ++j)
				{
					auto& vert = bone->mWeights[j].mVertexId;
					auto& weight = bone->mWeights[j].mWeight;
					for (uint k = 0; k != 4; ++k)
					{
						if (0.001f > mesh->boneWeights[vert].A[k])
						{
							mesh->boneIndices[vert].A[k] = i;
							mesh->boneWeights[vert].A[k] = weight;
							break;
						}
					}
				}//foreach boneWeight
			}//foreach bone
		}

		calcMeshAABBox(aMesh, mesh);

		return true;
	}

	/* count node's children, node self included */
	static u16 countChildrenNodes(const aiNode* node)
	{
		u16 count = 1;
		for (int i = 0; i != node->mNumChildren; ++i)
			count += countChildrenNodes(node->mChildren[i]);
		return count;
	}

	static void initNodeTree(
		aiNode* rootNode,
		aiNode** aNodes,
		AssetModel::Skeleton::Node* nodes,
		u16 nodeIdx,
		u16 parentIdx,
		u16 nextBroIdx,
		u16 &usedNum)
	{
		u16 firstChildIdx = usedNum;
		aNodes[nodeIdx] = rootNode;
		nodes[nodeIdx].parentIdx = parentIdx;
		nodes[nodeIdx].nextBroIdx = nextBroIdx;
		nodes[nodeIdx].boneIdx = (u16)-1;
		nodes[nodeIdx].isMeshNode = !!rootNode->mNumMeshes;
		memcpy(&nodes[nodeIdx].nodeMatrix, &rootNode->mTransformation.a1, sizeof(fmat4));

		if (aNodes[nodeIdx]->mName.length + 1 < ARRAYLENGTH(nodes[nodeIdx].name))
			memcpy(nodes[nodeIdx].name, aNodes[nodeIdx]->mName.C_Str(),
				aNodes[nodeIdx]->mName.length + 1);
		else
			nodes[nodeIdx].name[0] = '\0';

		if (rootNode->mNumChildren)
		{
			usedNum += rootNode->mNumChildren;
			u16 i = 0;
			for (; i != rootNode->mNumChildren; ++i)
			{
				initNodeTree(rootNode->mChildren[i],
					aNodes,
					nodes,
					firstChildIdx + i,
					nodeIdx,
					firstChildIdx + i + 1,
					usedNum);
			}

			nodes[nodeIdx].firstChildIdx = firstChildIdx;
			//mark last children, i == rootNode->mNumChildren (i >= 1)
			nodes[firstChildIdx + i - 1].nextBroIdx = (u16)-1;
		}
		else
			nodes[nodeIdx].firstChildIdx = (u16)-1;
	}

	static inline u16 findNodeByName(aiNode** aNodes, u16 nodeNum, aiString &nodeName)
	{
		for (uint i = 0; i != nodeNum; ++i)
		{
			if (aNodes[i]->mName == nodeName)
				return i;
		}
		return (u16)-1;
	}

	static bool constructSkeleton(
		const aiScene* aScene,
		AssetModel* model,
		aiNode** aNodes,
		DE_Allocator_NoLock* allocator)
	{
		auto &skel = model->skeleton;
		auto &nodeNum = skel->nodeNum;
		auto &nodes = skel->nodes;

		//mark bone nodes and set meshesBoneIndices
		NEW_R(u16*, meshesBoneIndices, allocator, aScene->mNumMeshes);
		for (uint meshIdx = 0; meshIdx != aScene->mNumMeshes; ++meshIdx)
		{
			auto aMesh = aScene->mMeshes[meshIdx];
			if (!aMesh->HasBones())
				continue;
			ALLOC_R(meshesBoneIndices[meshIdx], allocator, u16, aMesh->mNumBones);
			for (uint i = 0; i != aMesh->mNumBones; ++i)
			{
				u16 boneNodeIdx = findNodeByName(aNodes, nodeNum, aMesh->mBones[i]->mName);
				assert(boneNodeIdx <= nodeNum && (u16)-1 != boneNodeIdx);
				//store index of nodeArray temperarily
				meshesBoneIndices[meshIdx][i] = boneNodeIdx;
				nodes[boneNodeIdx].boneIdx = 0;//temperarily mark to 0
			}
		}

		//count bone number
		skel->boneNum = 0;
		for (u32 i = 0; i != nodeNum; ++i)
			if ((u16)-1 != nodes[i].boneIdx)
				nodes[i].boneIdx = skel->boneNum++;
		assert(skel->boneNum <= 256);
		ALLOC_AL(skel->boneOffsets, allocator, fmat4, skel->boneNum);

		for (uint meshIdx = 0; meshIdx != aScene->mNumMeshes; ++meshIdx)
		{
			if (!aScene->mMeshes[meshIdx]->HasBones())
				continue;

			/* for every mesh bone, copy bone offset */
			auto &meshBoneIndices = meshesBoneIndices[meshIdx];
			for (uint i = 0; i != aScene->mMeshes[meshIdx]->mNumBones; ++i)
			{
				auto &nodeIdx = meshBoneIndices[i];//stored the nodeArray index now
				auto &boneIdx = nodes[nodeIdx].boneIdx;
				assert((u16)-1 != boneIdx && boneIdx < skel->boneNum);
				/* boneOffset of same name bone must also the same */
				memcpy(&skel->boneOffsets[boneIdx],
					&aScene->mMeshes[meshIdx]->mBones[i]->mOffsetMatrix.a1,
					sizeof(fmat4));
				nodeIdx = boneIdx;
			}

			/* set vertex boneIndices for right values */
			for (uint i = 0; i != model->meshes[meshIdx].vertNum; ++i)
			{
				auto &vertBoneIdx = model->meshes[meshIdx].boneIndices[i];
				vertBoneIdx.X = meshBoneIndices[vertBoneIdx.X];
				vertBoneIdx.Y = meshBoneIndices[vertBoneIdx.Y];
				vertBoneIdx.Z = meshBoneIndices[vertBoneIdx.Z];
				vertBoneIdx.W = meshBoneIndices[vertBoneIdx.W];
			}
		}

		for (uint meshIdx = aScene->mNumMeshes; meshIdx != 0; --meshIdx)
			allocator->freeR(meshesBoneIndices[meshIdx - 1]);
		allocator->freeR(meshesBoneIndices);
		return true;
	}

	static bool allocIONodeAnimation(
		const aiNodeAnim* aNodeAnim,
		AssetModel::SkeletalAnimation::NodeAnim &nodeAnim,
		DE_Allocator_NoLock *allocator)
	{
		auto &posKeyNum = aNodeAnim->mNumPositionKeys;
		auto &rotKeyNum = aNodeAnim->mNumRotationKeys;
		auto &scaleKeyNum = aNodeAnim->mNumScalingKeys;

		ALLOC_AL(nodeAnim.positionKeyTimes, allocator, double, posKeyNum);
		ALLOC_L(nodeAnim.positionKeys, allocator, fvec3, posKeyNum);

		ALLOC_L(nodeAnim.rotationKeyTimes, allocator, double, rotKeyNum);
		ALLOC_L(nodeAnim.rotationKeys, allocator, fquat, rotKeyNum);

		ALLOC_L(nodeAnim.scaleKeyTimes, allocator, double, scaleKeyNum);
		ALLOC_L(nodeAnim.scaleKeys, allocator, fvec3, scaleKeyNum);

		nodeAnim.posKeyNum = posKeyNum;
		nodeAnim.rotKeyNum = rotKeyNum;
		nodeAnim.scaleKeyNum = scaleKeyNum;

		return true;
	}

	static void processNodeAnimKeys(
		const aiNodeAnim* aNodeAnim,
		AssetModel::SkeletalAnimation::NodeAnim &nodeAnim)
	{
		for (int k = 0; k != aNodeAnim->mNumPositionKeys; ++k)
		{
			auto &val = aNodeAnim->mPositionKeys[k].mValue;
			nodeAnim.positionKeys[k].X = val.x;
			nodeAnim.positionKeys[k].Y = val.y;
			nodeAnim.positionKeys[k].Z = val.z;
			nodeAnim.positionKeyTimes[k] = aNodeAnim->mPositionKeys[k].mTime;
		}
		for (int k = 0; k != aNodeAnim->mNumRotationKeys; ++k)
		{
			auto &val = aNodeAnim->mRotationKeys[k].mValue;
			nodeAnim.rotationKeys[k].W = val.w;
			nodeAnim.rotationKeys[k].X = val.x;
			nodeAnim.rotationKeys[k].Y = val.y;
			nodeAnim.rotationKeys[k].Z = val.z;
			nodeAnim.rotationKeyTimes[k] = aNodeAnim->mRotationKeys[k].mTime;
		}
		for (int k = 0; k != aNodeAnim->mNumScalingKeys; ++k)
		{
			auto &val = aNodeAnim->mScalingKeys[k].mValue;
			nodeAnim.scaleKeys[k].X = val.x;
			nodeAnim.scaleKeys[k].Y = val.y;
			nodeAnim.scaleKeys[k].Z = val.z;
			nodeAnim.scaleKeyTimes[k] = aNodeAnim->mScalingKeys[k].mTime;
		}
		nodeAnim.lastRotKey = 0;
		nodeAnim.lastScaleKey = 0;
		nodeAnim.lastTransKey = 0;
	}

	static bool constructAnimations(
		const aiScene* aScene,
		aiNode** aNodes,
		AssetModel* model,
		DE_Allocator_NoLock* allocator)
	{
		ALLOC_L(model->skelAnims, allocator, AssetModel::SkeletalAnimation, aScene->mNumAnimations);
		auto &anims = model->skelAnims;
		for (uint animIdx = 0; animIdx != aScene->mNumAnimations; ++animIdx)
		{
			auto aAnim = aScene->mAnimations[animIdx];
			ALLOC_AL(anims[animIdx].nodeAnims, allocator,
				AssetModel::SkeletalAnimation::NodeAnim,
				aScene->mAnimations[animIdx]->mNumChannels);
			for (uint nodeIdx = 0; nodeIdx != aAnim->mNumChannels; ++nodeIdx)
			{
				anims[animIdx].nodeAnims[nodeIdx].nodeIndex =
					findNodeByName(aNodes, model->skeleton->nodeNum,
						aAnim->mChannels[nodeIdx]->mNodeName);
				assert((u16)-1 != anims[animIdx].nodeAnims[nodeIdx].nodeIndex);
			}
		}
		for (u32 animIdx = 0; animIdx != aScene->mNumAnimations; ++animIdx)
		{
			auto aAnim = aScene->mAnimations[animIdx];
			anims[animIdx].ticksPerSecond =
				aAnim->mTicksPerSecond == 0 ? 25.0 : aAnim->mTicksPerSecond;
			anims[animIdx].duration = aAnim->mDuration;
			for (uint nodeIdx = 0; nodeIdx != aAnim->mNumChannels; ++nodeIdx)
			{
				auto aNodeAnim = aAnim->mChannels[nodeIdx];
				auto &nodeAnim = anims[animIdx].nodeAnims[nodeIdx];

				if (!allocIONodeAnimation(aNodeAnim, nodeAnim, allocator))
					return false;
				processNodeAnimKeys(aNodeAnim, nodeAnim);
			}
			anims[animIdx].nodeAnimNum = aAnim->mNumChannels;
		}
		model->animNum = aScene->mNumAnimations;
		return true;
	}

	static bool loadAssimpAnimations(
		const aiScene* aScene,
		AssetModel* model,
		DE_Allocator_NoLock* allocator)
	{
		u16 nodeNum = countChildrenNodes(aScene->mRootNode);
		ALLOC_L(model->skeleton, allocator, AssetModel::Skeleton, 1);
		ALLOC_L(model->skeleton->nodes, allocator, AssetModel::Skeleton::Node, nodeNum);
		NEW_R(aiNode*, aNodes, allocator, nodeNum);
		model->skeleton->nodeNum = nodeNum;

		u16 usedNodeNum = 1;//must 1 for the rootNode
		initNodeTree(aScene->mRootNode, aNodes, model->skeleton->nodes,
			0, -1, -1, usedNodeNum);
		assert(usedNodeNum == nodeNum);

		if (!constructSkeleton(aScene, model, aNodes, allocator))
			return false;
		if (!constructAnimations(aScene, aNodes, model, allocator))
			return false;

		allocator->freeR(aNodes);
		return true;
	}

	AssetModel* AssetImporter::loadModel(DE_Allocator_NoLock* allocator)
	{
		assert(m_aImporter);
		auto aScene = m_aImporter->GetScene();
		if (nullptr == aScene)
			return nullptr;

		m_model = AssetModel();

		if (aScene->HasMeshes())
		{
			ALLOC_R(m_model.meshes, allocator, AssetModel::Mesh, aScene->mNumMeshes);
			for (uint i = 0; i != aScene->mNumMeshes; ++i)
			{
				if (!loadMesh(aScene->mMeshes[i], &m_model.meshes[i], allocator))
					goto FAIL;
			}
			m_model.meshNum = aScene->mNumMeshes;
		}
		else
			goto FAIL;
		
		/* MUST be called after loadMesh() */
		if (aScene->HasAnimations())
		{
			if (!loadAssimpAnimations(aScene, &m_model, allocator))
				goto FAIL;
		}

		return &m_model;
	FAIL:
		//allocator->freeL(model);
		return nullptr;
	}


	uint addTexPath(
		uint &texNum, AssetPath*& paths,
		const char* newPath)
	{
		u64 hs = str::BKDRHash(newPath);
		for (uint i = 0; i != texNum; ++i)
		{
			if (paths[i].hash == hs && 0 == strcmp(paths[i].path, newPath))
				return i;
		}

		const char* str = newPath;
		char* path = paths[texNum].path;
		while (*path++ = *str++)
			;
		paths[texNum].strLen = path - paths[texNum].path;
		assert(path - paths[texNum].path < ARRAYLENGTH(paths[texNum].path));
		paths[texNum].hash = hs;

		return texNum++;
	}

	static inline uint checksDuplicateMaterial(
		uint matNum,
		AssetMaterial* materials,
		AssetMaterial &matForCheck)
	{
		for (uint i = 0; i != matNum; ++i)
		{
			if (matForCheck == materials[i])
				return i;
		}
		return matNum;
	}

	bool AssetImporter::getTexPathsAndMaterials(
		uint &pathNum, AssetPath* &paths,
		uint &matNum, AssetMaterial* &mats,
		u8* materialIndices,
		DE_Allocator_NoLock *allocator)
	{
		auto aScene = m_aImporter->GetScene();
		if (!aScene || !aScene->HasMaterials())
			return false;

		/* alloc space for paths and mats */
		ALLOC_R(mats, allocator, AssetMaterial, aScene->mNumMaterials);
		uint tempPathNum = 0;
		for (uint i = 0; i != aScene->mNumMaterials; ++i)
		{
			auto aMat = aScene->mMaterials[i];
			mats[i].texNum = 0;
			for (uint type = TEXTYPE_DIFFUSE; type != TEXTYPE_TYPEMAX; ++type)
				mats[i].texNum += aMat->GetTextureCount(aTexType[type]);
			assert(mats[i].texNum <= ARRAYLENGTH(mats[i].textures));
			tempPathNum += mats[i].texNum;
		}
		ALLOC_L(paths, allocator, AssetPath, tempPathNum);
		pathNum = matNum = 0;

		u8 matIndices[64];
		assert(aScene->mNumMaterials < ARRAYLENGTH(matIndices));
		aiString aStr;
		for (uint i = 0; i != aScene->mNumMaterials; ++i)
		{
			auto aMat = aScene->mMaterials[i];
			uint matTexNum = 0;
			for (uint type = TEXTYPE_DIFFUSE; type != TEXTYPE_TYPEMAX; ++type)
			{
				uint typeCount = aMat->GetTextureCount(aTexType[type]);
				for (uint j = 0; j != typeCount; ++j)
				{
					if (aiReturn_SUCCESS == aMat->GetTexture(aTexType[type], j, &aStr))
					{
						uint idx = addTexPath(pathNum, paths, aStr.C_Str());
						mats[matNum].textures[matTexNum].texIndex = idx;
						mats[matNum].textures[matTexNum].type = static_cast<TextureType>(type);
						++matTexNum;
					}
					else
						assert(0 && "aiMaterial get texture failed");
				}
			}
			assert(matTexNum == mats[i].texNum);

			uint matIdx = checksDuplicateMaterial(matNum, mats, mats[matNum]);
			if (matIdx == matNum) //unique material
				matIndices[i] = matNum++;
			else //duplicated material
				matIndices[i] = matIndices[matIdx];
		}

		if (materialIndices)
		{
			for (u8 i = 0; i != aScene->mNumMeshes; ++i)
				materialIndices[i] = matIndices[aScene->mMeshes[i]->mMaterialIndex];
		}
		return true;
	}

	void AssetImporter::closeModel3D()
	{
		m_aImporter->FreeScene();
	}

	static void initTreeNodeMatrices(
		const AssetModel::Skeleton* skel,
		const AssetModel::Skeleton::Node *root,
		u32 nodeIdx,
		fmat4* out)
	{
		out[nodeIdx] = (u16)-1 != root->parentIdx ?
			out[root->parentIdx] * root->nodeMatrix :
			root->nodeMatrix;

		/* recursively init children nodes' matrices */
		if ((u16)-1 != root->firstChildIdx)
		{
			auto *child = &skel->nodes[root->firstChildIdx];
			initTreeNodeMatrices(
				skel,
				child,
				root->firstChildIdx,
				out);
			while ((u16)-1 != child->nextBroIdx)
			{
				initTreeNodeMatrices(
					skel,
					&skel->nodes[child->nextBroIdx],
					child->nextBroIdx,
					out);
				child = &skel->nodes[child->nextBroIdx];
			}
		}
	}

	void updateMeshBoneMatrices(
		const AssetModel::Skeleton* skel,
		const AssetModel::Skeleton::Node *root,
		u32 nodeIdx,
		fmat4* nodeMatrices,
		fmat4* out)
	{
		auto &parentIdx = root->parentIdx;
		if ((u16)-1 != parentIdx && false == skel->nodes[parentIdx].isMeshNode)
			nodeMatrices[nodeIdx] = nodeMatrices[parentIdx] * nodeMatrices[nodeIdx];

		if ((u16)-1 != root->boneIdx)
			out[root->boneIdx] =
			nodeMatrices[nodeIdx] * skel->boneOffsets[root->boneIdx];

		if ((u16)-1 != root->firstChildIdx)
		{
			auto *child = &skel->nodes[root->firstChildIdx];
			updateMeshBoneMatrices(
				skel,
				child,
				root->firstChildIdx,
				nodeMatrices,
				out);
			while ((u16)-1 != child->nextBroIdx)
			{
				updateMeshBoneMatrices(
					skel,
					&skel->nodes[child->nextBroIdx],
					child->nextBroIdx,
					nodeMatrices,
					out);
				child = &skel->nodes[child->nextBroIdx];
			}
		}
	}

	void AssetImporter::calcSkelAnimMatrices(
		AssetModel::Skeleton* skel,
		AssetModel::SkeletalAnimation* anims,
		uint animIdx,
		bool repeat,
		double time,
		fmat4* nodeMatrices,
		fmat4* out)
	{
		//initTreeNodeMatrices(skel, &skel->nodes[0], 0, nodeMatrices);
		for (u16 i = 0; i != skel->nodeNum; ++i)
			nodeMatrices[i] = skel->nodes[i].nodeMatrix;

		time *= anims[animIdx].ticksPerSecond;

		if (anims[animIdx].duration > 0.0)
		{
			if (repeat)
				time = fmod(time, anims[animIdx].duration);
			else
				time = time > anims[animIdx].duration ? anims[animIdx].duration : time;
		}
		else
			time = 0;

		fquat rot;
		fvec3 scale;
		fvec3 trans;
		for (u32 i = 0; i != anims[animIdx].nodeAnimNum; ++i)
		{
			auto &nodeAnim = anims[animIdx].nodeAnims[i];
			nodeAnim.calcInterpolatedRotation(time, &rot);
			nodeAnim.calcInterpolatedScaling(time, &scale);
			nodeAnim.calcInterpolatedTranslation(time, &trans);

			sqtMat(scale, rot, trans, nodeMatrices[nodeAnim.nodeIndex]);
		}
		updateMeshBoneMatrices(
			skel,
			&skel->nodes[0],
			0,
			nodeMatrices,
			out);
	}

}