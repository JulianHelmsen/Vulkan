#include <stdlib.h>
#include <engine/engine.h>
#include <stdio.h>
#include <assert.h>




int main(const int argc, const char** argv) {
	
	// create the window
	window window{"Title", 1020, 720};


	// basic initialization of vulkan
	if (!render_api::init()) {
		window.destroy();
		return -1;
	}
	context* ctx = context::create_context(window.get_handle());
	if (ctx == NULL) {
		render_api::shutdown();
		window.destroy();
		return -1;
	}

	ctx->make_context_current();

	// create the renderpass
	render_pass_builder render_pass_builder;
	uint32_t output_location = render_pass_builder.add_attachment(render_pass_builder::attachment_description{ render_pass_builder::attachment_type::PRESENT_ATTACHMENT });
	render_pass_builder.begin_subpass();
	render_pass_builder.write_color_attachment(output_location);
	render_pass_builder.end_subpass();

	VkRenderPass render_pass = render_pass_builder.build();


	// create the graphics pipeline
	pipeline_builder pipeline_builder{render_pass};
	pipeline_builder.buffer_layout_push_floats(3);
	VkShaderModule vertex = shader::load_module_from_file("res/vertex.spv");
	VkShaderModule fragment = shader::load_module_from_file("res/fragment.spv");
	VkExtent2D swapchain_extent = context::get_swapchain().extent;
	pipeline_builder.set_viewport(0.0f, 0.0f, (float) swapchain_extent.width, (float)swapchain_extent.height);
	pipeline_builder.set_vertex_shader(vertex);
	pipeline_builder.set_fragment_shader(fragment);
	VkPipeline pipeline = pipeline_builder.build();
	
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
	std::shared_ptr<vertex_buffer> vbo = vertex_buffer::create();
	std::shared_ptr<index_buffer> ibo = index_buffer::create();
	{
		transfer_cmd_buf.start();
		vbo->set_buffer_data(transfer_cmd_buf, staging_buf, data, sizeof(data));
		transfer_cmd_buf.end();
		transfer_cmd_buf.submit(context::get_graphics_queue());
		vkQueueWaitIdle(context::get_graphics_queue());
	}

	{
		transfer_cmd_buf.start();
		ibo->set_buffer_data(transfer_cmd_buf, staging_buf, indices, sizeof(indices));
		transfer_cmd_buf.end();
		transfer_cmd_buf.submit(context::get_graphics_queue(), VK_NULL_HANDLE);
		vkQueueWaitIdle(context::get_graphics_queue());
	}
	
	transfer_cmd_buf.destroy();

	

	

	
	// record command buffers
	command_buffer cmd_buffers[2];
	context::set_framebuffer_change_callback([&]() -> void {
		for (int i = 0; i < stack_array_len(cmd_buffers); i++) {
			command_buffer& cmd_buf = cmd_buffers[i];
			cmd_buf.start();
			VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

			VkClearValue clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };

			VkRenderPassBeginInfo render_pass_begin_info = {};
			render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_begin_info.pNext = NULL;
			render_pass_begin_info.renderPass = render_pass;
			render_pass_begin_info.framebuffer = context::get_window_framebuffer(i).get_handle();
			render_pass_begin_info.clearValueCount = 1;
			render_pass_begin_info.pClearValues = &clear_value;
			render_pass_begin_info.renderArea.offset = { 0, 0 };
			render_pass_begin_info.renderArea.extent = context::get_swapchain().extent;

			VkDeviceSize offset = 0;
			vkCmdBeginRenderPass(cmd_buf.get_handle(), &render_pass_begin_info, contents);
			vkCmdBindPipeline(cmd_buf.get_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);


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

			// draw call
			uint32_t index_count = ibo->index_count();
			vkCmdDrawIndexed(cmd_buf.get_handle(), index_count, 1, 0, 0, 0);

			vkCmdEndRenderPass(cmd_buf.get_handle());

			cmd_buf.end();
		}
	});


	context::create_window_framebuffers(render_pass);

	VkSemaphore finished_rendering_semaphore  = create_semaphore();
	while (!window.is_closed_requsted()) {

		// acquire the image from the swapchain
		uint32_t image_index;
		if (context::begin_frame(&image_index) == VK_TIMEOUT)
			continue;

		// execute command buffer
		cmd_buffers[image_index].submit(context::get_graphics_queue(), VK_NULL_HANDLE, finished_rendering_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		VkResult present_result = context::end_frame(finished_rendering_semaphore);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
			while (window.is_minimized() && !window.is_closed_requsted())
				window.wait_events();
			context::recreate_swapchain(render_pass);
		}
		
		vkDeviceWaitIdle(context::get_device());
		// present image
		window.poll_events();
	}

	staging_buf->destroy();
	ibo->destroy();
	vbo->destroy();
	vkDestroySemaphore(context::get_device(), finished_rendering_semaphore, NULL);
	for(command_buffer& cmd_buffer : cmd_buffers)
		cmd_buffer.destroy();
	vkDestroyPipeline(context::get_device(), pipeline, NULL);
	vkDestroyRenderPass(context::get_device(), render_pass, NULL);
	delete ctx;
	render_api::shutdown();
	window.destroy();
	return 0;
}