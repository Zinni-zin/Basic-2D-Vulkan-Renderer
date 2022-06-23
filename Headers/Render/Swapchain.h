#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <string>
#include <functional>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "../../Headers/Core/Device.h"

#include "../../Headers/Events/Events.h"

namespace ZVK
{
	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class ZSwapchain
	{
	public:
		ZSwapchain();
		~ZSwapchain();

		void Create();

		void CreateSwapchain();
		void CreateImageViews();
		void CreateFrameBuffers();
		void CreateRenderPass();
		void CreateColourResources();
		void CreateDepthResources();

		void Cleanup();
		void RecreateSwapchain();

		inline const VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
		inline const VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
		inline const VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }

		inline const std::vector<VkImage> GetSwapchainImages() const { return m_swapchainImages; }
		inline const std::vector<VkImageView> GetSwapchainImageViews() const { return m_swapchainImageViews; }
		inline const std::vector<VkFramebuffer> GetSwapchainFrameBuffers() const { return m_swapchainFrameBuffers; }
		
		inline const VkRenderPass GetRenderPass() const { return m_renderpass; }
		
		inline const VkImage GetColourImage() const { return m_colourImage; };
		inline const VkDeviceMemory GetColourImageMemory() const { return m_colourImageMemory; }
		inline const VkImageView GetColourImageView() const { return m_colourImageView; }
		inline const VkImage GetDepthImage() const { return m_depthImage; }
		inline const VkDeviceMemory GetDepthImageMemory() const { return m_depthImageMemory; }
		inline const VkImageView GetDepthImageView() const { return m_depthImageView; }

		// inline const std::vector<VkFramebuffer> GetSwapchainFrameBuffers() const { return m_swapchainFrameBuffers; }

		inline const bool IsSwapchainRecreated() const { return m_isSwapchainRecreated; }
		inline void SwapchainRecreatedFinished() { m_isSwapchainRecreated = false; }

		inline const bool IsFrameBufferResized() const { return m_isFrameBufferResized; }
	private:

		SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void frameBufferResizeEvent(FrameBufferResizedEvent& e);

	private:
		
		VkSwapchainKHR m_swapchain;

		VkFormat m_swapchainImageFormat;
		VkExtent2D m_swapchainExtent;

		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
		std::vector<VkFramebuffer> m_swapchainFrameBuffers;


		VkRenderPass m_renderpass;

		VkImage m_colourImage;
		VkDeviceMemory m_colourImageMemory;
		VkImageView m_colourImageView;

		VkImage m_depthImage;
		VkDeviceMemory m_depthImageMemory;
		VkImageView m_depthImageView;

		bool m_isSwapchainRecreated = false;
		bool m_isFrameBufferResized = false;

		std::function<void(FrameBufferResizedEvent&)> m_frameBufferResizeEvent;
		
	};
}