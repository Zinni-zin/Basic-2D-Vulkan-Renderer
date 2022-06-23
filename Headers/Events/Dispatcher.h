#pragma once

#include <functional>
#include <vector>

namespace ZVK
{
	template<typename T>
	class Dispatcher
	{
	public:
		using EventFnc = std::function<void(T&)>;

		inline void Attach(EventFnc& fnc) { m_funcs.push_back(&fnc); }

		EventFnc* Detach(const EventFnc& fnc)
		{
			for (auto it = m_funcs.begin(); it != m_funcs.end(); ++it)
			{
				if (*it == &fnc)
				{
					EventFnc* eFnc = *it;
					m_funcs.erase(it);
					return eFnc;
				}
			}

			return nullptr;
		}

		void Notify(T& event)
		{
			for (auto it = m_funcs.begin(); it != m_funcs.end(); ++it)
				(*(*it))(event);
		}

	private:
		std::vector<EventFnc*> m_funcs;
	};
}