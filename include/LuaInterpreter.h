#pragma once
#include "libs/include/lua/lua.hpp"

namespace toy
{
	class LuaInterpreter
	{
	public:
		bool init(lua_Alloc f = nullptr, void* ud = nullptr);
		void destroy();
		lua_State* getState() { return m_L; }
		
		/* call a lua function, fmt only accept:
		   's' as a const char* for a string
		   'd' as a double
		   'i' as a int
		   'g' as a const char* for a global variable name
		   'l' as a void* for a light user data
		   eg.
		    int n = 42;
		    double d = .33;
		    char* str = "arg string";
		    call("foo", "isd", n, str, d);
		 */
		int call(const char* funcName, const char* fmt = nullptr, ...);

		/* call a lua_CFunction,
		   pass a va_list* to the func (by light user data)
		 */
		int call(lua_CFunction func, ...);

		//wrapper of luaL_requiref(), working similar to luaL_openlibs()
		int requireModules(luaL_Reg mod[], int global = 1/*TRUE*/);

		//wrapper of lua_gc()
		int gc(int what, int data);
	private:
		lua_State* m_L;
	};
}