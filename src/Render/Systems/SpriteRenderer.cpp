#include "../../../Headers/Render/Systems/SpriteRenderer.h"

#include <iostream>
#include <stdexcept>

#include <cassert>

#define MAX_QUAD_COUNT 880
#define MAX_VERTEX_COUNT MAX_QUAD_COUNT * 4
#define MAX_INDEX_COUNT MAX_QUAD_COUNT * 6

namespace ZVK
{

	SpriteRenderer::SpriteRenderer(Renderer& renderer,
		SpritePipeline* pPipeline, const std::string& errorTexturePath)
		: m_renderer(renderer), p_pipeline(pPipeline)
	{
		init(errorTexturePath);
	}

	SpriteRenderer::SpriteRenderer(Renderer& renderer, const std::string& errorTexturePath,
		const std::string& vertPath, const std::string& fragPath) :
		m_renderer(renderer)
	{
		try
		{
			p_pipeline = new SpritePipeline(vertPath, fragPath);
		}
		catch (std::exception e)
		{
			std::cout << "Error creating pipeline: " << e.what() << std::endl;
		}

		init(errorTexturePath);
		m_canDeletePipeline = true;
	}

	SpriteRenderer::~SpriteRenderer()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		vkDestroyBuffer(pDevice->GetDevice(), m_uniformBuffer, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_uniformMemory, nullptr);

		for (auto cmd : m_uploadUpdatedVerticesCmds)
		{
			m_renderer.RemoveUpdateVertexCmd(cmd);
			if (cmd)
			{
				delete cmd;
				cmd = nullptr;
			}
		}

		for (auto cmd : m_spriteRendererCmds)
		{
			m_renderer.RemoveRenderCmd(cmd);
			if (cmd)
			{
				delete cmd;
				cmd = nullptr;
			}
		}

		for (auto& renderData : m_renderInfos)
		{
			vkDestroyBuffer(pDevice->GetDevice(), renderData.Buffer, nullptr);
			vkFreeMemory(pDevice->GetDevice(), renderData.BufferMemory, nullptr);
		}

		vkDestroySampler(pDevice->GetDevice(), m_sampler, nullptr);
	}

	void SpriteRenderer::init(const std::string& errorTexturePath)
	{
		createUniformBuffer();

		p_errorTexture = std::make_shared<Texture2D>(errorTexturePath);

		m_swapchainRecreateEvent = std::bind(&SpriteRenderer::swapchainRecreateEvent,
			std::ref(*this), std::placeholders::_1);

		Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);

		// Create sampler
		{
			VkSamplerCreateInfo samplerInfo{};
			VkPhysicalDeviceProperties physicalDeviceProperties{};

			uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(32, 32)))) + 1;

			vkGetPhysicalDeviceProperties(Core::GetCore().GetDevice()->GetPhysicalDevice(),
				&physicalDeviceProperties);

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
			samplerInfo.maxLod = static_cast<float>(mipLevels);
			samplerInfo.mipLodBias = 0.f;

			if (vkCreateSampler(Core::GetCore().GetDevice()->GetDevice(),
				&samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
				throw std::runtime_error("Failed to create texture sampler!");

			VkDescriptorImageInfo textureImageInfo{};
			textureImageInfo.sampler = m_sampler;
			textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			m_samplerImageInfo = textureImageInfo;
		}

		// Set Default Viewport and Scissor Rect Extent
		{
			m_viewportInfo = Vec4{
				0.f,
				0.f,
				static_cast<float>(Core::GetCore().GetSwapchain()->GetSwapchainExtent().width),
				static_cast<float>(Core::GetCore().GetSwapchain()->GetSwapchainExtent().height)
			};

			m_scissorExtent = Core::GetCore().GetSwapchain()->GetSwapchainExtent();
		}

		m_textureData = std::make_unique<TextureData>(TextureData{});

		allocateDescriptorInfo();
	}

	void SpriteRenderer::Draw(std::vector<std::shared_ptr<Sprite>>& sprites, const Mat4& pv,
		const bool canUpdateVertexBuffer)
	{
		if (sprites.empty()) return;

		auto spriteList = m_loadedLists.find(&sprites);
		bool isFound = false;

		if (spriteList == m_loadedLists.end())
		{
			m_loadedLists[&sprites] = { (uint32_t)sprites.size(), m_listCount++ };
			spriteList = m_loadedLists.find(&sprites);
		}
		else if (spriteList->second.ListSize != (uint32_t)sprites.size())
		{
			// This is inefficient but it works
			for (SpriteRendererCmd* rCmd : m_spriteRendererCmds)
			{
				m_renderer.RemoveRenderCmd(rCmd);

				if (rCmd != nullptr)
				{
					delete rCmd;
					rCmd = nullptr;
				}
			}

			for (UploadUpdatedSpriteVerticesCmd* updateVCmd : m_uploadUpdatedVerticesCmds)
			{
				m_renderer.RemoveUpdateVertexCmd(updateVCmd);

				if (updateVCmd != nullptr)
				{
					delete updateVCmd;
					updateVCmd = nullptr;
				}
			}

			for (auto& renderData : m_renderInfos)
			{
				vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), renderData.Buffer, nullptr);
				vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), renderData.BufferMemory, nullptr);
			}

			changedListIndex = spriteList->second.Index;

			m_spriteRendererCmds.clear();
			m_uploadUpdatedVerticesCmds.clear();
			m_renderInfos.clear();
			m_updateVerticesCmds.clear();
			m_textureData->DescriptorImageInfos.clear();
			m_textureData->BindingInfos.clear();
			m_textureData->TextureCount = 0;
			m_listCount = 0;
			spriteList->second.ListSize = (uint32_t)sprites.size();

			for (auto& [key, val] : m_loadedLists)
				val.IsListAdded = false;

			return;
		}
		else if (spriteList->second.IsListAdded == true)
			isFound = true;

		if (changedListIndex >= 0 && spriteList->second.Index > changedListIndex)
			return;
		else if (spriteList->second.Index == changedListIndex)
			changedListIndex = -1;

		if (!isFound)
		{
			createTextureData(sprites);

			// Add new list to the render data 
			// Create a new one if we go over the max quad count
			if (!m_renderInfos.empty() &&
				m_renderInfos[m_renderInfos.size() - 1].SpriteCount < MAX_QUAD_COUNT)
			{
				uint32_t renderIndex = (uint32_t)m_renderInfos.size() - 1;

				RenderData& rData = m_renderInfos[renderIndex];

				uint32_t spriteIndex = 0;

				if (rData.SpriteCount + (uint32_t)sprites.size() > MAX_QUAD_COUNT)
				{
					// Make sure we caculate for a list size greater than MAX_QUAD_COUNT
					// We also need to account for a list size less than MAX_QUAD_COUNT

					std::vector<std::shared_ptr<Sprite>> tempSprites;

					uint32_t amountAdded = 0;
					for (size_t i = 0; rData.SpriteCount + i < MAX_QUAD_COUNT; ++i)
					{
						tempSprites.push_back(sprites[i]);
						++amountAdded;
					}

					rData.SpriteInfos.push_back(SpriteData{});
					spriteIndex = (uint32_t)m_renderInfos[renderIndex].SpriteInfos.size() - 1;

					rData.SpriteInfos[spriteIndex].CanUpdateVertices = canUpdateVertexBuffer;

					populateVertices(tempSprites, renderIndex, spriteIndex);

					if (canUpdateVertexBuffer)
					{
						std::copy(tempSprites.begin(), tempSprites.end(),
							std::back_inserter(rData.Sprites));
					}

					rData.SpriteCount += (uint32_t)tempSprites.size();

					std::vector<std::shared_ptr<Sprite>>
						newSprites(std::next(sprites.begin() + amountAdded - 1), sprites.end());

					SpriteRendererCmd* cmd = new SpriteRendererCmd(this, 
						(uint32_t)sprites.size() * 6, renderIndex, spriteIndex);
					m_spriteRendererCmds.push_back(cmd);
					m_renderer.AddRenderCmd(cmd);

					if (amountAdded != sprites.size())
						drawRecursive(newSprites, canUpdateVertexBuffer);
				}
				else
				{
					rData.SpriteInfos.push_back(SpriteData{});

					spriteIndex = (uint32_t)m_renderInfos[renderIndex].SpriteInfos.size() - 1;

					populateVertices(sprites, renderIndex, spriteIndex);

					if (canUpdateVertexBuffer)
						std::copy(sprites.begin(), sprites.end(), 
							std::back_inserter(rData.Sprites));

					rData.SpriteInfos[spriteIndex].CanUpdateVertices = canUpdateVertexBuffer;

					rData.SpriteCount += (uint32_t)sprites.size();

					SpriteRendererCmd* cmd = new SpriteRendererCmd(this,
						(uint32_t)sprites.size() * 6, renderIndex, spriteIndex);
					m_spriteRendererCmds.push_back(cmd);
					m_renderer.AddRenderCmd(cmd);
				}
			}
			else
				drawRecursive(sprites, canUpdateVertexBuffer);

			for (uint32_t i = 0; i < (uint32_t)m_renderInfos.size(); ++i)
				createBuffer(i);

			updateDescriptorWrites();

			spriteList->second.IsListAdded = true;
		}

		for (auto updateVCmd : m_updateVerticesCmds)
			updateVCmd->Execute();

		updateUBO(pv);
	}

	void SpriteRenderer::Draw(std::vector<std::shared_ptr<Sprite>>& sprites, const Camera& cam,
		const bool canUpdateVertexBuffer)
	{
		Draw(sprites, cam.GetPV(), canUpdateVertexBuffer);
	}

	void SpriteRenderer::drawRecursive(std::vector<std::shared_ptr<Sprite>>& sprites, 
		bool canUpdateVertexBuffer)
	{
		if (sprites.size() == 0) return;

		// Create render data for the shape list 
		// and create more if we go past the max quad count
		m_renderInfos.push_back(RenderData{});
		size_t index = m_renderInfos.size() - 1;

		size_t spritesSize = (sprites.size() > MAX_QUAD_COUNT) ? MAX_QUAD_COUNT : sprites.size();

		std::vector<std::shared_ptr<Sprite>> tempSprites(sprites.begin(), 
			std::next(sprites.begin() + spritesSize - 1));

		m_renderInfos[index].SpriteInfos.push_back(SpriteData{});
		populateVertices(tempSprites, (uint32_t)index, 0);

		SpriteRendererCmd* cmd = new SpriteRendererCmd(this, (uint32_t)sprites.size() * 6,
			(uint32_t)index, 0);
		m_spriteRendererCmds.push_back(cmd);
		m_renderer.AddRenderCmd(cmd);

		m_renderInfos[index].SpriteCount = (uint32_t)spritesSize;

		if (canUpdateVertexBuffer)
		{
			m_renderInfos[index].Sprites.resize(spritesSize);
			m_renderInfos[index].Sprites.assign(tempSprites.begin(), tempSprites.end());

			m_renderInfos[index].SpriteInfos[0].CanUpdateVertices = true;

			m_updateVerticesCmds.emplace_back(std::make_unique<UpdateVerticesCmd>(*this,
				m_renderInfos[index].Sprites, (uint32_t)index));
		}

		UploadUpdatedSpriteVerticesCmd* uploadUpdatedCmd =
			new UploadUpdatedSpriteVerticesCmd(this, (uint32_t)index);
		m_uploadUpdatedVerticesCmds.push_back(uploadUpdatedCmd);
		m_renderer.AddUpdateVertexCmd(uploadUpdatedCmd);

		if (sprites.size() < MAX_QUAD_COUNT)
			return;

		std::vector<std::shared_ptr<Sprite>> newSprites;
		newSprites.reserve(sprites.size() - MAX_QUAD_COUNT);
		for (size_t i = MAX_QUAD_COUNT; i < sprites.size(); ++i)
			newSprites.push_back(sprites[i]);

		drawRecursive(newSprites, canUpdateVertexBuffer);
	}

	void SpriteRenderer::draw(uint32_t indexCount, uint32_t renderDataIndex, 
		uint32_t spriteDataIndex)
	{
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

		RenderData& rData = m_renderInfos[renderDataIndex];
		SpriteData& sData = rData.SpriteInfos[spriteDataIndex];
		
		VkViewport viewport{};
		viewport.x = m_viewportInfo.x;
		viewport.y = m_viewportInfo.y;
		viewport.width = m_viewportInfo.z;
		viewport.height = m_viewportInfo.w;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		VkRect2D scissor{};
		scissor.offset = { m_scissorOffset.x, m_scissorOffset.y };
		scissor.extent = m_scissorExtent;

		vkCmdSetViewport(m_renderer.GetCurrentCommandBuffer(), 0, 1, &viewport);
		vkCmdSetScissor(m_renderer.GetCurrentCommandBuffer(), 0, 1, &scissor);

		vkCmdBindPipeline(m_renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipeline());

		vkCmdBindDescriptorSets(m_renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipelineLayout(),
			0, 1, &m_descriptorSet,
			0, nullptr);

		VkBuffer vertexBuffers[] = { rData.Buffer };
		VkDeviceSize offsets[] = { sData.StartVertex };

		vkCmdBindVertexBuffers(m_renderer.GetCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(m_renderer.GetCurrentCommandBuffer(),
			rData.Buffer, sData.StartIndex,
			VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_renderer.GetCurrentCommandBuffer(), 
			(uint32_t)sData.Indices.size(), 1, 0, 0, 0);
	}

	void SpriteRenderer::createTextureData(std::vector<std::shared_ptr<Sprite>>& sprites)
	{
		if (sprites.empty()) return;

		std::vector<std::pair<uint32_t, uint32_t>> existingTextures;
		std::vector<std::shared_ptr<Texture2D>> newTextures;
		
		newTextures.reserve(5);

		uint32_t maxTextureSlots = Core::GetCore().GetMaxTextureSlots();

		if (m_textureData->TextureCount == 0)
		{
			VkDescriptorImageInfo errorImageInfo{};
			errorImageInfo.sampler = p_errorTexture->GetSampler();
			errorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			errorImageInfo.imageView = p_errorTexture->GetImageView();

			m_textureData->DescriptorImageInfos.push_back(errorImageInfo);
			m_textureData->BindingInfos.emplace_back(std::make_pair(p_errorTexture->GetID(), 0));
			++m_textureData->TextureCount;
		}

		std::copy(m_textureData->BindingInfos.begin(), m_textureData->BindingInfos.end(),
			std::back_inserter(existingTextures));

		// Figure out what textures have and haven't been added
		for (size_t i = 0; i < sprites.size(); ++i)
		{
			if (!sprites[i]->GetTexture())
				continue;

			uint32_t spriteID = sprites[i]->GetTextureID();

			bool isFound = false;
			for (auto& [textureID, slotID] : existingTextures)
			{
				if (spriteID == textureID)
				{
					isFound = true;
					break;
				}
			}

			if (isFound)
				continue;

			existingTextures.push_back(std::make_pair(spriteID, 0));

			newTextures.push_back(sprites[i]->GetTexture());

			if ((uint32_t)newTextures.size() + m_textureData->TextureCount > maxTextureSlots)
				throw std::runtime_error("Out of texture slots, consider using a texture atlas!");
		}

		// Add new textures
		for(auto newTexture : newTextures)
		{
			VkDescriptorImageInfo textureImageInfo{};
			textureImageInfo.sampler = newTexture->GetSampler();
			textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureImageInfo.imageView = newTexture->GetImageView();

			m_textureData->DescriptorImageInfos.push_back(textureImageInfo);
			m_textureData->BindingInfos.emplace_back(std::make_pair(
				newTexture->GetID(),
				(uint32_t)m_textureData->DescriptorImageInfos.size() - 1
			));
			++m_textureData->TextureCount;
		}
	}

	void SpriteRenderer::populateVertices(std::vector<std::shared_ptr<Sprite>>& sprites, 
		uint32_t renderDataIndex, uint32_t spriteDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];
		SpriteData& sData = rData.SpriteInfos[spriteDataIndex];

		rData.TotalVerticeSize = 0;

		for (std::shared_ptr<Sprite> sprite : sprites)
		{
			float width = sprite->GetWidth();
			float height = sprite->GetHeight();
			float depth = sprite->GetDepth();
			float x = sprite->GetX();
			float y = sprite->GetY();
			float z = sprite->GetZ();

			int rangedID = 0;
			
			if (sprite->GetTexture())
			{
				for (auto& [textureID, slotID] : m_textureData->BindingInfos)
				{
					if (textureID == sprite->GetTextureID())
					{
						rangedID = slotID;
						break;
					}
				}
			}

			bool isScaled = sprite->GetScaleX() != 1.f || sprite->GetScaleY() != 1.f || sprite->GetScaleZ() != 1.f;
			bool isRotated = sprite->GetRotationX() != 0.f || sprite->GetRotationY() != 0.f || sprite->GetRotationZ() != 0.f;

			bool isZRotated = sprite->GetRotationZ() > 0.f;

			Vec3 posBL(x, y, z + depth);
			Vec3 posBR(x + width, y, z + depth);
			Vec3 posTR(x + width, y + height, z + depth);
			Vec3 posTL(x, y + height, z + depth);

			if (isScaled || isRotated)
			{
				float halfW = width / 2.f;
				float halfH = height / 2.f;

				// Set positions to be the origin so we rotate around that
				// We have to subtract by our sprite position because of our mvp
				Vec2 tempTL((posTL.x - x) - halfW, (posTL.y - y) - halfH);
				Vec2 tempTR((posTR.x - x) - halfW, (posTR.y - y) - halfH);
				Vec2 tempBL((posBL.x - x) - halfW, (posBL.y - y) - halfH);
				Vec2 tempBR((posBR.x - x) - halfW, (posBR.y - y) - halfH);

				// Create our translation matrices
				Mat4 scaleMat = Mat4::Scale(sprite->GetScaleX(), sprite->GetScaleY(), sprite->GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(sprite->GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(sprite->GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(sprite->GetRotationZ());

				Mat4 translateMat = scaleMat * (rotateXMat * rotateYMat * rotateZMat);

				// Rotate the positions
				tempTR = translateMat * tempTR;
				tempTL = translateMat * tempTL;
				tempBL = translateMat * tempBL;
				tempBR = translateMat * tempBR;

				// Set the positions back to their original position 
				// (doing it how we did will rotate around the center of the sprite)
				// We also have to add back our subtracted sprite positions because of the mvp
				posTR = Vec3((tempTR.x + x) + halfW, (tempTR.y + y) + halfH, posTR.z);
				posTL = Vec3((tempTL.x + x) + halfW, (tempTL.y + y) + halfH, posTL.z);
				posBL = Vec3((tempBL.x + x) + halfW, (tempBL.y + y) + halfH, posBL.z);
				posBR = Vec3((tempBR.x + x) + halfW, (tempBR.y + y) + halfH, posBR.z);
			}

			SpriteVertex topRight;
			SpriteVertex topLeft;
			SpriteVertex bottomLeft;
			SpriteVertex bottomRight;

			topRight.pos = posTR;
			topRight.colour = sprite->GetColour();
			topRight.texCoord = { sprite->GetUVInfo().x, sprite->GetUVInfo().w };
			topRight.texIndex = rangedID;

			topLeft.pos = posTL;
			topLeft.colour = sprite->GetColour();
			topLeft.texCoord = { sprite->GetUVInfo().z, sprite->GetUVInfo().w };
			topLeft.texIndex = rangedID;

			bottomLeft.pos = posBL;
			bottomLeft.colour = sprite->GetColour();
			bottomLeft.texCoord = { sprite->GetUVInfo().z, sprite->GetUVInfo().y };
			bottomLeft.texIndex = rangedID;

			bottomRight.pos = posBR;
			bottomRight.colour = sprite->GetColour();
			bottomRight.texCoord = { sprite->GetUVInfo().x, sprite->GetUVInfo().y };
			bottomRight.texIndex = rangedID;

			if (!sprite->GetTexture())
				sprite->SetTexture(p_errorTexture);

			sData.Vertices.push_back(topRight);
			sData.Vertices.push_back(topLeft);
			sData.Vertices.push_back(bottomLeft);
			sData.Vertices.push_back(bottomRight);
		}

		uint32_t offset = 0;
		for (size_t i = 0; i < sprites.size(); ++i)
		{
			sData.Indices.push_back(offset);
			sData.Indices.push_back(1 + offset);
			sData.Indices.push_back(2 + offset);
			sData.Indices.push_back(2 + offset);
			sData.Indices.push_back(3 + offset);
			sData.Indices.push_back(offset);

			offset += 4;
		}

		// Since all our vertices and indices are in one buffer we need data extract it at the correct places

		if (rData.SpriteInfos.size() > 1)
		{
			sData.StartVertex =
				m_renderInfos[renderDataIndex].SpriteInfos[spriteDataIndex - 1].StartVertex +
				m_renderInfos[renderDataIndex].SpriteInfos[spriteDataIndex - 1].SizeOfVerticesInBytes();
		}
		else
			sData.StartIndex = 0;

		uint32_t indexOffset = 0;
		for (auto& data : rData.SpriteInfos)
		{
			SpriteData& lastData = rData.SpriteInfos[rData.SpriteInfos.size() - 1];

			data.StartIndex = lastData.StartVertex +
				(sizeof(SpriteVertex) * lastData.Vertices.size()) + indexOffset;
			indexOffset += (uint32_t)data.SizeOfIndicesInBytes();
		}
	}

	void SpriteRenderer::allocateDescriptorInfo()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = p_pipeline->GetDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = p_pipeline->GetDescriptorSetLayoutPtr();

		if (vkAllocateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			&allocInfo, &m_descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");

		p_pipeline->SetIsDescriptorSetAllocated(true);
	}

	void SpriteRenderer::createBuffer(uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		if (rData.SpriteInfos.empty()) return;

		if (rData.IsBufferCreated)
		{
			vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), rData.Buffer, nullptr);
			vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), rData.BufferMemory, nullptr);
		}

		VkDeviceSize bufferSize = 0;

		rData.TotalVerticeSize = 0;
		for (auto& data : rData.SpriteInfos)
		{
			bufferSize += data.SizeInBytes();
			rData.TotalVerticeSize += (uint32_t)data.SizeOfVerticesInBytes();
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			stagingBufferMemory, 0, bufferSize, 0, &data);

		// Create the vertices
		for (auto& spriteInfo : rData.SpriteInfos)
			memcpy((char*)data + spriteInfo.StartVertex, spriteInfo.Vertices.data(), spriteInfo.SizeOfVerticesInBytes());

		// Create the indices
		for (auto& spriteInfo : rData.SpriteInfos)
			memcpy((char*)data + spriteInfo.StartIndex, spriteInfo.Indices.data(), spriteInfo.SizeOfIndicesInBytes());

		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory);

		Core::GetCore().CreateBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			rData.Buffer, rData.BufferMemory);

		Core::GetCore().CopyBuffer(stagingBuffer, rData.Buffer, bufferSize);

		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, nullptr);

		rData.IsBufferCreated = true;
	}

	void SpriteRenderer::createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_uniformBuffer, m_uniformMemory);
	}

	void SpriteRenderer::updateDescriptorWrites()
	{
		std::array<VkWriteDescriptorSet, 3> setWrites{};

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		setWrites[0] = {};
		setWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[0].dstBinding = 0;
		setWrites[0].dstArrayElement = 0;
		setWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setWrites[0].descriptorCount = 1;
		setWrites[0].pBufferInfo = &bufferInfo;
		setWrites[0].dstSet = m_descriptorSet;

		setWrites[1] = {};
		setWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[1].dstBinding = 1;
		setWrites[1].dstArrayElement = 0;
		setWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		setWrites[1].descriptorCount = 1;
		setWrites[1].pBufferInfo = 0;
		setWrites[1].pImageInfo = &m_samplerImageInfo;
		setWrites[1].dstSet = m_descriptorSet;

		setWrites[2] = {};
		setWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[2].dstBinding = 2;
		setWrites[2].dstArrayElement = 0;
		setWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		setWrites[2].descriptorCount = static_cast<uint32_t>(m_textureData->DescriptorImageInfos.size());
		setWrites[2].pBufferInfo = 0;
		setWrites[2].pImageInfo = m_textureData->DescriptorImageInfos.data();
		setWrites[2].dstSet = m_descriptorSet;

		vkUpdateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}

	void SpriteRenderer::updateBuffer(uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		VkDeviceSize bufferSize = rData.TotalVerticeSize;

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			rData.BufferMemory, 0, bufferSize, 0, &data);

		for (auto& spriteInfo : rData.SpriteInfos)
			memcpy((char*)data + spriteInfo.StartVertex, spriteInfo.Vertices.data(), spriteInfo.SizeOfVerticesInBytes());

		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), rData.BufferMemory);
	}

	void SpriteRenderer::updateUBO(const Mat4& pv)
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		UniformBufferObject ubo{};
		ubo.pv = pv;

		void* data;
		vkMapMemory(pDevice->GetDevice(), m_uniformMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(pDevice->GetDevice(), m_uniformMemory);
	}

	void SpriteRenderer::updateVertices(std::vector<std::shared_ptr<Sprite>>& sprites,
		uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		if (rData.Sprites.size() == 0) return;

		size_t i = 0;
		uint32_t sIndex = 0;
		for (std::shared_ptr<Sprite> sprite : rData.Sprites)
		{
			// Once we finish updating one set of vertices we want to update the next
			if (i >= rData.SpriteInfos[sIndex].Vertices.size())
			{
				++sIndex;

				for (int i = sIndex; i < rData.SpriteInfos.size(); ++i)
				{
					if (rData.SpriteInfos[sIndex].CanUpdateVertices)
						break;

					++sIndex;
				}

				i = 0;
			}

			float width = sprite->GetWidth();
			float height = sprite->GetHeight();
			float depth = sprite->GetDepth();
			float x = sprite->GetX();
			float y = sprite->GetY();
			float z = sprite->GetZ();

			bool isScaled = sprite->GetScaleX() != 1.f || sprite->GetScaleY() != 1.f || sprite->GetScaleZ() != 1.f;
			bool isRotated = sprite->GetRotationX() != 0.f || sprite->GetRotationY() != 0.f || sprite->GetRotationZ() != 0.f;

			Vec3 posBL(x, y, z + depth);
			Vec3 posBR(x + width, y, z + depth);
			Vec3 posTR(x + width, y + height, z + depth);
			Vec3 posTL(x, y + height, z + depth);

			if (isScaled || isRotated)
			{
				float halfW = width / 2.f;
				float halfH = height / 2.f;

				// Set positions to be the origin so we rotate around that
				// We have to subtract by our sprite position because of our mvp
				Vec2 tempTL((posTL.x - x) - halfW, (posTL.y - y) - halfH);
				Vec2 tempTR((posTR.x - x) - halfW, (posTR.y - y) - halfH);
				Vec2 tempBL((posBL.x - x) - halfW, (posBL.y - y) - halfH);
				Vec2 tempBR((posBR.x - x) - halfW, (posBR.y - y) - halfH);

				// Create our translation matrices
				Mat4 scaleMat = Mat4::Scale(sprite->GetScaleX(), sprite->GetScaleY(), sprite->GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(sprite->GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(sprite->GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(sprite->GetRotationZ());

				Mat4 translateMat = scaleMat * (rotateXMat * rotateYMat * rotateZMat);

				// Rotate the positions
				tempTR = translateMat * tempTR;
				tempTL = translateMat * tempTL;
				tempBL = translateMat * tempBL;
				tempBR = translateMat * tempBR;

				// Set the positions back to their original position 
				// (doing it how we did will rotate around the center of the sprite)
				// We also have to add back our subtracted sprite positions because of the mvp
				posTR = Vec3((tempTR.x + x) + halfW, (tempTR.y + y) + halfH, posTR.z);
				posTL = Vec3((tempTL.x + x) + halfW, (tempTL.y + y) + halfH, posTL.z);
				posBL = Vec3((tempBL.x + x) + halfW, (tempBL.y + y) + halfH, posBL.z);
				posBR = Vec3((tempBR.x + x) + halfW, (tempBR.y + y) + halfH, posBR.z);
			}

			rData.SpriteInfos[sIndex].Vertices[i].pos = posTR;
			rData.SpriteInfos[sIndex].Vertices[i].colour = sprite->GetColour();
			rData.SpriteInfos[sIndex].Vertices[i].texCoord = { sprite->GetUVInfo().x, sprite->GetUVInfo().w };

			rData.SpriteInfos[sIndex].Vertices[i + 1].pos = posTL;
			rData.SpriteInfos[sIndex].Vertices[i + 1].colour = sprite->GetColour();
			rData.SpriteInfos[sIndex].Vertices[i + 1].texCoord = { sprite->GetUVInfo().z, sprite->GetUVInfo().w };

			rData.SpriteInfos[sIndex].Vertices[i + 2].pos = posBL;
			rData.SpriteInfos[sIndex].Vertices[i + 2].colour = sprite->GetColour();
			rData.SpriteInfos[sIndex].Vertices[i + 2].texCoord = { sprite->GetUVInfo().z, sprite->GetUVInfo().y };

			rData.SpriteInfos[sIndex].Vertices[i + 3].pos = posBR;
			rData.SpriteInfos[sIndex].Vertices[i + 3].colour = sprite->GetColour();
			rData.SpriteInfos[sIndex].Vertices[i + 3].texCoord = { sprite->GetUVInfo().x, sprite->GetUVInfo().y };

			i += 4;
		}
	}

	void SpriteRenderer::swapchainRecreateEvent(SwapchainRecreateEvent& e)
	{
	}
}
