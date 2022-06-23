#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "Swapchain.h"
#include "../Events/Events.h"

#include "../Math/Vectors/Vector4.h"

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

namespace ZVK
{
	class RenderCmd
	{
	public:
		virtual void Execute() = 0;
	};

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
		inline VkFence& GetCurrentFence() { return m_renderFences[m_curFrame]; }

		inline Vec4 GetClearColour() const { return m_clearColour; }
		inline void SetClearColour(Vec4 colour) { m_clearColour = colour; }
		inline void SetClearColour(float r, float g, float b, float a = 1.0) { m_clearColour = { r,g,b,a }; };
		void SetClearColour(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 255)
		{
			m_clearColour.x = (float)r / 255.f;
			m_clearColour.y = (float)g / 255.f;
			m_clearColour.z = (float)b / 255.f;
			m_clearColour.w = (float)a / 255.f;
		}

		// This should only be called in render systems
		void AddRenderCmd(RenderCmd* cmd) { m_renderCmds.push_back(cmd); }

		// This should only be called in render systems
		void AddUpdateVertexCmd(RenderCmd* cmd) { m_updateVertexCmds.push_back(cmd); }

		void RemoveRenderCmd(RenderCmd* cmd)
		{ m_renderCmds.erase(std::find(m_renderCmds.begin(), m_renderCmds.end(), cmd)); }

		void RemoveUpdateVertexCmd(RenderCmd* cmd) 
		{
			m_updateVertexCmds.erase(std::find(m_updateVertexCmds.begin(), 
				m_updateVertexCmds.end(), cmd));
		}

		void ClearRenderCmds() { m_renderCmds.clear(); }

	private:
		void beginFlush();
		void endFlush();

		void createCommandBuffers();
		void createSyncObjects();

		void windowCloseEvent(WindowClosedEvent& e);

	private:
		uint32_t m_imageIndex;
		uint32_t m_curFrame;
		Vec4 m_clearColour;

		bool m_isFrameBufferResized;

		std::vector<VkCommandBuffer> m_cmdBuffers;

		std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_availableSemaphores;
		std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_finishedSemaphores;
		std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_renderFences;

		std::vector<RenderCmd*> m_renderCmds;
		std::vector<RenderCmd*> m_updateVertexCmds;

		std::function<void(WindowClosedEvent&)> m_windowCloseEvent;
	};
}