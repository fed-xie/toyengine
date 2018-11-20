#include "include/IWindow.h"
#include "include/IO.h"

#ifdef _WIN32
#include <windowsx.h>
#include <map>
#endif

namespace toy
{
#ifdef _WIN32
	const char* IWindow::sm_wcName = "toyWndClassName";
	static std::map<HWND, IWindow*> HWMap;

	static inline IWindow* HWND2WINWINDOW(HWND hwnd)
	{
		auto it = HWMap.find(hwnd);
		return it == HWMap.end() ? nullptr : it->second;
	}
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		IWindow* winWnd = HWND2WINWINDOW(hWnd);
		HIDAdapter *hid = nullptr;
		IRenderDriver *rdr = nullptr;
		if (winWnd)
		{
			hid = winWnd->getHIDAdapter();
			rdr = winWnd->getRenderDriver();
		}
		else
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			hid->keyboardAdapter.setKeyDown(static_cast<u8>(wParam));
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			hid->keyboardAdapter.setKeyUp(static_cast<u8>(wParam));
			break;
		case WM_LBUTTONUP:
			hid->mouseAdapter.setMouseButtonUp(HIDAdapter::MouseAdapter::MK_LEFT);
			ReleaseCapture();
			break;
		case WM_RBUTTONUP:
			hid->mouseAdapter.setMouseButtonUp(HIDAdapter::MouseAdapter::MK_RIGHT);
			ReleaseCapture();
			break;
		case WM_MBUTTONUP:
			hid->mouseAdapter.setMouseButtonUp(HIDAdapter::MouseAdapter::MK_MIDDLE);
			ReleaseCapture();
			break;
		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			hid->mouseAdapter.setMouseButtonDown(HIDAdapter::MouseAdapter::MK_LEFT);
			break;
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);
			hid->mouseAdapter.setMouseButtonDown(HIDAdapter::MouseAdapter::MK_RIGHT);
			break;
		case WM_MBUTTONDOWN:
			SetCapture(hWnd);
			hid->mouseAdapter.setMouseButtonDown(HIDAdapter::MouseAdapter::MK_MIDDLE);
			break;
		case WM_MOUSEMOVE:
			hid->mouseAdapter.setMousePos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_MOUSEWHEEL:
			hid->mouseAdapter.setRollStep(GET_WHEEL_DELTA_WPARAM(wParam) / -WHEEL_DELTA);
			break;
		case WM_SIZE:
			winWnd->setWindowWidthAndHeight(LOWORD(lParam), HIWORD(lParam));
			if(rdr)
				rdr->setWindowSize(LOWORD(lParam), HIWORD(lParam));
				//rdr->setViewPort(0, 0, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			//LOG_EXT_DEBUG("WM_CLOSE/WM_DESTROY message received\n");
			PostQuitMessage(0);
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		default:
			;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	ErrNo IWindow::createWindow(
		const char * title,
		unsigned width,
		unsigned height,
		rd::RenderDriverType rdt)
	{
		DWORD exStyle = 0;
		DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER |
			WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		WNDCLASS wc;
		wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_OWNDC | CS_PARENTDC;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = sm_wcName;
		if (!RegisterClass(&wc))
			return GetLastError();

		m_width = width;
		m_height = height;
		RECT wndSize;
		wndSize.left = wndSize.top = 0;
		wndSize.right = width;
		wndSize.bottom = height;
		AdjustWindowRectEx(&wndSize, style, FALSE, exStyle);

		m_hWnd = CreateWindowEx(
			exStyle,
			sm_wcName,
			title,
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			wndSize.right - wndSize.left,
			wndSize.bottom - wndSize.top,
			NULL,
			NULL,
			m_hInst,
			NULL);
		if (!m_hWnd)
		{
			DWORD err = GetLastError();
			UnregisterClass(sm_wcName, m_hInst);
			return err;
		}

		ShowWindow(m_hWnd, SW_SHOWNORMAL);
		UpdateWindow(m_hWnd);

		return setupRenderDriver(rdt);
	}

	void IWindow::destroy()
	{
		delete m_rdr;
		m_rdr = nullptr;
		if (m_hRC)
		{
			wglMakeCurrent(m_hDC, NULL);
			wglDeleteContext(m_hRC);
		}
		if (m_hDC)
		{
			wglMakeCurrent(NULL, NULL);
			ReleaseDC(m_hWnd, m_hDC);
		}
		if (m_hWnd)
		{
			HWMap[m_hWnd] = nullptr; //Delete the map
			DestroyWindow(m_hWnd);
		}
		UnregisterClass(sm_wcName, m_hInst);
	}

	void IWindow::setTitle(const char * title)
	{
		DWORD_PTR dwResult;
		SendMessageTimeout(m_hWnd, WM_SETTEXT, 0,
			reinterpret_cast<LPARAM>(title),
			SMTO_ABORTIFHUNG, 2000, &dwResult);
		//SetWindowText(m_hWnd, title);
	}

	int IWindow::checkOSMsg()
	{
		while (1)
		{
			if (PeekMessage(&m_msg, NULL, 0, 0, PM_REMOVE))
			{
				if (m_msg.message == WM_QUIT)
				{
					LOGDEBUG("Quit msg received\n");
					return 0;
				}
				TranslateMessage(&m_msg);
				DispatchMessage(&m_msg);
			}
			else
				break;
		}
		return 1;
	}

	ErrNo IWindow::bindToOpenGL()
	{
		int pixelFmt;
		static PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW |
			PFD_SUPPORT_OPENGL |
			PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			24,				// 24-bit color depth
			0, 0, 0, 0, 0, 0,		// color bits ignored
			0,				// no alpha buffer
			0,				// shift bit ignored
			0,				// no accumulation buffer
			0, 0, 0, 0, 			// accum bits ignored
			32,				// 32-bit z-buffer	
			32,				// 32-bit stencil buffer //no stencil buffer
			0,				// no auxiliary buffer
			PFD_MAIN_PLANE,			// main layer
			0,				// reserved
			0, 0, 0				// layer masks ignored
		};

		m_hDC = GetDC(m_hWnd);
		if (!m_hDC)
			return GetLastError();

		if ((pixelFmt = ChoosePixelFormat(m_hDC, &pfd)) == 0)
			return GetLastError();
		if (SetPixelFormat(m_hDC, pixelFmt, &pfd) == FALSE)
			return GetLastError();

		m_hRC = wglCreateContext(m_hDC);
		if (NULL == m_hRC)
		{
			LOG_EXT_FATAL("Fail to create context to openGL!\n");
			return GetLastError();
		}
		if (!wglMakeCurrent(m_hDC, m_hRC))
			return GetLastError();

		return 0;
	}

	ErrNo IWindow::setupRenderDriver(rd::RenderDriverType rdt)
	{
		if (rd::RenderDriverType::RDT_OPENGL == rdt)
		{
			ErrNo err = bindToOpenGL();
			if (err)
			{
				LOG_EXT_ERROR("Fail to create OpenGL env(%lld)\n", err);
				DestroyWindow(m_hWnd);
				UnregisterClass(sm_wcName, m_hInst);
				return err;
			}
			m_rdr = new GLRenderDriver;
			m_rdr->init();
			m_rdr->setWindowSize(m_width, m_height);
		}

		HWMap.insert(::std::make_pair(m_hWnd, this));
		return 0;
	}
#endif //_WIN32
}