#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace ZVK
{
	class Core;
	struct SwapchainSupportDetails;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	class ZDevice
	{
	public:
		void Init(Core* pCore);

		inline const VkDevice& GetDevice() const { return m_device; }
		inline const VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
		inline const VkQueue& GetGraphicsQueue() const { return m_graphicsQueue; }
		inline const VkQueue& GetPresentQueue() const { return m_presentQueue; }
		inline const uint32_t GetMaxTextureSlots() const { return m_maxTextureSlots; }
		inline const VkSampleCountFlagBits& GetMSAA_Samples() const { return m_msaaSamples; }


		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device) const;

	private:
		void pickDevice();
		void createLogicalDevice();

		SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		int rateDevice(VkPhysicalDevice device);

		VkSampleCountFlagBits getMaxUsableSampleCount();

		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	private:
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;

		uint32_t m_maxTextureSlots = 16;

		VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		const std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		Core* p_core = nullptr;
	};
}