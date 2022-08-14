#ifndef ENGINE_RENDERER_RENDER_API_H
#define ENGINE_RENDERER_RENDER_API_H

#include <vulkan/vulkan.h>
#include "engine/core/window.h"

class render_api {
public:
	static bool init(window& window);
	static void shutdown();

	static void recreate_swapchain();


	static VkSurfaceFormatKHR get_surface_format();
	static VkInstance get_instance();
	static VkDevice get_device();
	static VkQueue get_transfer_queue();
	static VkQueue get_graphics_queue();
	static uint32_t get_graphics_queue_familily();

	static uint32_t get_swapchain_image_count();
	static VkImage get_swapchain_image(uint32_t index);

};

#endif //ENGINE_RENDERER_RENDER_API_H