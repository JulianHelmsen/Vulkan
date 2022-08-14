#include <stdlib.h>
#include <engine/engine.h>
#include <stdio.h>
#include <assert.h>


void on_resize(int width, int height) {
	printf("%d, %d\n", width, height);
	render_api::recreate_swapchain();
}


int main(const int argc, const char** argv) {
	
	// create the window
	window window{"Title", 1020, 720};

	window.on_resize(on_resize);

	// basic initialization of vulkan
	if (!render_api::init(window)) {
		window.destroy();
		return -1;
	}

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
	VkExtent2D swapchain_extent = render_api::get_swapchain_extent();
	pipeline_builder.set_viewport(0.0f, 0.0f, (float) swapchain_extent.width, (float)swapchain_extent.height);
	pipeline_builder.set_vertex_shader(vertex);
	pipeline_builder.set_fragment_shader(fragment);
	VkPipeline pipeline = pipeline_builder.build();
	
	vkDestroyShaderModule(render_api::get_device(), vertex, NULL);
	vkDestroyShaderModule(render_api::get_device(), fragment, NULL);

	// create frame buffers
	framebuffer framebuffers[2];
	assert(stack_array_len(framebuffers) == render_api::get_swapchain_image_count());
	for (int i = 0; i < sizeof(framebuffers) / sizeof(framebuffers[0]); i++) {
		if (!framebuffers[i].add_color_attachment(render_api::get_swapchain_image(i), render_api::get_surface_format().format))
			return -1;
		if (!framebuffers[i].create(render_pass, window.get_width(), window.get_height()))
			return -1;
	}

	// record command buffers
	command_buffer cmd_buffers[2];
	for (int i = 0; i < stack_array_len(cmd_buffers); i++) {
		command_buffer& cmd_buf = cmd_buffers[i];
		cmd_buf.start();
		VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

		VkClearValue clear_value = {1.0f, 0.0f, 0.0f, 1.0f};

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.pNext = NULL;
		render_pass_begin_info.renderPass = render_pass;
		render_pass_begin_info.framebuffer = framebuffers[i].get_handle();
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent = render_api::get_swapchain_extent();

		vkCmdBeginRenderPass(cmd_buf.get_handle(), &render_pass_begin_info, contents);
		vkCmdBindPipeline(cmd_buf.get_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);


		// set scissors and viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width =  (float) render_pass_begin_info.renderArea.extent.width;
		viewport.height = (float) render_pass_begin_info.renderArea.extent.height;
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

	constexpr uint64_t aquire_image_timeout = (uint64_t)1e9;
	VkSemaphore acquired_semaphore = create_semaphore();
	VkFence acquired_fence = create_fence();
	VkSemaphore finished_rendering_semaphore  = create_semaphore();
	while (!window.is_closed_requsted()) {
		vkResetFences(render_api::get_device(), 1, &acquired_fence);
		uint32_t image_index;
		if (vkAcquireNextImageKHR(render_api::get_device(), render_api::get_swapchain(), aquire_image_timeout, acquired_semaphore, acquired_fence, &image_index) == VK_TIMEOUT)
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

		vkQueueSubmit(render_api::get_graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);


		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext = NULL;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &finished_rendering_semaphore;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &render_api::get_swapchain();
		present_info.pImageIndices = &image_index;
		present_info.pResults = NULL;

		vkQueuePresentKHR(render_api::get_graphics_queue(), &present_info);
		
		vkDeviceWaitIdle(render_api::get_device());
		// present image
		window.poll_events();
	}


	vkDestroyFence(render_api::get_device(), acquired_fence, NULL);
	vkDestroySemaphore(render_api::get_device(), acquired_semaphore, NULL);
	vkDestroySemaphore(render_api::get_device(), finished_rendering_semaphore, NULL);
	for(command_buffer& cmd_buffer : cmd_buffers)
		cmd_buffer.destroy();
	for(framebuffer framebuffer : framebuffers)
		framebuffer.destroy();
	vkDestroyPipeline(render_api::get_device(), pipeline, NULL);
	vkDestroyRenderPass(render_api::get_device(), render_pass, NULL);
	render_api::shutdown();
	window.destroy();
	return 0;
}