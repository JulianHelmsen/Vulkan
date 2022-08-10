#ifndef ENGINE_RENDERER_RENDER_API_H
#define ENGINE_RENDERER_RENDER_API_H

#include <vulkan/vulkan.h>
#include "engine/core/window.h"

class render_api {
public:
	static bool init(window& window);
	static void shutdown();


	static const VkInstance& get_instance();
private:
	static VkSurfaceKHR create_surface(window_handle_t handle);
};

#endif //ENGINE_RENDERER_RENDER_API_H