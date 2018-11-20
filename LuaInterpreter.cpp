#include "include/LuaInterpreter.h"
#include "include/IO.h"

#include <cstdarg> //for va_list

#pragma comment(lib, "libs/lua/lua53.lib")

namespace toy
{
	bool LuaInterpreter::init(lua_Alloc f, void* ud)
	{
		if(f)
			m_L = lua_newstate(f, ud);
		else
			m_L = luaL_newstate();
		
		assert(nullptr != m_L && "Create lua machine failed");
		return true;
	}
	void LuaInterpreter::destroy()
	{
		if (m_L)
		{
			lua_close(m_L);
			m_L = nullptr;
		}
	}

	static int luacf_callva(lua_State* L)
	{
		int argc = lua_gettop(L);
		const char* fname = (const char*)lua_touserdata(L, 1);
		lua_getglobal(L, fname);
		int inputn = 0;
		if (argc > 1)
		{
			const char* fmt = (const char*)lua_touserdata(L, 2);
			va_list ap = *reinterpret_cast<va_list*>(lua_touserdata(L, 3));

			luaL_checkstack(L, strlen(fmt), "Check stack failed");
			while ('\0' != fmt[inputn])
			{
				switch (fmt[inputn])
				{
				case 's': lua_pushstring(L, va_arg(ap, const char*)); break;
				case 'd': lua_pushnumber(L, va_arg(ap, double)); break;
				case 'i': lua_pushinteger(L, va_arg(ap, int)); break;
				case 'l': lua_pushlightuserdata(L, va_arg(ap, void*)); break;
				case 'g': lua_getglobal(L, va_arg(ap, const char*)); break;
				default:
					lua_pushstring(L, "luacf_call input format syntax error");
					lua_error(L);
				}
				++inputn;
			}
		}
		lua_call(L, inputn, 0);
		return 0;
	}
	int LuaInterpreter::call(const char* funcName, const char* fmt, ...)
	{
		assert(funcName);
		int err = LUA_OK;
		lua_pushcfunction(m_L, luacf_callva);
		lua_pushlightuserdata(m_L, const_cast<char*>(funcName));
		if (fmt)
		{
			va_list ap;
			va_start(ap, fmt);
			lua_pushlightuserdata(m_L, const_cast<char*>(fmt));
			lua_pushlightuserdata(m_L, &ap);
			err = lua_pcall(m_L, 3, 0, 0);
			va_end(ap);
		}
		else
		{
			err = lua_pcall(m_L, 1, 0, 0);
		}

		if (LUA_OK != err && LUA_TSTRING == lua_type(m_L, -1))
		{
			const char *msg = lua_tostring(m_L, -1);
			LOGERROR("LuaInterpreter::call(%d): %s\n", __LINE__, msg);
			lua_pop(m_L, 1);
		}
		return 0;
	}

	int LuaInterpreter::call(lua_CFunction func, ...)
	{
		va_list ap;
		va_start(ap, func);
		lua_pushcfunction(m_L, func);
		lua_pushlightuserdata(m_L, &ap);
		int err = lua_pcall(m_L, 1, 0, 0);
		va_end(ap);

		if (LUA_OK != err && LUA_TSTRING == lua_type(m_L, -1))
		{
			const char *msg = lua_tostring(m_L, -1);
			LOGERROR("LuaInterpreter::call(%d): %s\n", __LINE__, msg);
			lua_pop(m_L, 1);
		}
		return 0;
	}

	int luacf_reqLib(lua_State* L)
	{
		luaL_Reg* lib = (luaL_Reg*)lua_touserdata(L, 1);
		int glb = static_cast<int>(lua_tointeger(L, 2));
		for (; lib->func; ++lib)
		{
			luaL_requiref(L, lib->name, lib->func, glb);
			lua_pop(L, 1);  /* remove lib */
		}
		return 0;
	}
	int LuaInterpreter::requireModules(luaL_Reg mod[], int global)
	{
		lua_pushcfunction(m_L, luacf_reqLib);
		lua_pushlightuserdata(m_L, mod);
		lua_pushinteger(m_L, global);
		int err = lua_pcall(m_L, 2, 0, 0);
		if (LUA_OK != err && LUA_TSTRING == lua_type(m_L, -1))
		{
			const char *msg = lua_tostring(m_L, -1);
			LOGERROR("LuaInterpreter::requireModules: %s\n", msg);
			lua_pop(m_L, 1);
		}
		return 0;
	}

	int luacf_gc(lua_State* L)
	{
		auto what = lua_tointeger(L, 1);
		auto data = lua_tointeger(L, 2);
		auto ret = lua_gc(L, what, data);
		lua_pushinteger(L, ret);
		return 1;
	}
	int LuaInterpreter::gc(int what, int data)
	{
		LOGDEBUG("before gc, lua_gettop() = %d\n", lua_gettop(m_L));
		lua_pushcfunction(m_L, luacf_gc);
		lua_pushinteger(m_L, what);
		lua_pushinteger(m_L, data);
		int err = lua_pcall(m_L, 2, 1, 0);
		if (LUA_OK != err && LUA_TSTRING == lua_type(m_L, -1))
		{
			const char *msg = lua_tostring(m_L, -1);
			LOGERROR("LuaInterpreter::gc: %s\n", msg);
			lua_pop(m_L, 1);
			return -1;
		}
		int res = static_cast<int>(lua_tointeger(m_L, 1));
		lua_pop(m_L, 1);
		LOGDEBUG("after gc, lua_gettop() = %d\n", lua_gettop(m_L));
		return res;
	}
}