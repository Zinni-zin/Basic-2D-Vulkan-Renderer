#pragma once

#include "IShapes.h"

namespace ZVK
{
	class Rectangle : public IShape
	{
	public:
		Rectangle(Vec3 pos, Vec3 dimensions, Vec4 colour = Vec4::One())
			: IShape(pos, dimensions, colour)
		{}

		~Rectangle() override {}

		ShapeType GetShapeType() const override { return ShapeType::RECTANGLE; }
	};
}