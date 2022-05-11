#include "Input.h"

#include "../Core/Headers/Core.h"

namespace ZVK
{
	Input::Input()
	{
		m_window = Core::GetCore().GetGLFWWindnow();
	}

	bool Input::IsKeyPressed(KeyCode keycode)
	{
		auto state = glfwGetKey(m_window, (int)keycode);

		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsKeyReleased(KeyCode keycode)
	{
		auto state = glfwGetKey(m_window, (int)keycode);

		return state == GLFW_RELEASE;
	}

	bool Input::IsMouseButtonPressed(MouseCode mouseCode)
	{
		auto state = glfwGetMouseButton(m_window, (int)mouseCode);

		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonReleased(MouseCode mouseCode)
	{
		auto state = glfwGetMouseButton(m_window, (int)mouseCode);

		return state == GLFW_RELEASE;
	}

	Vec2 Input::GetMousePos() const
	{
		double xPos, yPos;

		glfwGetCursorPos(m_window, &xPos, &yPos);

		// Due to Vulkan drawing on the bottom left we have to flip the mouse's Y pos
		yPos = -1.f * (yPos - Core::GetCore().GetSwapchain()->GetSwapchainExtent().height);

		return { (float)xPos, (float)yPos };
	}

	Input& Input::GetInput()
	{
		static Input keyInput;
		return keyInput;
	}
}