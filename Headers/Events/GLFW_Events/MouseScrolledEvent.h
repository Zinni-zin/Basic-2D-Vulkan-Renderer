#pragma once

#include "../IEvent.h"

namespace ZVK
{
	class MouseScrolledEvent : public IEvent
	{
	public:
		MouseScrolledEvent(double xOffset, double yOffset)
			: m_xOffset(xOffset), m_yOffset(yOffset)
		{ }

		~MouseScrolledEvent() override { }

		inline const std::string GetEventName() const override { return "Mouse Scrolled Event"; };

		inline double GetXOffset() const { return m_xOffset; }
		inline double GetYOffset() const { return m_yOffset; }

	private:
		double m_xOffset, m_yOffset;
	};
}