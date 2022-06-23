#pragma once

#include "IPipeline.h"

#define ROW_MAJOR
#include "../../Math/ZMath.h"

namespace ZVK
{
	struct SpriteVertex
	{
		Vec3 pos;
		Vec4 colour;
		Vec2 texCoord;
		int texIndex;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(SpriteVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

			/* Formats:
			* float: VK_FORMAT_R32_SFLOAT
			* vec2:  VK_FORMAT_R32G32_SFLOAT
			* vec3:  VK_FORMAT_R32G32B32_SFLOAT
			* vec4:  VK_FORMAT_R32G32B32A32_SFLOAT
			*
			* ivec2: VK_FORMAT_R32G32_SINT
			* uvec4: VK_FORMAT_R32G32B32A32_UINT
			* double: VK_FORMAT_R64_SFLOAT
			*/

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(SpriteVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(SpriteVertex, colour);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(SpriteVertex, texCoord);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32_SINT;
			attributeDescriptions[3].offset = offsetof(SpriteVertex, texIndex);

			return attributeDescriptions;
		}

		bool operator==(const SpriteVertex& other) const
		{
			return pos == other.pos && colour == other.colour && texCoord == other.texCoord;
		}
	};

	struct UniformBufferObject
	{
		Mat4 pv;
	};

	class SpritePipeline : public IPipeline
	{
	public:
		SpritePipeline(const std::string& vertPath, const std::string& fragPath,
			PipelineConfigInfo* configInfo = nullptr);
		~SpritePipeline() override;

		SpritePipeline(const SpritePipeline&) = delete;
		SpritePipeline(SpritePipeline&&) = delete;
		SpritePipeline& operator=(const SpritePipeline&) = delete;
		SpritePipeline& operator=(SpritePipeline&&) = delete;

		void createGraphicsPipeline() override;
		void createDescriptorSetLayout() override;
		void createDescriptorPool() override;
	};
}