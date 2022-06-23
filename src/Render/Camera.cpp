#include "../../Headers/Render/Camera.h"

#include <iostream>

#include "../../Headers/Core/Window.h"

#include "../../Headers/Core/Core.h"

namespace ZVK
{
	Camera::Camera(int viewWidth, int viewHeight, bool canUseScrollEvent, float initZoomLevel)
		: m_viewWidth(viewWidth), m_viewHeight(viewHeight), m_zoomLevel(initZoomLevel),
		m_aspectRatio((float)viewWidth/(float)viewHeight), m_zoomSpeed(100.f), m_view(1.f),
		m_input(Input::GetInput()), m_canUseScrollEvent(canUseScrollEvent),
		m_translationSpeed(initZoomLevel / (initZoomLevel * 200))
	{
		m_mouseScrolledEvent = std::bind(&Camera::onMouseScrolled, std::ref(*this),
			std::placeholders::_1);

		m_swapchainRecreateEvent = std::bind(&Camera::onSwapchainRecreated, std::ref(*this),
			std::placeholders::_1);

		ZWindow::GetDispatchers().MouseScrolled.Attach(m_mouseScrolledEvent);
		Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);

		SetProjection(0.f, (float)viewWidth, (float)viewHeight, 0.f);
	}

	Camera::~Camera()
	{
		ZWindow::GetDispatchers().MouseScrolled.Detach(m_mouseScrolledEvent);
		Core::GetCore().GetSwapchainRecreateDispatcher().Detach(m_swapchainRecreateEvent);
	}

	void Camera::Update()
	{
		bool canUpdateView = false;

		// Move camera left or right
		if (m_input.IsKeyPressed(m_moveLeftKey))
		{
			m_pos.x += m_translationSpeed;
			canUpdateView = true;
		}
		else if (m_input.IsKeyPressed(m_moveRightKey))
		{
			m_pos.x -= m_translationSpeed;
			canUpdateView = true;
		}

		// Move camera up or down
		if (m_input.IsKeyPressed(m_moveUpKey))
		{
			m_pos.y += m_translationSpeed;
			canUpdateView = true;
		}
		else if (m_input.IsKeyPressed(m_moveDownKey))
		{
			m_pos.y -= m_translationSpeed;
			canUpdateView = true;
		}

		if (canUpdateView)
			calcViewMatrix();
	}

	Vec2 Camera::GetMousePos() const
	{
		Vec2 mousePos = Input::GetInput().GetMousePos();

		float mX = mousePos.x / (m_viewWidth * 0.5f) - 1.0f;
		float mY = mousePos.y / (m_viewHeight * 0.5f) - 1.0f;

		Vec4 screenPos(mX, -mY, 1.0f, 1.0f);
		Vec4 worldPos = m_pvInverse * screenPos;

		return { worldPos.x, worldPos.y };
	}

	Vec2 Camera::GetScreenPos(const Vec2& pos) const
	{
		Vec4 clipSpace = m_proj * (m_view * Vec4(pos.x, pos.y, 1.0f, 1.0f));
		Vec3 ndcSpace = Vec3(clipSpace.x / clipSpace.w, clipSpace.y / clipSpace.w, clipSpace.z / clipSpace.w);

		float x = ((ndcSpace.x + 1.0f) / 2.0f) * (float)m_viewWidth;
		float y = ((ndcSpace.y + 1.0f) / 2.0f) * (float)m_viewHeight;

		return { x, y };
	}

	void Camera::SetProjection(float left, float right, float bottom, float top)
	{
		m_proj = Mat4::OrthoLH(left, right, bottom, top, -1.f, 1.f);
		m_pv = m_proj * m_view;
		m_mvp = m_proj * m_view * m_model;
		m_pvInverse = Mat4::Inverse(m_pv);
	}

	void Camera::calcViewMatrix()
	{
		m_view = Mat4::Translate(Vec3(m_pos, 0.f));

		m_pv = m_proj * m_view;
		m_mvp = m_proj * m_view * m_model;
		m_pvInverse = Mat4::Inverse(m_pv);
	}

	void Camera::onMouseScrolled(MouseScrolledEvent& e)
	{
		if (m_canUseScrollEvent)
		{
			float prevZoomLevel = m_zoomLevel;

			m_zoomLevel -= (float)e.GetYOffset() * m_zoomSpeed;
			m_zoomLevel = std::max(m_zoomLevel, 0.25f);

			Vec2 mPos = GetMousePos();

			// Try to zoom in where the mouse cursor is
			if (prevZoomLevel > m_zoomLevel)
				SetProjection(mPos.x, (m_aspectRatio * m_zoomLevel) + mPos.x, m_zoomLevel + mPos.y, mPos.y);
			else
			{ // Try to zoom out via the center of the screen
				SetProjection(m_pos.x + (m_viewWidth / 2),
					(m_aspectRatio * m_zoomLevel) + m_pos.x + (m_viewWidth / 2),
					m_zoomLevel + m_pos.y + (m_viewHeight / 2), m_pos.y + (m_viewHeight / 2));
			}
		
			m_translationSpeed = m_zoomLevel / (m_zoomLevel * 200);
		}
	}

	void Camera::onSwapchainRecreated(SwapchainRecreateEvent& e)
	{
		m_viewWidth = e.GetWidth();
		m_viewHeight = e.GetHeight();
		SetProjection(0.f, (float)m_viewWidth, (float)m_viewHeight, 0.f);
	}

	void Camera::SetMoveKeys(KeyCode right, KeyCode left, KeyCode up, KeyCode down)
	{
		m_moveRightKey = right;
		m_moveLeftKey = left;
		m_moveUpKey = up;
		m_moveDownKey = down;
	}
}