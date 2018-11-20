#include "include/ToyLuaLibs.h"
#include "include/Toy.h"

using toy::uint;
static int eng_ref = LUA_NOREF;
static inline toy::ToyEngine* getEngine(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, eng_ref);
	toy::ToyEngine* eng = (toy::ToyEngine*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	assert(eng && "Engine not register to Lua registry");
	return eng;
}
static inline toy::IRenderDriver* getRenderDriver(lua_State* L)
{
	auto eng = getEngine(L);
	auto rdr = eng->getRenderDriver();
	assert(rdr && "Render driver not found in engine");
	return rdr;
}
static inline toy::IRenderDriver* getRenderDriver(lua_State* L, toy::ToyEngine* eng)
{
	auto rdr = eng->getRenderDriver();
	assert(rdr && "Render driver not found in engine");
	return rdr;
}

static int toy_free_glsl_shaders(lua_State* L)
{
	if (LUA_TTABLE != lua_type(L, 1))
		luaL_error(L, "Argument #1 isn't a table");
	lua_pushnil(L);
	while (0 != lua_next(L, 1))
	{
		auto sdr = (toy::rd::Shader*)lua_touserdata(L, -1);
		if (sdr)
		{
			auto eng = getEngine(L);
			toy::GLSLShaderProgram::freeShader(sdr);
		}
		lua_pop(L, 1);
	}
	return 0;
}
static int toy_free_glsl_programs(lua_State* L)
{
	if (LUA_TTABLE != lua_type(L, 1))
		luaL_error(L, "Argument #1 isn't a table");
	lua_pushnil(L);
	while (0 != lua_next(L, 1))
	{
		auto prog = (toy::IShaderProgram*)lua_touserdata(L, 1);
		if (prog)
		{
			auto eng = getEngine(L);
			auto rdr = eng->getRenderDriver();
			rdr->freeShaderProgram(prog);
		}
		lua_pop(L, 1);
	}
	return 0;
}
static int toy_free_glsl_ubos(lua_State* L)
{
	if (LUA_TTABLE != lua_type(L, 1))
		luaL_error(L, "Argument #1 isn't a table");
	lua_pushnil(L);
	while (0 != lua_next(L, 1))
	{
		auto ubo = (toy::rd::SBBO*)lua_touserdata(L, 1);
		if (ubo)
			toy::GLSLShaderProgram::freeSharedBlockBuffer(ubo);
		lua_pop(L, 1);
	}
	return 0;
}
//rd::Shader* load_shader(string path, int shader_type)
static int toy_load_glsl_shader(lua_State* L)
{
	if (2 != lua_gettop(L))
	{
		lua_pushstring(L, "Arguments number exception");
		lua_error(L);
	}
	const char* filename = luaL_checkstring(L, 1);
	auto type = luaL_checkinteger(L, 2);
	toy::rd::ShaderType sdrtype = static_cast<toy::rd::ShaderType>(type);
	toy::io::WPath wpath(filename);
	char* text = toy::io::readText(wpath);
	if (text)
	{
		auto sdr = toy::GLSLShaderProgram::createShader(text, sdrtype);
		toy::io::freeText(text);
		lua_pushlightuserdata(L, sdr);
	}
	else
		luaL_error(L, "Read file %s failed", filename);
	return 1;
}

//rd::GLSLShaderProgram* load_program(char* name, rd::Shader* vt, fg, cvl, eval, geom)
static int toy_load_glshader_program(lua_State* L)
{
	int argc = lua_gettop(L);
	size_t slen;
	auto prog_name = luaL_checklstring(L, 1, &slen);
	constexpr auto maxlen = ARRAYLENGTH(toy::IShaderProgram::name) - 1;
	if (slen > maxlen)
		luaL_error(L, "program name is too long, %d max", maxlen);

	toy::rd::Shader* sdrs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	for (int i = 1; i < argc; ++i)
		sdrs[i-1] = (toy::rd::Shader*)lua_touserdata(L, i+1);

	toy::ToyEngine* eng = getEngine(L);
	assert(eng && "Engine not register to Lua registry");
	toy::IRenderDriver* rdr = eng->getRenderDriver();
	if (rdr)
	{
		auto prog = rdr->createShaderProgram(toy::rd::SHADER_GLSL);
		prog->compile(sdrs[0], sdrs[1], sdrs[2], sdrs[3], sdrs[4]);
		toy::str::strnCpy(prog->name, sizeof(prog->name), prog_name, maxlen);
		prog->name[maxlen] = '\0';
		lua_pushlightuserdata(L, prog);
	}
	else
	{
		lua_pushstring(L, "Render driver not found");
		lua_error(L);
	}
	return 1;
}

//rd::SBBO* gen_ubo(GLSLShaderProgram** progs, char* blockName, char** varNames)
static int toy_load_ubo(lua_State* L)
{
	if (3 != lua_gettop(L))
		luaL_error(L, "Argument number not fit");
	
	/* create ubo */
	if (LUA_TLIGHTUSERDATA != lua_geti(L, 1, 1))
		luaL_error(L, "Argument type error");
	toy::GLSLShaderProgram* prog = (toy::GLSLShaderProgram*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	const char* blockName = lua_tostring(L, 2);
	auto sbbo = prog->genSharedBlockBuffer(blockName);

	/* get variables' offsets */
	size_t varNum = lua_rawlen(L, 3);
	if (varNum > 32)
		luaL_error(L, "Too many variable names");
	const char* varNames[32];
	for (size_t i = 1; i <= varNum; ++i)
	{
		lua_rawgeti(L, 3, i);
		const char* varNamei = lua_tostring(L, -1);
		varNames[i - 1] = varNamei;
		lua_pop(L, 1);
	}
	lua_settop(L, 3);
	prog->getSBBOVarOffsets(sbbo, varNum, varNames);

	/* bind ubo to other programs */
	size_t progNum = lua_rawlen(L, 1);
	for (size_t i = 2; i <= progNum; ++i)
	{
		lua_rawgeti(L, 1, i);
		prog = (toy::GLSLShaderProgram*)lua_touserdata(L, -1);
		prog->bindSharedBlockBuffer(sbbo, blockName);
		lua_pop(L, 1);
	}

	lua_pushlightuserdata(L, sbbo);
	return 1;
}

int toy_switch_glsl_program(lua_State* L)
{
	if (LUA_TLIGHTUSERDATA != lua_type(L, 1))
		luaL_error(L, "Not a glsl shader program");
	
	auto prog = (toy::GLSLShaderProgram*)lua_touserdata(L, 1);
	prog->use();
	return 0;
}

static luaL_Reg toy_glsllib[] = 
{
	{ "load_shader", toy_load_glsl_shader },
	{ "load_program", toy_load_glshader_program },
	{ "load_ubo", toy_load_ubo },
	{ "free_shaders", toy_free_glsl_shaders },
	{ "free_programs", toy_free_glsl_programs },
	{ "free_ubos", toy_free_glsl_ubos },
	{ "switch_program", toy_switch_glsl_program },
	{ NULL, NULL },
};

static int toy_setcwd(lua_State* L)
{
	const char* path = lua_tostring(L, 1);
	if (path)
	{
		toy::io::WPath wpath(path);
		auto err = toy::io::setCWD(wpath.c_wcs());
		if (err)
		{
			lua_pushstring(L, "set cwd failed");
			lua_error(L);
		}
		return 0;
	}
	else
	{
		luaL_error(L, "setcwd() need an UTF-8 string");
		return 0;
	}
}
static int toy_loginfo(lua_State* L)
{
	const char* msg = luaL_checkstring(L, 1);
	LOGINFO("%s\n", msg);
	return 0;
}

// load_textures(char* paths[])
static int scene_load_textures(lua_State* L)
{
	auto eng = getEngine(L);
	auto rdr = getRenderDriver(L, eng);
	auto scene = eng->getSceneManager()->getSceneNowLoading();
	assert(scene && "No scene is loading");
	if (LUA_TTABLE != lua_type(L, 1))
		luaL_error(L, "Arg #1 need an array table");

	const char* paths[100];
	uint texNum = lua_rawlen(L, 1);
	assert(texNum < 100);
	for (uint i = 0; i != texNum; ++i)
	{
		lua_rawgeti(L, 1, i + 1);
		paths[i] = luaL_checkstring(L, -1);
		lua_pop(L, 1);
	}

	auto texs = scene->loadTextures(paths, texNum);
	if (nullptr == texs)
		return 0;

	lua_createtable(L, texNum, 0);
	for (uint i = 0; i != texNum; ++i)
	{
		lua_pushlightuserdata(L, &texs[i]);
		lua_rawseti(L, -1, i + 1);
	}
	return 1;
}

static luaL_Reg toy_iolib[] =
{
	{ "setcwd", toy_setcwd },
	{ "loginfo", toy_loginfo },
	{ NULL, NULL },
};

static int open_toylib(lua_State* L)
{
	lua_newtable(L);

	luaL_newlib(L, toy_glsllib);
	lua_setfield(L, -2, "glsl");

	luaL_newlib(L, toy_iolib);
	lua_setfield(L, -2, "io");

	return 1;
}

int luacf_regToy(lua_State* L, int engRef)
{
	//luaL_checkstack()
	eng_ref = engRef;

	luaL_requiref(L, "toy", open_toylib, 1);
	lua_pop(L, 1);
	return 0;
}
