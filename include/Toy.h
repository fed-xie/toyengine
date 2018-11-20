#pragma once
#include "EngineConfig.h"
#include "Types.h"
#include "HID.h"
#include "IWindow.h"
#include "LuaInterpreter.h"
#include "Timer.h"
//#include "TaskQueue.h"
#include "SceneManager.h"

namespace toy
{
	class ToyEngine
	{
	public:
		using HIDReflectionFunc = void(*)(ToyEngine* eng, s64 ftime);
		ErrNo createDevice(
			const char* wndTitle,
			uint wndWidth,
			uint wndHeight,
			rd::RenderDriverType rdt);
		void destroy();
		
		IWindow* getWindow() { return &m_wnd; }
		HIDAdapter* getHIDAdapter() { return &m_hid; }
		IRenderDriver* getRenderDriver() { return m_wnd.getRenderDriver(); }
		LuaInterpreter* getLuaInterpreter() { return &m_lua; }
		io::Logger* getLogger() { return &m_logger; }
		SceneManager* getSceneManager() { return &m_smgr; }
		Timer* getMainTimer() { return &m_mainTimer; }

		void setHIDReflection(HIDReflectionFunc f) { m_hidFun = f; }
		int mainLoop();
	private:
		HIDAdapter m_hid;
		IWindow m_wnd;
		LuaInterpreter m_lua;
		io::Logger m_logger;
		SceneManager m_smgr;
		HIDReflectionFunc m_hidFun;
		Timer m_mainTimer;
	};
}