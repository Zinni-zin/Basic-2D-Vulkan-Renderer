#pragma once

#include "../../Core/Core.h"

#include "../Renderer.h"
#include "../Sprite.h"

#include "../Pipelines/SpritePipeline.h"

#include "../Camera.h"

#include <array>
#include <tuple>

namespace ZVK
{
	class SpriteRenderer
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

		struct SpriteData;
		struct RenderData;
		struct TextureData;
		class UpdateVerticesCmd; // For updating vertices
		friend class SpriteRendererCmd;
		friend class UploadUpdatedSpriteVerticesCmd; // For uploading updated vertices

	public:
		SpriteRenderer(Renderer& renderer, SpritePipeline* pPipeline, const std::string& errorTexturePath);
		SpriteRenderer(Renderer& renderer, const std::string& errorTexturePath,
			const std::string& vertPath = "resources/shaders/spir-v/SpriteVert.spv",
			const std::string& fragPath = "resources/shaders/spir-v/SpriteFrag.spv");

		SpriteRenderer(const Renderer&) = delete;
		SpriteRenderer(Renderer&&) = delete;
		SpriteRenderer& operator=(const Renderer&) = delete;
		SpriteRenderer& operator=(Renderer&&) = delete;

		~SpriteRenderer();

		// mvp = model * view * projection
		void Draw(std::vector<std::shared_ptr<Sprite>>& sprites, const Mat4& mvp,
			const bool canUpdateVertexBuffer = true);

		void Draw(std::vector<std::shared_ptr<Sprite>>& sprites, const Camera& cam,
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
		void init(const std::string& errorTexturePat);

		void drawRecursive(std::vector<std::shared_ptr<Sprite>>& sprites, 
			bool canUpdateVertexBuffer);

		void draw(uint32_t indexCount, uint32_t renderDataIndex, uint32_t spriteDataIndex);

		void createTextureData(std::vector<std::shared_ptr<Sprite>>& sprites);

		void populateVertices(std::vector<std::shared_ptr<Sprite>>& sprites,
			uint32_t renderDataIndex, uint32_t shapeDataIndex);

		void allocateDescriptorInfo();
		
		void createBuffer(uint32_t renderDataIndex);
		void createUniformBuffer();
		
		void updateDescriptorWrites();
		void updateBuffer(uint32_t renderDataIndex);
		void updateUBO(const Mat4& mvp);

		void updateVertices(std::vector<std::shared_ptr<Sprite>>& sprites, 
			uint32_t renderDataIndex);

		void swapchainRecreateEvent(SwapchainRecreateEvent& e);

	private:
		Renderer& m_renderer;
		SpritePipeline* p_pipeline;

		VkBuffer m_uniformBuffer;
		VkDeviceMemory m_uniformMemory;

		std::vector<RenderData> m_renderInfos;
		std::unique_ptr<TextureData> m_textureData;

		std::vector<SpriteRendererCmd*> m_spriteRendererCmds;
		std::vector<UploadUpdatedSpriteVerticesCmd*> m_uploadUpdatedVerticesCmds; // Uploading updated vertices
		std::vector<std::shared_ptr<UpdateVerticesCmd>> m_updateVerticesCmds; // Change values in the vertices

		std::unordered_map<std::vector<std::shared_ptr<Sprite>>*, ListInfo> m_loadedLists;
		uint32_t m_listCount = 0;
		int32_t changedListIndex = -1;

		VkSampler m_sampler;
		VkDescriptorImageInfo m_samplerImageInfo;

		VkDescriptorSet m_descriptorSet;

		uint32_t m_mipLevels;

		bool m_canDeletePipeline = true;

		std::shared_ptr<Texture2D> p_errorTexture;

		std::function<void(SwapchainRecreateEvent&)> m_swapchainRecreateEvent;

		Vec4 m_viewportInfo;
		Vec2i m_scissorOffset = { 0,0 };
		VkExtent2D m_scissorExtent;

	private:
		struct SpriteData
		{
			std::vector<SpriteVertex> Vertices;
			std::vector<uint32_t> Indices;

			size_t StartVertex = 0;
			size_t StartIndex = 0;

			bool CanUpdateVertices = false;

			size_t SizeOfVerticesInBytes()
			{
				return sizeof(SpriteVertex) * Vertices.size();
			}

			size_t SizeOfIndicesInBytes()
			{
				return sizeof(uint32_t) * Indices.size();
			}

			size_t SizeInBytes()
			{
				return SizeOfVerticesInBytes() + SizeOfIndicesInBytes();
			}
		};

		struct RenderData
		{
		public:
			VkBuffer Buffer;
			VkDeviceMemory BufferMemory;

			std::vector<SpriteData> SpriteInfos;
			std::vector<std::shared_ptr<Sprite>> Sprites;

			uint32_t SpriteCount = 0;
			uint32_t TotalVerticeSize = 0;

			bool IsBufferCreated = false;
		};

		struct TextureData
		{
		public:

			std::vector<VkDescriptorImageInfo> DescriptorImageInfos;
			std::vector<std::pair<uint32_t, uint32_t>> BindingInfos;

			uint32_t TextureCount = 0;
		};

		// For updating vertices
		class UpdateVerticesCmd : public Cmd
		{
		public:
			UpdateVerticesCmd(SpriteRenderer& parent, std::vector<std::shared_ptr<Sprite>>& sprites,
				uint32_t renderDataIndex)
				: m_parent(parent), m_sprites(sprites), m_renderDataIndex(renderDataIndex)
			{}

			void Execute() override { m_parent.updateVertices(m_sprites, m_renderDataIndex);  }

		private:
			SpriteRenderer& m_parent;

			std::vector<std::shared_ptr<Sprite>>& m_sprites;
			uint32_t m_renderDataIndex;
		};
	};

	class SpriteRendererCmd : public RenderCmd
	{
	public:
		SpriteRendererCmd(SpriteRenderer* pSpriteRenderer, uint32_t indexCount,
			uint32_t renderDataIndex, uint32_t spriteDataIndex) :
			p_spriteRenderer(pSpriteRenderer), m_indexCount(indexCount),
			m_renderDataIndex(renderDataIndex), m_spriteDataIndex(spriteDataIndex)
		{ }

		void Execute() override {
			p_spriteRenderer->draw(m_indexCount, m_renderDataIndex, m_spriteDataIndex);
		}

	private:
		SpriteRenderer* p_spriteRenderer;
		uint32_t m_indexCount;
		uint32_t m_renderDataIndex;
		uint32_t m_spriteDataIndex;
	};

	// For uploading new vertices
	class UploadUpdatedSpriteVerticesCmd : public RenderCmd
	{
	public:
		UploadUpdatedSpriteVerticesCmd(SpriteRenderer* pSpriteRenderer, uint32_t renderDataIndex) 
			: p_spriteRenderer(pSpriteRenderer), m_renderDataIndex(renderDataIndex)
		{ }

		void Execute() override { p_spriteRenderer->updateBuffer(m_renderDataIndex); }

	private:
		SpriteRenderer* p_spriteRenderer;
		uint32_t m_renderDataIndex;
	};
}
