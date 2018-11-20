work_dir = "D:/Projects/Test/Test/"
glsl = 
{
	dir = "GLShaders/",
	shader_types = { vt = 0, fg = 1, ctl = 2, eval = 3, geom = 4, },
	programs = 
	{
		stat_model = { vt = "StaticModel.vert", fg = "StaticModel.frag", },
		anim_model = { vt = "AnimatedModel.vert", fg = "StaticModel.frag", },
	},
	ubos = 
	{
		--1st table is indices of binding programs, must same to programs's key
		--2nd string is block name,
		--3rd table are variable names
		{
			[1] = { "anim_model", }, --can use # to get the number of variable
			[2] = "UBoneMatrices",
			[3] = { "boneMatrices", }, --can use # to get the number of variable
		},
	},
}

function setup_glsl()
	local load_shader = toy.glsl.load_shader
	local load_program = toy.glsl.load_program
	local load_ubo = toy.glsl.load_ubo
	local pairs = pairs
	
	local loaded_shaders = setmetatable({}, { __gc = toy.glsl.free_shaders })
	local loaded_progs = setmetatable({}, { __gc = toy.glsl.free_programs })
	local loaded_ubos = setmetatable({}, { __gc = toy.glsl.free_ubos })
	local shader_types = glsl.shader_types
	for progname, spaths in pairs(glsl.programs) do
		local prog_shaders = {}
		local shader_types = glsl.shader_types
		local shader_dir = glsl.dir
		for stype, spath in pairs(spaths) do
			if nil == loaded_shaders[spath] then
				loaded_shaders[spath] = load_shader(shader_dir .. spath, shader_types[stype])
			end
			prog_shaders[stype] = loaded_shaders[spath]
		end
		loaded_progs[progname] = load_program(
			progname,
			prog_shaders.vt,
			prog_shaders.fg,
			prog_shaders.ctl,
			prog_shaders.eval,
			prog_shaders.geom)
	end
	for idx, props in pairs(glsl.ubos) do
		local progs = {}
		local prog_names = props[1]
		for i = 1, #prog_names do
			progs[i] = loaded_progs[prog_names[i]]
			if nil == progs[i] then
				error("Unknown shader program " .. prog_names[i])
			end
		end
		loaded_ubos[idx] = load_ubo(progs, props[2], props[3])
	end
	glsl.loaded_shaders = loaded_shaders
	glsl.loaded_progs = loaded_progs
	glsl.loaded_ubos = loaded_ubos
	
	print("setup glsl seccess")
	return loaded_shaders, loaded_progs, loaded_ubos
end

require("scripts/utils")
--toy.io.setcwd(work_dir)
--setup_glsl()

--print_m(glsl.loaded_progs)
