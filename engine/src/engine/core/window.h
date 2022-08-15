#ifndef ENGINE_CORE_WINDOW_H
#define ENGINE_CORE_WINDOW_H

#include <stdint.h>
#include <functional>

typedef uint64_t window_handle_t;

class window {
public:
	using resize_event_handler = std::function<void(int, int)>;
	window(const char* title, int width, int height);

	void poll_events();

	void wait_events();

	inline bool is_closed() const { return m_closed; }
	inline bool is_closed_requsted() const { return m_close_requested; }
	bool is_minimized() const;

	void destroy();

	inline window_handle_t get_handle() const { return m_window_handle; }

	int get_width() const { return m_width; };
	int get_height() const { return m_height; };


	void on_resize(resize_event_handler handler) { m_resize_handler = handler; }
private:

	int m_width;
	int m_height;

	window_handle_t m_window_handle;
	bool m_close_requested;
	bool m_closed;


	resize_event_handler m_resize_handler;
	friend void on_window_close_requested(window* window);
	friend void on_window_resize(window* window, int new_width, int new_height);
};


#endif //ENGINE_CORE_WINDOW_H