#pragma once

#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define ROW_MAJOR
#include "../Math/ZMath.h"

namespace ZVK
{

	class IDrawableObject
	{
	public:
		IDrawableObject(Vec3 pos, Vec3 dimensions, Vec4 colour = { 1.f, 1.f, 1.f, 1.f })
			: m_pos(pos), m_dimensions(dimensions), m_colour(colour) {}

		virtual ~IDrawableObject() { }

		inline Vec3 GetPos() const { return m_pos; }
		inline Vec3 GetDimensions() const { return m_dimensions; }
		inline Vec3 GetScale() const { return m_scale; }
		inline Vec3 GetRotation() const { return m_rotation; }
		inline Vec4 GetColour() const { return m_colour; }

		inline float GetX() const { return m_pos.x; }
		inline float GetY() const { return m_pos.y; }
		inline float GetZ() const { return m_pos.z; }

		inline float GetWidth()  const { return m_dimensions.x; }
		inline float GetHeight() const { return m_dimensions.y; }
		inline float GetDepth()  const { return m_dimensions.z; }

		inline float GetScaleX() const { return m_scale.x; }
		inline float GetScaleY() const { return m_scale.y; }
		inline float GetScaleZ() const { return m_scale.z; }

		inline float GetRotationX() const { return m_rotation.x; }
		inline float GetRotationY() const { return m_rotation.y; }
		inline float GetRotationZ() const { return m_rotation.z; }

		inline float GetR() const { return m_colour.x; }
		inline float GetG() const { return m_colour.y; }
		inline float GetB() const { return m_colour.z; }
		inline float GetA() const { return m_colour.w; }

		inline void SetPos(Vec3 pos)               { m_pos        = pos;        }
		inline void SetDimensions(Vec3 dimensions) { m_dimensions = dimensions; }
		inline void SetScale(Vec3 scale)           { m_scale      = scale;      }
		inline void SetColour(Vec4 colour)         { m_colour     = colour;     }
		inline void SetRotation(Vec3 rotation)     { m_rotation   = rotation;   }

		inline void SetX(float x) { m_pos.x = x; }
		inline void SetY(float y) { m_pos.y = y; }
		inline void SetW(float z) { m_pos.z = z; }

		inline void SetWidth(float width)   { m_dimensions.x = width;  }
		inline void SetHeight(float height) { m_dimensions.y = height; }
		inline void SetDepth(float depth)   { m_dimensions.z = depth;  }

		inline void SetScale(float scale) { m_scale = Vec3(scale, scale, scale); }
		inline void SetScaleX(float scale) { m_scale.x = scale; }
		inline void SetScaleY(float scale) { m_scale.y = scale; }
		inline void SetScaleZ(float scale) { m_scale.z = scale; }
		
		inline void SetRotationX(float rotation) { m_rotation.x = rotation; }
		inline void SetRotationY(float rotation) { m_rotation.y = rotation; }
		inline void SetRotationZ(float rotation) { m_rotation.z = rotation; }

		inline void SetR(float r) { m_colour.x = r; }
		inline void SetG(float g) { m_colour.y = g; }
		inline void SetB(float b) { m_colour.z = b; }
		inline void SetA(float a) { m_colour.w = a; }

	protected:
		Vec3 m_pos;
		Vec3 m_dimensions;
		Vec3 m_scale = Vec3::One();
		Vec3 m_rotation = Vec3::Zero();
		Vec4 m_colour;
	};
}