#pragma once

#include <optional>
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "../../Core/Core.h"

#include "../../Events/Events.h"

namespace ZVK
{
	struct ShaderHelper
	{
		VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code)
		{
			VkShaderModule shaderModule;
			VkShaderModuleCreateInfo shaderModuleCreateInfo{};
			shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCreateInfo.codeSize = code.size();
			shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			if (vkCreateShaderModule(device, &shaderModuleCreateInfo,
				nullptr, &shaderModule) != VK_SUCCESS)
				throw std::runtime_error("Failed to Create Shader Module!\n");

			return shaderModule;
		}

		static std::vector<char> readFile(const std::string& filePath)
		{
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("Failed to open file: " + filePath);

			size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();
			return buffer;
		}
	};

	struct PipelineConfigInfo
	{
		std::vector<VkDynamicState> DynamicStates;
		std::unique_ptr<VkPipelineDynamicStateCreateInfo> DynamicStateInfo;

		std::unique_ptr<VkPipelineColorBlendAttachmentState> ColourBlendInfo;
		std::unique_ptr<VkPipelineColorBlendStateCreateInfo> ColourBlending;

		void SetColourBlending(bool enableBlend, VkBlendFactor srcFactor, VkBlendFactor dstFactor,
			VkBlendOp blendOp, VkBlendOp alphaBlendOp, bool enableLogic = false,
			VkLogicOp logicOp = VK_LOGIC_OP_COPY, Vec4 blendConstants = Vec4::Zero())
		{
			using std::make_unique;
			using std::move;

			VkPipelineColorBlendAttachmentState colourBlendAttachment{};
			colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colourBlendAttachment.blendEnable = enableBlend ? VK_TRUE : VK_FALSE;
			colourBlendAttachment.srcColorBlendFactor = srcFactor; 
			colourBlendAttachment.dstColorBlendFactor = dstFactor; 
			colourBlendAttachment.colorBlendOp = blendOp; 
			colourBlendAttachment.srcAlphaBlendFactor = srcFactor; 
			colourBlendAttachment.dstAlphaBlendFactor = dstFactor; 
			colourBlendAttachment.alphaBlendOp = alphaBlendOp;

			ColourBlendInfo = make_unique<VkPipelineColorBlendAttachmentState>(move(colourBlendAttachment));

			VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
			colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colourBlendingInfo.logicOpEnable = enableLogic ? VK_TRUE : VK_FALSE;
			colourBlendingInfo.logicOp = logicOp; 
			colourBlendingInfo.attachmentCount = 1;
			colourBlendingInfo.pAttachments = ColourBlendInfo.get();
			colourBlendingInfo.blendConstants[0] = blendConstants.x; 
			colourBlendingInfo.blendConstants[1] = blendConstants.y; 
			colourBlendingInfo.blendConstants[2] = blendConstants.z; 
			colourBlendingInfo.blendConstants[3] = blendConstants.w;

			ColourBlending = make_unique<VkPipelineColorBlendStateCreateInfo>(move(colourBlendingInfo));
		}

		void SetColourBlending(bool enableBlend, VkBlendFactor srcFactor, VkBlendFactor dstFactor,
			VkBlendFactor srcAlphaFactor, VkBlendFactor dstAlphaFactor, VkBlendOp blendOp, 
			VkBlendOp alphaBlendOp, bool enableLogic = false, VkLogicOp logicOp = VK_LOGIC_OP_COPY, 
			Vec4 blendConstants = Vec4::Zero())
		{
			using std::make_unique;
			using std::move;

			VkPipelineColorBlendAttachmentState colourBlendAttachment{};
			colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colourBlendAttachment.blendEnable = enableBlend ? VK_TRUE : VK_FALSE;
			colourBlendAttachment.srcColorBlendFactor = srcFactor;
			colourBlendAttachment.dstColorBlendFactor = dstFactor;
			colourBlendAttachment.colorBlendOp = blendOp;
			colourBlendAttachment.srcAlphaBlendFactor = srcAlphaFactor;
			colourBlendAttachment.dstAlphaBlendFactor = dstAlphaFactor;
			colourBlendAttachment.alphaBlendOp = alphaBlendOp;

			ColourBlendInfo = make_unique<VkPipelineColorBlendAttachmentState>(move(colourBlendAttachment));

			VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
			colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colourBlendingInfo.logicOpEnable = enableLogic ? VK_TRUE : VK_FALSE;
			colourBlendingInfo.logicOp = logicOp;
			colourBlendingInfo.attachmentCount = 1;
			colourBlendingInfo.pAttachments = ColourBlendInfo.get();
			colourBlendingInfo.blendConstants[0] = blendConstants.x;
			colourBlendingInfo.blendConstants[1] = blendConstants.y;
			colourBlendingInfo.blendConstants[2] = blendConstants.z;
			colourBlendingInfo.blendConstants[3] = blendConstants.w;

			ColourBlending = make_unique<VkPipelineColorBlendStateCreateInfo>(move(colourBlendingInfo));
		}

		void SetDynamicState()
		{
			DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicInfo{};

			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.pDynamicStates = DynamicStates.data();
			dynamicInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
			dynamicInfo.flags = 0;

			DynamicStateInfo = std::make_unique<VkPipelineDynamicStateCreateInfo>(std::move(dynamicInfo));
		}

		void SetDefaults()
		{
			using std::make_unique;
			using std::move;

			/* ---- Blending ---- */

			VkPipelineColorBlendAttachmentState colourBlendAttachment{};
			colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colourBlendAttachment.blendEnable = VK_TRUE;
			colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
			colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
			colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 

			ColourBlendInfo = make_unique<VkPipelineColorBlendAttachmentState>(move(colourBlendAttachment));
			
			VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
			colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colourBlendingInfo.logicOpEnable = VK_FALSE;
			colourBlendingInfo.logicOp = VK_LOGIC_OP_COPY; 
			colourBlendingInfo.attachmentCount = 1;
			colourBlendingInfo.pAttachments = ColourBlendInfo.get();
			colourBlendingInfo.blendConstants[0] = 0.0f;
			colourBlendingInfo.blendConstants[1] = 0.0f;
			colourBlendingInfo.blendConstants[2] = 0.0f;
			colourBlendingInfo.blendConstants[3] = 0.0f;

			ColourBlending = make_unique<VkPipelineColorBlendStateCreateInfo>(move(colourBlendingInfo));
			
			/* ---- Dynamic State ---- */

			DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			
			VkPipelineDynamicStateCreateInfo dynamicInfo{};

			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.pDynamicStates = DynamicStates.data();
			dynamicInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
			dynamicInfo.flags = 0;

			DynamicStateInfo = std::make_unique<VkPipelineDynamicStateCreateInfo>(std::move(dynamicInfo));
		}

		void Setup()
		{
			if (!ColourBlendInfo || !ColourBlending)
			{
				SetColourBlending(true, VK_BLEND_FACTOR_SRC_ALPHA,
					VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA, 
					VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD);
			}

			if (DynamicStates.empty() || DynamicStateInfo)
				SetDynamicState();
		}
	};

	class IPipeline
	{
	public:
		
		IPipeline(const std::string& vertPath, const std::string& fragPath,
			PipelineConfigInfo* configInfo = nullptr)
			: m_vertPath(vertPath), m_fragPath(fragPath), m_configInfo(configInfo)
		{
			m_swapchainRecreateEvent = std::bind(&IPipeline::onSwapchainRecreateEvent, 
				std::ref(*this), std::placeholders::_1);

			m_swapchainCleanupEvent = std::bind(&IPipeline::onSwapchainCleanupEvent,
				std::ref(*this), std::placeholders::_1);

			Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);
			Core::GetCore().GetSwapchainCleanupDispatcher().Attach(m_swapchainCleanupEvent);

			if (!configInfo)
			{
				m_configInfo = new PipelineConfigInfo();
				m_configInfo->SetDefaults();
				canDeleteConfigInfo = true;
			}
			else
				m_configInfo->Setup();
		}
		
		virtual ~IPipeline() 
		{
			Core::GetCore().GetSwapchainRecreateDispatcher().Detach(m_swapchainRecreateEvent);
			Core::GetCore().GetSwapchainCleanupDispatcher().Detach(m_swapchainCleanupEvent);

			if (canDeleteConfigInfo)
			{
				if (m_configInfo)
				{
					delete m_configInfo;
					m_configInfo = nullptr;
				}
			}
		};

		IPipeline(const IPipeline&) = delete;
		IPipeline(IPipeline&&) = delete;
		virtual IPipeline& operator=(const IPipeline&) = delete;
		virtual IPipeline& operator=(IPipeline&&) = delete;

		inline const VkDescriptorSetLayout GetDecriptorSetLayout() const { return m_descriptorSetLayout; }
		inline const VkDescriptorSetLayout* GetDescriptorSetLayoutPtr() const { return &m_descriptorSetLayout; }
		inline const VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		inline const VkPipeline GetPipeline() const { return m_pipeline; }
		inline const VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }

		inline bool IsDescriptorSetAllocated() const { return m_isDiscriptorSetAllocated; }
		inline void SetIsDescriptorSetAllocated(bool value) { m_isDiscriptorSetAllocated = value; }

	protected:
		virtual void createGraphicsPipeline() = 0;

		virtual void createDescriptorSetLayout() = 0;
		virtual void createDescriptorPool() = 0;

		void onSwapchainRecreateEvent(SwapchainRecreateEvent& e)
		{
			createGraphicsPipeline();
		}

		void onSwapchainCleanupEvent(SwapchainCleanupEvent& e)
		{
			cleanup();
		}

		void cleanup()
		{
			ZDevice* pDevice = Core::GetCore().GetDevice();
			
			vkDestroyPipeline(Core::GetCore().GetDevice()->GetDevice(), m_pipeline, nullptr);
			vkDestroyPipelineLayout(Core::GetCore().GetDevice()->GetDevice(), m_pipelineLayout, nullptr);
		}

	protected:

		PipelineConfigInfo* m_configInfo;
		bool canDeleteConfigInfo = false;

		VkDescriptorSetLayout m_descriptorSetLayout;

		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		VkDescriptorPool m_descriptorPool;
		
		std::string m_vertPath, m_fragPath;

		bool m_isDiscriptorSetAllocated = false;

		std::function<void(SwapchainRecreateEvent&)> m_swapchainRecreateEvent;
		std::function<void(SwapchainCleanupEvent&)> m_swapchainCleanupEvent;
	};
}
