#ifndef ENGINE_RENDERER_CONTEXT_H
#define ENGINE_RENDERER_CONTEXT_H

#include "engine/core/window.h"
#include <vulkan/vulkan.h>

class context {
public:
	static context* create_context(window_handle_t handle);
	void make_context_current();

	~context();
	static context* get_current() { return s_current; }

	struct swapchain {
		VkSwapchainKHR swapchain;
		VkExtent2D extent;
		uint32_t image_count;
		VkImage* images;
	};

	struct surface {
		VkSurfaceKHR surface;
		VkSurfaceFormatKHR surface_format;
	};

	static VkDevice get_device() { return s_current->m_device; }
	static VkPhysicalDevice get_physical_device() { return s_current->m_physical_device; }
	static const surface& get_surface() { return s_current->m_surface; }
	static const swapchain& get_swapchain() { return s_current->m_swapchain; }
	static const VkCommandPool& get_command_pool() { return s_current->m_command_pool; }
	static const VkQueue& get_graphics_queue() { return s_current->m_graphics_queue; }
	static const VkQueue& get_transfer_queue() { return s_current->m_transfer_queue; }

private:


	struct queue_family_indices {
		int graphics;
		int transfer;
		bool dedicated_transfer = false;
	};

	bool init(window_handle_t handle);
	bool create_surface(window_handle_t handle);

	bool select_physical_device(const std::vector<const char*>& extensions);
	bool create_logical_device(const std::vector<const char*>& extensions);
	bool create_swapchain();
	bool create_command_pool();

	static context* s_current;
	surface m_surface{};
	swapchain m_swapchain{};

	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	VkQueue m_transfer_queue = VK_NULL_HANDLE;
	VkQueue m_graphics_queue = VK_NULL_HANDLE;

	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	queue_family_indices m_queue_family_indices;



};

#endif //ENGINE_RENDERER_CONTEXT_H