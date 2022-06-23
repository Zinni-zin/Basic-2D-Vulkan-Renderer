#pragma once

#include <string>

namespace ZVK
{
	class IEvent
	{
	public:
		virtual ~IEvent() { }

		inline virtual const std::string GetEventName() const = 0;
	};
}