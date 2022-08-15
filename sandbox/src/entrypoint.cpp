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
	VkShaderModule vertex = shader::load_module_from_file("res/vertex.spv");
	VkShaderModule fragment = shader::load_module_from_file("res/fragment.spv");
	VkExtent2D swapchain_extent = context::get_swapchain().extent;
	pipeline_builder.set_viewport(0.0f, 0.0f, (float) swapchain_extent.width, (float)swapchain_extent.height);
	pipeline_builder.set_vertex_shader(vertex);
	pipeline_builder.set_fragment_shader(fragment);
	VkPipeline pipeline = pipeline_builder.build();
	
	vkDestroyShaderModule(context::get_device(), vertex, NULL);
	vkDestroyShaderModule(context::get_device(), fragment, NULL);

	
	// record command buffers
	command_buffer cmd_buffers[2];
	context::set_framebuffer_change_callback([&cmd_buffers, &pipeline, &render_pass]() -> void {
		for (int i = 0; i < stack_array_len(cmd_buffers); i++) {
			command_buffer& cmd_buf = cmd_buffers[i];
			cmd_buf.start();
			VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

			VkClearValue clear_value = { 1.0f, 0.0f, 0.0f, 1.0f };

			VkRenderPassBeginInfo render_pass_begin_info = {};
			render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_begin_info.pNext = NULL;
			render_pass_begin_info.renderPass = render_pass;
			render_pass_begin_info.framebuffer = context::get_window_framebuffer(i).get_handle();
			render_pass_begin_info.clearValueCount = 1;
			render_pass_begin_info.pClearValues = &clear_value;
			render_pass_begin_info.renderArea.offset = { 0, 0 };
			render_pass_begin_info.renderArea.extent = context::get_swapchain().extent;

			vkCmdBeginRenderPass(cmd_buf.get_handle(), &render_pass_begin_info, contents);
			vkCmdBindPipeline(cmd_buf.get_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);


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
			vkCmdDraw(cmd_buf.get_handle(), 3, 1, 0, 0);

			vkCmdEndRenderPass(cmd_buf.get_handle());

			cmd_buf.end();
		}
	});

	context::create_window_framebuffers(render_pass);

	constexpr uint64_t aquire_image_timeout = (uint64_t)1e9;
	VkSemaphore acquired_semaphore = create_semaphore();
	VkFence acquired_fence = create_fence();
	VkSemaphore finished_rendering_semaphore  = create_semaphore();
	while (!window.is_closed_requsted()) {
		vkResetFences(context::get_device(), 1, &acquired_fence);
		uint32_t image_index;
		if (vkAcquireNextImageKHR(context::get_device(), context::get_swapchain().swapchain, aquire_image_timeout, acquired_semaphore, acquired_fence, &image_index) == VK_TIMEOUT)
			continue;
		waitFence(acquired_fence, aquire_image_timeout);
		command_buffer& to_execute = cmd_buffers[image_index];


		// execute command buffer
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = NULL;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &acquired_semaphore;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitDstStageMask = &wait_stage;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &to_execute.get_handle();
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &finished_rendering_semaphore;

		vkQueueSubmit(context::get_graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);


		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext = NULL;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &finished_rendering_semaphore;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &context::get_swapchain().swapchain;
		present_info.pImageIndices = &image_index;
		present_info.pResults = NULL;

		VkResult present_result = vkQueuePresentKHR(context::get_graphics_queue(), &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
			context::recreate_swapchain(render_pass);
		}
		
		vkDeviceWaitIdle(context::get_device());
		// present image
		window.poll_events();
	}


	vkDestroyFence(context::get_device(), acquired_fence, NULL);
	vkDestroySemaphore(context::get_device(), acquired_semaphore, NULL);
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