#pragma once

#include "../IEvent.h"

namespace ZVK
{
	class SwapchainCleanupEvent : public IEvent
	{
	public:
		SwapchainCleanupEvent() { }

		inline const std::string GetEventName() const override { return "Swapchain Cleanup Event"; }
	};
}