#pragma once
#include "lua/lua.hpp"

// create a key for registry, using a string to confirm this key is unique
inline void createRegistryKey(lua_State* L, void* &key, const char* uniqueName)
{
	if (nullptr == key)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, uniqueName);
		key = lua_touserdata(L, -1);
		lua_pop(L, 1);
		if (nullptr == key)
		{
			key = (void*)&key;
			lua_pushlightuserdata(L, key);
			lua_setfield(L, LUA_REGISTRYINDEX, uniqueName);
		}
	}
}

int luacf_regToy(lua_State* L, int engRef);