#pragma once
#include "../Math/ZMath.h"
#include "../Render/DrawableObject.h"

namespace ZVK
{
	enum class ShapeType { RECTANGLE = 0, CIRCLE = 1 };

	class IShape : public IDrawableObject
	{
	public:
		IShape(Vec3 pos, Vec3 dimensions, Vec4 colour, float circleThickness = 0.f, float circleFade = 0.f)
			: IDrawableObject(pos, dimensions, colour), 
			m_circleThickness(circleThickness), m_circleFade(circleFade)
		{}

		virtual ~IShape() {}

		virtual ShapeType GetShapeType() const = 0;

		// Used for circles, it's in the shape class so it can easily be passed to the shape shader
		const inline float GetCircleThickness() const { return m_circleThickness; }

		// Used for circles, it's in the shape class so it can easily be passed to the shape shader
		const inline float GetCircleFade() const { return m_circleFade; }

	protected:
		float m_circleThickness;
		float m_circleFade;
	};
}