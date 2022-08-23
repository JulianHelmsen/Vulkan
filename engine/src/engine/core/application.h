#ifndef ENGINE_CORE_APPLICATION_H
#define ENGINE_CORE_APPLICATION_H

#include "window.h"
#include "engine/renderer/context.h"
#include "engine/renderer/command_buffer.h"
#include <chrono>

class application {
public:

	virtual bool on_create() { return true; }
	virtual bool on_update(command_buffer& cmd_buf, float delta_time) { return true; }
	virtual void on_terminate() {}


	float get_time() const;
protected:
	VkRenderPass m_render_pass = VK_NULL_HANDLE;
	bool m_running;
private:

	bool create();
	bool start();
	bool update(float delta_time);
	void terminate();


	VkSemaphore m_finished_rendering;

	std::chrono::steady_clock::time_point m_app_start_time;

	window* m_window;
	context* m_rendering_context;

	command_buffer* m_command_buffers;

	friend int main(const int, const char**);
};

#endif //ENGINE_CORE_APPLICATION_H