#ifndef ENGINE_CORE_WINDOW_H
#define ENGINE_CORE_WINDOW_H

#include <stdint.h>
#include <vulkan/vulkan.h>

typedef uint64_t window_handle_t;

class window {
public:
	window(const char* title, int width, int height);

	void poll_events();

	void wait_events();

	inline bool is_closed() const { return m_closed; }
	inline bool is_closed_requsted() const { return m_close_requested; }

	void destroy();

	inline window_handle_t get_handle() const { return m_window_handle; }

	int get_width() const { return m_width; };
	int get_height() const { return m_height; };


	inline VkSurfaceKHR get_surface() const { return m_surface; }

	bool create_surface(VkInstance instance);
private:

	int m_width;
	int m_height;

	VkSurfaceKHR m_surface;

	window_handle_t m_window_handle;
	bool m_close_requested;
	bool m_closed;

	friend void on_window_close_requested(window* window);
	friend void on_window_resize(window* window, int new_width, int new_height);
};


#endif //ENGINE_CORE_WINDOW_H