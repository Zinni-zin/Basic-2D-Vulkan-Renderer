#pragma once

#include "../IEvent.h"

namespace ZVK
{
	class SwapchainRecreateEvent : public IEvent
	{
	public:
		SwapchainRecreateEvent(const int width, const int height) 
			: m_width(width), m_height(height)
		{}

		inline const std::string GetEventName() const override { return "Swapchain Recreate Event"; }

		inline int GetWidth() const { return m_width;   }
		inline int GetHeight() const { return m_height; }
	private:
		int m_width, m_height;
	};
}