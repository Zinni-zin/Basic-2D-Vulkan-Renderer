#pragma once

#include "IPipeline.h"

#define ROW_MAJOR
#include "../../Math/ZMath.h"

namespace ZVK
{
	struct ShapeVertex
	{
		Vec3 pos;
		Vec4 colour;
		Vec2 localPos;
		float circleThickness;
		float circleFade;
		float shapeType;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(ShapeVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(ShapeVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(ShapeVertex, colour);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(ShapeVertex, localPos);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(ShapeVertex, circleThickness);

			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
			attributeDescriptions[4].offset = offsetof(ShapeVertex, circleFade);

			attributeDescriptions[5].binding = 0;
			attributeDescriptions[5].location = 5;
			attributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
			attributeDescriptions[5].offset = offsetof(ShapeVertex, shapeType);

			return attributeDescriptions;
		}

		bool operator==(const ShapeVertex& other) const
		{
			return pos == other.pos && colour == other.colour;
		}
	};

	struct ShapeUniformBufferObject
	{
		Mat4 pv;
	};

	class ShapePipeline : public IPipeline
	{
	public:
		ShapePipeline(const std::string& vertPath, const std::string& fragPath,
			PipelineConfigInfo* configInfo = nullptr);
		~ShapePipeline() override;

		ShapePipeline(const ShapePipeline&) = delete;
		ShapePipeline(ShapePipeline&&) = delete;
		ShapePipeline& operator=(const ShapePipeline&) = delete;
		ShapePipeline& operator=(ShapePipeline&&) = delete;

		void createGraphicsPipeline() override;
		void createDescriptorSetLayout() override;
		void createDescriptorPool() override;
	};
}