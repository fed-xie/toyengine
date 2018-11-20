#pragma once
#include "Types.h"
#include "Memory.h"
#include "IRenderDriver.h"
#include "AssetImporter.h"
#include <vector>

namespace toy
{
	struct ShaderSource
	{
		const char *name;
		rd::ShaderType type;
	};

	class SceneObjectProperty
	{
	public:
		enum SceneObjFlag
		{
			SOF_NONE = bit(0),
			SOF_VISIBLE = bit(1),
			SOF_COLLISION_OBJ = bit(2),
			//SOF_GROUP = bit(3),
			SOF_ANIMATED = bit(4),
			SOF_DEFAULT = SOF_VISIBLE,
		};
		SceneObjectProperty(SceneObjFlag flag = SOF_DEFAULT) : m_flag(flag) {}

		bool getProperty(SceneObjFlag prop) { return m_flag & prop; }
		void setProperty(SceneObjFlag prop) { m_flag |= prop; }
		void clearProperty(SceneObjFlag prop) { m_flag &= ~prop; }
		void reverseProperty(SceneObjFlag prop) { m_flag ^= prop; }

	private:
		u8 m_flag;
	};

	class Camera
	{
	public:
		Camera();

		const fmat4& getProjMatrix() const { return m_projMatrix; }
		void setProjectionMatrixPersp(float fovy, float aspect, float zNear, float zFar);
		void setProjectionMatrixOrtho(
			float left, float right, float bottom, float top, float zNear, float zFar);

		const fvec3& getPosition() const { return m_pos; }
		void setPosition(const fvec3& pos) { m_pos = pos; }

		fmat4 getViewMatrix() const;

		void setZoom(float zoom) { m_zoom = zoom; }
		float getZoom() const { return m_zoom; }

		fmat4 getPVMatrix() const;

		void moveRight(float distance);
		void moveUp(float distance);
		void moveForward(float distance);

		void rotateRight(float radian);
		void rotateUp(float radian);
		void rotateClockwise(float radian);
	private:
		fvec3 m_pos;
		fvec3 m_front;
		fvec3 m_up;
		fmat4 m_projMatrix;
		float m_zoom;
	};
	
	class ScenePrefab
	{
	public:
		AssetModel::Skeleton *skel;
		AssetModel::SkeletalAnimation *skelAnims;
		rd::RenderMesh *rMeshes;
		uint meshNum;
		IShaderProgram* prog;
	};

	struct ScenePrefabInstance
	{
		fmat4 mmat;//model matrix
		ScenePrefab* prefab;
		fmat4 *amats;//skeletal animation matrices
		SceneObjectProperty flag;
	};
	
	class Scene
	{
	public:
		struct SceneTex
		{
			u64 hash;
			rd::TBO tbo;
			char name[1024 - sizeof(u64) - sizeof(rd::TBO)];
		};

		Scene(const char* name,
			DE_Allocator_NoLock* allocator,
			IRenderDriver* rdr,
			AssetImporter* imp);
		
		void freeScene();

		DE_Allocator_NoLock::StackPtr getTopL() { return m_allocator->allocL(0); }

		ScenePrefabInstance* newPrefabInstances(
			ScenePrefab* prefab, uint instNum, SceneObjectProperty flag);

		SceneTex* loadTexture(const char* path, u64 hash = 0, uint slen = 0);
		SceneTex* loadTexture(AssetPath* path)
		{
			return loadTexture(path->path, path->hash, path->strLen);
		}
		//For Lua loading
		SceneTex* loadTextures(const char* const *filepath, uint texNum);
		void loadModelMaterials(ScenePrefab* prefab, const char* modelPath);
		ScenePrefab* loadModel(const char* filepath, bool loadMats = true);

		void sort();

		void update(Camera* cam, double time);
		void drawAll(Camera* cam);

		const char* getSceneName() { return m_name; }

	private:
		std::vector<ScenePrefab*> m_pPrefs;
		std::vector<ScenePrefabInstance*> m_objs;
		std::vector<SceneTex*> m_texs;
		DE_Allocator_NoLock *m_allocator;
		AssetImporter* m_imp;
		IRenderDriver *m_rdr;
		const char* m_name;
	};
	
	class SceneManager
	{
	public:
		void init(size_t spaceSize, IRenderDriver *rdr);
		void destroy();
		
		Scene* getSharedScene() { return m_sharedScene; }
		Scene* newScene(const char* name);
		Camera* newCamera();
		void freeScene(Scene* &scene);
		Scene* getSceneNowLoading() { return m_loadingScene; }
		void endLoadScene(Scene* scene);

		void update(double time);
		void drawAll();
		
		Scene* getCurrentScene() { return m_curScene; }
		void switchScene(Scene* scene);
	private:
		Scene* m_sharedScene;
		Scene* m_prevScene = nullptr;
		Scene* m_curScene = nullptr;
		Scene* m_loadingScene = nullptr;
		Scene* (m_scenes)[8] = { nullptr };
		std::vector<Camera> m_cams;
		DE_Allocator *m_allocator = nullptr;
		IRenderDriver *m_rdr = nullptr;
		AssetImporter m_imp;
	};
}