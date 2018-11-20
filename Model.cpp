#include "include/Model.h"

#include "libs/include/assimp/Importer.hpp"
#include "libs/include/assimp/scene.h"
#include "libs/include/assimp/postprocess.h"
#pragma comment(lib, "libs/Assimp/assimp-vc140-mt.lib")

namespace toy
{
#define ALLOC_L(ptrToStore, allocator, unitNum) \
	ptrToStore = (decltype(ptrToStore))allocator.allocL(sizeof(decltype(*(ptrToStore)))*(unitNum));\
	if(nullptr == ptrToStore)\
		return false;
#define ALLOC_ALIGNEDL4(ptrToStore, allocator, unitNum) \
	ptrToStore = (decltype(ptrToStore))allocator.allocAlignedL(sizeof(decltype(*(ptrToStore)))*(unitNum), 4);\
	if(nullptr == ptrToStore)\
		return false;
#define ALLOC_R(ptrToStore, allocator, unitNum) \
	ptrToStore = (decltype(ptrToStore))allocator.allocR(sizeof(decltype(*(ptrToStore)))*(unitNum));\
	if(nullptr == ptrToStore)\
		return false;

	namespace io
	{
		static bool constructIOMeshes(
			const aiScene* aScene,
			Model3D &model,
			DE_Allocator_NoLock &allocator)
		{
			/* Construct model */
			ALLOC_L(model.meshes, allocator, aScene->mNumMeshes);
			model.meshNum = aScene->mNumMeshes;
			model.meshes = ::new (model.meshes) Model3D::Mesh[aScene->mNumMeshes];

			for (uint i = 0; i != aScene->mNumMeshes; ++i)
			{
				const aiMesh* aMesh = aScene->mMeshes[i];
				auto &mesh = model.meshes[i];

				/* calc data size */
				mesh.posSize = aMesh->mNumVertices * sizeof(float) * 3;
				mesh.normalSize = mesh.posSize;
				mesh.coordSize = mesh.posSize * 8;
				if (aMesh->HasBones())
				{
					mesh.boneIndexSize = aMesh->mNumVertices * sizeof(u32) * 4;
					mesh.boneWeightSize = aMesh->mNumVertices * sizeof(float) * 4;
				}
				mesh.vertexDataSize = mesh.posSize + mesh.normalSize + mesh.coordSize + 
					mesh.boneIndexSize + mesh.boneWeightSize;

				/* alloc vertex data space */
				mesh.vertexData = allocator.allocAlignedL(mesh.vertexDataSize, 4);
				if (nullptr == mesh.vertexData)
					return false;

				if (aMesh->HasFaces())
				{
					mesh.indicesNum = aMesh->mNumFaces * 3;
					ALLOC_L(mesh.vertIndices, allocator, mesh.indicesNum);
				}

				/* calc the data offset of pointers */
				size_t ptrOffset = 0;
				if (aMesh->HasPositions())
				{
					mesh.positionPtr = reinterpret_cast<decltype(mesh.positionPtr)>(mesh.vertexData);
					ptrOffset += mesh.posSize;
				}
				if (aMesh->HasNormals())
				{
					mesh.normalPtr = reinterpret_cast<decltype(mesh.normalPtr)>((size_t)mesh.vertexData + ptrOffset);
					ptrOffset += mesh.normalSize;
				}
				mesh.coordPtr = reinterpret_cast<decltype(mesh.coordPtr)>((size_t)mesh.vertexData + ptrOffset);
				ptrOffset += mesh.coordSize;
				if (aMesh->HasBones())
				{
					mesh.boneIdx = reinterpret_cast<decltype(mesh.boneIdx)>((size_t)mesh.vertexData + ptrOffset);
					ptrOffset += aMesh->mNumVertices * sizeof(u32) * 4;
					mesh.boneWeight = reinterpret_cast<decltype(mesh.boneWeight)>(
						(size_t)mesh.vertexData + ptrOffset);
					//ptrOffset += mesh->mNumVertices * sizeof(float) * 4;
				}

				mesh.vertNum = aMesh->mNumVertices;
			}
			return true;
		}
		static void calcMeshAABBox(
			const aiMesh *aMesh,
			Model3D::Mesh &mesh)
		{
			mesh.aabboxMin[0] = aMesh->mVertices[0].x;
			mesh.aabboxMin[1] = aMesh->mVertices[0].y;
			mesh.aabboxMin[2] = aMesh->mVertices[0].z;
			mesh.aabboxMax[0] = aMesh->mVertices[0].x;
			mesh.aabboxMax[1] = aMesh->mVertices[0].y;
			mesh.aabboxMax[2] = aMesh->mVertices[0].z;
			for (uint i = 1; i != aMesh->mNumVertices; ++i)
			{
				if (mesh.aabboxMin[0] > aMesh->mVertices[i].x)
					mesh.aabboxMin[0] = aMesh->mVertices[i].x;
				if (mesh.aabboxMin[1] > aMesh->mVertices[i].y)
					mesh.aabboxMin[1] = aMesh->mVertices[i].y;
				if (mesh.aabboxMin[2] > aMesh->mVertices[i].z)
					mesh.aabboxMin[2] = aMesh->mVertices[i].z;
				if (mesh.aabboxMax[0] < aMesh->mVertices[i].x)
					mesh.aabboxMax[0] = aMesh->mVertices[i].x;
				if (mesh.aabboxMax[1] < aMesh->mVertices[i].y)
					mesh.aabboxMax[1] = aMesh->mVertices[i].y;
				if (mesh.aabboxMax[2] < aMesh->mVertices[i].z)
					mesh.aabboxMax[2] = aMesh->mVertices[i].z;
			}
		}
		/* process Assimp mesh data, may be processed by other functions */
		static void processAssimpMesh(
			const aiMesh *aMesh,
			Model3D::Mesh &mesh)
		{
			for (uint i = 0; i != mesh.vertNum; ++i)
			{
				mesh.positionPtr[i][0] = aMesh->mVertices[i].x;
				mesh.positionPtr[i][1] = aMesh->mVertices[i].y;
				mesh.positionPtr[i][2] = aMesh->mVertices[i].z;

				mesh.normalPtr[i][0] = aMesh->mNormals[i].x;
				mesh.normalPtr[i][1] = aMesh->mNormals[i].y;
				mesh.normalPtr[i][2] = aMesh->mNormals[i].z;
			}
			calcMeshAABBox(aMesh, mesh);

			memset(mesh.coordPtr, 0, mesh.coordSize);
			for (int i = 0; i != AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
			{
				if (aMesh->HasTextureCoords(i))
				{
					for (uint j = 0; j != mesh.vertNum; ++j)
					{
						mesh.coordPtr[j][i][0] = aMesh->mTextureCoords[i][j].x;
						mesh.coordPtr[j][i][1] = aMesh->mTextureCoords[i][j].y;
						mesh.coordPtr[j][i][2] = aMesh->mTextureCoords[i][j].z;
					}
				}
			}

			//mesh.materialIdx = aMesh->mMaterialIndex; //leave for other function to process

			for (uint i = 0; i != aMesh->mNumFaces; ++i)
			{
				//assert(3 == mesh->mFaces[i].mNumIndices);
				mesh.vertIndices[i * 3] = aMesh->mFaces[i].mIndices[0];
				mesh.vertIndices[i * 3 + 1] = aMesh->mFaces[i].mIndices[1];
				mesh.vertIndices[i * 3 + 2] = aMesh->mFaces[i].mIndices[2];
			}

			if (aMesh->HasBones())
			{
				memset(mesh.boneIdx, 0, mesh.boneIndexSize);
				memset(mesh.boneWeight, 0, mesh.boneWeightSize);
				for (uint boneIdx = 0; boneIdx != aMesh->mNumBones; ++boneIdx)
				{
					for (uint i = 0; i != aMesh->mBones[boneIdx]->mNumWeights; ++i)
					{
						uint &vertIdx = aMesh->mBones[boneIdx]->mWeights[i].mVertexId;
						float &boneWeight = aMesh->mBones[boneIdx]->mWeights[i].mWeight;
						for (uint j = 0; j != 4; ++j)
						{
							if (0.001f > mesh.boneWeight[vertIdx][j])
							{
								mesh.boneIdx[vertIdx][j] = boneIdx;
								mesh.boneWeight[vertIdx][j] = boneWeight;
								break;
							}
						}
					}
				}
			}
		}

		static inline uint checksDuplicatedPath(
			uint pathNum,
			aiString* paths,
			aiString &pathForCheck)
		{
			for (uint i = 0; i != pathNum; ++i)
			{
				if (pathForCheck == paths[i])
					return i;
			}
			return pathNum;
		}
		static inline uint checksDuplicateMaterial(
			uint matNum,
			Model3D::Material* materials,
			Model3D::Material &matForCheck)
		{
			for (uint i = 0; i != matNum; ++i)
			{
				if (matForCheck == materials[i])
					return i;
			}
			return matNum;
		}
		static bool getMaterialsPaths(
			const aiScene* aScene,
			Model3D &model,
			DE_Allocator_NoLock &allocator)
		{
			//assimp assert every mesh has only 1 material
			/* Make a temp space to filter duplicated textures and materials */
			Model3D::Material * tempMats;
			ALLOC_R(tempMats, allocator, aScene->mNumMaterials);

			uint tempTexNum = 0;
			for (int i = 0; i != aScene->mNumMaterials; ++i)
			{
				aiMaterial* aMat = aScene->mMaterials[i];
				tempMats[i].texNum = 0;
				for (int texType = aiTextureType_DIFFUSE; texType != aiTextureType_UNKNOWN; ++texType)
					tempMats[i].texNum += aMat->GetTextureCount(static_cast<aiTextureType>(texType));
				ALLOC_R(tempMats[i].textures, allocator, tempMats[i].texNum);
				tempTexNum += tempMats[i].texNum;
			}

			aiString *tempPaths;
			ALLOC_R(tempPaths, allocator, tempTexNum);
			uint *meshMatIndex;
			ALLOC_R(meshMatIndex, allocator, aScene->mNumMaterials);

			uint usedMatNum = 0, usedTexNum = 0;
			for (int i = 0; i != aScene->mNumMaterials; ++i)
			{
				aiMaterial* mat = aScene->mMaterials[i];
				int matTexIdx = 0;
				for (int texType = aiTextureType_DIFFUSE; texType != aiTextureType_UNKNOWN; ++texType)
				{
					aiTextureType aitype = static_cast<aiTextureType>(texType);
					uint typeCount = mat->GetTextureCount(aitype);
					for (uint j = 0; j != typeCount; ++j)
					{
						if (aiReturn_SUCCESS == mat->GetTexture(aitype, j, &tempPaths[usedTexNum]))
						{
							uint idx = checksDuplicatedPath(usedTexNum, tempPaths, tempPaths[usedTexNum]);
							if (idx == usedTexNum)
								++usedTexNum;
							tempMats[i].textures[matTexIdx].texIndex = idx;
							tempMats[i].textures[matTexIdx].type = static_cast<Model3D::Material::TextureType>(texType);
							++matTexIdx;
						}
						else
							assert(0 && "aiMaterial get texture failed");
					}
				}
				uint matIdx = checksDuplicateMaterial(i, tempMats, tempMats[i]);
				if (matIdx == i) //unique material
					meshMatIndex[i] = usedMatNum++;
				else //duplicated material
					meshMatIndex[i] = meshMatIndex[matIdx];
			}

			/* copy textures and materials */
			ALLOC_L(model.texPaths, allocator, usedTexNum);
			for (uint i = 0; i != usedTexNum; ++i)
			{
				auto &texPath = model.texPaths[i];
				auto &tempPath = tempPaths[i];

				ALLOC_L(texPath.path, allocator, tempPath.length + 1);
				str::strnCpy(texPath.path, tempPath.length + 1, tempPath.C_Str(), tempPath.length + 1);
				texPath.path[tempPath.length] = L'\0';

				texPath.strLen = tempPath.length;

				for (uint j = 0; j != texPath.strLen; ++j)
				{
#ifdef _WIN32
					if (texPath.path[j] == '/')
						texPath.path[j] = '\\';
#else //__linux__
					if (texPath.path[j] == '\\')
						texPath.path[j] = '/';
#endif
				}
			}
			model.texNum = usedTexNum;

			ALLOC_L(model.materials, allocator, usedMatNum);
			uint ioMatIdx = 0;
			for (uint i = 0; i != aScene->mNumMaterials; ++i)
			{
				if (meshMatIndex[i] == ioMatIdx) //unique material
				{
					ALLOC_L(model.materials[ioMatIdx].textures, allocator, tempMats[i].texNum);
					for (uint j = 0; j != tempMats[i].texNum; ++j)
						model.materials[ioMatIdx].textures[j] = tempMats[i].textures[j];
					model.materials[ioMatIdx].texNum = tempMats[i].texNum;

					++ioMatIdx;
				}
			}
			model.materialNum = usedMatNum;

			for (uint i = 0; i != aScene->mNumMeshes; ++i)
				model.meshes[i].materialIdx = meshMatIndex[aScene->mMeshes[i]->mMaterialIndex];

			allocator.clearR();
			return true;
		}

		struct ArrayNode
		{
			aiNode* node;
			u32 parentIdx; //0xFFFFFFFF if not exist
			u32 nextBroIdx; //0xFFFFFFFF if not exist
			u32 firstChildIdx; //0xFFFFFFFF if not exist
			u32 boneIdx; //0xFFFFFFFF if not bone node
			bool isMeshNode;
		};

		/* count node's children, node self included */
		static u32 countChildrenNodes(const aiNode* node)
		{
			u32 count = 1;
			for (int i = 0; i != node->mNumChildren; ++i)
				count += countChildrenNodes(node->mChildren[i]);
			return count;
		}

		static void initNodeTree(
			aiNode* rootNode,
			ArrayNode* nodeArray,
			u32 nodeIdx,
			u32 parentIdx,
			u32 nextBroIdx,
			u32 &usedNum)
		{
			u32 firstChildIdx = usedNum;
			nodeArray[nodeIdx].node = rootNode;
			nodeArray[nodeIdx].parentIdx = parentIdx;
			nodeArray[nodeIdx].nextBroIdx = nextBroIdx;
			nodeArray[nodeIdx].boneIdx = 0xFFFFFFFF;
			nodeArray[nodeIdx].isMeshNode = !!rootNode->mNumMeshes;

			usedNum += rootNode->mNumChildren;
			u32 i = 0;
			for (; i != rootNode->mNumChildren; ++i)
			{
				initNodeTree(rootNode->mChildren[i],
					nodeArray,
					firstChildIdx + i,
					nodeIdx,
					firstChildIdx + i + 1,
					usedNum);
			}

			if (rootNode->mNumChildren)
			{
				nodeArray[nodeIdx].firstChildIdx = firstChildIdx;
				//mark last children, i == node->mNumChildren (i >= 1)
				nodeArray[firstChildIdx + i - 1].nextBroIdx = 0xFFFFFFFF;
			}
			else
				nodeArray[nodeIdx].firstChildIdx = 0xFFFFFFFF;
		}

		static inline u32 findNodeByName(ArrayNode* nodeArray, u32 nodeNum, aiString &nodeName)
		{
			for (uint i = 0; i != nodeNum; ++i)
			{
				if (nodeArray[i].node->mName == nodeName)
					return i;
			}
			return 0xFFFFFFFF;
		}

		static bool constructSkeleton(
			const aiScene* aScene,
			Model3D &model,
			ArrayNode* nodeArray,
			u32 arrayNodeNum,
			DE_Allocator_NoLock &allocator)
		{
			auto &skel = model.skeleton;

			u32 **meshBoneIdx;
			ALLOC_R(meshBoneIdx, allocator, aScene->mNumMeshes);

			for (uint meshIdx = 0; meshIdx != aScene->mNumMeshes; ++meshIdx)
			{
				aiMesh* aMesh = aScene->mMeshes[meshIdx];
				if (aMesh->HasBones())
				{
					/* alloc mesh bone indices */
					meshBoneIdx[meshIdx] = (uint*)allocator.allocR(sizeof(uint)*aMesh->mNumBones);
					if (nullptr == meshBoneIdx[meshIdx])
						return false;
					for (uint i = 0; i != aMesh->mNumBones; ++i)
					{
						u32 boneNodeIdx = findNodeByName(nodeArray, arrayNodeNum, aMesh->mBones[i]->mName);
						assert(0xFFFFFFFF != boneNodeIdx);
						//store index of nodeArray temperarily
						meshBoneIdx[meshIdx][i] = boneNodeIdx;
						nodeArray[boneNodeIdx].boneIdx = 0;//temperarily mark to 0
					}
				}
			}

			skel.boneNum = 0;
			for (u32 i = 0; i != arrayNodeNum; ++i)
				if (0xFFFFFFFF != nodeArray[i].boneIdx)
					nodeArray[i].boneIdx = skel.boneNum++;
			assert(skel.boneNum <= 256);

			ALLOC_ALIGNEDL4(skel.nodes, allocator, arrayNodeNum);
			//skel.nodes = ::new (skel.nodes) Model3D::Skeleton::SkeletalNode[arrayNodeNum];
			skel.nodeNum = arrayNodeNum;

			using fMat4 = float[16];
			fMat4* skelBoneOffsets;
			ALLOC_ALIGNEDL4(skelBoneOffsets, allocator, skel.boneNum);
			for (u32 i = 0; i != arrayNodeNum; ++i)
			{
				memcpy(skel.nodes[i].nodeMatrix,
					&nodeArray[i].node->mTransformation.a1, sizeof(float) * 16);
				skel.nodes[i].boneIdx = nodeArray[i].boneIdx;
				skel.nodes[i].parentIdx = nodeArray[i].parentIdx;
				skel.nodes[i].firstChildIdx = nodeArray[i].firstChildIdx;
				skel.nodes[i].nextBroIdx = nodeArray[i].nextBroIdx;
				if (nodeArray[i].node->mName.length + 1 < ARRAYLENGTH(skel.nodes[i].name))
					memcpy(skel.nodes[i].name, nodeArray[i].node->mName.C_Str(),
						nodeArray[i].node->mName.length + 1);
				else
					skel.nodes[i].name[0] = '\0';
				skel.nodes[i].isMeshNode = nodeArray[i].isMeshNode;
			}
			skel.boneOffsets = reinterpret_cast<float*>(skelBoneOffsets);

			for (uint meshIdx = 0; meshIdx != aScene->mNumMeshes; ++meshIdx)
			{
				if (aScene->mMeshes[meshIdx]->HasBones())
				{
					for (uint i = 0; i != aScene->mMeshes[meshIdx]->mNumBones; ++i)
					{
						auto &nodeIdx = meshBoneIdx[meshIdx][i];//stored the nodeArray index now
						auto &boneIdx = nodeArray[nodeIdx].boneIdx;
						assert(0xFFFFFFFF != boneIdx && boneIdx < skel.boneNum);
						/* boneOffset of same name bone must also the same */
						memcpy(&skel.boneOffsets[boneIdx * 16],
							&aScene->mMeshes[meshIdx]->mBones[i]->mOffsetMatrix.a1,
							sizeof(fMat4));
						nodeIdx = boneIdx;
					}
					for (uint i = 0; i != model.meshes[meshIdx].vertNum; ++i)
					{
						auto &vertBoneIdx = model.meshes[meshIdx].boneIdx[i];
						vertBoneIdx[0] = meshBoneIdx[meshIdx][vertBoneIdx[0]];
						vertBoneIdx[1] = meshBoneIdx[meshIdx][vertBoneIdx[1]];
						vertBoneIdx[2] = meshBoneIdx[meshIdx][vertBoneIdx[2]];
						vertBoneIdx[3] = meshBoneIdx[meshIdx][vertBoneIdx[3]];
					}
				}
			}

			allocator.clearR();
			return true;
		}

		static bool allocIONodeAnimation(
			const aiNodeAnim* aNodeAnim,
			Model3D::SkeletalAnimation::NodeAnim &nodeAnim,
			DE_Allocator_NoLock &allocator)
		{
			size_t posKeySize = aNodeAnim->mNumPositionKeys * sizeof(float) * 3;
			size_t posKeyTimeSize = aNodeAnim->mNumPositionKeys * sizeof(double);
			size_t rotKeySize = aNodeAnim->mNumRotationKeys * sizeof(float) * 4;
			size_t rotKeyTimeSize = aNodeAnim->mNumRotationKeys * sizeof(double);
			size_t scaleKeySize = aNodeAnim->mNumScalingKeys * sizeof(float) * 3;
			size_t scaleKeyTimeSize = aNodeAnim->mNumScalingKeys * sizeof(double);
			size_t nodeAnimDataSize =
				posKeySize + posKeyTimeSize +
				rotKeySize + rotKeyTimeSize +
				scaleKeySize + scaleKeyTimeSize;

			size_t nodeAnimData = (size_t)allocator.allocAlignedL(nodeAnimDataSize, 4);
			if (nullptr == (void*)nodeAnimData)
				return false;

			size_t offset = 0;
			nodeAnim.positionKeyTimes = (double*)(nodeAnimData + offset);
			offset += posKeyTimeSize;
			nodeAnim.positionKeys = (float*)(nodeAnimData + offset);
			offset += posKeySize;

			nodeAnim.rotationKeyTimes = (double*)(nodeAnimData + offset);
			offset += rotKeyTimeSize;
			nodeAnim.rotationKeys = (float*)(nodeAnimData + offset);
			offset += rotKeySize;

			nodeAnim.scaleKeyTimes = (double*)(nodeAnimData + offset);
			offset += scaleKeyTimeSize;
			nodeAnim.scaleKeys = (float*)(nodeAnimData + offset);
			//offset += scaleKeySize;

			nodeAnim.posKeyNum = aNodeAnim->mNumPositionKeys;
			nodeAnim.rotKeyNum = aNodeAnim->mNumRotationKeys;
			nodeAnim.scaleKeyNum = aNodeAnim->mNumScalingKeys;

			nodeAnim.dataSize = nodeAnimDataSize;
			nodeAnim.keyData = (void*)nodeAnimData;
			return true;
		}

		static void processNodeAnimKeys(
			const aiNodeAnim* aNodeAnim,
			Model3D::SkeletalAnimation::NodeAnim &nodeAnim,
			ArrayNode* nodeArray)
		{
			for (int k = 0; k != aNodeAnim->mNumPositionKeys; ++k)
			{
				auto &val = aNodeAnim->mPositionKeys[k].mValue;
				nodeAnim.positionKeys[3 * k + 0] = val.x;
				nodeAnim.positionKeys[3 * k + 1] = val.y;
				nodeAnim.positionKeys[3 * k + 2] = val.z;
				nodeAnim.positionKeyTimes[k] = aNodeAnim->mPositionKeys[k].mTime;
			}
			for (int k = 0; k != aNodeAnim->mNumRotationKeys; ++k)
			{
				auto &val = aNodeAnim->mRotationKeys[k].mValue;
				nodeAnim.rotationKeys[4 * k + 0] = val.w;
				nodeAnim.rotationKeys[4 * k + 1] = val.x;
				nodeAnim.rotationKeys[4 * k + 2] = val.y;
				nodeAnim.rotationKeys[4 * k + 3] = val.z;
				nodeAnim.rotationKeyTimes[k] = aNodeAnim->mRotationKeys[k].mTime;
			}
			for (int k = 0; k != aNodeAnim->mNumScalingKeys; ++k)
			{
				auto &val = aNodeAnim->mScalingKeys[k].mValue;
				nodeAnim.scaleKeys[3 * k + 0] = val.x;
				nodeAnim.scaleKeys[3 * k + 1] = val.y;
				nodeAnim.scaleKeys[3 * k + 2] = val.z;
				nodeAnim.scaleKeyTimes[k] = aNodeAnim->mScalingKeys[k].mTime;
			}
			//nodeAnim.nodeIndex = nodeArray[nodeAnim.nodeIndex].skeletonIdx;
		}

		static bool constructAnimations(
			const aiScene* aScene,
			Model3D &model,
			ArrayNode* nodeArray,
			u32 arrayNodeNum,
			DE_Allocator_NoLock &allocator)
		{
			ALLOC_L(model.skelAnims, allocator, aScene->mNumAnimations);
			for (uint i = 0; i != aScene->mNumAnimations; ++i)
			{
				aiAnimation* aAnim = aScene->mAnimations[i];
				ALLOC_ALIGNEDL4(model.skelAnims[i].nodeAnims, allocator, aScene->mAnimations[i]->mNumChannels);
				for (uint j = 0; j != aAnim->mNumChannels; ++j)
				{
					model.skelAnims[i].nodeAnims[j].nodeIndex = //store index of nodeArray
						findNodeByName(nodeArray, arrayNodeNum, aAnim->mChannels[j]->mNodeName);
					assert(0xFFFFFFFF != model.skelAnims[i].nodeAnims[j].nodeIndex);
				}

			}
			for (u32 i = 0; i != aScene->mNumAnimations; ++i)
			{
				aiAnimation* aAnim = aScene->mAnimations[i];
				model.skelAnims[i].ticksPerSecond =
					aAnim->mTicksPerSecond == 0 ? 25.0 : aAnim->mTicksPerSecond;
				model.skelAnims[i].duration = aAnim->mDuration;
				for (u32 j = 0; j != aAnim->mNumChannels; ++j)
				{
					aiNodeAnim* aNodeAnim = aAnim->mChannels[j];
					auto &nodeAnim = model.skelAnims[i].nodeAnims[j];

					if (!allocIONodeAnimation(aNodeAnim, nodeAnim, allocator))
						return false;
					processNodeAnimKeys(aNodeAnim, nodeAnim, nodeArray);
				}
				model.skelAnims[i].nodeAnimNum = aAnim->mNumChannels;
			}
			model.animNum = aScene->mNumAnimations;
			return true;
		}

		static bool loadAssimpAnimation(
			const aiScene* aScene,
			Model3D &model,
			DE_Allocator_NoLock &allocator)
		{
			u32 arrayNodeNum = countChildrenNodes(aScene->mRootNode);
			ArrayNode* nodeArray = (ArrayNode*)allocator.allocAlignedR(sizeof(ArrayNode)*arrayNodeNum, 4);
			if (nullptr == nodeArray)
				return false;

			u32 usedArrayNodeNum = 1;//must 1 for the rootNode
			initNodeTree(aScene->mRootNode, nodeArray, 0, 0xFFFFFFFF, 0xFFFFFFFF, usedArrayNodeNum);
			assert(usedArrayNodeNum == arrayNodeNum);

			if (!constructSkeleton(aScene, model, nodeArray, arrayNodeNum, allocator))
				return false;
			if (!constructAnimations(aScene, model, nodeArray, arrayNodeNum, allocator))
				return false;
			return true;
		}

		static bool processAssimpScene(
			const aiScene* aScene,
			Model3D &model,
			DE_Allocator_NoLock &allocator)
		{
			if (aScene->HasMeshes() && constructIOMeshes(aScene, model, allocator))
			{
				for (int i = 0; i != aScene->mNumMeshes; ++i)
				{
					processAssimpMesh(aScene->mMeshes[i], model.meshes[i]);
				}
			}
			else
				return false;
			if (aScene->HasMaterials())
			{
				if (!getMaterialsPaths(aScene, model, allocator))
					return false;
			}
			/* MUST be called after processAssimpMesh() */
			if (aScene->HasAnimations())
			{
				if (!loadAssimpAnimation(aScene, model, allocator))
					return false;
			}
			return true;
		}

		bool Model3D::loadFromMemory(
			void* srcData, size_t srcSize, const char* typeHint,
			DE_Allocator_NoLock &allocator)
		{
			Assimp::Importer aiImporter;
			auto aScene = aiImporter.ReadFileFromMemory(
				srcData,
				srcSize,
				aiProcess_Triangulate | aiProcess_LimitBoneWeights |
				aiProcess_ImproveCacheLocality | aiProcess_FlipUVs,
				typeHint);
			if (!aScene ||
				aScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE ||
				!aScene->mRootNode)
			{
				LOG_EXT_WARNING("%s\n", aiImporter.GetErrorString());
				return false;
			}

			return processAssimpScene(aScene, *this, allocator);
		}
	}
}