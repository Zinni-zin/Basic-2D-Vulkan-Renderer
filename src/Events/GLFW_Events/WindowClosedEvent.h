#pragma once

#include "../IEvent.h"

namespace ZVK
{
	class WindowClosedEvent : public IEvent
	{
	public:
		WindowClosedEvent()
		{ }

		~WindowClosedEvent() override { }

		inline const std::string GetEventName() const override { return "Window Closed Event"; };
	};
}