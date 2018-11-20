#include "include/Toy.h"
#include "include/ToyLuaLibs.h"


namespace toy
{
	static int run_entry(lua_State* L)
	{
		io::WPath wpath(L"scripts/toy.lua");
		size_t fsize = 0;
		u8* data = io::readFile(wpath, &fsize);
		if (data)
		{
			char* buf = reinterpret_cast<char*>(data);
			int err = luaL_loadbuffer(L, buf, fsize, "toy");
			io::freeFile(data);
			if (LUA_OK != err)
			{
				const char *msg = lua_tostring(L, -1);
				LOGERROR("run_entry: %s\n", msg);
				lua_pop(L, 1);
				return 0;
			}

			lua_call(L, 0, 0);
		}
		return 0;
	}
	static int regToy(lua_State* L)
	{
		va_list ap = *reinterpret_cast<va_list*>(lua_touserdata(L, 1));
		ToyEngine* eng = va_arg(ap, ToyEngine*);

		luaL_checkversion(L);
		luaL_openlibs(L);

		lua_pushlightuserdata(L, eng);
		lua_pushvalue(L, -1);
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);
		assert(LUA_REFNIL != ref && LUA_NOREF != ref && "ref engine to lua failed");
		lua_setfield(L, LUA_REGISTRYINDEX, "engine");

		luacf_regToy(L, ref);//register toy lua libs

		run_entry(L);//setup envirenment

		return 0;
	}
	ErrNo ToyEngine::createDevice(
		const char* wndTitle,
		uint wndWidth,
		uint wndHeight,
		rd::RenderDriverType rdt)
	{
		m_logger.logInit();

		m_hid.init();
		m_hidFun = nullptr;
		ErrNo err = m_wnd.init(&m_hid);
		err = m_wnd.createWindow(wndTitle, wndWidth, wndHeight, rdt);
		if (err)
		{
			LOGFATAL("Create window failed\n");
			return err;
		}

		Timer::initTimerEnv();
		m_mainTimer.init();

		size_t spaceSize = 1024 * 1024 * 128;//128M
		m_smgr.init(spaceSize, m_wnd.getRenderDriver());

		//Todo: put lua to other thread
		m_lua.init(nullptr, nullptr);
		m_lua.call(regToy, this); //register this engine instance to lua
		m_lua.gc(LUA_GCCOLLECT, 0);

		return ErrNo(0);
	}

	void ToyEngine::destroy()
	{
		m_lua.destroy();
		m_smgr.destroy();
		m_wnd.destroy();
	}

	//Timer MUST be initilized before call this function
	static void calcFPS(IWindow* wnd, Timer &osTimer)
	{
		static toy::s64 timeLastFrame = 0;
		static toy::s64 timeFPS = 0;
		static int fps = 0;

		++fps;

		auto timeThisFrame = osTimer.getTime();
		DWORD frameDuration = timeThisFrame - timeLastFrame;//careful not to be zero
		timeFPS += frameDuration;
		if (timeFPS > 1000)
		{
			char title[64];
			sprintf_s(title, "FPS: %d", fps);
			wnd->setTitle(title);
			fps = 0;
			timeFPS -= 1000;
		}
		if (frameDuration < 17)
			Sleep(17 - frameDuration);
		timeLastFrame = timeThisFrame;
	}

	int ToyEngine::mainLoop()
	{
		if (nullptr == m_hidFun)
			m_hidFun = [](ToyEngine* eng, s64 ftime) {};

		auto rdr = m_wnd.getRenderDriver();
		assert(rdr);

		m_mainTimer.reset();
		
		while (1)
		{
			auto frameTime = m_mainTimer.getTime();
			if (!m_wnd.checkOSMsg())
				break;
			m_hidFun(this, frameTime);
			m_hid.synchronizeStates();

			m_smgr.update(frameTime);

			rdr->beginFrame();
			rdr->clearFramebuffer();

			m_smgr.drawAll();

			rdr->endFrame();

			m_wnd.swapBuffers();

			calcFPS(&m_wnd, m_mainTimer);
		}
		return 0;
	}
}