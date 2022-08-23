#include <stdlib.h>
#include <engine/engine.h>
#include <stdio.h>
#include <assert.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "engine/core/entrypoint.h"

class sandbox_app : public application {

	bool on_create() override {
		// create the renderpass
		render_pass_builder render_pass_builder;
		uint32_t output_location = render_pass_builder.add_attachment(render_pass_builder::attachment_description{ render_pass_builder::attachment_type::PRESENT_ATTACHMENT });
		render_pass_builder.begin_subpass();
		render_pass_builder.write_color_attachment(output_location);
		render_pass_builder.end_subpass();

		m_render_pass = render_pass_builder.build();


		struct p_constant {
			glm::mat4 view_projection_matrix;
			glm::mat4 model_matrix;
		};

		// create the graphics pipeline
		pipeline_builder pipeline_builder{ m_render_pass };
		pipeline_builder.buffer_layout_push_floats(3);
		VkShaderModule vertex = shader::load_module_from_file("res/vertex.spv");
		VkShaderModule fragment = shader::load_module_from_file("res/fragment.spv");
		VkExtent2D swapchain_extent = context::get_swapchain().extent;
		pipeline_builder.set_viewport(0.0f, 0.0f, (float)swapchain_extent.width, (float)swapchain_extent.height);
		pipeline_builder.set_vertex_shader(vertex);
		pipeline_builder.set_fragment_shader(fragment);
		pipeline_builder.push_constant<p_constant>(VK_SHADER_STAGE_VERTEX_BIT, 0);
		pipeline_builder.build(&m_pipeline, &m_layout);

		vkDestroyShaderModule(context::get_device(), vertex, NULL);
		vkDestroyShaderModule(context::get_device(), fragment, NULL);


		float data[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f
		};
		uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

		command_buffer transfer_cmd_buf;
		std::shared_ptr<staging_buffer> staging_buf = staging_buffer::create();
		vbo = vertex_buffer::create();
		ibo = index_buffer::create();
		transfer_cmd_buf.start();
		vbo->set_buffer_data(transfer_cmd_buf, staging_buf, data, sizeof(data));
		ibo->set_buffer_data(transfer_cmd_buf, staging_buf, indices, sizeof(indices));
		transfer_cmd_buf.end();
		transfer_cmd_buf.submit(context::get_graphics_queue());
		vkQueueWaitIdle(context::get_graphics_queue());
		staging_buf->destroy();
		transfer_cmd_buf.destroy();

		return true;
	}

	bool on_update(command_buffer& cmd_buf, float delta_time) override {
		VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

		VkClearValue clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.pNext = NULL;
		render_pass_begin_info.renderPass = m_render_pass;
		render_pass_begin_info.framebuffer = context::get_current_framebuffer().get_handle();
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		VkExtent2D swapchain_extent = context::get_swapchain().extent;
		render_pass_begin_info.renderArea.extent = swapchain_extent;

		VkDeviceSize offset = 0;
		vkCmdBeginRenderPass(cmd_buf.get_handle(), &render_pass_begin_info, contents);
		vkCmdBindPipeline(cmd_buf.get_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);


		


		vkCmdBindIndexBuffer(cmd_buf.get_handle(), ibo->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmd_buf.get_handle(), 0, 1, &vbo->get_handle(), &offset);

		// set scissors and viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)render_pass_begin_info.renderArea.extent.width;
		viewport.height = (float)render_pass_begin_info.renderArea.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = render_pass_begin_info.renderArea.extent;

		vkCmdSetViewport(cmd_buf.get_handle(), 0, 1, &viewport);
		vkCmdSetScissor(cmd_buf.get_handle(), 0, 1, &scissor);


		p_constant constants;
		glm::mat4 projection_matrix = glm::perspective(3.14159f / 2.0f, (float)swapchain_extent.width / swapchain_extent.height, 0.01f, 1000.0f);
		projection_matrix[1][1] = -projection_matrix[1][1];
		glm::mat4 view_matrix = glm::mat4(1.0f);
		constants.view_projection_matrix = projection_matrix * view_matrix;
		constants.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -get_time()));

		// draw call
		vkCmdPushConstants(cmd_buf.get_handle(), m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants), &constants);
		vkCmdDrawIndexed(cmd_buf.get_handle(), ibo->index_count(), 1, 0, 0, 0);
		
		constants.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, -5.0f)) * glm::rotate(glm::mat4(1.0f), get_time(), glm::vec3(1.0f, 1.0f, 0.0f));
		vkCmdPushConstants(cmd_buf.get_handle(), m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants), &constants);
		vkCmdDrawIndexed(cmd_buf.get_handle(), ibo->index_count(), 1, 0, 0, 0);




		vkCmdEndRenderPass(cmd_buf.get_handle());

		return true;
	}

	void on_terminate() override {
		vbo->destroy();
		ibo->destroy();
		vkDestroyPipelineLayout(context::get_device(), m_layout, NULL);
		vkDestroyPipeline(context::get_device(), m_pipeline, NULL);
		vkDestroyRenderPass(context::get_device(), m_render_pass, NULL);
	}
private:
	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;

	std::shared_ptr<vertex_buffer> vbo;
	std::shared_ptr<index_buffer> ibo;

	struct p_constant {
		glm::mat4 view_projection_matrix;
		glm::mat4 model_matrix;
	};
};

application* create_app() {
	return new sandbox_app;
}

