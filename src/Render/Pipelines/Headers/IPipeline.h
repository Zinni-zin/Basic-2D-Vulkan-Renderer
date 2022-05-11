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

#include "../../../Core/Headers/Core.h"

#include "../../../Events/Events.h"

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
		VkPipelineDynamicStateCreateInfo DynamicStateInfo;

		VkPipelineColorBlendAttachmentState ColourBlendInfo;
		VkPipelineColorBlendStateCreateInfo ColourBlending;

		void SetColourBlending(bool enableBlend, VkBlendFactor srcFactor, VkBlendFactor dstFactor,
			VkBlendOp blendOp, VkBlendOp alphaBlendOp, bool enableLogic = false,
			VkLogicOp logicOp = VK_LOGIC_OP_COPY, Vec4 blendConstants = Vec4::Zero())
		{
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

			ColourBlendInfo = colourBlendAttachment;

			VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
			colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colourBlendingInfo.logicOpEnable = enableLogic ? VK_TRUE : VK_FALSE;
			colourBlendingInfo.logicOp = logicOp; 
			colourBlendingInfo.attachmentCount = 1;
			colourBlendingInfo.pAttachments = &ColourBlendInfo;
			colourBlendingInfo.blendConstants[0] = blendConstants.x; 
			colourBlendingInfo.blendConstants[1] = blendConstants.y; 
			colourBlendingInfo.blendConstants[2] = blendConstants.z; 
			colourBlendingInfo.blendConstants[3] = blendConstants.w;

			ColourBlending = colourBlendingInfo;
		}

		void SetDynamicState()
		{
			DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicInfo{};

			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.pDynamicStates = DynamicStates.data();
			dynamicInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
			dynamicInfo.flags = 0;

			DynamicStateInfo = dynamicInfo;
		}

		void SetDefaults()
		{
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

			ColourBlendInfo = colourBlendAttachment;
			
			VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
			colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colourBlendingInfo.logicOpEnable = VK_FALSE;
			colourBlendingInfo.logicOp = VK_LOGIC_OP_COPY; 
			colourBlendingInfo.attachmentCount = 1;
			colourBlendingInfo.pAttachments = &ColourBlendInfo;
			colourBlendingInfo.blendConstants[0] = 0.0f;
			colourBlendingInfo.blendConstants[1] = 0.0f;
			colourBlendingInfo.blendConstants[2] = 0.0f;
			colourBlendingInfo.blendConstants[3] = 0.0f;

			ColourBlending = colourBlendingInfo;
			
			/* ---- Dynamic State ---- */

			DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			
			VkPipelineDynamicStateCreateInfo dynamicInfo{};

			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.pDynamicStates = DynamicStates.data();
			dynamicInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
			dynamicInfo.flags = 0;

			DynamicStateInfo = dynamicInfo;
		}
	};

	class IPipeline
	{
	public:
		IPipeline(const std::string& vertPath, const std::string& fragPath,
			PipelineConfigInfo* configInfo = nullptr)
			: m_vertPath(vertPath), m_fragPath(fragPath), m_configInfo(configInfo)
		{
			m_swapchainRecreateEvent = std::bind(&IPipeline::swapchainRecreateEvent, 
				std::ref(*this), std::placeholders::_1);

			Core::GetCore().GetSwapchainRecreateDispatcher().Attach(m_swapchainRecreateEvent);

			if (!configInfo)
			{
				m_configInfo = new PipelineConfigInfo();
				m_configInfo->SetDefaults();
				canDeleteConfigInfo = true;
			}
		}
		
		virtual ~IPipeline() 
		{
			Core::GetCore().GetSwapchainRecreateDispatcher().Detach(m_swapchainRecreateEvent);

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
		inline const VkRenderPass GetRenderPass() const { return m_renderpass; }
		inline const VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		inline const VkPipeline GetPipeline() const { return m_pipeline; }
		inline const VkImage GetColourImage() const { return m_colourImage; };
		inline const VkDeviceMemory GetColourImageMemory() const { return m_colourImageMemory; }
		inline const VkImageView GetColourImageView() const { return m_colourImageView; }
		inline const VkImage GetDepthImage() const { return m_depthImage; }
		inline const VkDeviceMemory GetDepthImageMemory() const { return m_depthImageMemory; }
		inline const VkImageView GetDepthImageView() const { return m_depthImageView; }
		inline const VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
		inline const std::vector<VkFramebuffer> GetSwapchainFrameBuffers() const { return m_swapchainFrameBuffers; }

	protected:
		virtual void createGraphicsPipeline() = 0;

		virtual void createDescriptorSetLayout() = 0;
		virtual void createDescriptorPool() = 0;

		void createRenderPass()
		{
			VkAttachmentDescription colourAttachment{};
			colourAttachment.format = Core::GetCore().GetSwapchain()->GetSwapchainImageFormat();
			colourAttachment.samples = Core::GetCore().GetDevice()->GetMSAA_Samples();
			colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription colourAttachmentResolve{};
			colourAttachmentResolve.format = Core::GetCore().GetSwapchain()->GetSwapchainImageFormat();
			colourAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colourAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colourAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colourAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colourAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colourAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colourAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = Core::GetCore().FindDepthFormat();
			depthAttachment.samples = Core::GetCore().GetDevice()->GetMSAA_Samples();
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colourAttachmentRef{};
			colourAttachmentRef.attachment = 0;
			colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colourAttachmentResolverRef{};
			colourAttachmentResolverRef.attachment = 2;
			colourAttachmentResolverRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colourAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
			subpass.pResolveAttachments = &colourAttachmentResolverRef;

			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			std::array<VkAttachmentDescription, 3> attachments =
			{ colourAttachment, depthAttachment, colourAttachmentResolve };

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if (vkCreateRenderPass(Core::GetCore().GetDevice()->GetDevice(), &renderPassInfo,
				nullptr, &m_renderpass) != VK_SUCCESS)
				throw std::runtime_error("Failed to create render pass!");
		}

		void createColourResources()
		{
			ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

			VkFormat colourFormat = pSwapchain->GetSwapchainImageFormat();

			Core::GetCore().CreateImage(
				pSwapchain->GetSwapchainExtent().width, pSwapchain->GetSwapchainExtent().height,
				1, Core::GetCore().GetDevice()->GetMSAA_Samples(),
				colourFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colourImage, m_colourImageMemory
			);

			m_colourImageView = Core::GetCore().CreateImageView(m_colourImage, colourFormat,
				VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		void createDepthResources()
		{
			ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

			VkFormat depthFormat = Core::GetCore().FindDepthFormat();

			Core::GetCore().CreateImage(
				pSwapchain->GetSwapchainExtent().width, pSwapchain->GetSwapchainExtent().height, 1,
				Core::GetCore().GetDevice()->GetMSAA_Samples(), depthFormat,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory
			);

			m_depthImageView = Core::GetCore().CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}

		void createSwapchainFrameBuffers()
		{
			ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();

			m_swapchainFrameBuffers.resize(pSwapchain->GetSwapchainImageViews().size());

			for (size_t i = 0; i < pSwapchain->GetSwapchainImageViews().size(); ++i)
			{
				std::array<VkImageView, 3> attachments =
				{
					m_colourImageView,
					m_depthImageView,
					pSwapchain->GetSwapchainImageViews()[i]
				};

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = m_renderpass;
				framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				framebufferInfo.pAttachments = attachments.data();
				framebufferInfo.width = pSwapchain->GetSwapchainExtent().width;
				framebufferInfo.height = pSwapchain->GetSwapchainExtent().height;
				framebufferInfo.layers = 1;

				if (vkCreateFramebuffer(Core::GetCore().GetDevice()->GetDevice(), &framebufferInfo,
					nullptr, &m_swapchainFrameBuffers[i]) != VK_SUCCESS)
					throw std::runtime_error("Failed to create framebuffer!");
			}
		}

		void swapchainRecreateEvent(SwapchainRecreateEvent& e)
		{
			cleanup();

			createRenderPass();
			createGraphicsPipeline();
			createColourResources();
			createDepthResources();
			createSwapchainFrameBuffers();
		}

		void cleanup()
		{
			ZDevice* pDevice = Core::GetCore().GetDevice();

			vkDestroyImageView(pDevice->GetDevice(), m_colourImageView, nullptr);
			vkDestroyImage(pDevice->GetDevice(), m_colourImage, nullptr);
			vkFreeMemory(pDevice->GetDevice(), m_colourImageMemory, nullptr);

			vkDestroyImageView(pDevice->GetDevice(), m_depthImageView, nullptr);
			vkDestroyImage(pDevice->GetDevice(), m_depthImage, nullptr);
			vkFreeMemory(pDevice->GetDevice(), m_depthImageMemory, nullptr);

			for (size_t i = 0; i < m_swapchainFrameBuffers.size(); ++i)
				vkDestroyFramebuffer(pDevice->GetDevice(), m_swapchainFrameBuffers[i], nullptr);

			vkDestroyPipeline(pDevice->GetDevice(), m_pipeline, nullptr);
			vkDestroyPipelineLayout(pDevice->GetDevice(), m_pipelineLayout, nullptr);
			vkDestroyRenderPass(pDevice->GetDevice(), m_renderpass, nullptr);
		}

	protected:

		PipelineConfigInfo* m_configInfo;
		bool canDeleteConfigInfo = false;

		VkDescriptorSetLayout m_descriptorSetLayout;

		VkRenderPass m_renderpass;
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		VkImage m_colourImage;
		VkDeviceMemory m_colourImageMemory;
		VkImageView m_colourImageView;

		VkImage m_depthImage;
		VkDeviceMemory m_depthImageMemory;
		VkImageView m_depthImageView;

		VkDescriptorPool m_descriptorPool;

		std::vector<VkFramebuffer> m_swapchainFrameBuffers;

		std::string m_vertPath, m_fragPath;

		std::function<void(SwapchainRecreateEvent&)> m_swapchainRecreateEvent;
	};
}