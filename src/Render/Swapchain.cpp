#include "Headers/Swapchain.h"

#include <stdexcept>
#include <algorithm>
#include <limits>
#include <array>

#include <iostream>

#include "../Core/Headers/Core.h"

namespace ZVK
{
	ZSwapchain::ZSwapchain()
	{
		m_frameBufferResizeEvent = std::bind(&ZSwapchain::frameBufferResizeEvent, std::ref(*this),
			std::placeholders::_1);

		ZWindow::GetDispatchers().FrameBufferResize.Attach(m_frameBufferResizeEvent);
	}

	ZSwapchain::~ZSwapchain()
	{
		ZWindow::GetDispatchers().FrameBufferResize.Detach(m_frameBufferResizeEvent);
	}

	void ZSwapchain::CreateSwapchain()
	{
		const ZDevice* device = Core::GetCore().GetDevice();

		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device->GetPhysicalDevice());

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

		if (swapchainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapchainSupport.capabilities.maxImageCount)
		{
			imageCount = swapchainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		QueueFamilyIndices indices = device->FindQueueFamilies(device->GetPhysicalDevice());
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
			indices.presentFamily.value() };

		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = Core::GetCore().GetSurface();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device->GetDevice(), &swapchainCreateInfo, nullptr, &m_swapchain)
			!= VK_SUCCESS)
			throw std::runtime_error("Failed to create swapchain!");

		vkGetSwapchainImagesKHR(device->GetDevice(), m_swapchain, &imageCount, nullptr);
		m_swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device->GetDevice(), m_swapchain, &imageCount, m_swapchainImages.data());

		m_swapchainImageFormat = surfaceFormat.format;
		m_swapchainExtent = extent;
	}

	void ZSwapchain::CreateImageViews()
	{
		m_swapchainImageViews.resize(m_swapchainImages.size());

		for (size_t i = 0; i < m_swapchainImages.size(); ++i)
		{
			m_swapchainImageViews[i] = Core::GetCore().CreateImageView(
				m_swapchainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}

	void ZSwapchain::Cleanup()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		for (size_t i = 0; i < m_swapchainFrameBuffers.size(); ++i)
			vkDestroyFramebuffer(pDevice->GetDevice(), m_swapchainFrameBuffers[i], nullptr);

		for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
			vkDestroyImageView(pDevice->GetDevice(), m_swapchainImageViews[i], nullptr);

		vkDestroySwapchainKHR(pDevice->GetDevice(), m_swapchain, nullptr);
	}

	void ZSwapchain::RecreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(Core::GetCore().GetGLFWWindnow(), &width, &height);

		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(Core::GetCore().GetGLFWWindnow(), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(Core::GetCore().GetDevice()->GetDevice());

		SwapchainRecreateEvent recreateEvent(width, height);
	
		Cleanup();

		CreateSwapchain();
		CreateImageViews();

		Core::GetCore().GetSwapchainRecreateDispatcher().Notify(recreateEvent);

		m_isFrameBufferResized = false;
		m_isSwapchainRecreated = true;
	}

	SwapchainSupportDetails ZSwapchain::querySwapchainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Core::GetCore().GetSurface(),
			&details.capabilities);

		uint32_t formatCount;
		uint32_t presentModeCount;

		vkGetPhysicalDeviceSurfaceFormatsKHR(device, Core::GetCore().GetSurface(), 
			&formatCount, nullptr);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, Core::GetCore().GetSurface(),
			&presentModeCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, Core::GetCore().GetSurface(),
				&formatCount, details.formats.data());
		}

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, Core::GetCore().GetSurface(),
				&presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ZSwapchain::chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}

	VkPresentModeKHR ZSwapchain::chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ZSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		int width, height;
		glfwGetFramebufferSize(Core::GetCore().GetGLFWWindnow(), &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);

		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

		return actualExtent;
	}

	void ZSwapchain::frameBufferResizeEvent(FrameBufferResizedEvent& e)
	{
		m_isFrameBufferResized = true;
	}
}
