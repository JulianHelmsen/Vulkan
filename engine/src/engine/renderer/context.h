#ifndef ENGINE_RENDERER_CONTEXT_H
#define ENGINE_RENDERER_CONTEXT_H

#include "engine/core/window.h"
#include <vulkan/vulkan.h>
#include "framebuffer.h"
#include "memory.h"
#include <functional>


class context {
public:
	using framebuffer_change_callback = std::function<void()>;
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
	struct queue_family_indices {
		int graphics;
		int transfer;
		bool dedicated_transfer = false;
	};


	const VkDevice& device() const { return m_device; }
	const VkPhysicalDevice& physical_device() const { return m_physical_device; }
	allocator& memory_allocator() { return m_allocator; }
	const allocator& memory_allocator() const { return m_allocator; }


	static VkDevice get_device() { return s_current->m_device; }
	static VkPhysicalDevice get_physical_device() { return s_current->m_physical_device; }
	static const surface& get_surface() { return s_current->m_surface; }
	static const swapchain& get_swapchain() { return s_current->m_swapchain; }
	static const VkCommandPool& get_command_pool() { return s_current->m_command_pool; }
	static const VkQueue& get_graphics_queue() { return s_current->m_graphics_queue; }
	static const VkQueue& get_transfer_queue() { return s_current->m_transfer_queue; }
	static allocator& get_memory_allocator() { return s_current->m_allocator; }

	static bool recreate_swapchain(VkRenderPass render_pass) { return s_current->recreate_swapchain_impl(render_pass); }
	static bool create_window_framebuffers(VkRenderPass render_pass) { return s_current->create_window_framebuffers_impl(render_pass); }

	static const framebuffer& get_window_framebuffer(uint32_t index) { return s_current->m_window_framebuffers[index]; }

	static void set_framebuffer_change_callback(framebuffer_change_callback callback) { s_current->m_framebuffer_change_callback = callback; }

	static const queue_family_indices& get_queue_families() { return s_current->m_queue_family_indices; }

	static VkResult begin_frame(uint32_t* image_index) { return s_current->begin_frame_impl(image_index); }

	static VkResult end_frame(VkSemaphore wait_semaphore) { return s_current->end_frame_impl(wait_semaphore); }

private:
	bool recreate_swapchain_impl(VkRenderPass render_pass);

	VkResult begin_frame_impl(uint32_t* image_index);
	VkResult end_frame_impl(VkSemaphore wait_semaphore);

	
	bool init(window_handle_t handle);
	bool create_surface(window_handle_t handle);

	bool select_physical_device(const std::vector<const char*>& extensions);
	bool create_logical_device(const std::vector<const char*>& extensions);
	bool create_swapchain();
	bool create_command_pool();
	bool create_window_framebuffers_impl(VkRenderPass render_pass);


	framebuffer_change_callback m_framebuffer_change_callback;
	static context* s_current;
	surface m_surface{};
	swapchain m_swapchain{};

	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	VkQueue m_transfer_queue = VK_NULL_HANDLE;
	VkQueue m_graphics_queue = VK_NULL_HANDLE;

	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	queue_family_indices m_queue_family_indices;

	framebuffer* m_window_framebuffers = NULL;

	allocator m_allocator;

	uint32_t m_current_image_index;
	VkFence m_acquired_fence;

};

#endif //ENGINE_RENDERER_CONTEXT_H