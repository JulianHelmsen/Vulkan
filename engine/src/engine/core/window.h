#ifndef ENGINE_CORE_WINDOW_H
#define ENGINE_CORE_WINDOW_H

#include <stdint.h>

class window {
public:
	window(const char* title, int width, int height);

	void poll_events();

	void wait_events();

	inline bool is_closed() const { return m_closed; }

	void destroy();

	inline uint64_t get_handle() const { return m_window_handle; }

private:
	uint64_t m_window_handle;
	bool m_closed;
};


#endif //ENGINE_CORE_WINDOW_H