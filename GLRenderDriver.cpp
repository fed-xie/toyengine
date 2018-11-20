#include "include/GLRenderDriver.h"
#include "include/IO.h"

#define GLEW_STATIC
#include "libs/include/glew/GL/glew.h"
#include "libs/include/glew/GL/wglew.h"
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "libs/glew/glew32s.lib")

namespace toy
{
	static const GLuint loadShader(const char* source, GLenum type)
	{
		GLuint shader = glCreateShader(type);
		if (!shader)
		{
			io::Logger::log(io::Logger::LOG_ERROR, L"Fail to create GLSL shader!\n");
			return 0;
		}
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			GLint len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
			char* log = new char[len + 1];
			log[len] = '\0';
			glGetShaderInfoLog(shader, len, &len, log);
			io::Logger::log(io::Logger::LOG_ERROR, log);
			delete[] log;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}
	
	/* return prog if success, else return 0 */
	static GLuint linkShaderProgram(GLuint prog)
	{
		glLinkProgram(prog);
		GLint status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		if (!status)
		{
			GLint len;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
			char* log = new char[len + 1];
			log[len] = '\0';
			glGetProgramInfoLog(prog, len, &len, log);
			io::Logger::log(io::Logger::LOG_ERROR, log);
			delete[] log;
			return 0;
		}
		return prog;
	}
	
	constexpr GLenum getGLSLType(rd::ShaderType type)
	{
		switch (type)
		{
		case rd::ShaderType::SHADER_VERTEX:		return GL_VERTEX_SHADER;
		case rd::ShaderType::SHADER_FRAGMENT:	return GL_FRAGMENT_SHADER;
		case rd::ShaderType::SHADER_CONTROL:	return GL_TESS_CONTROL_SHADER;
		case rd::ShaderType::SHADER_EVALUATION: return GL_TESS_EVALUATION_SHADER;
		case rd::ShaderType::SHADER_GEOMETRY:	return GL_GEOMETRY_SHADER;
		default:
			return GL_NONE;
		}
	}
	
	rd::Shader* GLSLShaderProgram::createShader(const char * source, rd::ShaderType type)
	{
		GLenum glslType = getGLSLType(type);
		assert(GL_NONE != glslType);

		GLuint shader = loadShader(source, glslType);
		return new rd::Shader(shader);
	}

	void GLSLShaderProgram::freeShader(rd::Shader* &shader)
	{
		GLuint sdr = shader->getHandle();
		if (glIsShader(sdr))
		{
			glDeleteShader(sdr);
			delete shader;
			shader = nullptr;
		}
	}
	
	void GLSLShaderProgram::compile(
		rd::Shader* vertexShader,
		rd::Shader* fragmentShader,
		rd::Shader* controlShader,
		rd::Shader* evaluationShader,
		rd::Shader* geometryShader)
	{
		GLuint prog = glCreateProgram();
		if (!prog)
		{
			io::Logger::log(io::Logger::LOG_ERROR, "Fail to create GLSL program!\n");
			return;
		}
		if (vertexShader && vertexShader->isValid())
			glAttachShader(prog, vertexShader->getHandle());

		if (fragmentShader && fragmentShader->isValid())
			glAttachShader(prog, fragmentShader->getHandle());

		if (controlShader && controlShader->isValid())
			glAttachShader(prog, controlShader->getHandle());

		if (evaluationShader && evaluationShader->isValid())
			glAttachShader(prog, evaluationShader->getHandle());

		if (geometryShader && geometryShader->isValid())
			glAttachShader(prog, geometryShader->getHandle());

		GLuint ret = linkShaderProgram(prog);
		if (!ret)
		{
			glDeleteProgram(prog);
			prog = 0;
		}
		m_handle = prog;
	}

	void GLSLShaderProgram::compile(
		const char * vertexShaderSource,
		const char * fragmentShaderSource)
	{
		assert(vertexShaderSource && fragmentShaderSource);

		GLuint vtShader = loadShader(vertexShaderSource, GL_VERTEX_SHADER);
		GLuint fgShader = loadShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
		if (vtShader && fgShader)
		{
			GLuint prog = glCreateProgram();
			if (!prog)
			{
				io::Logger::log(io::Logger::LOG_ERROR, "Fail to create GLSL program!\n");
				glDeleteShader(vtShader);
				glDeleteShader(fgShader);
				return;
			}
			glAttachShader(prog, vtShader);
			glAttachShader(prog, fgShader);
			GLuint ret = linkShaderProgram(prog);
			if (!ret)
			{
				glDeleteProgram(prog);
				prog = 0;
			}
			glDeleteShader(vtShader);
			glDeleteShader(fgShader);
			m_handle = prog;
		}
	}
	
	void GLSLShaderProgram::use()
	{
		glUseProgram(m_handle);
	}

	void GLSLShaderProgram::destroy()
	{
		if (glIsProgram(m_handle))
		{
			glDeleteProgram(m_handle);
			m_handle = 0;
		}
	}
	
	void GLSLShaderProgram::setUniform(
		const char* varName,
		const fmat4& value)
	{
		GLint uniloc = glGetUniformLocation(m_handle, varName);
		assert(uniloc != GL_INVALID_INDEX); //-1 means various not found
		glUniformMatrix4fv(uniloc, 1, GL_TRUE, value.a);
	}
	void GLSLShaderProgram::setUniform(
		const char* varName,
		const fvec3& value)
	{
		GLint uniloc = glGetUniformLocation(m_handle, varName);
		assert(uniloc != GL_INVALID_INDEX); //-1 means various not found
		glUniform3f(uniloc, value.X, value.Y, value.Z);
	}
	void GLSLShaderProgram::setUniform(
		const char* varName,
		const uint value)
	{
		GLint uniloc = glGetUniformLocation(m_handle, varName);
		assert(uniloc != GL_INVALID_INDEX); //-1 means various not found
		glUniform1ui(uniloc, value);
	}
	void GLSLShaderProgram::GLSLShaderProgram::setUniform(
		const char* varName,
		const int value)
	{
		GLint uniloc = glGetUniformLocation(m_handle, varName);
		assert(uniloc != GL_INVALID_INDEX); //-1 means various not found
		glUniform1i(uniloc, value);
	}
	
	class UniformBlock : public rd::SBBO
	{
	public:
		int m_bindPoint;
	};
	static int g_usedBindPointNum = 0;

	rd::SBBO* GLSLShaderProgram::genSharedBlockBuffer(const char* blockName)
	{
		GLuint progHdl = m_handle;
		GLuint blockIdx = glGetUniformBlockIndex(progHdl, blockName);
		if (blockIdx == GL_INVALID_INDEX)
			return nullptr;
		GLint uboSize;
		glGetActiveUniformBlockiv(progHdl, blockIdx, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);

		GLint max_ubo_bindings;
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_ubo_bindings);
		assert(g_usedBindPointNum < 36 &&
			g_usedBindPointNum < max_ubo_bindings - 1 &&
			"Too much SBBO generated");
		UniformBlock* ub = new UniformBlock;
		ub->m_bindPoint = g_usedBindPointNum++;
		GLuint ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, ub->m_bindPoint, ubo);
		glUniformBlockBinding(progHdl, blockIdx, ub->m_bindPoint);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		ub->setHandle(ubo);
		ub->size = uboSize;
		return ub;
	}
	
	bool GLSLShaderProgram::setSharedBlockBufferData(
		rd::SBBO* sbbo, int offset, void* data, size_t dataSize)
	{
		UniformBlock* ub = static_cast<UniformBlock*>(sbbo);
		GLuint ubo = ub->getHandle();
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		void *bufData = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		if (bufData)
		{
			memcpy((void*)((size_t)bufData + offset), data, dataSize);
			if (GL_TRUE != glUnmapBuffer(GL_UNIFORM_BUFFER))
			{
				auto err = glGetError();
				return false;
			}
		}
		else
			glBufferSubData(GL_UNIFORM_BUFFER, offset, dataSize, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		return true;
	}
	
	void GLSLShaderProgram::bindSharedBlockBuffer(rd::SBBO * sbbo, const char* blockName)
	{

		/* For the whole life cycle of the engine, all
		the shared block buffers must be generated
		by the same render driver, so it's safe to
		use static_cast */
		UniformBlock* ub = static_cast<UniformBlock*>(sbbo);
		GLuint blockIdx = glGetUniformBlockIndex(m_handle, blockName);
		if (blockIdx == GL_INVALID_INDEX)
			return;
		glUniformBlockBinding(m_handle, blockIdx, ub->m_bindPoint);
	}
	
	static int getTypeSize(GLenum type)
	{
		switch (type)
		{
		case GL_FLOAT:			   return sizeof(GLfloat);
		case GL_FLOAT_VEC2:		   return sizeof(GLfloat) * 2;
		case GL_FLOAT_VEC3:		   return sizeof(GLfloat) * 3;
		case GL_FLOAT_VEC4:		   return sizeof(GLfloat) * 4;
		case GL_INT:			   return sizeof(GLint);
		case GL_INT_VEC2:		   return sizeof(GLint) * 2;
		case GL_INT_VEC3:		   return sizeof(GLint) * 3;
		case GL_INT_VEC4:		   return sizeof(GLint) * 4;
		case GL_UNSIGNED_INT:	   return sizeof(GLuint);
		case GL_UNSIGNED_INT_VEC2: return sizeof(GLuint) * 2;
		case GL_UNSIGNED_INT_VEC3: return sizeof(GLuint) * 3;
		case GL_UNSIGNED_INT_VEC4: return sizeof(GLuint) * 4;
		case GL_BOOL:			   return sizeof(GLboolean);
		case GL_BOOL_VEC2:		   return sizeof(GLboolean) * 2;
		case GL_BOOL_VEC3:		   return sizeof(GLboolean) * 3;
		case GL_BOOL_VEC4:		   return sizeof(GLboolean) * 4;
		case GL_FLOAT_MAT2:		   return sizeof(GLfloat) * 4;
		case GL_FLOAT_MAT2x3:	   return sizeof(GLfloat) * 6;
		case GL_FLOAT_MAT2x4:	   return sizeof(GLfloat) * 8;
		case GL_FLOAT_MAT3:		   return sizeof(GLfloat) * 9;
		case GL_FLOAT_MAT3x2:	   return sizeof(GLfloat) * 6;
		case GL_FLOAT_MAT3x4:	   return sizeof(GLfloat) * 12;
		case GL_FLOAT_MAT4:		   return sizeof(GLfloat) * 16;
		case GL_FLOAT_MAT4x2:	   return sizeof(GLfloat) * 8;
		case GL_FLOAT_MAT4x3:	   return sizeof(GLfloat) * 12;
		default:
			LOG_EXT_FATAL("Unknown type: 0x%x\n", type);
			return 0;
		}
	}

	void GLSLShaderProgram::getSBBOVarOffsets(
		rd::SBBO* sbbo,
		int varNum, const char* const*  varNames)
	{
		assert(nullptr != sbbo);

		GLuint indices[32];
		assert(varNum <= ARRAYLENGTH(indices));
		glGetUniformIndices(m_handle, varNum, varNames, indices);
		auto &outOffsets = sbbo->properties.ubo.varOffsets;
		auto &outSize = sbbo->properties.ubo.varSize;
		glGetActiveUniformsiv(m_handle, varNum, indices, GL_UNIFORM_SIZE, outSize);
		glGetActiveUniformsiv(m_handle, varNum, indices, GL_UNIFORM_TYPE, outOffsets);
		for (uint i = 0; i != varNum; ++i)
			outSize[i] *= getTypeSize(outOffsets[i]);

		glGetActiveUniformsiv(m_handle, varNum, indices, GL_UNIFORM_OFFSET, outOffsets);
	}
	
	void GLSLShaderProgram::freeSharedBlockBuffer(rd::SBBO * sbbo)
	{
		if (sbbo)
		{
			GLuint ubo = sbbo->getHandle();
			glDeleteBuffers(1, &ubo);
			/* For the whole life cycle of the engine, all
			the shared block buffers must be generated
			by the same render driver, so it's safe to
			use static_cast */
			UniformBlock* ub = static_cast<UniformBlock*>(sbbo);
			delete ub;
		}
	}
	
	struct GLVersion
	{
		unsigned int ver[3] = { 1, 1, 0 };

		GLVersion() {}
		GLVersion(uint v0, uint v1, uint v2)
		{
			ver[0] = v0;
			ver[1] = v1;
			ver[2] = v2;
		}
		bool operator>=(const GLVersion &var)
		{
			return (ver[0] >= var.ver[0] &&
				ver[1] >= var.ver[1] &&
				ver[2] >= var.ver[2]);
		}
	};
	static GLVersion glver;
	static int wndWidth, wndHeight;

	static void getGLVersion()
	{
		const char* glString = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		if (glString)
		{
			char buffer[64];
			io::Logger::log(io::Logger::LOG_INFO, "OpenGL version %s\n", glString);
#ifdef _WIN32
			strncpy_s(buffer, glString, 64);
			buffer[63] = '\0';

			char* context[3] = { nullptr, nullptr, nullptr };
			char* ver = strtok_s(buffer, " ", context);
			glver.ver[0] = std::atoi(strtok_s(ver, ".", context));
			glver.ver[1] = std::atoi(strtok_s(nullptr, ".", context));
			glver.ver[2] = std::atoi(strtok_s(nullptr, ".", context));
#else
			std::strncpy(buffer, glString, 64);
			buffer[63] = '\0';
			char* ver = std::strsep(buffer, " ");
			glver.ver[0] = std::atoi(std::strsep(ver, "."));
			glver.ver[1] = std::atoi(std::strsep(nullptr, "."));
			glver.ver[2] = std::atoi(std::strsep(nullptr, "."));
#endif
			//printf("glVersion = %d.%d.%d\n", m_version[0], m_version[1], m_version[2]);
			assert(glver >= GLVersion(3, 3, 0) && "OpenGL version is too low");
		}
	}
	
	void GLRenderDriver::init()
	{
		glewInit();
		getGLVersion();
	}
	void GLRenderDriver::beginFrame()
	{

	}
	void GLRenderDriver::endFrame()
	{
		glFinish();
	}
	void GLRenderDriver::setWindowSize(int width, int height)
	{
		wndWidth = width;
		wndHeight = height;
	}
	void GLRenderDriver::getWindowSize(int &width, int &height)
	{
		width = wndWidth;
		height = wndHeight;
	}
	void GLRenderDriver::setViewPort(int left, int bottom, int width, int height)
	{
		glViewport(left, bottom, width, height);
	}

	void GLRenderDriver::enableFunction(rd::RenderFuns rf, bool enable)
	{
		if (enable)
		{
			switch (rf)
			{
			case rd::RF_DEPTH_TEST:		glEnable(GL_DEPTH_TEST); break;
			case rd::RF_STENCIL_TEST:	glEnable(GL_STENCIL_TEST); break;
			case rd::RF_CULL_FACE:		glEnable(GL_CULL_FACE); break;
			case rd::RF_BLEND:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				//	GL_ONE, GL_ZERO);
				break;
			case rd::RF_DRAW_LINE:		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
			case rd::RF_DRAW_FACES:		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
			case rd::RF_VSYNC:
#if defined(_WIN32) && defined(WGL_EXT_swap_control)
				if (wglSwapIntervalEXT)
					wglSwapIntervalEXT(1);
#elif defined(GLX_EXT_swap_control)
				glXSwapIntervalEXT();
#endif
				break;
			default: break;
			}
		}
		else
		{
			switch (rf)
			{
			case rd::RF_DEPTH_TEST:		glDisable(GL_DEPTH_TEST); break;
			case rd::RF_STENCIL_TEST:	glDisable(GL_STENCIL_TEST);	break;
			case rd::RF_CULL_FACE:		glDisable(GL_CULL_FACE); break;
			case rd::RF_BLEND:			glDisable(GL_BLEND); break;
			case rd::RF_DRAW_LINE:		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
			case rd::RF_DRAW_FACES:		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
			case rd::RF_VSYNC:
#if defined(_WIN32) && defined(WGL_EXT_swap_control)
				if (wglSwapIntervalEXT)
					wglSwapIntervalEXT(0);
#elif defined(GLX_EXT_swap_control)
				glXSwapIntervalEXT();
#endif
				break;
			default: break;
			}
		}
	}

	bool GLRenderDriver::getVsyncState()
	{
#if defined(_WIN32) && defined(WGL_EXT_swap_control)
		return (wglGetSwapIntervalEXT() > 0);
#else
#error "Unsupported OS yet(GLRenderDriver::getVsyncState())"
		return false;
#endif
	}

	rd::TBO GLRenderDriver::loadNewTexture(rd::RawImage2D * img2D, int alignment)
	{
		GLuint tbo;
		glGenTextures(1, &tbo);
		glBindTexture(GL_TEXTURE_2D, tbo);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (alignment)
			glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
		glTexStorage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,//img2D->glColorFmt,
			img2D->width,
			img2D->height);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA8, //img2D->glColorFmt,
			img2D->width, img2D->height,
			0,
			img2D->glColorFmt, img2D->glDataType,
			img2D->data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return rd::TBO(tbo);
	}
	void GLRenderDriver::switchTexture(rd::TBO &tbo, uint slotIndex)
	{
		if (m_tboSlots[slotIndex] != tbo.getHandle())
		{
			glActiveTexture(GL_TEXTURE0 + slotIndex);
			glBindTexture(GL_TEXTURE_2D, tbo.getHandle());
			m_tboSlots[slotIndex] = tbo.getHandle();
		}
	}
	void GLRenderDriver::freeTextures(rd::TBO *tbos, uint tboNum)
	{
		/* No need to call glIsTexture() because
		   glDeleteTextures(1, &tbo) is safe when tbo == 0 */
		if (512 < tboNum)
		{
			GLuint tboHdls[512];
			for (uint i = 0; i != tboNum; ++i)
				tboHdls[i] = tbos[i].getHandle();
			glDeleteTextures(tboNum, tboHdls);
			for (uint i = 0; i != tboNum; ++i)
				tbos[i].setHandle(0);
		}
		else
		{
			GLuint *tboHdls = new GLuint[tboNum];
			for (uint i = 0; i != tboNum; ++i)
				tboHdls[i] = tbos[i].getHandle();
			glDeleteTextures(tboNum, tboHdls);
			delete [] tboHdls;
			for (uint i = 0; i != tboNum; ++i)
				tbos[i].setHandle(0);
		}
	}

	void GLRenderDriver::loadRenderMesh(
		io::Model3D::Mesh & mesh,
		rd::RenderMesh & out)
	{
		GLuint vao, vbo;
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER,
			mesh.vertexDataSize,
			mesh.vertexData,
			GL_STATIC_DRAW);
		//Vertex positions
		if (mesh.posSize)
		{
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
				sizeof(float) * 3,
				(void*)POINTOFFSET(mesh.vertexData, mesh.positionPtr));
			glEnableVertexAttribArray(0);
		}
		else
			glDisableVertexAttribArray(0);
		//Vertex normals
		if (mesh.normalSize)
		{
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
				sizeof(float) * 3,
				(void*)POINTOFFSET(mesh.vertexData, mesh.normalPtr));
			glEnableVertexAttribArray(1);
		}
		else
			glDisableVertexAttribArray(1);
		//Vertex coordination
		if (mesh.coordSize)
		{
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
				sizeof(float) * 3 * 8,
				(void*)POINTOFFSET(mesh.vertexData, mesh.coordPtr));
			glEnableVertexAttribArray(2);
		}
		else
			glDisableVertexAttribArray(2);
		//Vertex bone weight
		if (mesh.boneIndexSize && mesh.boneWeightSize)
		{
			glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT,
				sizeof(uint) * 4,
				(void*)POINTOFFSET(mesh.vertexData, mesh.boneIdx));
			glEnableVertexAttribArray(3); //boneIdx

			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
				sizeof(float) * 4,
				(void*)POINTOFFSET(mesh.vertexData, mesh.boneWeight));
			glEnableVertexAttribArray(4); //boneWeight
		}
		else
		{
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4);
		}

		out.vao.setHandle(vao);
		out.vbo.setHandle(vbo);

		if (mesh.indicesNum)
		{
			GLuint ebo;
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				mesh.indicesNum * sizeof(*mesh.vertIndices),
				mesh.vertIndices,
				GL_STATIC_DRAW);
			out.ebo.setHandle(ebo);
			out.ebo.size = mesh.indicesNum;
		}
		else
		{
			out.ebo.setHandle(0);
			out.ebo.size = 0;
		}

		glBindVertexArray(0);
	}

	void GLRenderDriver::loadRenderMesh(
		uint numVertices, fvec3* posData, fvec3* normalData, fvec2* texCoordData,
		uvec4* boneIndices, fvec4* boneWeights,
		uint numTriangles, uvec3* indices,
		rd::RenderMesh &out)
	{
		GLuint vao, vbo;
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		size_t posSize = numVertices * sizeof(fvec3);
		size_t normalSize = posSize;
		size_t coordSize = numVertices * sizeof(fvec2);
		size_t indicesSize = numTriangles * sizeof(uvec3);
		size_t boneIndicesSize = numVertices * sizeof(uvec4);
		size_t boneWeightSize = numVertices * sizeof(fvec4);
		size_t bufSize = posSize + normalSize + coordSize +
			indicesSize +
			boneIndicesSize + boneWeightSize;
		size_t offset = 0;
		glBufferData(GL_ARRAY_BUFFER, bufSize, NULL, GL_STATIC_DRAW);
		if (posData)//Vertex positions
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset, posSize, posData);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
				sizeof(fvec3), (void*)offset);
			glEnableVertexAttribArray(0);
			offset += posSize;
		}
		if (normalData)//Vertex normals
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset, normalSize, normalData);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
				sizeof(fvec3), (void*)offset);
			glEnableVertexAttribArray(1);
			offset += normalSize;
		}
		if (texCoordData)//Vertex coordination
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset, coordSize, texCoordData);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
				sizeof(fvec2), (void*)offset);
			glEnableVertexAttribArray(2);
			offset += coordSize;
		}
		if (boneIndices && boneWeights)//Vertex bone weight
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset, boneIndicesSize, boneIndices);
			glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT,
				sizeof(uvec4), (void*)offset);
			glEnableVertexAttribArray(3); //boneIdx
			offset += boneIndicesSize;
			glBufferSubData(GL_ARRAY_BUFFER, offset, boneWeightSize, boneWeights);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
				sizeof(fvec4), (void*)offset);
			glEnableVertexAttribArray(4); //boneWeight
			offset += boneWeightSize;
		}
		out.vao.setHandle(vao);
		out.vbo.setHandle(vbo);

		if (indices)
		{
			GLuint ebo;
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				numTriangles * sizeof(uvec3),
				indices,
				GL_STATIC_DRAW);
			out.ebo.setHandle(ebo);
			out.ebo.size = numTriangles * 3;
		}
		else
		{
			out.ebo.setHandle(0);
			out.ebo.size = 0;
		}

		glBindVertexArray(0);
	}

	void GLRenderDriver::drawRenderMesh(rd::RenderMesh & mesh)
	{
		GLuint vao = mesh.vao.getHandle();
		if (mesh.mat)
			switchMaterial(*mesh.mat);
		if (mesh.ebo.isValid())
		{
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, mesh.ebo.size, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void GLRenderDriver::freeRenderMesh(rd::RenderMesh &rdrMesh)
	{
		GLuint vao = rdrMesh.vao.getHandle();
		GLuint vbo = rdrMesh.vbo.getHandle();
		GLuint ebo = rdrMesh.ebo.getHandle();
		if (ebo)
		{
			glDeleteBuffers(1, &ebo);
			rdrMesh.ebo.size = 0;
			rdrMesh.ebo.setHandle(0);
		}
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		rdrMesh.vao.setHandle(0);
		rdrMesh.vbo.setHandle(0);
	}

	void GLRenderDriver::clearFramebuffer()
	{
		glClear(GL_COLOR_BUFFER_BIT |
			GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);
	}
}