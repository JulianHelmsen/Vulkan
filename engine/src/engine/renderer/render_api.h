#ifndef ENGINE_RENDERER_RENDER_API_H
#define ENGINE_RENDERER_RENDER_API_H

#include <vulkan/vulkan.h>

class render_api {
public:

	static bool init();
	static void shutdown();
	static VkInstance get_instance();

};


#endif //ENGINE_RENDERER_RENDER_API_H