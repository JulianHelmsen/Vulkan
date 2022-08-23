#include "application.h"
#include "engine/renderer/render_api.h"
#include "engine/renderer/synchronization.h"
#include "log.h"



bool application::create() {
	m_window = new window("window", 1020, 720);
	// basic initialization of vulkan
	if (!render_api::init())
		return false;

	m_rendering_context= context::create_context(m_window->get_handle());
	if (m_rendering_context == NULL)
		return false;

	m_rendering_context->make_context_current();
	m_finished_rendering = create_semaphore();
	if (m_finished_rendering == VK_NULL_HANDLE)
		return false;

	if (!on_create())
		return false;
	
	if (m_render_pass == VK_NULL_HANDLE) {
		err("The client did not create a render pass!\n");
		return false;
	}
	m_command_buffers = new command_buffer[context::get_swapchain().image_count];

	context::create_window_framebuffers(m_render_pass);

	return true;
}


bool application::update(float delta_time) {
	bool success = true;

	uint32_t image_index;
	if (context::begin_frame(&image_index) == VK_TIMEOUT)
		return false;

	command_buffer& cmd_buf = m_command_buffers[image_index];
	if (cmd_buf.reset() != VK_SUCCESS) {
		err("Failed to reset command buffer\n");
	}

	cmd_buf.start();
	success &= on_update(cmd_buf, delta_time);
	cmd_buf.end();
	
	
	cmd_buf.submit(context::get_graphics_queue(), context::get_acquired_semaphore(), m_finished_rendering, context::get_in_flight_fence(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	
	if (context::end_frame(m_finished_rendering) == VK_ERROR_OUT_OF_DATE_KHR) {
		while (m_window->is_minimized() && !m_window->is_closed_requsted())
			m_window->wait_events();
		context::recreate_swapchain(m_render_pass);
	}


	return success;
}

bool application::start() {
	m_running = true;
	auto prev = std::chrono::high_resolution_clock::now();

	m_app_start_time = std::chrono::high_resolution_clock::now();
	
	while (m_running && !m_window->is_closed_requsted()) {
		auto now = std::chrono::high_resolution_clock::now();
		float delta_time = 1e-6f * std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count();
		prev = now;

		if (!update(delta_time))
			m_running = false;
		

		m_window->poll_events();
	}


	return true;
}

void application::terminate() {
	vkDeviceWaitIdle(context::get_device());
	on_terminate();

	for (uint32_t i = 0; i < context::get_swapchain().image_count; i++) {
		m_command_buffers[i].destroy();
	}

	delete[] m_command_buffers;

	vkDestroySemaphore(m_rendering_context->device(), m_finished_rendering, NULL);
	delete m_rendering_context;
	render_api::shutdown();

	if (m_window) {
		m_window->destroy();
		delete m_window;
		m_window = NULL;
	}
}
float application::get_time() const {
	auto now = std::chrono::high_resolution_clock::now();
	return 1e-6f * std::chrono::duration_cast<std::chrono::microseconds>(now - m_app_start_time).count();
}