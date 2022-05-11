#include "Headers/Device.h"

#include <string>

#include <set>
#include <map>

#include <iostream>
#include <stdexcept>


#include "Headers/Core.h"

#include "../Render/Headers/Swapchain.h"

namespace ZVK
{

	void ZDevice::Init(Core* pCore)
	{
		p_core = pCore;

		pickDevice();
		createLogicalDevice();
	}

	void ZDevice::pickDevice()
	{
		uint32_t deviceCount = 0;

		vkEnumeratePhysicalDevices(p_core->GetInstance(), &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan Support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		std::map<int, VkPhysicalDevice> candidates;

		vkEnumeratePhysicalDevices(p_core->GetInstance(), &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			int score = rateDevice(device);
			candidates.insert(std::make_pair(score, device));
		}

		if (candidates.rbegin()->first > 0)
		{
			m_physicalDevice = candidates.rbegin()->second;
			m_msaaSamples = getMaxUsableSampleCount();
		}
		else
			throw std::runtime_error("Failed to find a suitable GPU!");

		if (m_physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
		m_maxTextureSlots = deviceProperties.limits.maxDescriptorSetSampledImages;
		// std::cout << "Max Texture Slots: " << m_maxTextureSlots << std::endl;
	}

	void ZDevice::createLogicalDevice()
	{
		float queuePriority = 1.f;

		QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
		indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		indexingFeatures.pNext = nullptr;
		indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		indexingFeatures.runtimeDescriptorArray = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo{};

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamiles =
		{
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		for (uint32_t queueFamily : uniqueQueueFamiles)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
		deviceCreateInfo.pNext = &indexingFeatures;

		if (p_core->EnabledValidationLayers())
		{
			deviceCreateInfo.enabledLayerCount = 
				static_cast<uint32_t>(p_core->GetValidationLayers().size());

			deviceCreateInfo.ppEnabledLayerNames = p_core->GetValidationLayers().data();
		}
		else
			deviceCreateInfo.enabledLayerCount = 0;
		
		if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device!");

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	int ZDevice::rateDevice(VkPhysicalDevice device)
	{
		int score = 0;

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices indices = FindQueueFamilies(device);

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
		indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		indexingFeatures.pNext = nullptr;

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &indexingFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

		bool extensionsSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		if (extensionsSupported)
		{
			SwapchainSupportDetails swapchainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		}

		score += deviceProperties.limits.maxImageDimension2D;
		score += deviceProperties.limits.maxPerStageDescriptorSampledImages;

		if (!indices.IsComplete() && !extensionsSupported && !swapChainAdequate &&
			!deviceFeatures.samplerAnisotropy &&
			!indexingFeatures.descriptorBindingUpdateUnusedWhilePending &&
			!indexingFeatures.descriptorBindingPartiallyBound &&
			!indexingFeatures.runtimeDescriptorArray)
		{
			return 0;
		}

		return score;
	}

	QueueFamilyIndices ZDevice::FindQueueFamilies(const VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (int i = 0; i < queueFamilies.size(); ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, p_core->GetSurface(), &presentSupport);

				indices.graphicsFamily = i;
				if (presentSupport)
					indices.presentFamily = i;
			}

			if (indices.IsComplete()) break;
		}

		return indices;
	}

	SwapchainSupportDetails ZDevice::querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, p_core->GetSurface(), &details.capabilities);

		uint32_t formatCount;
		uint32_t presentModeCount;

		vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_core->GetSurface(), &formatCount, nullptr);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_core->GetSurface(),
			&presentModeCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_core->GetSurface(),
				&formatCount, details.formats.data());
		}

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_core->GetSurface(),
				&presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSampleCountFlagBits ZDevice::getMaxUsableSampleCount()
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts
			& physicalDeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	bool ZDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionsCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr,
			&extensionsCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_deviceExtensions.begin(),
			m_deviceExtensions.end());

		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}
}