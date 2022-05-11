#include "Headers/SpritePipeline.h"

#include "../../Core/Headers/Core.h"

namespace ZVK
{
	SpritePipeline::SpritePipeline(const std::string& vertPath, const std::string& fragPath,
		PipelineConfigInfo* configInfo)
		: IPipeline(vertPath, fragPath, configInfo)
	{
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createColourResources();
		createDepthResources();
		createSwapchainFrameBuffers();
		createDescriptorPool();
	}

	SpritePipeline::~SpritePipeline()
	{
		ZDevice* pDevice = Core::GetCore().GetDevice();

		Core::GetCore().GetSwapchainRecreateDispatcher().Detach(m_swapchainRecreateEvent);

		cleanup();

		vkDestroyDescriptorSetLayout(pDevice->GetDevice(), m_descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(pDevice->GetDevice(), m_descriptorPool, nullptr);
	}

	void SpritePipeline::createGraphicsPipeline()
	{
		ShaderHelper sHelper;

		ZDevice* pDevice = Core::GetCore().GetDevice();
		ZSwapchain* pSwapchain = Core::GetCore().GetSwapchain();;

		auto vertCode = sHelper.readFile(m_vertPath);
		auto fragCode = sHelper.readFile(m_fragPath);

		VkShaderModule vertShaderModule = sHelper.createShaderModule(pDevice->GetDevice(), vertCode);
		VkShaderModule fragShaderModule = sHelper.createShaderModule(pDevice->GetDevice(), fragCode);

		VkPipelineShaderStageCreateInfo vertCreateInfo{};
		VkPipelineShaderStageCreateInfo fragCreateInfo{};
		vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertCreateInfo.module = vertShaderModule;
		vertCreateInfo.pName = "main";

		fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragCreateInfo.module = fragShaderModule;
		fragCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertCreateInfo, fragCreateInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = SpriteVertex::GetBindingDescription();
		auto attributeDescriptions = SpriteVertex::GetAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.f;
		//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.f; // Optional
		rasterizer.depthBiasClamp = 0.f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.rasterizationSamples = pDevice->GetMSAA_Samples();
		multisampling.minSampleShading = .2f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

		if (vkCreatePipelineLayout(pDevice->GetDevice(), &pipelineLayoutInfo,
			nullptr, &m_pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!\n");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &m_configInfo->ColourBlending;
		pipelineInfo.pDynamicState = &m_configInfo->DynamicStateInfo;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderpass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; 
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(pDevice->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &m_pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline!");

		vkDestroyShaderModule(pDevice->GetDevice(), vertShaderModule, nullptr);
		vkDestroyShaderModule(pDevice->GetDevice(), fragShaderModule, nullptr);
	}

	void SpritePipeline::createDescriptorSetLayout()
	{
#ifdef FLUSH_MAX_TEXTURES
		VkDescriptorBindingFlagsEXT bindFlags[] =
		{
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
		};
#else
		VkDescriptorBindingFlagsEXT bindFlags[] =
		{
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
		};
#endif

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		extendedInfo.bindingCount = 3;
		extendedInfo.pBindingFlags = bindFlags;
		extendedInfo.pNext = nullptr;

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		VkDescriptorSetLayoutBinding sampledImageLayoutBinding{};

		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerLayoutBinding.descriptorCount = Core::GetCore().GetMaxTextureSlots();
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		sampledImageLayoutBinding.binding = 2;
		sampledImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		sampledImageLayoutBinding.descriptorCount = Core::GetCore().GetMaxTextureSlots();
		sampledImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampledImageLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings =
		{ uboLayoutBinding, samplerLayoutBinding, sampledImageLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		layoutInfo.pNext = &extendedInfo;

		if (vkCreateDescriptorSetLayout(Core::GetCore().GetDevice()->GetDevice(), &layoutInfo,
			nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!");
	}

	void SpritePipeline::createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[1].descriptorCount = Core::GetCore().GetMaxTextureSlots();
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[2].descriptorCount = Core::GetCore().GetMaxTextureSlots();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 3;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(Core::GetCore().GetDevice()->GetDevice(), &poolInfo,
			nullptr, &m_descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool!");
	}
}