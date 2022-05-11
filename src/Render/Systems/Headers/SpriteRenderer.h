#pragma once

#include "../../../Core/Headers/Core.h"

#include "../../Headers/Renderer.h"
#include "../../Headers/Sprite.h"

#include "../../Pipelines/Headers/SpritePipeline.h"

#include "../../Headers/Camera.h"

#include <array>

#define MAX_QUAD_COUNT 2000
#define MAX_VERTEX_COUNT MAX_QUAD_COUNT * 4
#define MAX_INDEX_COUNT MAX_QUAD_COUNT * 6

namespace ZVK
{
	class SpriteRenderer
	{
	public:
		SpriteRenderer(SpritePipeline* pPipeline, const std::string& errorTexturePath);
		SpriteRenderer(const std::string& errorTexturePath, 
			const std::string& vertPath = "resources/shaders/spir-v/SpriteVert.spv", 
			const std::string& fragPath = "resources/shaders/spir-v/SpriteFrag.spv");

		SpriteRenderer(const Renderer&) = delete;
		SpriteRenderer(Renderer&&) = delete;
		SpriteRenderer& operator=(const Renderer&) = delete;
		SpriteRenderer& operator=(Renderer&&) = delete;

		~SpriteRenderer();

		/*
		* If there are more unique textures than texture slots sprites should be sorted
		* Due to ranges if it's not sorted there will be some weird errors
		* I'm too lazy to fix it, I will probably try to in the future
		* For now try to avoid going over the texture limit and use texture atlas
		*/
		void Draw(Renderer& renderer, std::vector<Sprite>& sprites, const Camera& cam,
			bool canUpdateVertexBuffer = true, bool canSortSprites = false);

		inline Vec4 GetClearColour() const { return m_clearColour; }
		inline void SetClearColour(Vec4 colour) { m_clearColour = colour; }
		inline void SetClearColour(float r, float g, float b, float a) { m_clearColour = { r,g,b,a }; };

	private:
		void init(const std::string& errorTexturePat);

		void draw(Renderer& renderer);
		void beginFlush(Renderer& renderer);
		void flush(Renderer& renderer, uint32_t indexCount, uint32_t firstIndex = 0);
		void endFlush(Renderer& renderer);

		void updateUBO(const Camera& cam);

		void populateVertices(std::vector<Sprite>& sprites);
		void allocateDescriptorInfo();
		void updateDescriptorWrites();

		void createVertexBuffer();
		void updateVertexBuffer(std::vector<Sprite>& sprites);
		void createIndexBuffer();
		void createUniformBuffer();

		void swapchainRecreateEvent(SwapchainRecreateEvent& e);

	private:
		SpritePipeline* p_pipeline;

		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;
		VkBuffer m_uniformBuffer;
		
		VkDeviceMemory m_vertexMemory;
		VkDeviceMemory m_indexMemory;
		VkDeviceMemory m_uniformMemory;

		std::vector<SpriteVertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<uint32_t> m_indicesCount;
		uint32_t m_spriteCount = 0;

		std::vector<Texture2D*> m_inUseTextures;

		bool m_vertexBufferCreated = false;
		bool m_descriptorInfoCreated = false;
		bool m_canDeletePipeline = false;
		bool m_swapchainRecreated = false;

		VkDescriptorSet m_descriptorSet;
		std::vector<VkDescriptorImageInfo> m_descriptorImageInfos;
		uint32_t m_mipLevels;

		Vec4 m_clearColour;

		std::shared_ptr<Texture2D> p_errorTexture;

		std::function<void(SwapchainRecreateEvent&)> m_swapchainRecreateEvent;
	};
}