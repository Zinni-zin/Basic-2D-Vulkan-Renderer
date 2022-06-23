#pragma once

#include "../IEvent.h"

namespace ZVK
{
	class FrameBufferResizedEvent : public IEvent
	{
	public:
		FrameBufferResizedEvent(int width, int height)
			: m_width(width), m_height(height)
		{ }

		~FrameBufferResizedEvent() override { }

		inline const std::string GetEventName() const override { return "Frame Buffer Resized Event"; };

		inline int GetWidth() const { return m_width; }
		inline int GetHeight() const { return m_height; }

	private:
		int m_width, m_height;
	};
}