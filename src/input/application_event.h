#pragma once

#include "event.h"

#include <sstream>

namespace saf {

	class WindowEvent : public Event
	{
	public:
		EVENT_CLASS_CATEGORY(EventCategoryWindow);
	};

	class WindowCloseEvent : public WindowEvent
	{
	public:
		WindowCloseEvent() {}

		EVENT_CLASS_TYPE(WindowClose);
	};

	class WindowResizeEvent : public WindowEvent
	{
	public:
		WindowResizeEvent(int width, int height)
			: m_Width(width), m_Height(height)
		{
		
		}

		inline const int& GetWidth() const { return m_Width; }
		inline const int& GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize);
	private:
		int m_Width, m_Height;
	};

	class WindowFocusEvent : public WindowEvent
	{
	public:
		WindowFocusEvent() {}

		EVENT_CLASS_TYPE(WindowFocus);
	};

	class WindowLostFocusEvent : public WindowEvent
	{
	public:
		WindowLostFocusEvent() {}

		EVENT_CLASS_TYPE(WindowLostFocus);
	};

	class WindowMovedEvent : public WindowEvent
	{
	public:
		WindowMovedEvent(int x, int y)
			: m_WindowX(x), m_WindowY(y)
		{
		
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowMovedEvent: " << m_WindowX << ", " << m_WindowY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowMoved);
	private:
		int m_WindowX, m_WindowY;
	};

	class AppEvent : public Event
	{
	public:
		EVENT_CLASS_CATEGORY(EventCategoryApplication);
	};

	class AppTickEvent : public AppEvent
	{
	public:
		AppTickEvent() {}

		EVENT_CLASS_TYPE(AppTick);
	};

	class AppUpdateEvent : public AppEvent
	{
	public:
		AppUpdateEvent() {}

		EVENT_CLASS_TYPE(AppUpdate);
	};

	class AppRenderEvent : public AppEvent
	{
	public:
		AppRenderEvent() {}

		EVENT_CLASS_TYPE(AppRender);
	};

}