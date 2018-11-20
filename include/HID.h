#pragma once
#include "Types.h"

namespace toy
{
	class HIDAdapter
	{
	public:
		class MouseAdapter
		{
		public:
			enum MouseKey { MK_LEFT, MK_RIGHT, MK_MIDDLE, MK_MAX };
			void init() {}

			void setMousePos(int x, int y) { m_state.posX = x; m_state.posY = y; }
			void getMousePos(int &x, int &y) { x = m_state.posX; y = m_state.posY; }
			//void moveMouseTo(int x, int y);

			void setMouseButtonDown(MouseKey key) { m_state.mouseKeysDown[key] = true; }
			bool isMouseButtonDown(MouseKey key) { return m_state.mouseKeysDown[key]; }

			void setMouseButtonUp(MouseKey key) { m_state.mouseKeysDown[key] = false; }
			bool isMouseButtonUp(MouseKey key) { return !m_state.mouseKeysDown[key]; }

			void setRollStep(int step) { m_state.rollSteps = step; }
			int getRollStep() { return m_state.rollSteps; }
			bool getMouseButtonPress(MouseKey key)
			{
				return isMouseButtonDown(key) && !m_oldState.mouseKeysDown[key];
			}
			bool getMouseBottonRelease(MouseKey key)
			{
				return isMouseButtonUp(key);
			}
			void synchronizeMouseState() { m_oldState = m_state; }
		private:
			struct MouseState
			{
				int posX = 0;
				int posY = 0;
				bool mouseKeysDown[MouseKey::MK_MAX] = { false };
				int rollSteps = 0;
				MouseState operator=(const MouseState& v)
				{
					posX = v.posX;
					posY = v.posY;
					for (register int i = 0; i != MouseKey::MK_MAX; ++i)
						mouseKeysDown[i] = v.mouseKeysDown[i];
					rollSteps = v.rollSteps;
					return *this;
				}
			};
			MouseState m_state;
			MouseState m_oldState;
		};

		class KeyboardAdapter
		{
		public:
			void init() {}
			void synchronizeKeyState()
			{
				for (register int i = 0; i != 256; ++i)
					m_oldKeyIsDown[i] = m_keyIsDown[i];
			}
			void setKeyDown(u8 key) { m_keyIsDown[key] = true; }
			void setKeyUp(u8 key) { m_keyIsDown[key] = false; }
			bool isKeyDown(u8 key) const { return m_keyIsDown[key]; }
			bool isKeyUp(u8 key) const { return !m_keyIsDown[key]; }
			bool getKeyPress(u8 key) const
			{
				return isKeyDown(key) && !m_oldKeyIsDown[key];
			}
			bool getKeyRelease(u8 key) const
			{
				return isKeyUp(key) && m_oldKeyIsDown[key];
			}
		private:
			bool m_keyIsDown[256] = { false };
			bool m_oldKeyIsDown[256] = { false }; //keep the state of the last frame
		};
	public:
		MouseAdapter mouseAdapter;
		KeyboardAdapter keyboardAdapter;
		void init()
		{
			mouseAdapter.init();
			keyboardAdapter.init();
		}
		void synchronizeStates()
		{
			mouseAdapter.synchronizeMouseState();
			keyboardAdapter.synchronizeKeyState();
		}
	};
}