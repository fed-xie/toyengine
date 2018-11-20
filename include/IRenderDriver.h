#pragma once
#include "Types.h"
#include "Model.h"

namespace toy
{
	namespace rd
	{
		enum RenderDriverType { RDT_OPENGL /*, RDT_DIRECTX9, RDT_VULKAN */ };
		enum ShaderLang { SHADER_GLSL /*, SHADER_CG, SHADER_HLSL */ };
		enum ShaderType
		{ 
			SHADER_VERTEX,
			SHADER_FRAGMENT,
			SHADER_CONTROL,
			SHADER_EVALUATION,
			SHADER_GEOMETRY,
			
			SHADER_TYPENUM
		};
		enum RenderFuns
		{
			RF_DEPTH_TEST,
			RF_STENCIL_TEST,
			RF_CULL_FACE,
			RF_BLEND,
			RF_DRAW_LINE,
			RF_DRAW_FACES,
			RF_VSYNC,
		};
		class BufferObjectHandle
		{
		public:
			explicit BufferObjectHandle(uint handle = 0) : m_handle(handle) {}
			bool isValid() { return m_handle != 0; }
			uint& getHandle() { return m_handle; }
			/* Called by render driver, do NOT use it */
			void setHandle(uint handle) { m_handle = handle; }
			bool operator==(const BufferObjectHandle& v) { return m_handle == v.m_handle; }
			bool operator!=(const BufferObjectHandle& v) { return m_handle != v.m_handle; }
		protected:
			unsigned int m_handle = 0;
		};
		typedef BufferObjectHandle TBO;
		typedef BufferObjectHandle VAO;
		typedef BufferObjectHandle VBO;
		class EBO : public BufferObjectHandle
		{
		public:
			size_t size = 0;
		};
		//Shared block buffer Object
		class SBBO : public BufferObjectHandle
		{
		public:
			size_t size = 0;
			union
			{
				struct
				{
					int varOffsets[32];
					int varSize[32];
				}ubo;
			}properties;
		};
		class Shader : public BufferObjectHandle
		{
		public:
			Shader(uint handle = 0) : BufferObjectHandle(handle) {}
		};
		struct RawImage2D
		{
			u64 size = 0; //image size or sizeof(*data)
			u32 width = 0; //image width
			u32 height = 0;//image height
			unsigned glColorFmt = 0; //color format for openGL
			unsigned glDataType = 0; //unsigned bytes, float, half, etc...

			void* data = nullptr;
		};
		struct Material
		{
			TBO tbos[8];
			bool operator==(const Material &mat)
			{
				for (int i = 0; i != ARRAYLENGTH(tbos); ++i)
				{
					if (tbos[i] != mat.tbos[i])
						return false;
				}
				return true;
			}
			Material& operator=(const Material &mat)
			{
				for (int i = 0; i != ARRAYLENGTH(tbos); ++i)
					tbos[i] = mat.tbos[i];
				return *this;
			}
		};
		struct RenderMesh
		{
			VAO vao;
			VBO vbo;
			EBO ebo;
			Material *mat;
		};
	}
	
	class IShaderProgram
	{
	public:
		virtual void compile(
			rd::Shader* vertexShader,
			rd::Shader* fragmentShader,
			rd::Shader* controlShader,
			rd::Shader* evaluationShader,
			rd::Shader* geometryShader) = 0;
		virtual void compile(
			const char* vertexShaderSource,
			const char* fragmentShaderSource) = 0;
		uint getHandle() { return m_handle; }
		virtual void use() = 0;
		virtual void destroy() = 0;

		virtual void setUniform(const char* varName, const fmat4& value) = 0;
		virtual void setUniform(const char* varName, const fvec3& value) = 0;
		virtual void setUniform(const char* varName, const uint value) = 0;
		virtual void setUniform(const char* varName, const int value) = 0;

		virtual rd::SBBO* genSharedBlockBuffer(const char* blockName) = 0;
		virtual void bindSharedBlockBuffer(rd::SBBO* sbbo, const char* blockName) = 0;
		virtual void getSBBOVarOffsets(
			rd::SBBO* sbbo,
			int varNum, const char* const*  varNames) = 0;
		virtual bool setSharedBlockBufferData(
			rd::SBBO* sbbo, int offset, void* data, size_t dataSize) = 0;
		
		typedef int(*PreDrawFunc)(IShaderProgram* prog, void* args);
		PreDrawFunc preDrawFunc = defPreDrawFunc;

		char name[32];
	protected:
		static int defPreDrawFunc(IShaderProgram* prog, void* args)
		{
			prog->setUniform("MMat", *reinterpret_cast<fmat4*>(args));
			return 0;
		}
		uint m_handle = 0;
	};
	
	class GLSLShaderProgram : public IShaderProgram
	{
	public:
		static rd::Shader* createShader(const char* source, rd::ShaderType type);
		static void freeShader(rd::Shader* &shader);
		void compile(
			rd::Shader* vertexShader,
			rd::Shader* fragmentShader,
			rd::Shader* controlShader,
			rd::Shader* evaluationShader,
			rd::Shader* geometryShader) override;
		void compile(
			const char* vertexShaderSource,
			const char* fragmentShaderSource) override;
		void use() override;
		void destroy() override;

		void setUniform(const char* varName, const fmat4& value) override;
		void setUniform(const char* varName, const fvec3& value) override;
		void setUniform(const char* varName, const uint value) override;
		void setUniform(const char* varName, const int value) override;

		rd::SBBO* genSharedBlockBuffer(const char* blockName) override;
		void bindSharedBlockBuffer(rd::SBBO* sbbo, const char* blockName) override;
		void getSBBOVarOffsets(
			rd::SBBO* sbbo,
			int varNum, const char* const* varNames) override;
		bool setSharedBlockBufferData(
			rd::SBBO* sbbo, int offset, void* data, size_t dataSize) override;

		/* Don't call this function until game exit */
		static void freeSharedBlockBuffer(rd::SBBO *sbbo);
	};
	
	class IRenderDriver
	{
	public:
		virtual void init() = 0;
		virtual rd::RenderDriverType getType() = 0;

		IShaderProgram* createShaderProgram(rd::ShaderLang st);
		IShaderProgram* getShaderProgram(uint index) { return m_shaderProgs[index]; }
		/* Don't call this function until game exit */
		void freeShaderProgram(IShaderProgram* shaderProg);

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual void setWindowSize(int width, int height) = 0;
		virtual void getWindowSize(int &width, int &height) = 0;

		virtual void setViewPort(int left, int bottom, int width, int height) = 0;

		virtual void enableFunction(rd::RenderFuns rf, bool enable) = 0;

		virtual rd::TBO loadNewTexture(rd::RawImage2D *img2D, int alignment = 0) = 0;
		virtual void switchTexture(rd::TBO& tbo, uint slotIndex = 0) = 0;
		virtual void freeTextures(rd::TBO *tbos, uint tboNum) = 0;
		virtual void switchMaterial(rd::Material &mat)
		{
			for (uint i = 0; i != ARRAYLENGTH(mat.tbos); ++i)
			{
				if(mat.tbos[i].isValid())
					switchTexture(mat.tbos[i], i);
			}
		}

		/* generate a render mesh, no material included */
		virtual void loadRenderMesh(io::Model3D::Mesh &mesh, rd::RenderMesh &out) = 0;
		virtual void loadRenderMesh(
			uint numVertices, fvec3* posData, fvec3* normalData, fvec2* texCoordData,
			uvec4* boneIndices, fvec4* boneWeights,
			uint numTriangles, uvec3* indices,
			rd::RenderMesh &out) = 0;
		virtual void drawRenderMesh(rd::RenderMesh &mesh) = 0;
		virtual void freeRenderMesh(rd::RenderMesh &rdrMesh) = 0;

		virtual void clearFramebuffer() = 0;

	protected:
		IShaderProgram* (m_shaderProgs)[20] = { nullptr };
	};
}

#include "GLRenderDriver.h"