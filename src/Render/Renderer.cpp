#include "../../Headers/Render/Renderer.h"

#include <iostream>
#include <stdexcept>
#include <array>

#include "../../Headers/Core/Core.h"

namespace ZVK
{
	Renderer::Renderer()
		: m_clearColour(0.05f, 0.05f, 0.05f, 1.f)
	{
		createCommandBuffers();
		createSyncObjects();

		m_windowCloseEvent = std::bind(&Renderer::windowCloseEvent, std::ref(*this), 
			std::placeholders::_1);

		ZWindow::GetDispatchers().WindowClosed.Attach(m_windowCloseEvent);
	}

	Renderer::~Renderer()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		ZWindow::GetDispatchers().WindowClosed.Detach(m_windowCloseEvent);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(pDevice->GetDevice(), m_availableSemaphores[i], nullptr);
			vkDestroySemaphore(pDevice->GetDevice(), m_finishedSemaphores[i], nullptr);
			vkDestroyFence(pDevice->GetDevice(), m_renderFences[i], nullptr);
		}
	}
	
	void Renderer::Begin()
	{
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();
		ZDevice* pDevice = Core::GetCore().GetDevice();
		
		VkResult result = vkAcquireNextImageKHR(pDevice->GetDevice(), pSwapchain->GetSwapchain(),
			UINT64_MAX, m_availableSemaphores[m_curFrame], VK_NULL_HANDLE, &m_imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			pSwapchain->RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to aquire swapchain image!");
		}

		vkResetFences(pDevice->GetDevice(), 1, &m_renderFences[m_curFrame]);

		vkResetCommandBuffer(m_cmdBuffers[m_curFrame], 0);

		beginFlush();
	}

	void Renderer::End()
	{
		endFlush();

		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();
		ZDevice* pDevice = Core::GetCore().GetDevice();

		VkSubmitInfo submitInfo{};
		VkSemaphore waitSemaphores[] = { m_availableSemaphores[m_curFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphore[] = { m_finishedSemaphores[m_curFrame] };

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_cmdBuffers[m_curFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphore;

		if (vkQueueSubmit(pDevice->GetGraphicsQueue(), 1,
			&submitInfo, m_renderFences[m_curFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer!");

		VkPresentInfoKHR presentInfo{};
		VkSwapchainKHR swapchains[] = { pSwapchain->GetSwapchain() };

		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &m_imageIndex;
		presentInfo.pResults = nullptr;

		auto result = vkQueuePresentKHR(pDevice->GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
			pSwapchain->IsFrameBufferResized())
		{
			pSwapchain->RecreateSwapchain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create present swapchain");

		vkWaitForFences(pDevice->GetDevice(), 1, &m_renderFences[m_curFrame], VK_TRUE, UINT64_MAX);

		for (RenderCmd* renderCmd : m_updateVertexCmds)
			if (renderCmd != nullptr)
				renderCmd->Execute();

		m_curFrame = (m_curFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::beginFlush()
	{
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color =
		{ { m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w } };
		clearValues[1].depthStencil = { 1.f, 0 };

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(GetCurrentCommandBuffer(), &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin command buffer!");

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = pSwapchain->GetRenderPass();
		renderPassBeginInfo.framebuffer = pSwapchain->GetSwapchainFrameBuffers()[m_imageIndex];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = pSwapchain->GetSwapchainExtent();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(GetCurrentCommandBuffer(),
			&renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Renderer::endFlush()
	{
		for (RenderCmd* renderCmd : m_renderCmds)
			if (renderCmd != nullptr)
				renderCmd->Execute();

		vkCmdEndRenderPass(GetCurrentCommandBuffer());

		if (vkEndCommandBuffer(GetCurrentCommandBuffer()) != VK_SUCCESS)
			throw std::runtime_error("Failed to end command buffer");
	}

	void Renderer::createCommandBuffers()
	{
		m_cmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = Core::GetCore().GetCmdPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_cmdBuffers.size();

		if (vkAllocateCommandBuffers(Core::GetCore().GetDevice()->GetDevice(),
			&allocInfo, m_cmdBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!");
	}

	void Renderer::createSyncObjects()
	{
		ZDevice* device = Core::GetCore().GetDevice();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(device->GetDevice(), &semaphoreInfo,
				nullptr, &m_availableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device->GetDevice(), &semaphoreInfo,
					nullptr, &m_finishedSemaphores[i]) != VK_SUCCESS || 
				vkCreateFence(device->GetDevice(), &fenceInfo,
					nullptr, &m_renderFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create synchronization objects!");
			}
		}
	}

	void Renderer::windowCloseEvent(WindowClosedEvent& e)
	{
		vkDeviceWaitIdle(Core::GetCore().GetDevice()->GetDevice());
	}
}
