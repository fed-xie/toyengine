#include "include/SceneManager.h"
#include "include/Mathlib.h"
#include "GLSLShaderConfig.hpp"

namespace toy
{
	Camera::Camera() :
		m_pos(0.0f, 0.0f, 5.0f),
		m_front(0.0f, 0.0f, -1.0f),
		m_up(0.0f, 1.0f, 0.0f),
		m_zoom(1.0f)
	{
		perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f, m_projMatrix);
	}
	void Camera::setProjectionMatrixPersp(float fovy, float aspect, float zNear, float zFar)
	{
		perspective(fovy, aspect, zNear, zFar, m_projMatrix);
	}
	void Camera::setProjectionMatrixOrtho(
		float left, float right, float bottom, float top, float zNear, float zFar)
	{
		ortho(left, right, bottom, top, zNear, zFar, m_projMatrix);
	}
	fmat4 Camera::getViewMatrix() const
	{
		fmat4 ret;
		view(m_pos, m_pos + m_front, m_up, fvec3(m_zoom), ret);
		return ret;
	}
	fmat4 Camera::getPVMatrix() const { return getProjMatrix() * getViewMatrix(); }
	void Camera::moveRight(float distance)
	{
		m_pos += cross(m_front, m_up) * distance;
	}
	void Camera::moveUp(float distance) { m_pos += m_up * distance; }
	void Camera::moveForward(float distance) { m_pos += m_front * distance; }
	void Camera::rotateRight(float radian)
	{
		fquat q(-radian, m_up);
		m_front = q.rotate(m_front);
	}
	void Camera::rotateUp(float radian)
	{
		fquat q(radian, cross(m_front, m_up));
		m_up = q.rotate(m_up);
		m_front = q.rotate(m_front);
	}
	void Camera::rotateClockwise(float radian)
	{
		fquat q(radian, m_front);
		m_up = q.rotate(m_up);
	}

	Scene::Scene(const char* name,
		DE_Allocator_NoLock* allocator,
		IRenderDriver* rdr,
		AssetImporter* imp)
		: m_allocator(allocator), m_rdr(rdr), m_name(name), m_imp(imp)
	{
	}

	void Scene::freeScene()
	{
		for (auto& model : m_pPrefs)
		{
			for(uint i=0;i!=model->meshNum;++i)
				m_rdr->freeRenderMesh(model->rMeshes[i]);
		}
		for (auto& tex : m_texs)
		{
			m_rdr->freeTextures(&tex->tbo, 1);
		}
		m_allocator->clear();
	}

	ScenePrefabInstance* Scene::newPrefabInstances(
		ScenePrefab* prefab, uint instNum, SceneObjectProperty flag)
	{
		auto ins = m_allocator->allocAlignedL<ScenePrefabInstance>(1);
		ins->prefab = prefab;
		ins->flag = flag;
		ins->mmat = fmat4(1.0f);
		if (flag.getProperty(SceneObjectProperty::SOF_ANIMATED))
		{
			ins->amats = m_allocator->allocAlignedL<fmat4>(prefab->skel->boneNum);
		}
		else
		{
			ins->amats = nullptr;
		}
		m_objs.push_back(ins);
		return ins;
	}

	Scene::SceneTex* Scene::loadTexture(const char* path, u64 hash, uint slen)
	{
		hash = hash ? hash : str::BKDRHash(path);
		slen = slen ? slen : strlen(path);

		for (auto& stex : m_texs)
		{
			if (stex->hash == hash)
			{
				assert(0 == strcmp(path, stex->name));
				return stex;
			}
		}
		auto stex = m_allocator->allocL<SceneTex>(1);
		if (nullptr == stex)
			return nullptr;

		size_t fsize;
		io::WPath wp(path);
		auto suffix = io::Path::getSuffix(path);
		auto fdata = io::readFile(wp, &fsize);
		if (nullptr == fdata)
			return nullptr;

		stex->tbo = m_imp->loadImage2D(fdata, fsize, suffix);
		io::freeFile(fdata);
		stex->hash = hash;
		str::strnCpy(stex->name, sizeof(stex->name),
			path, slen);
		stex->name[slen] = '\0';

		m_texs.push_back(stex);
		LOGINFO("load tex %s\n", path);

		return stex;
	}

	Scene::SceneTex* Scene::loadTextures(const char* const *filepath, uint texNum)
	{
		auto stexs = m_allocator->allocL<SceneTex>(texNum);
		assert(stexs);

		for (uint i = 0; i != texNum; ++i)
		{
			size_t fsize;
			io::WPath wp(filepath[i]);
			auto suffix = io::Path::getSuffix(filepath[i]);
			auto fdata = io::readFile(wp, &fsize);
			if (nullptr == fdata)
			{
				m_allocator->freeL(stexs);
				return nullptr;
			}
			stexs[i].tbo = m_imp->loadImage2D(fdata, fsize, suffix);
			io::freeFile(fdata);
			stexs[i].hash = str::BKDRHash(filepath[i]);

			const char* path = filepath[i];
			char* name = stexs[i].name;
			while (*name++ = *path++)
				;
			assert(name - stexs[i].name <= sizeof(stexs[i].name));
		}

		for (uint i = 0; i != texNum; ++i)
			m_texs.push_back(&stexs[i]);

		return stexs;
	}

	void Scene::loadModelMaterials(ScenePrefab* prefab, const char* modelPath)
	{
		size_t bufSize = 1024 * 1024;//1M
		auto buffer = m_allocator->allocR(bufSize);
		if (nullptr == buffer)
			return;
		DE_Allocator_NoLock mem(buffer, bufSize);

		uint pathNum, matNum;
		AssetPath* paths;
		AssetMaterial* mats;
		u8* matIndices = mem.allocR<u8>(prefab->meshNum);
		auto success = m_imp->getTexPathsAndMaterials(
			pathNum, paths, matNum, mats, matIndices, &mem);
		if (!success)
			return;

		io::Path path(modelPath);
		auto texs = mem.allocL<SceneTex*>(pathNum);
		assert(texs);
		for (int i = 0; i != pathNum; ++i)
		{
			path.setFileName(paths[i].path);
			texs[i] = loadTexture(path.c_str(), paths[i].hash, paths[i].strLen);
		}

		auto sMats = m_allocator->allocL<rd::Material>(matNum);
		sMats = ::new (sMats) rd::Material[matNum];

		for (int i = 0; i != matNum; ++i)
		{
			for (int j = 0; j != mats[i].texNum; ++j)
				sMats[i].tbos[j] = texs[mats[i].textures[j].texIndex]->tbo;
		}

		for (uint i = 0; i != prefab->meshNum; ++i)
			prefab->rMeshes[i].mat = &sMats[matIndices[i]];

		m_allocator->freeR(buffer);
		return;
	}

	ScenePrefab* Scene::loadModel(const char* filepath, bool loadMats)
	{
		assert(filepath && m_imp);
		auto suffix = io::Path::getSuffix(filepath);
		io::WPath wp(filepath);
		size_t fsize, msize;
		auto fdata = io::readFile(wp, &fsize);
		if (nullptr == fdata)
			return nullptr;
		auto meshNum = m_imp->openModel3D(fdata, fsize, suffix, &msize);
		io::freeFile(fdata);

		auto rMeshes = m_allocator->allocL<rd::RenderMesh>(meshNum);
		assert(rMeshes);
		rMeshes = ::new (rMeshes) rd::RenderMesh[meshNum];

		void* buffer = m_allocator->allocL(msize);
		DE_Allocator_NoLock mem(buffer, msize);
		auto model = m_imp->loadModel(&mem);
		assert(model);

		for (toy::u32 i = 0; i != model->meshNum; ++i)
		{
			auto &mesh = model->meshes[i];
			m_rdr->loadRenderMesh(
				mesh.vertNum,
				mesh.pos, mesh.normal, mesh.coord,
				mesh.boneIndices, mesh.boneWeights,
				mesh.triNum, mesh.triIndices,
				rMeshes[i]);
		}

		auto lBottom = mem.allocL(0);
		m_allocator->freeL(lBottom);

		auto spref = m_allocator->allocAlignedL<ScenePrefab>(1);
		spref->skel = model->skeleton;
		spref->skelAnims = model->skelAnims;
		spref->rMeshes = rMeshes;
		spref->meshNum = meshNum;
		spref->prog = nullptr;

		m_pPrefs.push_back(spref);

		if (loadMats)
			loadModelMaterials(spref, filepath);
		
		return spref;
	}

	void sortMeshByMaterial(rd::RenderMesh* meshes, uint num)
	{
		//Todo: use a quicker algorithm
		auto cur = meshes;
		auto const end = meshes + num;
		while (end != cur)
		{
			auto meshToCmp = cur;
			while (end != ++meshToCmp)
			{
				if (meshToCmp->mat == cur->mat)
				{
					++cur;
					if (meshToCmp != cur)
					{
						auto temp = *meshToCmp;
						*meshToCmp = *cur;
						*cur = temp;
					}
				}
			}
			++cur;
		}
	}

	void Scene::sort()
	{
		//std::qsort(m_pPrefs.data(), m_pPrefs.size(), sizeof(ScenePrefab*),
		//	[](const void* a, const void* b)
		//	{
		//		auto pa = reinterpret_cast<const ScenePrefab*>(a);
		//		auto pb = reinterpret_cast<const ScenePrefab*>(b);
		//		if (pa->prog < pb->prog) return -1;
		//		else if (pa->prog > pb->prog) return 1;
		//		else return 0;
		//	});

		for (auto& pref : m_pPrefs)
		{
			sortMeshByMaterial(pref->rMeshes, pref->meshNum);
		}
	}

	static fmat4 nodemats[512];
	struct
	{
		fmat4 mmat;
		fmat4 *bonemats;
		uint bonenum;
	}data[2];
	struct
	{
		fmat4 pvmat;
		fmat4 pmat;
		fmat4 vmat;
	}camm;
	void Scene::update(Camera* cam, double time)
	{
		assert(cam);
		camm.pmat = cam->getProjMatrix();
		camm.vmat = cam->getViewMatrix();
		camm.pvmat = camm.pmat * camm.vmat;

		setCameraMatrices(m_rdr->getShaderProgram(0), &camm);

		for (uint i = 0; i != m_objs.size(); ++i)
		{
			auto &obj = m_objs[i];
			if (obj->flag.getProperty(SceneObjectProperty::SOF_ANIMATED))
			{
				auto &pref = obj->prefab;
				m_imp->calcSkelAnimMatrices(
					pref->skel,
					pref->skelAnims,
					0,
					true,
					time / 1000.0,
					nodemats,
					obj->amats);
				//transpose(obj->amats, pref->skel->boneNum);

				data[i].bonemats = obj->amats;
				data[i].bonenum = obj->prefab->skel->boneNum;
			}
			else
			{
				data[i].bonemats = nullptr;
				data[i].bonenum = 0;
			}

			data[i].mmat = obj->mmat;
		}
	}

	void Scene::drawAll(Camera* cam)
	{
		//Todo
		assert(cam);
		auto pvmat = cam->getPVMatrix();
		IShaderProgram* curProg = nullptr;
		for (uint i = 0; i != m_objs.size(); ++i)
		{
			auto& pref = m_objs[i]->prefab;
			if (curProg != pref->prog)
			{
				curProg = pref->prog;
				assert(curProg);
				curProg->use();
			}
			assert(curProg);

			curProg->preDrawFunc(curProg, &data[i]);

			for (uint i = 0; i != pref->meshNum; ++i)
				m_rdr->drawRenderMesh(pref->rMeshes[i]);
		}
	}

	void SceneManager::init(size_t spaceSize, IRenderDriver* rdr)
	{
		delete m_allocator;
		m_allocator = new DE_Allocator(spaceSize, "SceneManagerAllocator");
		m_rdr = rdr;
		m_imp.init(rdr);
		loadDefaultShaders(rdr);
	}

	void SceneManager::destroy()
	{
		//m_sharedScene.freeScene();

		/* For all render data in graphic memory will be
		recycled by os, no need to do extra process */
		for (auto& scene : m_scenes)
		{
			delete scene;
			scene = nullptr;
		}
		delete m_loadingScene;
		delete m_prevScene;
		delete m_curScene;
		delete m_sharedScene;

		delete m_allocator;

		m_imp.destroy();
		m_rdr = nullptr;
	}

	Scene* SceneManager::newScene(const char* name)
	{
		assert(nullptr == m_loadingScene);

		auto strSize = strlen(name) + 1;
		auto sceneName = m_allocator->allocL<char>(strSize);
		str::strnCpy(sceneName, strSize, name, strSize - 1);
		sceneName[strSize - 1] = '\0';

		auto allocator = m_allocator->allocL<DE_Allocator_NoLock>(1);
		auto spaceSize = m_allocator->getAvailSize();
		auto space = m_allocator->allocL(spaceSize);
		allocator = ::new (allocator) 
			DE_Allocator_NoLock(space, spaceSize, "SceneAllocator");

		auto scene = new Scene(sceneName, allocator, m_rdr, &m_imp);

		m_loadingScene = scene;
		return scene;
	}

	Camera* SceneManager::newCamera()
	{
		m_cams.push_back(Camera());
		return &m_cams.back();
	}

	void SceneManager::freeScene(Scene* &scene)
	{
		if (scene)
		{
			assert(m_curScene == scene);
			auto stackptr = scene->getSceneName();
			scene->freeScene();
			m_allocator->freeL(const_cast<char*>(stackptr));
			delete scene;
			scene = nullptr;
		}
	}

	void SceneManager::endLoadScene(Scene* scene)
	{
		m_allocator->freeL(scene->getTopL());
		m_imp.closeModel3D();
	}

	void SceneManager::update(double time)
	{
		assert(m_curScene);
		for (auto& cam : m_cams)
		{
			m_curScene->update(&cam, time);
		}
	}

	void SceneManager::drawAll()
	{
		assert(m_curScene);
		for (auto& cam : m_cams)
		{
			m_curScene->drawAll(&cam);
		}
	}
	
	void SceneManager::switchScene(Scene* scene)
	{
		if (scene == m_loadingScene)
		{
			if (m_prevScene)
				freeScene(m_prevScene);

			//Todo: check loadingScene is loaded, otherwise load it
			m_prevScene = m_curScene;
			m_curScene = m_loadingScene;
			m_loadingScene = nullptr;
		}
		else if (scene == m_prevScene)
		{
			m_prevScene = m_curScene;
			m_curScene = scene;
		}
		else
		{
			assert(0);//Todo
		}
	}
}