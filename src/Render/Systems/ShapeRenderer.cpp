#include "../../../Headers/Render/Systems/ShapeRenderer.h"

#include <iostream>
#include <stdexcept>

#include <cassert>

#define MAX_QUAD_COUNT 880
#define MAX_VERTEX_COUNT MAX_QUAD_COUNT * 4
#define MAX_INDEX_COUNT MAX_QUAD_COUNT * 6

namespace ZVK
{
	ShapeRenderer::ShapeRenderer(Renderer& renderer, ShapePipeline* pPipeline)
		: m_renderer(renderer), p_pipeline(pPipeline)
	{
		init();
	}

	ShapeRenderer::ShapeRenderer(Renderer& renderer, const std::string& vertPath,
		const std::string& fragPath)
		: m_renderer(renderer)
	{
		try
		{
			p_pipeline = new ShapePipeline(vertPath, fragPath);
		}
		catch (std::exception e)
		{
			std::cout << "Error creating pipeline: " << e.what() << std::endl;
		}

		init();
		m_canDeletePipeline = true;
	}

	ShapeRenderer::~ShapeRenderer()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		for (auto cmd : m_uploadUpdatedVerticesCmds)
		{
			m_renderer.RemoveUpdateVertexCmd(cmd);
			if (cmd)
			{
				delete cmd;
				cmd = nullptr;
			}
		}

		for (auto cmd : m_shapeRendererCmds)
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

		if (m_uboCreated)
		{
			vkDestroyBuffer(pDevice->GetDevice(), m_uniformBuffer, nullptr);
			vkFreeMemory(pDevice->GetDevice(), m_uniformMemory, nullptr);
		}

		vkDestroyDescriptorPool(pDevice->GetDevice(), m_descriptorPool, nullptr);

		if (m_canDeletePipeline)
		{
			if (p_pipeline)
			{
				delete p_pipeline;
				p_pipeline = nullptr;
			}
		}
	}

	void ShapeRenderer::init()
	{
		createUniformBuffer();
		createDescriptorPool();

		m_swapchainRecreateEvent = std::bind(&ShapeRenderer::swapchainRecreateEvent,
			std::ref(*this), std::placeholders::_1);

		Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);

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

		allocateDescriptorInfo();
		updateDescriptorWrites();
	}

	void ShapeRenderer::allocateDescriptorInfo()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = p_pipeline->GetDescriptorSetLayoutPtr();

		if (vkAllocateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			&allocInfo, &m_descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");

		p_pipeline->SetIsDescriptorSetAllocated(true);
	}

	void ShapeRenderer::updateDescriptorWrites()
	{
		std::array<VkWriteDescriptorSet, 1> setWrites{};

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ShapeUniformBufferObject);

		setWrites[0] = {};
		setWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[0].dstBinding = 0;
		setWrites[0].dstArrayElement = 0;
		setWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setWrites[0].descriptorCount = 1;
		setWrites[0].pBufferInfo = &bufferInfo;
		setWrites[0].dstSet = m_descriptorSet;

		vkUpdateDescriptorSets(Core::GetCore().GetDevice()->GetDevice(),
			static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}

	void ShapeRenderer::createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(ShapeUniformBufferObject);

		Core::GetCore().CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_uniformBuffer, m_uniformMemory);

		m_uboCreated = true;
	}

	void ShapeRenderer::createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 1> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(Core::GetCore().GetDevice()->GetDevice(), &poolInfo,
			nullptr, &m_descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool!");
	}

	void ShapeRenderer::Draw(std::vector<std::shared_ptr<IShape>>& shapes, const Mat4& mvp,
		bool canUpdateVertexBuffer)
	{
		if (shapes.empty()) return;

		auto shapeList = m_loadedLists.find(&shapes);
		bool isFound = false;

		if (shapeList == m_loadedLists.end())
		{
			m_loadedLists[&shapes] = { (uint32_t)shapes.size(), m_listCount++ };
			shapeList = m_loadedLists.find(&shapes);
		}
		else if (shapeList->second.ListSize != (uint32_t)shapes.size())
		{
			// This is inefficient but it works
			for (ShapeRendererCmd* rCmd : m_shapeRendererCmds)
			{
				m_renderer.RemoveRenderCmd(rCmd);

				if (rCmd != nullptr)
				{
					delete rCmd;
					rCmd = nullptr;
				}
			}

			for (UploadUpdatedShapeVerticesCmd* updateVCmd : m_uploadUpdatedVerticesCmds)
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

			changedListIndex = shapeList->second.Index;

			m_shapeRendererCmds.clear();
			m_uploadUpdatedVerticesCmds.clear();
			m_renderInfos.clear();
			m_updateVerticesCmds.clear();
			m_listCount = 0;
			shapeList->second.ListSize = (uint32_t)shapes.size();

			for (auto& [key, val] : m_loadedLists)
				val.IsListAdded = false;

			return;
		}
		else if (shapeList->second.IsListAdded == true)
			isFound = true;

		if (changedListIndex >= 0 && shapeList->second.Index > changedListIndex)
			return;
		else if (shapeList->second.Index == changedListIndex)
			changedListIndex = -1;

		if (!isFound)
		{
			// Add new list to the render data and create a new one if we go over the max quad count
			if (!m_renderInfos.empty() &&
				m_renderInfos[m_renderInfos.size() - 1].ShapeCount < MAX_QUAD_COUNT)
			{
				uint32_t renderIndex = (uint32_t)m_renderInfos.size() - 1;

				RenderData& rData = m_renderInfos[renderIndex];

				uint32_t shapeIndex = 0;

				if (rData.ShapeCount + (uint32_t)shapes.size() > MAX_QUAD_COUNT)
				{
					// Make sure we caculate for a list size greater than MAX_QUAD_COUNT
					// We also need to account for a list size less than MAX_QUAD_COUNT

					std::vector<std::shared_ptr<IShape>> tempShapes;

					uint32_t amountAdded = 0;
					for (size_t i = 0; rData.ShapeCount + i < MAX_QUAD_COUNT; ++i)
					{
						tempShapes.push_back(shapes[i]);
						++amountAdded;
					}

					rData.ShapeInfos.push_back(ShapeData{});
					shapeIndex = (uint32_t)m_renderInfos[renderIndex].ShapeInfos.size() - 1;

					rData.ShapeInfos[shapeIndex].canUpdateVertices = canUpdateVertexBuffer;

					populateVertices(tempShapes, renderIndex, shapeIndex);

					if (canUpdateVertexBuffer)
						std::copy(tempShapes.begin(), tempShapes.end(), std::back_inserter(rData.Shapes));

					rData.ShapeCount += (uint32_t)tempShapes.size();

					std::vector<std::shared_ptr<IShape>> 
						newShapes(std::next(shapes.begin() + amountAdded - 1), shapes.end());

					ShapeRendererCmd* cmd = new ShapeRendererCmd(this, (uint32_t)shapes.size() * 6,
						renderIndex, shapeIndex);
					m_shapeRendererCmds.push_back(cmd);
					m_renderer.AddRenderCmd(cmd);

					if (amountAdded != shapes.size())
						drawRecursive(newShapes, canUpdateVertexBuffer);
				}
				else
				{
					rData.ShapeInfos.push_back(ShapeData{});

					shapeIndex = (uint32_t)m_renderInfos[renderIndex].ShapeInfos.size() - 1;

					populateVertices(shapes, renderIndex, shapeIndex);

					if (canUpdateVertexBuffer)
						std::copy(shapes.begin(), shapes.end(), std::back_inserter(rData.Shapes));

					rData.ShapeInfos[shapeIndex].canUpdateVertices = canUpdateVertexBuffer;

					rData.ShapeCount += (uint32_t)shapes.size();

					ShapeRendererCmd* cmd = new ShapeRendererCmd(this, (uint32_t)shapes.size() * 6,
						renderIndex, shapeIndex);
					m_shapeRendererCmds.push_back(cmd);
					m_renderer.AddRenderCmd(cmd);
				}
			}
			else
				drawRecursive(shapes, canUpdateVertexBuffer);

			shapeList->second.IsListAdded = true;
		}

		for (auto updateVCmd : m_updateVerticesCmds)
			updateVCmd->Execute();

		updateUBO(mvp);
	}

	void ShapeRenderer::Draw(std::vector<std::shared_ptr<IShape>>& shapes, const Camera& cam,
		bool canUpdateVertexBuffer)
	{
		Draw(shapes, cam.GetPV(), canUpdateVertexBuffer);
	}

	void ShapeRenderer::drawRecursive(std::vector<std::shared_ptr<IShape>>& shapes, bool canUpdateVertexBuffer)
	{
		if (shapes.size() == 0) return;

		// Create render data for the shape list 
		// and create more if we go past the max quad count
		m_renderInfos.push_back(RenderData{});
		size_t index = m_renderInfos.size() - 1;

		size_t shapeSize = (shapes.size() > MAX_QUAD_COUNT) ? MAX_QUAD_COUNT : shapes.size();

		std::vector<std::shared_ptr<IShape>> tempShapes(shapes.begin(), std::next(shapes.begin() + shapeSize - 1));

		m_renderInfos[index].ShapeInfos.push_back(ShapeData{});
		populateVertices(tempShapes, (uint32_t)index, 0);

		ShapeRendererCmd* cmd = new ShapeRendererCmd(this, (uint32_t)shapes.size() * 6,
			(uint32_t)index, 0);
		m_shapeRendererCmds.push_back(cmd);
		m_renderer.AddRenderCmd(cmd);

		m_renderInfos[index].ShapeCount = (uint32_t)shapeSize;

		if (canUpdateVertexBuffer)
		{ 
			m_renderInfos[index].Shapes.resize(shapeSize);
			m_renderInfos[index].Shapes.assign(tempShapes.begin(), tempShapes.end());

			m_renderInfos[index].ShapeInfos[0].canUpdateVertices = true;

			m_updateVerticesCmds.emplace_back(std::make_unique<UpdateVerticesCmd>(*this,
				m_renderInfos[index].Shapes, (uint32_t)index));
		}

		UploadUpdatedShapeVerticesCmd* uploadUpdatedCmd =
			new UploadUpdatedShapeVerticesCmd(this, (uint32_t)index);
		m_uploadUpdatedVerticesCmds.push_back(uploadUpdatedCmd);
		m_renderer.AddUpdateVertexCmd(uploadUpdatedCmd);

		if (shapes.size() < MAX_QUAD_COUNT)
			return;

		std::vector<std::shared_ptr<IShape>> newShapes;
		newShapes.reserve(shapes.size() - MAX_QUAD_COUNT);
		for (size_t i = MAX_QUAD_COUNT; i < shapes.size(); ++i)
			newShapes.push_back(shapes[i]);

		drawRecursive(newShapes, canUpdateVertexBuffer);
	}

	void ShapeRenderer::draw(uint32_t indexCount, uint32_t renderDataIndex,
		uint32_t shapeDataIndex)
	{
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

		RenderData& rData = m_renderInfos[renderDataIndex];
		ShapeData& sData = rData.ShapeInfos[shapeDataIndex];

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
			0, 1, &m_descriptorSet, 0, 0);

		VkBuffer vertexBuffers[] = { rData.Buffer };
		VkDeviceSize offsets[] = { sData.StartVertex };

		vkCmdBindVertexBuffers(m_renderer.GetCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(m_renderer.GetCurrentCommandBuffer(),
			rData.Buffer, sData.StartIndex,
			VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_renderer.GetCurrentCommandBuffer(),
			(uint32_t)sData.Indices.size(), 1, 0, 0, 0);
	}
	
	void ShapeRenderer::populateVertices(std::vector<std::shared_ptr<IShape>>& shapes,
		uint32_t renderDataIndex, uint32_t shapeDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];
		ShapeData& sData = rData.ShapeInfos[shapeDataIndex];

		rData.TotalVerticeSize = 0;

		if (rData.IsBufferCreated)
		{
			vkDestroyBuffer(Core::GetCore().GetDevice()->GetDevice(), rData.Buffer, nullptr);
			vkFreeMemory(Core::GetCore().GetDevice()->GetDevice(), rData.BufferMemory, nullptr);
		}

		for (std::shared_ptr<IShape>  shape : shapes)
		{
			float width = shape->GetWidth();
			float height = shape->GetHeight();
			float depth = shape->GetDepth();
			float x = shape->GetX() + (width / 2.f);
			float y = shape->GetY() + (height / 2.f);
			float z = shape->GetZ() + (depth / 2.f);

			ShapeType shapeType = shape->GetShapeType();

			bool isScaled = shape->GetScaleX() != 1.f || shape->GetScaleY() != 1.f || shape->GetScaleZ() != 1.f;
			bool isRotated = shape->GetRotationX() != 0.f || shape->GetRotationY() != 0.f || shape->GetRotationZ() != 0.f;

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
				Mat4 scaleMat = Mat4::Scale(shape->GetScaleX(), shape->GetScaleY(), shape->GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(shape->GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(shape->GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(shape->GetRotationZ());

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

			ShapeVertex topRight;
			ShapeVertex topLeft;
			ShapeVertex bottomLeft;
			ShapeVertex bottomRight;

			topRight.pos = posTR;
			topRight.colour = shape->GetColour();
			topRight.localPos = Vec2{ m_tR.x, m_tR.y } *2.f;
			topRight.circleThickness = shape->GetCircleThickness();
			topRight.circleFade = shape->GetCircleFade();
			topRight.shapeType = (float)shapeType;

			topLeft.pos = posTL;
			topLeft.colour = shape->GetColour();
			topLeft.localPos = Vec2{ m_tL.x, m_tL.y } *2.f;
			topLeft.circleThickness = shape->GetCircleThickness();
			topLeft.circleFade = shape->GetCircleFade();
			topLeft.shapeType = (float)shapeType;

			bottomLeft.pos = posBL;
			bottomLeft.colour = shape->GetColour();
			bottomLeft.localPos = Vec2{ m_bL.x, m_bL.y } *2.f;
			bottomLeft.circleThickness = shape->GetCircleThickness();
			bottomLeft.circleFade = shape->GetCircleFade();
			bottomLeft.shapeType = (float)shapeType;

			bottomRight.pos = posBR;
			bottomRight.colour = shape->GetColour();
			bottomRight.localPos = Vec2{ m_bR.x, m_bR.y } *2.f;
			bottomRight.circleThickness = shape->GetCircleThickness();
			bottomRight.circleFade = shape->GetCircleFade();
			bottomRight.shapeType = (float)shapeType;

			sData.Vertices.push_back(topRight);
			sData.Vertices.push_back(topLeft);
			sData.Vertices.push_back(bottomLeft);
			sData.Vertices.push_back(bottomRight);
		}

		uint32_t offset = 0;
		for (size_t i = 0; i < shapes.size(); ++i)
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

		if (rData.ShapeInfos.size() > 1)
		{
			sData.StartVertex =
				m_renderInfos[renderDataIndex].ShapeInfos[shapeDataIndex - 1].StartVertex +
				m_renderInfos[renderDataIndex].ShapeInfos[shapeDataIndex - 1].SizeOfVerticesInBytes();
		}
		else
			sData.StartIndex = 0;

		uint32_t indexOffset = 0;
		for (auto& data : rData.ShapeInfos)
		{
			ShapeData lastData = rData.ShapeInfos[rData.ShapeInfos.size() - 1];

			data.StartIndex = lastData.StartVertex +
				(sizeof(ShapeVertex) * lastData.Vertices.size()) + indexOffset;
			indexOffset += (uint32_t)data.SizeOfIndicesInBytes();
		}

		createBuffer(renderDataIndex);
	}

	void ShapeRenderer::createBuffer(uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		if (rData.ShapeInfos.empty()) return;

		VkDeviceSize bufferSize = 0;

		rData.TotalVerticeSize = 0;
		for (auto& data : rData.ShapeInfos)
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
		for (auto& dataInfo : rData.ShapeInfos)
			memcpy((char*)data + dataInfo.StartVertex, dataInfo.Vertices.data(), dataInfo.SizeOfVerticesInBytes());

		// Create the indices
		for (auto& dataInfo : rData.ShapeInfos)
			memcpy((char*)data + dataInfo.StartIndex, dataInfo.Indices.data(), dataInfo.SizeOfIndicesInBytes());

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

	void ShapeRenderer::updateBuffer(uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		VkDeviceSize bufferSize = rData.TotalVerticeSize;

		void* data;
		vkMapMemory(Core::GetCore().GetDevice()->GetDevice(),
			rData.BufferMemory, 0, bufferSize, 0, &data);

		for (auto& dataInfo : rData.ShapeInfos)
			memcpy((char*)data + dataInfo.StartVertex, dataInfo.Vertices.data(), dataInfo.SizeOfVerticesInBytes());

		vkUnmapMemory(Core::GetCore().GetDevice()->GetDevice(), rData.BufferMemory);
	}


	void ShapeRenderer::updateUBO(const Mat4& mvp)
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		ShapeUniformBufferObject ubo{};
		ubo.pv = mvp;

		void* data;
		vkMapMemory(pDevice->GetDevice(), m_uniformMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(pDevice->GetDevice(), m_uniformMemory);
	}

	void ShapeRenderer::updateVertices(std::vector<std::shared_ptr<IShape>>& shapes, uint32_t renderDataIndex)
	{
		RenderData& rData = m_renderInfos[renderDataIndex];

		if (rData.Shapes.size() == 0) return;

		size_t i = 0;
		uint32_t sIndex = 0;
		for (std::shared_ptr<IShape> shape : rData.Shapes)
		{
			float width = shape->GetWidth();
			float height = shape->GetHeight();
			float depth = shape->GetDepth();
			float x = shape->GetX();
			float y = shape->GetY();
			float z = shape->GetZ();

			ShapeType shapeType = shape->GetShapeType();
			
			bool isScaled = shape->GetScaleX() != 1.f || shape->GetScaleY() != 1.f || shape->GetScaleZ() != 1.f;
			bool isRotated = shape->GetRotationX() != 0.f || shape->GetRotationY() != 0.f || shape->GetRotationZ() != 0.f;

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
				Mat4 scaleMat = Mat4::Scale(shape->GetScaleX(), shape->GetScaleY(), shape->GetScaleZ());
				Mat4 rotateXMat = Mat4::RotateX(shape->GetRotationX());
				Mat4 rotateYMat = Mat4::RotateY(shape->GetRotationY());
				Mat4 rotateZMat = Mat4::RotateZ(shape->GetRotationZ());

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

			ShapeVertex topRight;
			ShapeVertex topLeft;
			ShapeVertex bottomLeft;
			ShapeVertex bottomRight;

			topRight.pos = posTR;
			topRight.colour = shape->GetColour();
			topRight.localPos = Vec2{ m_tR.x, m_tR.y } * 2.f;
			topRight.circleThickness = shape->GetCircleThickness();
			topRight.circleFade = shape->GetCircleFade();
			topRight.shapeType = (float)shapeType;

			topLeft.pos = posTL;
			topLeft.colour = shape->GetColour();
			topLeft.localPos = Vec2{ m_tL.x, m_tL.y } *2.f;
			topLeft.circleThickness = shape->GetCircleThickness();
			topLeft.circleFade = shape->GetCircleFade();
			topLeft.shapeType = (float)shapeType;

			bottomLeft.pos = posBL;
			bottomLeft.colour = shape->GetColour();
			bottomLeft.localPos = Vec2{ m_bL.x, m_bL.y } *2.f;
			bottomLeft.circleThickness = shape->GetCircleThickness();
			bottomLeft.circleFade = shape->GetCircleFade();
			bottomLeft.shapeType = (float)shapeType;

			bottomRight.pos = posBR;
			bottomRight.colour = shape->GetColour();
			bottomRight.localPos = Vec2{ m_bR.x, m_bR.y } *2.f;
			bottomRight.circleThickness = shape->GetCircleThickness();
			bottomRight.circleFade = shape->GetCircleFade();
			bottomRight.shapeType = (float)shapeType;

			// Our shapes to updates in one list
			// So once we finish updating one set of vertices we want to update the next
			if (i >= rData.ShapeInfos[sIndex].Vertices.size())
			{
				++sIndex;

				for (int i = sIndex; i < rData.ShapeInfos.size(); ++i)
				{
					if (rData.ShapeInfos[sIndex].canUpdateVertices)
						break;

					++sIndex;
				}

				i = 0;
			}

			rData.ShapeInfos[sIndex].Vertices[i] = topRight;
			rData.ShapeInfos[sIndex].Vertices[i + 1] = topLeft;
			rData.ShapeInfos[sIndex].Vertices[i + 2] = bottomLeft;
			rData.ShapeInfos[sIndex].Vertices[i + 3] = bottomRight;

			i += 4;
		}
	}

	void ShapeRenderer::swapchainRecreateEvent(SwapchainRecreateEvent& e)
	{
		m_swapchainRecreated = true;
	}
}
