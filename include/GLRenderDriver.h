#pragma once
#include "IRenderDriver.h"

namespace toy
{
	class GLRenderDriver : public IRenderDriver
	{
	public:
		void init() override;

		rd::RenderDriverType getType() override { return rd::RDT_OPENGL; }

		void beginFrame() override;
		void endFrame() override;

		void setWindowSize(int width, int height) override;
		void getWindowSize(int &width, int &height) override;

		void setViewPort(int left, int bottom, int width, int height) override;

		void enableFunction(rd::RenderFuns rf, bool enable) override;
		bool getVsyncState();

		rd::TBO loadNewTexture(rd::RawImage2D *img2D, int alignment = 0) override;
		void switchTexture(rd::TBO &tbo, uint slotIndex = 0) override;
		void freeTextures(rd::TBO *tbos, uint tboNum) override;

		void loadRenderMesh(io::Model3D::Mesh &mesh, rd::RenderMesh &out) override;
		void loadRenderMesh(
			uint numVertices, fvec3* posData, fvec3* normalData, fvec2* texCoordData,
			uvec4* boneIndices, fvec4* boneWeights,
			uint numTriangles, uvec3* indices,
			rd::RenderMesh &out) override;
		void drawRenderMesh(rd::RenderMesh &mesh) override;
		void freeRenderMesh(rd::RenderMesh &rdrMesh) override;

		void clearFramebuffer() override;
	private:
		uint m_tboSlots[8] = { 0 };
	};
}