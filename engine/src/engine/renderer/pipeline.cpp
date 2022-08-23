#include "pipeline.h"
#include "context.h"
#include <stdlib.h>
#include <assert.h>

VkFormat pipeline_builder::convert_to_vk_format(data_type type, uint32_t count) {
	if(count == 1) {
		if(type == data_type::FLOAT)
			return VK_FORMAT_R32_SFLOAT;
		else if (type == data_type::INT)
			return VK_FORMAT_R32_SINT;
		else if (type == data_type::UINT)
			return VK_FORMAT_R32_UINT;
	}else if (count == 2) {
		if (type == data_type::FLOAT)
			return VK_FORMAT_R32G32_SFLOAT;
		else if (type == data_type::INT)
			return VK_FORMAT_R32G32_SINT;
		else if (type == data_type::UINT)
			return VK_FORMAT_R32G32_UINT;
	}else if (count == 3) {
		if (type == data_type::FLOAT)
			return VK_FORMAT_R32G32B32_SFLOAT;
		else if (type == data_type::INT)
			return VK_FORMAT_R32G32B32_SINT;
		else if (type == data_type::UINT)
			return VK_FORMAT_R32G32B32_UINT;
	}else if (count == 3) {
		if (type == data_type::FLOAT)
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		else if (type == data_type::INT)
			return VK_FORMAT_R32G32B32A32_SINT;
		else if (type == data_type::UINT)
			return VK_FORMAT_R32G32B32A32_UINT;
	}
	assert(false);
	return VK_FORMAT_R32G32B32A32_UINT;
}


void init_shader_stage_create_info(VkPipelineShaderStageCreateInfo& info, VkShaderStageFlagBits stage, VkShaderModule shader_module) {
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = stage;
	info.pNext = NULL;
	info.flags = 0;
	info.pName = "main";
	info.module = shader_module;
	info.pSpecializationInfo = NULL;
}

void pipeline_builder::set_vertex_shader(VkShaderModule vertex_module) {
	VkPipelineShaderStageCreateInfo& info = m_shader_stages.emplace_back();
	init_shader_stage_create_info(info, VK_SHADER_STAGE_VERTEX_BIT, vertex_module);
}

void pipeline_builder::set_fragment_shader(VkShaderModule fragment_module) {
	VkPipelineShaderStageCreateInfo& info = m_shader_stages.emplace_back();
	init_shader_stage_create_info(info, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_module);
}

void pipeline_builder::set_geometry_shader(VkShaderModule geometry_module) {
	// TODO: geometry shader feature has to be enabled
	// according to https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateInfo.html
	VkPipelineShaderStageCreateInfo& info = m_shader_stages.emplace_back();
	init_shader_stage_create_info(info, VK_SHADER_STAGE_GEOMETRY_BIT, geometry_module);
}

void pipeline_builder::build(VkPipeline* pipeline, VkPipelineLayout* layout) {

	VkResult result;
	VkGraphicsPipelineCreateInfo create_info = { };

	char* vertex_buffer_description = (char*)malloc(m_buffer_layout.size() * (sizeof(VkVertexInputBindingDescription) + sizeof(VkVertexInputAttributeDescription)));
	VkVertexInputBindingDescription* bindings = (VkVertexInputBindingDescription*)vertex_buffer_description;
	VkVertexInputAttributeDescription* attributes = (VkVertexInputAttributeDescription*)(vertex_buffer_description + m_buffer_layout.size() * sizeof(VkVertexInputBindingDescription));
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = { };
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.flags = 0;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = (uint32_t)m_buffer_layout.size();
	vertex_input_state_create_info.vertexBindingDescriptionCount = (uint32_t)m_buffer_layout.size();
	vertex_input_state_create_info.pVertexAttributeDescriptions = attributes;
	vertex_input_state_create_info.pVertexBindingDescriptions = bindings;
	init_vertex_input_state_create_info(bindings, attributes);

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.pNext = NULL;
	input_assembly_state_create_info.flags = 0;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// special index indicating to restart rendering of strips and fans.
	// obviously does not work for LISTS though 
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;


	VkRect2D scissors;
	scissors.offset.x = (int) m_viewport.x;
	scissors.offset.y = (int)m_viewport.y;
	scissors.extent.width = (int)m_viewport.width;
	scissors.extent.height = (int)m_viewport.height;

	VkPipelineViewportStateCreateInfo viewport_state = { };
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;
	viewport_state.pViewports = &m_viewport;
	viewport_state.pScissors = &scissors;

	VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info = { };
	rasterizer_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_state_create_info.pNext = NULL;
	rasterizer_state_create_info.flags = 0;
	rasterizer_state_create_info.depthBiasClamp = VK_FALSE; // if true it clamps the depth of each fragment between the viewports min and max depth
	rasterizer_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_state_create_info.cullMode = m_culling_enabled;
	rasterizer_state_create_info.lineWidth = 1.0f;
	rasterizer_state_create_info.depthBiasEnable = VK_FALSE;
	rasterizer_state_create_info.depthBiasClamp = 0.0f;
	rasterizer_state_create_info.depthBiasConstantFactor = 0.0f;
	rasterizer_state_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = { };
	multi_sample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multi_sample_state_create_info.pNext = NULL;
	multi_sample_state_create_info.flags = 0;
	multi_sample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)m_samples;
	multi_sample_state_create_info.sampleShadingEnable = VK_FALSE;
	multi_sample_state_create_info.minSampleShading = 1.0f;// not used because of sampleShadingEnable = VK_FALSE
	multi_sample_state_create_info.pSampleMask = NULL;
	multi_sample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multi_sample_state_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = { };
	depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_create_info.pNext = NULL;
	depth_stencil_create_info.flags = 0;
	depth_stencil_create_info.depthTestEnable = m_depth_test;
	depth_stencil_create_info.depthWriteEnable = m_depth_test;
	depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_create_info.minDepthBounds = VK_FALSE;
	depth_stencil_create_info.maxDepthBounds = VK_FALSE;
	depth_stencil_create_info.stencilTestEnable = m_stencil_test;

	VkPipelineColorBlendAttachmentState attachment_blending = {};
	attachment_blending.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	attachment_blending.blendEnable = m_blending;
	attachment_blending.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment_blending.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_blending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment_blending.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment_blending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment_blending.alphaBlendOp = VK_BLEND_OP_ADD;
	attachment_blending.colorBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state = { };
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.pNext = NULL;
	color_blend_state.flags = 0;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.logicOp = VK_LOGIC_OP_SET; // Dont't care only for integer framebuffer attachments
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &attachment_blending;
	for(int i = 0; i < 4; i++)
		color_blend_state.blendConstants[i] = 0.0f;


	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = NULL;
	dynamic_state.flags = 0;
	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamic_state.pDynamicStates = states;
	dynamic_state.dynamicStateCount = (uint32_t)sizeof(states) / sizeof(states[0]);


	
	VkResult layout_result = create_pipeline_layout(layout);
	if(layout_result != VK_SUCCESS) {
		result = layout_result;
		goto return_label;
	}




	// deriving will make creating and switching between pipelines slightly faster
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.flags = 0; // see VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT, VK_PIPELINE_CREATE_DERIVATIVE_BIT 
	create_info.pNext = NULL;
	create_info.stageCount = (uint32_t)m_shader_stages.size();
	create_info.pStages = m_shader_stages.data();
	create_info.renderPass = m_render_pass;
	create_info.pVertexInputState = &vertex_input_state_create_info;
	create_info.pInputAssemblyState = &input_assembly_state_create_info;
	create_info.pTessellationState = NULL; // VUID-VkGraphicsPipelineCreateInfo-pStages-00731 implies that this can be NULL if you don't use a tesselation shader
	create_info.pViewportState = &viewport_state;
	create_info.pRasterizationState = &rasterizer_state_create_info;
	create_info.pMultisampleState = &multi_sample_state_create_info;
	create_info.pDepthStencilState = &depth_stencil_create_info;
	create_info.pColorBlendState = &color_blend_state;
	create_info.pDynamicState = &dynamic_state;
	create_info.layout = *layout;

	// do not derive for now
	create_info.basePipelineIndex = -1;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	
	result = vkCreateGraphicsPipelines(context::get_device(), VK_NULL_HANDLE, 1, &create_info, NULL, pipeline);
return_label:
	free(vertex_buffer_description);
}

void pipeline_builder::init_vertex_input_state_create_info(VkVertexInputBindingDescription* bindings, VkVertexInputAttributeDescription* attributes) {
	for (uint32_t i = 0; i < m_buffer_layout.size(); i++) {
		buffer_layout_element& e = m_buffer_layout[i];
		bindings[i].binding = i;
		bindings[i].inputRate = e.input_rate;
		bindings[i].stride = m_buffer_layout_stride;

		attributes[i].binding = i;
		attributes[i].format = convert_to_vk_format(e.type, e.count);
		attributes[i].location = i;
		attributes[i].offset = e.offset;
	}

}

void pipeline_builder::buffer_layout_push_floats(uint32_t count) {
	uint32_t offset_to_old_end = m_buffer_layout_stride;
	buffer_layout_element& e = m_buffer_layout.emplace_back();
	e.count = count;
	e.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	e.type = data_type::FLOAT;
	e.offset = offset_to_old_end;
	m_buffer_layout_stride += sizeof(float) * count;
}

void pipeline_builder::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth) {
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.width = width;
	m_viewport.height = height;
	m_viewport.minDepth = min_depth;
	m_viewport.maxDepth = max_depth;
}

VkResult pipeline_builder::create_pipeline_layout(VkPipelineLayout* layout) {
	

	

	VkPipelineLayoutCreateInfo layout_create_info = { };
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.pNext = NULL;
	layout_create_info.flags = 0;
	layout_create_info.setLayoutCount = 0;
	layout_create_info.pSetLayouts = NULL;
	layout_create_info.pushConstantRangeCount = (uint32_t)m_push_constant_ranges.size();
	layout_create_info.pPushConstantRanges = m_push_constant_ranges.data();

	
	return vkCreatePipelineLayout(context::get_device(), &layout_create_info, NULL, layout);
}


void pipeline_builder::push_constant(VkShaderStageFlags shader_stage, size_t offset, size_t size) {
	VkPushConstantRange& range = m_push_constant_ranges.emplace_back();
	range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	range.offset = (uint32_t) offset;
	range.size = (uint32_t)size;
}