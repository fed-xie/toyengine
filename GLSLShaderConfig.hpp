#pragma once
#include "include/SceneManager.h"

namespace toy
{
	static const ShaderSource shader_srcs[] =
	{
	/* 0*/{ "StaticModel.vert",		rd::SHADER_VERTEX	},
	/* 1*/{ "StaticModel.frag",		rd::SHADER_FRAGMENT	},
	/* 2*/{ "AnimatedModel.vert",	rd::SHADER_VERTEX	},
	};
	static rd::Shader* def_shaders[ARRAYLENGTH(shader_srcs)];
	
	struct ShaderProgramSource
	{
		int sdr_indices[rd::SHADER_TYPENUM];
		rd::ShaderLang type;
	};
	static const ShaderProgramSource sprog_srcs[] =
	{
	/* 0*/{ { 0, 1, -1, -1, -1 }, rd::SHADER_GLSL },
	/* 1*/{{2, 1, -1, -1, -1}, rd::SHADER_GLSL},
	};

	static IShaderProgram* def_sdr_progs[ARRAYLENGTH(sprog_srcs)];
	static const char* UBoneMatricesVars[] =
	{
		"boneMatrices",
	};
	static rd::SBBO* UBoneMatrices;
	static const char* UCameraMatricesVars[] =
	{
		"PVMat",
		"PMat",
		"VMat"
	};
	static rd::SBBO* UCameraMatrices;

	static int setCameraMatrices(IShaderProgram* prog, void* args)
	{
		struct CameraMatrices
		{
			fmat4 pvmat;
			fmat4 pmat;
			fmat4 vmat;
		};
		
		auto data = reinterpret_cast<CameraMatrices*>(args);
		prog->setSharedBlockBufferData(
			UCameraMatrices, 
			UCameraMatrices->properties.ubo.varOffsets[0],
			&data->pvmat,
			sizeof(CameraMatrices));
		return 0;
	}

	static int preSkelAnimDraw(IShaderProgram* prog, void* args)
	{
		struct PreData
		{
			fmat4 pvmmat;
			fmat4* bonemats;
			uint bonenum;
		};
		auto data = reinterpret_cast<PreData*>(args);
		prog->setUniform("MMat", data->pvmmat);
		prog->setSharedBlockBufferData(
			UBoneMatrices,
			UBoneMatrices->properties.ubo.varOffsets[0],
			data->bonemats,
			sizeof(fmat4)*data->bonenum);
		return 0;
	}

	static void loadDefaultShaders(IRenderDriver* rdr)
	{
		io::WPath wp("");
		wp.getCWD();
		wp.setFileName("GLShaders/");
		wp.getDirStrLen();
		for (uint i = 0; i != ARRAYLENGTH(shader_srcs); ++i)
		{
			auto &sdr_src = shader_srcs[i];
			wp.setFileName(sdr_src.name);
			auto text = io::readText(wp);
			assert(text);
			def_shaders[i] = GLSLShaderProgram::createShader(text, sdr_src.type);
			io::freeText(text);
		}
		
		for (uint i = 0; i != ARRAYLENGTH(sprog_srcs); ++i)
		{
			auto& indices = sprog_srcs[i].sdr_indices;
			def_sdr_progs[i] = rdr->createShaderProgram(sprog_srcs[i].type);
			def_sdr_progs[i]->compile(
				-1 != indices[0] ? def_shaders[indices[0]] : nullptr,
				-1 != indices[1] ? def_shaders[indices[1]] : nullptr,
				-1 != indices[2] ? def_shaders[indices[2]] : nullptr,
				-1 != indices[3] ? def_shaders[indices[3]] : nullptr,
				-1 != indices[4] ? def_shaders[indices[4]] : nullptr);
		}

		auto &stat_model = def_sdr_progs[0];
		auto &anim_model = def_sdr_progs[1];

		UCameraMatrices = stat_model->genSharedBlockBuffer("UCameraMatrices");
		assert(UCameraMatrices);
		stat_model->getSBBOVarOffsets(UCameraMatrices,
			ARRAYLENGTH(UCameraMatricesVars), UCameraMatricesVars);
		assert(sizeof(fmat4) * 2 == UCameraMatrices->properties.ubo.varOffsets[2]);
		anim_model->bindSharedBlockBuffer(UCameraMatrices, "UCameraMatrices");

		UBoneMatrices = anim_model->genSharedBlockBuffer("UBoneMatrices");
		assert(UBoneMatrices);
		anim_model->getSBBOVarOffsets(UBoneMatrices,
			ARRAYLENGTH(UBoneMatricesVars), UBoneMatricesVars);
		anim_model->preDrawFunc = preSkelAnimDraw;
	}
}