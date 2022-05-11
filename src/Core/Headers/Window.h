#pragma once

#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"

#include <string>

#include "../../Events/Events.h"

#include "../../Math/Vectors.h"

namespace ZVK
{
	class ZWindow
	{
	private:
		class DispatcherInfo;

	public:
		ZWindow(const std::string& title, int width, int height);
		ZWindow(const std::string& title, Vec2i dimensions);
		~ZWindow();

		ZWindow(const ZWindow&) = delete;
		ZWindow& operator=(const ZWindow&) = delete;

		inline GLFWwindow* GetWindow() const { return p_window; }

		inline Vec2i GetDimensions() const { return m_dimensions; }
		inline int GetWidth() const { return m_dimensions.x; }
		inline int GetHight() const { return m_dimensions.y; }

		inline bool ShouldClose() const { return m_isWindowClosed;  }
		inline bool IsVsync() const { return m_isVsync; }

		inline bool SetVsync(bool val) { m_isVsync = val; }
	
		inline static DispatcherInfo& GetDispatchers() { return *p_dispatcher; }

	private:
		
		void init(const std::string& title, int width, int height);

		void initCallbakcs();

		void windowCloseEvent(WindowClosedEvent& e);

	private:
		GLFWwindow* p_window;
		Vec2i m_dimensions;
		bool m_isVsync;

		bool m_isWindowClosed = false;

		static std::unique_ptr<DispatcherInfo> p_dispatcher;
		std::function<void(WindowClosedEvent&)> m_windowCloseEvent;

	private:
		class DispatcherInfo
		{
		public:
			Dispatcher<MouseScrolledEvent> MouseScrolled;
			Dispatcher<FrameBufferResizedEvent> FrameBufferResize;
			Dispatcher<WindowClosedEvent> WindowClosed;
		};
	};
}