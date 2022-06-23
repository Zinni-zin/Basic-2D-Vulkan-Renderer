#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <queue>

namespace ZVK
{
	class Texture2D
	{
	public:

		Texture2D();
		Texture2D(const std::string& filePath, VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM);
		~Texture2D();

		void LoadTexture(const std::string& filePath, VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM);

		//void CreateTextureSampler();

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		void CreateImage( VkSampleCountFlagBits numSamples, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

		void TransitionImageLayout(VkFormat format, VkImageLayout oldLayout,
			VkImageLayout newLayout);

		void CopyBufferToImage(VkBuffer buffer);

		void GenerateMipmaps(VkImage image, VkFormat format);

		inline const VkSampler GetSampler() const { return m_sampler; }
		inline const VkImage GetImage() const { return m_image; }
		inline const VkDeviceMemory GetDeviceMemory() const { return m_imageMemory; }
		inline const VkImageView GetImageView() const { return m_imageView; }

		inline int GetWidth() const { return m_width; }
		inline int GetHeight() const { return m_height; }
		inline uint32_t GetMipLevels() const { return m_mipLevels; }

		inline uint32_t GetID() const { return m_id; }

		inline void SetID(uint32_t id) { m_id = id; }
	private:

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:
		VkSampler m_sampler;
		VkImage m_image;
		VkDeviceMemory m_imageMemory;
		VkImageView m_imageView;

		int m_width, m_height;
		uint32_t m_mipLevels;

		uint32_t m_id;
		//static std::queue<uint32_t> s_freedIDs;
		static uint32_t s_idCounter;
	};
}
