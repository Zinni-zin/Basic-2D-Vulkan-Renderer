#include "Headers/SpriteRenderer.h"

#include <iostream>
#include <stdexcept>

#define USE_SPRITE_PIPELINE

#include <cassert>

namespace ZVK
{
	struct SpriteSort
	{
		uint32_t errorTextureID;

		bool operator()(Sprite& v1, Sprite& v2)
		{
			uint32_t v1ID = (v1.GetTexture() == nullptr) ? errorTextureID : v1.GetTextureID();
			uint32_t v2ID = (v2.GetTexture() == nullptr) ? errorTextureID : v2.GetTextureID();

			return v1ID < v2ID;
		}
	};

	SpriteRenderer::SpriteRenderer(SpritePipeline* pPipeline, const std::string& errorTexturePath)
		: p_pipeline(pPipeline), m_clearColour(0.f, 0.f, 0.f, 1.f)
	{
		init(errorTexturePath);
	}

	SpriteRenderer::SpriteRenderer(const std::string& errorTexturePath, const std::string& vertPath,
		const std::string& fragPath) 
		:m_clearColour(0.f, 0.f, 0.f, 1.f)
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

		vkDestroyBuffer(pDevice->GetDevice(), m_indexBuffer, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_indexMemory, nullptr);

		vkDestroyBuffer(pDevice->GetDevice(), m_vertexBuffer, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_vertexMemory, nullptr);

		vkDestroyBuffer(pDevice->GetDevice(), m_uniformBuffer, nullptr);
		vkFreeMemory(pDevice->GetDevice(), m_uniformMemory, nullptr);

		if (m_descriptorInfoCreated)
		{
			vkFreeDescriptorSets(pDevice->GetDevice(), p_pipeline->GetDescriptorPool(),
				1, &m_descriptorSet);

			m_descriptorImageInfos.clear();
		}
		m_vertexBufferCreated = false;

		if (m_canDeletePipeline)
		{
			if (p_pipeline)
			{
				delete p_pipeline;
				p_pipeline = nullptr;
			}
		}
	}

	void SpriteRenderer::init(const std::string& errorTexturePath)
	{
		m_vertices.reserve(MAX_VERTEX_COUNT);
		m_indices.resize(MAX_INDEX_COUNT);

		m_inUseTextures.reserve(Core::GetCore().GetMaxTextureSlots());
		m_descriptorImageInfos.reserve(Core::GetCore().GetMaxTextureSlots());

		SpriteVertex vertex;
		vertex.pos = { 32, 32, 0 };
		vertex.colour = { 1.f, 1.f, 1.f };
		vertex.texCoord = { 0.f, 0.f };
		vertex.texIndex = 0;

		m_vertices.push_back(vertex);

		createVertexBuffer();

		uint32_t offset = 0;
		for (size_t i = 0; i < MAX_INDEX_COUNT; i += 6)
		{
			m_indices[i] = offset;
			m_indices[i + 1] = 1 + offset;
			m_indices[i + 2] = 2 + offset;

			m_indices[i + 3] = 2 + offset;
			m_indices[i + 4] = 3 + offset;
			m_indices[i + 5] = offset;

			offset += 4;
		}

		createIndexBuffer();
		createUniformBuffer();

		p_errorTexture = std::make_shared<Texture2D>(errorTexturePath);

		m_swapchainRecreateEvent = std::bind(&SpriteRenderer::swapchainRecreateEvent,
			std::ref(*this), std::placeholders::_1);

		Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);

		// Delete the vertex... it's buffer and memory
		// The Vertex buffer was only a temp variable to not get the weird bug
		// The bug is probably due to the size of the index buffer

		m_vertices.clear();

		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), m_vertexBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), m_vertexMemory, nullptr);

		m_vertexBufferCreated = false;
	}

	void SpriteRenderer::Draw(Renderer& renderer, std::vector<Sprite>& sprites, const Camera& cam, 
		bool canUpdateVertexBuffer, bool canSortSprites)
	{
		if (sprites.empty()) return;
	
		// I'll probably work ona  general flushing system sometime....
		if (sprites.size() > MAX_QUAD_COUNT)
			throw std::runtime_error("Sprite size is greater than the quad count( " + 
				std::to_string(MAX_QUAD_COUNT) + ")!");

		if(canUpdateVertexBuffer && m_vertexBufferCreated)
			updateVertexBuffer(sprites);

		if (m_swapchainRecreated)
		{
			allocateDescriptorInfo();
			updateDescriptorWrites();
			m_swapchainRecreated = false;
		}

		if (sprites.size() != m_spriteCount)
		{
			if (canSortSprites)
			{
				SpriteSort spriteSort = { p_errorTexture->GetID() };
				std::sort(sprites.begin(), sprites.end(), spriteSort);
			}

			populateVertices(sprites);
			allocateDescriptorInfo();
			updateDescriptorWrites();
		}

		updateUBO(cam);

#ifdef FLUSH_MAX_TEXTURES
		/* We need to change our textures per flush(draw indexed command)
		* I don't recommend using FLUSH_MAX_TEXTURES because the descriptor sets are updated
		* before the command buffer ends...
		*/
		if (Core::GetCore().Get2DTextures().size() > Core::GetCore().GetMaxTextureSlots())
		{
			// Some lines would be long without this, so this is just the texture size
			size_t tSize = Core::GetCore().Get2DTextures().size();

			// Change all the first max textures that we are going to be using
			for (size_t i = 0; i < Core::GetCore().GetMaxTextureSlots(); ++i)
				m_inUseTextures[i] = Core::GetCore().Get2DTextures()[i];
			
			// Change the descriptorInfo, tbh the line above could probably be removed 
			// and we could just use the 2D texture vector in core, but I'm lazy
			for (uint32_t i = 0; i < m_inUseTextures.size(); ++i)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.sampler = m_inUseTextures[i]->GetSampler();
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = m_inUseTextures[i]->GetImageView();

				m_descriptorImageInfos[i] = imageInfo;
			}

			updateDescriptorWrites();

			beginFlush(renderer);

			flush(renderer, static_cast<uint32_t>(m_indicesCount[0]));

			int k = 1; // k allows us to know which set of indices to use
			uint32_t firstIndex = 0; // First index tells us when to start drawing the next batch of indices

			const uint32_t textureSlots = Core::GetCore().GetMaxTextureSlots();

			// Loop through all textures and increase it by the max texture array size
			for (size_t i = textureSlots; i < Core::GetCore().Get2DTextures().size(); i += textureSlots)
			{
				firstIndex += m_indicesCount[k - 1]; 
				
				// If the end batch doesn't extend to the max texture array size we get the remaining amount
				size_t size = (i + textureSlots > tSize)? tSize - i : textureSlots;

				// Update the textures we're using, probably don't need the two lines below this comment
				for (size_t j = 0; j < size; ++j)
					m_inUseTextures[j] = Core::GetCore().Get2DTextures()[i + j];
				
				for (uint32_t i = 0; i < m_inUseTextures.size(); ++i)
				{
					VkDescriptorImageInfo imageInfo{};
					imageInfo.sampler = m_inUseTextures[i]->GetSampler();
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = m_inUseTextures[i]->GetImageView();

					m_descriptorImageInfos[i] = imageInfo;
				}
				updateDescriptorWrites();

				flush(renderer, m_indicesCount[k], firstIndex);
				++k;
			}
			endFlush(renderer);
			return;
		}
#else
		assert(Core::GetCore().Get2DTextures().size() < Core::GetCore().GetMaxTextureSlots() &&
		"To use more than the max texture amount add: FLUSH_MAX_TEXTURES as a preprocessor definition");
#endif
		
		draw(renderer);
	}

	void SpriteRenderer::draw(Renderer& renderer)
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

		if (vkBeginCommandBuffer(renderer.GetCurrentCommandBuffer(), &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin command buffer!");

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = p_pipeline->GetRenderPass();
		renderPassBeginInfo.framebuffer = p_pipeline->GetSwapchainFrameBuffers()[renderer.GetImageIndex()];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = pSwapchain->GetSwapchainExtent();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(renderer.GetCurrentCommandBuffer(),
			&renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = static_cast<float>(pSwapchain->GetSwapchainExtent().width);
		viewport.height = static_cast<float>(pSwapchain->GetSwapchainExtent().height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = pSwapchain->GetSwapchainExtent();
		vkCmdSetViewport(renderer.GetCurrentCommandBuffer(), 0, 1, &viewport);
		vkCmdSetScissor(renderer.GetCurrentCommandBuffer(), 0, 1, &scissor);

		vkCmdBindPipeline(renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipeline());

		vkCmdBindDescriptorSets(renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipelineLayout(),
			0, 1, &m_descriptorSet, 0, 0);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(renderer.GetCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(renderer.GetCurrentCommandBuffer(), m_indexBuffer,
			0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(renderer.GetCurrentCommandBuffer(),
			static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(renderer.GetCurrentCommandBuffer());

		if (vkEndCommandBuffer(renderer.GetCurrentCommandBuffer()) != VK_SUCCESS)
			throw std::runtime_error("Failed to end command buffer");
	}

	void SpriteRenderer::beginFlush(Renderer& renderer)
	{
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

		// updateUBO();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { { 0.f, 0.f, 0.f, 1.f } };
		clearValues[1].depthStencil = { 1.f, 0 };

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(renderer.GetCurrentCommandBuffer(), &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin command buffer!");

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = p_pipeline->GetRenderPass();
		renderPassBeginInfo.framebuffer = pSwapchain->GetSwapchainFrameBuffers()[renderer.GetImageIndex()];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = pSwapchain->GetSwapchainExtent();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(renderer.GetCurrentCommandBuffer(),
			&renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void SpriteRenderer::flush(Renderer& renderer, uint32_t indexCount, uint32_t firstIndex)
	{
		vkCmdBindPipeline(renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipeline());

		// Maybe use different discriptor sets??
		vkCmdBindDescriptorSets(renderer.GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->GetPipelineLayout(),
			0, 1, &m_descriptorSet, 0, 0);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(renderer.GetCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(renderer.GetCurrentCommandBuffer(), m_indexBuffer,
			0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(renderer.GetCurrentCommandBuffer(), indexCount, 1, firstIndex, 0, 0);
	}

	void SpriteRenderer::endFlush(Renderer& renderer)
	{
		vkCmdEndRenderPass(renderer.GetCurrentCommandBuffer());

		if (vkEndCommandBuffer(renderer.GetCurrentCommandBuffer()) != VK_SUCCESS)
			throw std::runtime_error("Failed to end command buffer");
	}

	void SpriteRenderer::updateUBO(const Camera& cam)
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		UniformBufferObject ubo{};
		ubo.pv = cam.GetPV();

		void* data;
		vkMapMemory(pDevice->GetDevice(), m_uniformMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(pDevice->GetDevice(), m_uniformMemory);
	}

	void SpriteRenderer::populateVertices(std::vector<Sprite>& sprites)
	{
		m_spriteCount = 0;

		if (m_vertexBufferCreated)
		{
			vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), m_vertexBuffer, nullptr);
			vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), m_vertexMemory, nullptr);

			m_vertices.clear();
			m_inUseTextures.clear();
		}

		uint32_t errorID = p_errorTexture->GetRangedID();

#ifdef FLUSH_MAX_TEXTURES
		m_indicesCount.clear();
		std::vector<uint32_t> newIndicesFlush;
		uint32_t indicesIndex = 0;
#endif

		for (Sprite& sprite : sprites)
		{
			float x = sprite.GetX();
			float y = sprite.GetY();
			float z = sprite.GetZ();
			float width = sprite.GetWidth();
			float height = sprite.GetHeight();
			float depth = sprite.GetDepth();

			int rangedID = (sprite.GetTexture() != nullptr) ? sprite.GetTextureRangedID() : errorID;

			bool isScaled = sprite.GetScaleX() != 1.f || sprite.GetScaleY() != 1.f|| sprite.GetScaleZ() != 1.f;
			bool isRotated = sprite.GetRotationX() != 0.f || sprite.GetRotationY() != 0.f || sprite.GetRotationZ() != 0.f;

			Vec3 posBL(x, y, z + depth);
			Vec3 posBR(x + width, y, z + depth);
			Vec3 posTR(x + width, y + height, z + depth);
			Vec3 posTL(x, y + height, z + depth);

			if (isScaled || isRotated)
			{
				float halfW = sprite.GetWidth() / 2.f;
				float halfH = sprite.GetHeight() / 2.f;

				// Set positions to be the origin so we rotate around that
				// We have to subtract by our sprite position because of our mvp
				Vec2 tempTL((posTL.x - sprite.GetX()) - halfW, (posTL.y - sprite.GetY()) - halfH);
				Vec2 tempTR((posTR.x - sprite.GetX()) - halfW, (posTR.y - sprite.GetY()) - halfH);
				Vec2 tempBL((posBL.x - sprite.GetX()) - halfW, (posBL.y - sprite.GetY()) - halfH);
				Vec2 tempBR((posBR.x - sprite.GetX()) - halfW, (posBR.y - sprite.GetY()) - halfH);

				// Create our translation matrices
				Mat4 scaleMat = Mat4::Scale(sprite.GetScaleX(), sprite.GetScaleY(), sprite.GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(sprite.GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(sprite.GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(sprite.GetRotationZ());

				Mat4 translateMat = scaleMat * (rotateXMat * rotateYMat * rotateZMat);

				// Scale and rotate the positions
				tempTR = translateMat * tempTR;
				tempTL = translateMat * tempTL;
				tempBL = translateMat * tempBL;
				tempBR = translateMat * tempBR;

				// Set the positions back to their original position 
				// (doing it how we did will rotate around the center of the sprite)
				// We also have to add back our subtracted sprite positions because of the mvp
				posTR = Vec3((tempTR.x + sprite.GetX()) + halfW, (tempTR.y + sprite.GetY()) + halfH, posTR.z);
				posTL = Vec3((tempTL.x + sprite.GetX()) + halfW, (tempTL.y + sprite.GetY()) + halfH, posTL.z);
				posBL = Vec3((tempBL.x + sprite.GetX()) + halfW, (tempBL.y + sprite.GetY()) + halfH, posBL.z);
				posBR = Vec3((tempBR.x + sprite.GetX()) + halfW, (tempBR.y + sprite.GetY()) + halfH, posBR.z);
			}

			SpriteVertex topRight;
			SpriteVertex topLeft;
			SpriteVertex bottomLeft;
			SpriteVertex bottomRight;

			bottomLeft.pos = posBL;
			bottomLeft.colour = sprite.GetColour();
			bottomLeft.texCoord = { sprite.GetUVInfo().z, sprite.GetUVInfo().y };
			bottomLeft.texIndex = rangedID;

			bottomRight.pos = posBR;
			bottomRight.colour = sprite.GetColour();
			bottomRight.texCoord = { sprite.GetUVInfo().x, sprite.GetUVInfo().y };
			bottomRight.texIndex = rangedID;

			topRight.pos = posTR;
			topRight.colour = sprite.GetColour();
			topRight.texCoord = { sprite.GetUVInfo().x, sprite.GetUVInfo().w };
			topRight.texIndex = rangedID;

			topLeft.pos = posTL;
			topLeft.colour = sprite.GetColour();
			topLeft.texCoord = { sprite.GetUVInfo().z, sprite.GetUVInfo().w };
			topLeft.texIndex = rangedID;

			if (!sprite.GetTexture())
				sprite.SetTexture(p_errorTexture);

			m_vertices.push_back(bottomLeft);
			m_vertices.push_back(bottomRight);
			m_vertices.push_back(topRight);
			m_vertices.push_back(topLeft);

			++m_spriteCount;
			
#ifdef FLUSH_MAX_TEXTURES
			// Figure out how many indices we have per flush
			// There is a bug I'm too lazy to fix and that is if no sprite except the first has a rangedIndex of 0

			// If we have no indices add one
			if (m_indicesCount.empty())
			{
				newIndicesFlush.push_back(sprite.GetTextureID());
				m_indicesCount.push_back(0);
			}
			
			// Figure out if we have reached the first set of sprites to draw
			bool isNewFlush = true;
			for (uint32_t i : newIndicesFlush)
			{
				if (rangedID != 0 || sprite.GetTextureID() == i)
					isNewFlush = false;
			}

			// If we have a new set of sprites to draw to our two lists and increase our index
			if (isNewFlush)
			{
				m_indicesCount.push_back(0);
				newIndicesFlush.push_back(sprite.GetTextureID());
				++indicesIndex;
			}

			// If the list is not empty add 6 indices to whatever index we are on
			if (!m_indicesCount.empty())
				m_indicesCount[indicesIndex] += 6;
#endif
		}
		
		for (auto& texture : Core::GetCore().Get2DTextures())
		{
			if (m_inUseTextures.size() < Core::GetCore().GetMaxTextureSlots())
				m_inUseTextures.push_back(texture);
			else
				break;
		}

		createVertexBuffer();
	}

	void SpriteRenderer::allocateDescriptorInfo()
	{
		if (m_descriptorInfoCreated)
		{
			vkFreeDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
				p_pipeline->GetDescriptorPool(), 1, &m_descriptorSet);

			m_descriptorImageInfos.clear();
		}

		for (uint32_t i = 0; i < m_inUseTextures.size(); ++i)
		{
			VkDescriptorImageInfo imageInfo{};

			imageInfo.sampler = m_inUseTextures[i]->GetSampler();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_inUseTextures[i]->GetImageView();

			m_descriptorImageInfos.push_back(imageInfo);
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = p_pipeline->GetDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = p_pipeline->GetDescriptorSetLayoutPtr();

		if (vkAllocateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			&allocInfo, &m_descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");

		m_descriptorInfoCreated = true;
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
		setWrites[1].descriptorCount = static_cast<uint32_t>(m_descriptorImageInfos.size());
		setWrites[1].dstSet = m_descriptorSet;
		setWrites[1].pBufferInfo = 0;
		setWrites[1].pImageInfo = m_descriptorImageInfos.data();

		setWrites[2] = {};
		setWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[2].dstBinding = 2;
		setWrites[2].dstArrayElement = 0;
		setWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		setWrites[2].descriptorCount = static_cast<uint32_t>(m_descriptorImageInfos.size());
		setWrites[2].dstSet = m_descriptorSet;
		setWrites[2].pBufferInfo = 0;
		setWrites[2].pImageInfo = m_descriptorImageInfos.data();

		vkUpdateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}

	void SpriteRenderer::createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory);

		Core::GetCore().CreateBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexMemory);

		Core::GetCore().CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, nullptr);

		m_vertexBufferCreated = true;
	}

	void SpriteRenderer::updateVertexBuffer(std::vector<Sprite>& sprites)
	{
		VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

		uint32_t errorID = p_errorTexture->GetRangedID();

		size_t i = 0;
		for (Sprite& sprite : sprites)
		{
			float x = sprite.GetX();
			float y = sprite.GetY();
			float z = sprite.GetZ();
			float width = sprite.GetWidth();
			float height = sprite.GetHeight();
			float depth = sprite.GetDepth();

			int rangedID = (sprite.GetTexture() != nullptr) ? sprite.GetTextureRangedID() : errorID;

			bool isScaled = sprite.GetScaleX() != 1.f || sprite.GetScaleY()  != 1.f|| sprite.GetScaleZ() != 1.f;
			bool isRotated = sprite.GetRotationX() != 0.f || sprite.GetRotationY() != 0.f || sprite.GetRotationZ() != 0.f;

			Vec3 posTR(x + width, y + height, z + depth);
			Vec3 posTL(x, y + height, z + depth);
			Vec3 posBL(x, y, z + depth);
			Vec3 posBR(x + width, y, z + depth);

			if (isScaled || isRotated)
			{
				float halfW = sprite.GetWidth() / 2.f;
				float halfH = sprite.GetHeight() / 2.f;

				// Set positions to be the origin so we rotate around that
				// We have to subtract by our sprite position because of our mvp
				Vec2 tempTL((posTL.x - sprite.GetX()) - halfW, (posTL.y - sprite.GetY()) - halfH);
				Vec2 tempTR((posTR.x - sprite.GetX()) - halfW, (posTR.y - sprite.GetY()) - halfH);
				Vec2 tempBL((posBL.x - sprite.GetX()) - halfW, (posBL.y - sprite.GetY()) - halfH);
				Vec2 tempBR((posBR.x - sprite.GetX()) - halfW, (posBR.y - sprite.GetY()) - halfH);

				// Create our translation matrices
				Mat4 scaleMat = Mat4::Scale(sprite.GetScaleX(), sprite.GetScaleY(), sprite.GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(sprite.GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(sprite.GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(sprite.GetRotationZ());

				Mat4 translateMat = scaleMat * (rotateXMat * rotateYMat * rotateZMat);
				
				// Rotate the positions
				tempTR = translateMat * tempTR;
				tempTL = translateMat * tempTL;
				tempBL = translateMat * tempBL;
				tempBR = translateMat * tempBR;

				// Set the positions back to their original position 
				// (doing it how we did will rotate around the center of the sprite)
				// We also have to add back our subtracted sprite positions because of the mvp
				posTR = Vec3((tempTR.x + sprite.GetX()) + halfW, (tempTR.y + sprite.GetY()) + halfH, posTR.z);
				posTL = Vec3((tempTL.x + sprite.GetX()) + halfW, (tempTL.y + sprite.GetY()) + halfH, posTL.z);
				posBL = Vec3((tempBL.x + sprite.GetX()) + halfW, (tempBL.y + sprite.GetY()) + halfH, posBL.z);
				posBR = Vec3((tempBR.x + sprite.GetX()) + halfW, (tempBR.y + sprite.GetY()) + halfH, posBR.z);
			}

			SpriteVertex topRight;
			SpriteVertex topLeft;
			SpriteVertex bottomLeft;
			SpriteVertex bottomRight;

			topRight.pos = posTR;
			topRight.colour = sprite.GetColour();
			topRight.texCoord = { sprite.GetUVInfo().x, sprite.GetUVInfo().w };
			topRight.texIndex = rangedID;

			topLeft.pos = posTL;
			topLeft.colour = sprite.GetColour();
			topLeft.texCoord = { sprite.GetUVInfo().z, sprite.GetUVInfo().w };
			topLeft.texIndex = rangedID;

			bottomLeft.pos = posBL;
			bottomLeft.colour = sprite.GetColour();
			bottomLeft.texCoord = { sprite.GetUVInfo().z, sprite.GetUVInfo().y };
			bottomLeft.texIndex = rangedID;

			bottomRight.pos = posBR;
			bottomRight.colour = sprite.GetColour();
			bottomRight.texCoord = { sprite.GetUVInfo().x, sprite.GetUVInfo().y };
			bottomRight.texIndex = rangedID;

			if (!sprite.GetTexture())
				sprite.SetTexture(p_errorTexture);

			m_vertices[i] =topRight;
			m_vertices[i+1] =topLeft;
			m_vertices[i+2] =bottomLeft;
			m_vertices[i+3] =bottomRight;

			i += 4;
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory);

		Core::GetCore().CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, nullptr);
	}

	void SpriteRenderer::createIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_indices.data(), (size_t)bufferSize);
		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory);

		Core::GetCore().CreateBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexMemory);

		Core::GetCore().CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), stagingBufferMemory, nullptr);
	}

	void SpriteRenderer::createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_uniformBuffer, m_uniformMemory);
	}

	void SpriteRenderer::swapchainRecreateEvent(SwapchainRecreateEvent& e)
	{
		vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), m_vertexBuffer, nullptr);
		vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), m_vertexMemory, nullptr);

		createVertexBuffer();

		m_swapchainRecreated = true;
	}
}
