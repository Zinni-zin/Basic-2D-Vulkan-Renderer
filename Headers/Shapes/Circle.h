#pragma once

#include "IShapes.h"

namespace ZVK
{
	class Circle : public IShape
	{
	public:
		Circle(Vec3 pos, Vec3 dimensions, Vec4 colour = Vec4::One())
			: IShape(pos, dimensions, colour, 1.f, 0.005f)
		{}

		~Circle() override {}

		ShapeType GetShapeType() const override { return ShapeType::CIRCLE; }

		void SetThickness(float thickness) { m_circleThickness = thickness; }
		void SetFade(float fade) { m_circleFade = fade; }
	};
}