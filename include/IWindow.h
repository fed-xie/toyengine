#pragma once
#include "HID.h"
#include "IRenderDriver.h"

namespace toy
{
#ifdef _WIN32

	/* Window created by OS */
	class IWindow
	{
	public:
		ErrNo init(HIDAdapter *hid) { m_hid = hid; return 0; }
		ErrNo createWindow(
			const char* title,
			unsigned width,
			unsigned height,
			rd::RenderDriverType rdt);
		void destroy();
		void setTitle(const char* title);
		void setWindowWidthAndHeight(int width, int height)
		{
			m_width = width;
			m_height = height;
		}
		const int getWindowWidth() const { return m_width; }
		const int getWindowHeight() const { return m_height; }
		void swapBuffers() { SwapBuffers(m_hDC); } 
		HIDAdapter* getHIDAdapter() { return m_hid; }
		IRenderDriver* getRenderDriver() { return m_rdr; }
		int checkOSMsg(); //Must be called each frame
	private:
		IRenderDriver * m_rdr = nullptr;
		int m_width = 0;
		int m_height = 0;
		HIDAdapter *m_hid = nullptr;
		ErrNo setupRenderDriver(rd::RenderDriverType rdt);
		ErrNo bindToOpenGL();
		HWND m_hWnd = 0;
		HINSTANCE m_hInst = 0;
		HDC m_hDC = 0;
		HGLRC m_hRC = 0;
		MSG m_msg;
		static const char *sm_wcName;
	};
#endif
}