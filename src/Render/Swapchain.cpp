#include "../../Headers/Render/Swapchain.h"

#include <stdexcept>
#include <algorithm>
#include <limits>
#include <array>

#include <iostream>

#include "../../Headers/Core/Core.h"

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

	void ZSwapchain::Create()
	{
		CreateSwapchain();
		CreateImageViews();
		CreateRenderPass();
		CreateColourResources();
		CreateDepthResources();
		CreateFrameBuffers();
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

	void ZSwapchain::CreateFrameBuffers()
	{
		m_swapchainFrameBuffers.resize(m_swapchainImageViews.size());

		for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
		{
			std::array<VkImageView, 3> attachments =
			{
				m_colourImageView,
				m_depthImageView,
				m_swapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderpass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_swapchainExtent.width;
			framebufferInfo.height = m_swapchainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(Core::GetCore().GetDevice()->GetDevice(), &framebufferInfo,
				nullptr, &m_swapchainFrameBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer!");
		}
	}

	void ZSwapchain::CreateRenderPass()
	{
		VkAttachmentDescription colourAttachment{};
		colourAttachment.format = m_swapchainImageFormat;
		colourAttachment.samples = Core::GetCore().GetDevice()->GetMSAA_Samples();
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colourAttachmentResolve{};
		colourAttachmentResolve.format = m_swapchainImageFormat;
		colourAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = Core::GetCore().FindDepthFormat();
		depthAttachment.samples = Core::GetCore().GetDevice()->GetMSAA_Samples();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colourAttachmentRef{};
		colourAttachmentRef.attachment = 0;
		colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colourAttachmentResolverRef{};
		colourAttachmentResolverRef.attachment = 2;
		colourAttachmentResolverRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colourAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colourAttachmentResolverRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments =
		{ colourAttachment, depthAttachment, colourAttachmentResolve };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(Core::GetCore().GetDevice()->GetDevice(), &renderPassInfo,
			nullptr, &m_renderpass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create render pass!");
	}

	void ZSwapchain::CreateColourResources()
	{
		Core::GetCore().CreateImage(
			m_swapchainExtent.width, m_swapchainExtent.height, 1, 
			Core::GetCore().GetDevice()->GetMSAA_Samples(),
			m_swapchainImageFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colourImage, m_colourImageMemory
		);

		m_colourImageView = Core::GetCore().CreateImageView(m_colourImage, m_swapchainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void ZSwapchain::CreateDepthResources()
	{
		VkFormat depthFormat = Core::GetCore().FindDepthFormat();

		Core::GetCore().CreateImage(
			m_swapchainExtent.width, m_swapchainExtent.height, 1,
			Core::GetCore().GetDevice()->GetMSAA_Samples(), depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory
		);

		m_depthImageView = Core::GetCore().CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	}

	void ZSwapchain::Cleanup()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
			vkDestroyImageView(pDevice->GetDevice(), m_swapchainImageViews[i], nullptr);

		vkDestroySwapchainKHR(pDevice->GetDevice(), m_swapchain, nullptr);

		vkDestroyImageView(pDevice->GetDevice(), m_colourImageView, nullptr);
		vkDestroyImage(pDevice->GetDevice(), m_colourImage, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_colourImageMemory, nullptr);

		vkDestroyImageView(pDevice->GetDevice(), m_depthImageView, nullptr);
		vkDestroyImage(pDevice->GetDevice(), m_depthImage, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_depthImageMemory, nullptr);

		SwapchainCleanupEvent e;
		if(!Core::GetCore().GetWindow().ShouldClose())
			Core::GetCore().GetSwapchainCleanupDispatcher().Notify(e);

		for (size_t i = 0; i < m_swapchainFrameBuffers.size(); ++i)
			vkDestroyFramebuffer(pDevice->GetDevice(), m_swapchainFrameBuffers[i], nullptr);

		vkDestroyRenderPass(pDevice->GetDevice(), m_renderpass, nullptr);
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
		CreateRenderPass();
		CreateColourResources();
		CreateDepthResources();
		CreateFrameBuffers();

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
