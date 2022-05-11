#include "Headers/Texture2D.h"

#include <iostream>
#include <cmath>
#include <stdexcept>

#include "../../vendor/stb_image/stb_image.h"

#include "../Core/Headers/Core.h"


namespace ZVK
{
	std::queue<uint32_t> Texture2D::s_freedIDs;
	std::queue<int> Texture2D::s_freedRangedIDs;
	uint32_t Texture2D::s_idCounter = 0;
	int Texture2D::s_idRangedCounter = 0;

	Texture2D::Texture2D()
		: m_width(0), m_height(0), m_mipLevels(0)
	{ 
		m_id = s_idCounter;

		if (s_idRangedCounter == Core::GetCore().GetMaxTextureSlots())
			s_idRangedCounter = 0;

		m_rangedID = s_idRangedCounter;
		++s_idCounter;
		++s_idRangedCounter;
	}

	Texture2D::Texture2D(const std::string& filePath, VkFormat colourFormat)
	{
		m_id = s_idCounter;

		if (s_idRangedCounter == Core::GetCore().GetMaxTextureSlots())
			s_idRangedCounter = 0;

		m_rangedID = s_idRangedCounter;
		++s_idCounter;
		++s_idRangedCounter;

		LoadTexture(filePath, colourFormat);
	}

	Texture2D::~Texture2D()
	{
		const VkDevice& device = Core::GetCore().GetDevice()->GetDevice();

		vkDestroySampler(device, m_sampler, nullptr);
		vkDestroyImageView(device, m_imageView, nullptr);

		vkDestroyImage(device, m_image, nullptr);
		vkFreeMemory(device, m_imageMemory, nullptr);
		
		if (Core::GetCore().Get2DTextures().size() > 0)
		{
			Core::GetCore().SwapTextures(this);
			Core::GetCore().PopBackTexture2D();
		}
		else
			Core::GetCore().PopBackTexture2D();
	}

	// VK_FORMAT_R8G8B8A8_UNORM
	// VK_FORMAT_R8G8B8A8_SRGB
	void Texture2D::LoadTexture(const std::string& filePath, VkFormat colourFormat)
	{
		int channels;

		stbi_uc* pixels = stbi_load(filePath.c_str(), &m_width, &m_height, &channels,
			STBI_rgb_alpha);

		VkDeviceSize imageSize = m_width * m_height * 4;
		m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;

		if (!pixels)
			throw std::runtime_error("Failed to load image!");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		Core::GetCore().CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, 0, imageSize,
			0, &data);
		memcpy(data, pixels, static_cast<uint32_t>(imageSize));
		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory);

		stbi_image_free(pixels);

		CreateImage(VK_SAMPLE_COUNT_1_BIT,
			colourFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TransitionImageLayout(colourFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		
		CopyBufferToImage(stagingBuffer);
		
		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, nullptr);
	
		GenerateMipmaps(m_image, colourFormat);
		
		m_imageView = Core::GetCore().CreateImageView(m_image, colourFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);

		CreateTextureSampler();
		Core::GetCore().AddTexture2D(this);
	}

	void Texture2D::CreateTextureSampler()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();
		
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(pDevice->GetPhysicalDevice(), &physicalDeviceProperties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = static_cast<float>(m_mipLevels);
		samplerInfo.mipLodBias = 0.f;

		if (vkCreateSampler(pDevice->GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler!");
	}

	uint32_t Texture2D::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(Core::GetCore().GetDevice()->GetPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)
				== properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find a suitable memory type!");
	}

	void Texture2D::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(Core::GetCore().GetDevice()->GetDevice(), &bufferInfo, nullptr, &buffer)
			!= VK_SUCCESS)
			throw std::runtime_error("Failed to Create Buffer!");

		VkMemoryRequirements memRequirements{};
		vkGetBufferMemoryRequirements(Core::GetCore().GetDevice()->GetDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(Core::GetCore().GetDevice()->GetDevice(),
			&allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate buffer memory!");

		vkBindBufferMemory(Core::GetCore().GetDevice()->GetDevice(), buffer, bufferMemory, 0);
	}

	void Texture2D::CreateImage(VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_width;
		imageInfo.extent.height = m_height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = numSamples;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(Core::GetCore().GetDevice()->GetDevice(), &imageInfo, nullptr, &m_image)
			!= VK_SUCCESS)
			throw std::runtime_error("Failed to Create Image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(Core::GetCore().GetDevice()->GetDevice(),
			m_image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(Core::GetCore().GetDevice()->GetDevice(), &allocInfo,
			nullptr, &m_imageMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to Allocate Image Memory!");

		vkBindImageMemory(Core::GetCore().GetDevice()->GetDevice(), m_image, m_imageMemory, 0);
	}

	void Texture2D::TransitionImageLayout(VkFormat format, VkImageLayout oldLayout,
		VkImageLayout newLayout)
	{
		VkCommandBuffer cmdBuffer = Core::GetCore().BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = m_mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags dstStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("Unsupported Layout Transition!");
		}

		vkCmdPipelineBarrier(
			cmdBuffer,
			sourceStage, dstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		Core::GetCore().EndSingleTimeCommands(cmdBuffer);
	}

	void Texture2D::CopyBufferToImage(VkBuffer buffer)
	{
		VkCommandBuffer cmdBuffer = Core::GetCore().BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };

		vkCmdCopyBufferToImage(
			cmdBuffer,
			buffer,
			m_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		Core::GetCore().EndSingleTimeCommands(cmdBuffer);
	}

	void Texture2D::GenerateMipmaps(VkImage image, VkFormat format)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(Core::GetCore().GetDevice()->GetPhysicalDevice(),
			format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			throw std::runtime_error("Texture Image Format Does Not Support Linear Blitting!");

		VkCommandBuffer cmdBuffer = Core::GetCore().BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = m_width;
		int32_t mipHeight = m_height;

		for (uint32_t i = 1; i < m_mipLevels; ++i)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] =
			{
				mipWidth > 1 ? mipWidth / 2 : 1,
				mipHeight > 1 ? mipHeight / 2 : 1,
				1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
				cmdBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR
			);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		Core::GetCore().EndSingleTimeCommands(cmdBuffer);
	}
}