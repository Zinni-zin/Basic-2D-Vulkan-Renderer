#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <map>
#include <list>

#include "Window.h"
#include "Device.h"

#include "../Render/Swapchain.h"

#include "../Render/Texture2D.h"

#include "../Events/Events.h"

#define MAX_FRAMES_IN_FLIGHT 2

namespace ZVK
{
	class Core
	{
	public:
		Core(const Core&) = delete;
		Core(Core&&) = delete;
		Core& operator=(const Core&) = delete;
		Core& operator=(Core&&) = delete;

		~Core();

		void Init(const std::string& title, int width, int height);

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer cmdBuffer);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
			VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags,
			uint32_t mipLevels);

		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
			VkDeviceMemory& imageMemory);

		VkFormat FindSupportFormat(const std::vector<VkFormat>& candidates,
			VkImageTiling tiling, VkFormatFeatureFlags features);

		VkFormat FindDepthFormat();

		inline ZWindow& GetWindow() const { return *p_window; }
		inline GLFWwindow* GetGLFWWindnow() const { return p_window->GetWindow(); }
		inline ZDevice* GetDevice() const { return p_device; }
		inline ZSwapchain* GetSwapchain() const { return p_swapchain; }

		inline const VkInstance& GetInstance() const { return m_instance; }
		inline const VkSurfaceKHR& GetSurface() const { return m_surface; }

		inline const VkCommandPool GetCmdPool() const { return m_cmdPool; }

		inline bool EnabledValidationLayers() const { return m_enableValidationLayers; }
		inline const VkDebugUtilsMessengerEXT& GetDebugMessenger() const { return m_debugMessenger; }
		inline const std::vector<const char*>& GetValidationLayers() const { return m_validationLayers; }

		inline Dispatcher<SwapchainRecreateEvent>& GetSwapchainRecreateDispatcher() { return m_recreateDispatcher; }
		inline Dispatcher<SwapchainCleanupEvent>& GetSwapchainCleanupDispatcher() { return m_swapchainCleanupDispatcher; }

		inline const uint32_t GetMaxTextureSlots() const { return p_device->GetMaxTextureSlots(); }

		static Core& CreateCore();
		static Core& GetCore();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		Core();

		void createInstance(const char* appName);
		void createSurface();

		void createCommandPool();
			
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		std::vector<const char*> getRequiredExtensions();

		void setupDebugMessenger();
		bool checkValidationLayerSupport();

		static bool s_isCreated;

	private:
		std::unique_ptr<ZWindow> p_window;
		ZDevice* p_device;
		ZSwapchain* p_swapchain;

		Dispatcher<SwapchainRecreateEvent> m_recreateDispatcher;
		Dispatcher<SwapchainCleanupEvent> m_swapchainCleanupDispatcher;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkSurfaceKHR m_surface;

		VkCommandPool m_cmdPool;
		std::vector<VkCommandBuffer> m_cmdBuffers;

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif 

	};
}
