#include "safpch.h"
#include "input.h"

#include "input/key_event.h"
#include "input/mouse_event.h"

namespace saf {

	Input::Input()
		: m_Keys(false), m_MouseX(0), m_MouseY(0), m_MouseDX(0), m_MouseDY(0), m_ScrollX(0), m_ScrollY(0)
	{

	}

	void Input::OnEvent(Event& e)
	{
		EventDispatcher d(e);

		d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				m_Keys[e.GetKeyCode()] = true;
				return false;
			});

		d.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& e)
			{
				m_Keys[e.GetKeyCode()] = false;
				return false;
			});

		d.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
			{
				m_MouseButtons[e.GetButtonCode()] = true;
				return false;
			});

		d.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e)
			{
				m_MouseButtons[e.GetButtonCode()] = false;
				return false;
			});

		d.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e)
			{
				m_MouseX = e.GetX();
				m_MouseY = e.GetY();
				m_MouseDX = e.GetDX();
				m_MouseDY = e.GetDY();
				return false;
			});

		d.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e)
			{
				m_ScrollX = e.GetScrollX();
				m_ScrollY = e.GetScrollY();
				return false;
			});
	}

}