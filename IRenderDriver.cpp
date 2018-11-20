#include "include/IRenderDriver.h"
#include <stdexcept>

namespace toy
{
	IShaderProgram* IRenderDriver::createShaderProgram(rd::ShaderLang st)
	{
		if (rd::SHADER_GLSL == st)
		{
			for (auto &prog : m_shaderProgs)
			{
				if (nullptr == prog)
				{
					prog = new GLSLShaderProgram;
					return prog;
				}
			}
			assert(0);//if reach this, just simply expand m_shaderProgs's length is OK
			return nullptr;
		}
		else
		{
			throw std::runtime_error("Unsupported shader type.");
			return nullptr;
		}
	}
	void IRenderDriver::freeShaderProgram(IShaderProgram* shaderProg)
	{
		for (auto &prog : m_shaderProgs)
		{
			if (prog == shaderProg)
			{
				prog->destroy();
				delete prog;
				prog = nullptr;
			}
		}
	}
}