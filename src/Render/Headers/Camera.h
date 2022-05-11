#pragma once

#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../../Events/Events.h"

#include "../../Input/Input.h"

#define ROW_MAJOR
#define DEPTH_ZERO_TO_ONE
#include "../../Math/ZMath.h"

namespace ZVK
{
	class Camera
	{
	public:

		Camera(int viewWidth, int viewHeight, bool canUseScrollEvent = false, float initZoomLevel = 500.f);
		~Camera();

		/*
		* Moves camera with key input 
		* Don't call if you want a static camera or for it to move another way
		*/ 
		void Update();

		// Gets the mouse position in world space
		Vec2 GetMousePos() const;

		Vec2 GetScreenPos(const Vec2& pos) const;

		void SetProjection(float left, float right, float bottom, float top);

		// void SetTranslationSpeed(float speed) { if(speed >= 0) m_translationSpeed = speed; }

		inline const Vec2 GetPos()   const { return m_pos; }
		inline const Mat4 GetProj()  const { return m_proj; };
		inline const Mat4 GetView()  const { return m_view; };
		inline const Mat4 GetModel() const { return m_model; };
		inline const Mat4 GetPV()    const { return m_pv; };
		inline const Mat4 GetMVP()   const { return m_mvp; };

		void SetPos(Vec2 pos) { m_pos = pos; calcViewMatrix(); }
		void CanUseScrollEvent(bool value) { m_canUseScrollEvent = value; }

		inline void SetMoveRightKey(KeyCode key) { m_moveRightKey = key; }
		inline void SetMoveLeftKey(KeyCode key)  { m_moveLeftKey  = key; }
		inline void SetMoveUpKey(KeyCode key)    { m_moveUpKey    = key; }
		inline void SetMoveDownKey(KeyCode key)  { m_moveDownKey  = key; }

		void SetMoveKeys(KeyCode right, KeyCode left, KeyCode up, KeyCode down);

	private:
		
		void calcViewMatrix();
		
		void onMouseScrolled(MouseScrolledEvent& e);

	private:
		
		Vec2 m_pos;
		
		Mat4 m_proj;
		Mat4 m_view;
		Mat4 m_model;
		Mat4 m_pv;
		Mat4 m_mvp;
		Mat4 m_pvInverse;

		float m_aspectRatio;
		float m_translationSpeed = 0.f;
		float m_zoomLevel, m_zoomSpeed;

		int m_viewWidth, m_viewHeight;

		std::function<void(MouseScrolledEvent&)> m_mouseScrolledEvent;

		Input& m_input;
		KeyCode m_moveRightKey = KeyCode::D;
		KeyCode m_moveLeftKey = KeyCode::A;
		KeyCode m_moveUpKey = KeyCode::W;
		KeyCode m_moveDownKey = KeyCode::S;
		bool m_canUseScrollEvent = false;
	};
}