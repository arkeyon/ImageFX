#pragma once

#include "Event.h"

namespace saf {

	class MouseEvent : public Event
	{
	public:
		EVENT_CLASS_CATEGORY(EventCategoryMouse);
	};

	class MouseMovedEvent : public MouseEvent
	{
	public:
		MouseMovedEvent(float x, float y, float dx = 0.f, float dy = 0.f)
			: m_MouseX(x), m_MouseY(y), m_DMouseX(dx), m_DMouseY(dy)
		{

		}

		inline const float& GetX() const { return m_MouseX; };
		inline const float& GetY() const { return m_MouseY; };
		inline const float& GetDX() const { return m_DMouseX; };
		inline const float& GetDY() const { return m_DMouseY; };

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved);

	private:
		float m_MouseX, m_MouseY;
		float m_DMouseX, m_DMouseY;
	};

	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent(float scrollx, float scrolly)
			: m_ScrollX(scrollx), m_ScrollY(scrolly)
		{

		}

		inline const float& GetScrollX() const { return m_ScrollX; };
		inline const float& GetScrollY() const { return m_ScrollY; };

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_ScrollX << ", " << m_ScrollY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled);

	private:
		float m_ScrollX, m_ScrollY;
	};

	class MouseButtonEvent : public MouseEvent
	{
	public:

		MouseButtonEvent(int button)
			: m_Button(button)
		{

		}

		inline const int& GetButtonCode() const { return m_Button; }
	protected:
		int m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button)
		{

		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed);
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button)
		{

		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased);
	};
}