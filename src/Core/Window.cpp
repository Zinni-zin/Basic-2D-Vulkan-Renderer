#include "Headers/Window.h"

#include "Headers/Core.h"

namespace ZVK
{
	std::unique_ptr<ZWindow::DispatcherInfo> ZWindow::p_dispatcher = std::make_unique<DispatcherInfo>();

	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void windowCloseCallback(GLFWwindow* window);

	ZWindow::ZWindow(const std::string& title, int width, int height)
		: m_dimensions(width, height), m_isVsync(true), p_window(nullptr)
	{
		init(title, width, height);
	}

	ZWindow::ZWindow(const std::string& title, Vec2i dimensions)
		: m_dimensions(dimensions), m_isVsync(true), p_window(nullptr)
	{

		init(title, dimensions.x, dimensions.y);
	}

	ZWindow::~ZWindow()
	{
		glfwDestroyWindow(p_window);

		p_dispatcher->WindowClosed.Detach(m_windowCloseEvent);
	}

	void ZWindow::init(const std::string& title, int width, int height)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		p_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

		m_windowCloseEvent = std::bind(&ZWindow::windowCloseEvent, std::ref(*this),
			std::placeholders::_1);

		p_dispatcher->WindowClosed.Attach(m_windowCloseEvent);

		initCallbakcs();
	}

	void ZWindow::initCallbakcs()
	{
		glfwSetScrollCallback(p_window, mouseScrollCallback);
		glfwSetFramebufferSizeCallback(p_window, framebufferResizeCallback);
		glfwSetWindowCloseCallback(p_window, windowCloseCallback);
	}



	void ZWindow::windowCloseEvent(WindowClosedEvent& e)
	{
		m_isWindowClosed = true;
	}

	// Mouse Scroll
	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		MouseScrolledEvent e(xOffset, yOffset);

		ZWindow::GetDispatchers().MouseScrolled.Notify(e);
	}

	void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		FrameBufferResizedEvent e(width, height);

		ZWindow::GetDispatchers().FrameBufferResize.Notify(e);
	}

	void windowCloseCallback(GLFWwindow* window)
	{
		WindowClosedEvent e;
		ZWindow::GetDispatchers().WindowClosed.Notify(e);
	}
}