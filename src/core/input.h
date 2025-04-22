#pragma once

#include <bitset>
#include <GLFW/glfw3.h>
#include "input/event.h"

namespace saf {

	class Input
	{
	public:
		Input();

		inline bool KeyDown(int code) const { return m_Keys[code]; }
		inline int GetMouseX() const { return m_MouseX; }
		inline int GetMouseY() const { return m_MouseY; }
		inline int GetMouseDX() const { return m_MouseDX; }
		inline int GetMouseDY() const { return m_MouseDY; }
		inline int GetScrollX() const { return m_ScrollX; }
		inline int GetScrollY() const { return m_ScrollY; }

		void OnEvent(Event& e);
	private:
		std::bitset<GLFW_KEY_LAST> m_Keys;
		std::bitset<GLFW_MOUSE_BUTTON_LAST> m_MouseButtons;
		int m_MouseX;
		int m_MouseY;
		int m_MouseDX;
		int m_MouseDY;
		int m_ScrollX;
		int m_ScrollY;
	};

}