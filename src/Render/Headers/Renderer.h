#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "Swapchain.h"
#include "../../Events/Events.h"

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

namespace ZVK
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		void Begin();
		void End();
		inline VkCommandBuffer& GetCurrentCommandBuffer() { return m_cmdBuffers[m_curFrame]; }
		inline uint32_t GetImageIndex() const { return m_imageIndex; }
		inline uint32_t GetCurFrame() const { return m_curFrame; }
		inline bool isFrameInProgress() const { return m_isFrameStarted; }

	private:
		void createCommandBuffers();
		void createSyncObjects();

		void windowCloseEvent(WindowClosedEvent& e);

	private:
		uint32_t m_imageIndex;
		uint32_t m_curFrame;

		bool m_isFrameStarted;

		bool m_isFrameBufferResized;

		std::vector<VkCommandBuffer> m_cmdBuffers;

		std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_availableSemaphores;
		std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_finishedSemaphores;
		std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_renderFences;

		std::function<void(WindowClosedEvent&)> m_windowCloseEvent;
	};
}