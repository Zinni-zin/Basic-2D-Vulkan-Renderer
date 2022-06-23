#pragma once

#include "../../../Headers/Core/Core.h"

#include "../Renderer.h"
#include "../Sprite.h"

#include "../Pipelines/ShapePipeline.h"

#include "../../Shapes/IShapes.h"

#include "../Camera.h"

#include <array>

#include <unordered_map>

namespace ZVK
{
	class ShapeRenderer
	{
	private:
		class Cmd
		{
		public:
			virtual void Execute() = 0;
		};

		struct ListInfo
		{
		public:
			ListInfo() = default;
			ListInfo(uint32_t listSize, uint32_t index, bool isListAdded = false)
				: ListSize(listSize), Index(index), IsListAdded(isListAdded) {}

			uint32_t ListSize;
			int32_t Index;
			bool IsListAdded;
		};

		struct ShapeData;
		struct RenderData;
		class UpdateVerticesCmd; // For updating vertices
		friend class ShapeRendererCmd;
		friend class UploadUpdatedShapeVerticesCmd; // For uploading updated vertices

	public:
		ShapeRenderer(Renderer& renderer, ShapePipeline* pPipeline);
		ShapeRenderer(Renderer& renderer, const std::string& vertPath = "resources/shaders/spir-v/ShapeVert.spv",
			const std::string& fragPath = "resources/shaders/spir-v/ShapeFrag.spv");

		ShapeRenderer(const Renderer&) = delete;
		ShapeRenderer(Renderer&&) = delete;
		ShapeRenderer& operator=(const Renderer&) = delete;
		ShapeRenderer& operator=(Renderer&&) = delete;

		~ShapeRenderer();

		// mvp = model * view * projection
		void Draw(std::vector<std::shared_ptr<IShape>>& shapes, const Mat4& mvp,
			const bool canUpdateVertexBuffer = true);

		void Draw(std::vector<std::shared_ptr<IShape>>& shapes, const Camera& cam,
			const bool canUpdateVertexBuffer = true);

		inline void SetViewport(Vec4 viewportInfo) { m_viewportInfo = viewportInfo; }
		void SetViewport(float x, float y, float width, float height)
		{
			m_viewportInfo.x = x; m_viewportInfo.y = y;
			m_viewportInfo.z = width; m_viewportInfo.w = height;
		}

		inline void SetScissorOffset(Vec2i offset) { m_scissorOffset = offset; }
		inline void SetScissorExtent(VkExtent2D extent) { m_scissorExtent = extent; };

		void SetScissorOffset(int32_t x, int32_t y) { m_scissorOffset.x = x, m_scissorOffset.y = y; };

		void SetScissorExtent(Vec2ui extent)
		{ m_scissorExtent.width = extent.x; m_scissorExtent.height = extent.y; }
		void SetScissorExtent(uint32_t width, uint32_t height)
		{ m_scissorExtent.width = width; m_scissorExtent.height = height; }

		void SetScissorRect(Vec2i offset, VkExtent2D extent)
		{ SetScissorOffset(offset); SetScissorExtent(extent); }
		void SetScissorRect(Vec2i offset, Vec2ui extent)
		{ SetScissorOffset(offset); SetScissorExtent(extent); }
		void SetScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height)
		{ SetScissorOffset(x, y); SetScissorExtent(width, height); }

	private:
		void init();

		void allocateDescriptorInfo();
		void updateDescriptorWrites();
		void createDescriptorPool();

		// Used when we go over max quad count
		void drawRecursive(std::vector<std::shared_ptr<IShape>>& shapes, bool canUpdateVertexBuffer);

		void draw(uint32_t indexCount, uint32_t renderDataIndex, uint32_t shapeDataIndex);

		void populateVertices(std::vector<std::shared_ptr<IShape>>& shapes, uint32_t renderDataIndex,
			uint32_t shapeDataIndex);

		void createBuffer(uint32_t renderDataIndex);

		void updateUBO(const Mat4& cam);
		void updateBuffer(uint32_t renderDataIndex);

		void updateVertices(std::vector<std::shared_ptr<IShape>>& shapes, uint32_t renderDataIndex);

		void createUniformBuffer();

		void swapchainRecreateEvent(SwapchainRecreateEvent& e);

	private:
		Renderer& m_renderer;
		ShapePipeline* p_pipeline;

		VkBuffer m_uniformBuffer;

		VkDeviceMemory m_uniformMemory;

		std::vector<RenderData> m_renderInfos;

		std::vector<ShapeRendererCmd*> m_shapeRendererCmds;
		std::vector<UploadUpdatedShapeVerticesCmd*> m_uploadUpdatedVerticesCmds; // Uploading updated vertices
		std::vector<std::shared_ptr<UpdateVerticesCmd>> m_updateVerticesCmds; // Change values in the vertices
		
		std::unordered_map<std::vector<std::shared_ptr<IShape>>*, ListInfo> m_loadedLists;
		uint32_t m_listCount = 0;
		int32_t changedListIndex = -1;

		bool m_uboCreated = false;
		bool m_descriptorInfoCreated = false;
		bool m_canDeletePipeline = false;
		bool m_swapchainRecreated = false;

		VkDescriptorPool m_descriptorPool;
		
		VkDescriptorSet m_descriptorSet;

		std::function<void(SwapchainRecreateEvent&)> m_swapchainRecreateEvent;

		Vec4 m_viewportInfo;
		Vec2i m_scissorOffset = { 0,0 };
		VkExtent2D m_scissorExtent;

		const Vec3 m_tR{  0.5f,  0.5f, 0.f };
		const Vec3 m_tL{ -0.5 ,  0.5f, 0.f };
		const Vec3 m_bL{ -0.5f, -0.5f, 0.f };
		const Vec3 m_bR{  0.5f, -0.5f, 0.f };

	private:
		struct ShapeData
		{
		public:
			std::vector<ShapeVertex> Vertices;
			std::vector<uint32_t> Indices;

			size_t StartVertex = 0;
			size_t StartIndex = 0;

			bool canUpdateVertices = false;

			size_t SizeOfVerticesInBytes()
			{ return sizeof(ShapeVertex) * Vertices.size(); }
			
			size_t SizeOfIndicesInBytes()
			{ return sizeof(uint32_t) * Indices.size(); }

			size_t SizeInBytes()
			{ return SizeOfVerticesInBytes() + SizeOfIndicesInBytes(); }
		};

		struct RenderData
		{
		public:
			VkBuffer Buffer;
			VkDeviceMemory BufferMemory;
			
			std::vector<ShapeData> ShapeInfos;
			std::vector<std::shared_ptr<IShape>> Shapes;

			uint32_t ShapeCount = 0;
			uint32_t TotalVerticeSize = 0;

			bool IsBufferCreated = false;
		};
		
		// For updating vertices
		class UpdateVerticesCmd : public Cmd
		{
		public:
			UpdateVerticesCmd(ShapeRenderer& parent, std::vector<std::shared_ptr<IShape>>& shapes,
				uint32_t renderDataIndex)
				: m_parent(parent), m_shapes(shapes), m_renderDataIndex(renderDataIndex)
			{}

			void Execute() override { m_parent.updateVertices(m_shapes, m_renderDataIndex); }

		private:
			ShapeRenderer& m_parent;

			std::vector<std::shared_ptr<IShape>>& m_shapes;
			uint32_t m_renderDataIndex;
		};
	};

	class ShapeRendererCmd : public RenderCmd
	{
	public:
		ShapeRendererCmd(ShapeRenderer* pShapeRenderer, uint32_t indexCount, 
			uint32_t renderDataIndex, uint32_t shapeDataIndex) :
			p_shapeRenderer(pShapeRenderer), m_indexCount(indexCount), 
			m_renderDataIndex(renderDataIndex), m_shapeDataIndex(shapeDataIndex)
		{ }

		void Execute() override { p_shapeRenderer->draw(m_indexCount, 
			m_renderDataIndex, m_shapeDataIndex); }
		
	private:
		ShapeRenderer* p_shapeRenderer;
		uint32_t m_indexCount;
		uint32_t m_renderDataIndex;
		uint32_t m_shapeDataIndex;
	};

	// For uploading new vertices
	class UploadUpdatedShapeVerticesCmd : public RenderCmd
	{
	public:
		UploadUpdatedShapeVerticesCmd(ShapeRenderer* pShapeRenderer, uint32_t renderDataIndex) 
			: p_shapeRenderer(pShapeRenderer), m_renderDataIndex(renderDataIndex)
		{ }

		void Execute() override { p_shapeRenderer->updateBuffer(m_renderDataIndex); }

	private:
		ShapeRenderer* p_shapeRenderer;
		uint32_t m_renderDataIndex;
	};
}
