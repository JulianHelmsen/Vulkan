#ifndef ENGINE_RENDERER_BUFFER_H
#define ENGINE_RENDERER_BUFFER_H

#include <vulkan/vulkan.h>
#include <memory>
#include "memory.h"


class memory {
public:
	static VkDeviceMemory allocate_host_visible(size_t size, uint32_t memory_type_bits = 0);
	static VkDeviceMemory allocate_device_visible(size_t size, uint32_t memory_type_bits = 0);

	static bool memcpy_host_to_device(VkDeviceMemory memory, const void* data, size_t size);


	static void free(VkDeviceMemory memory);
private:

	static bool s_initialized_types;
	static void init();
};

struct buffer_info {
	buffer_info() : capacity(0), memory(VK_NULL_HANDLE), handle(VK_NULL_HANDLE) {}
	size_t capacity;
	VkDeviceMemory memory;
	VkMemoryRequirements memory_requirements;

	VkBuffer handle;
};

bool create_buffer(buffer_info& info, size_t capacity, VkBufferUsageFlags usage, bool host_visible = false);


class vertex_buffer {
public:
	static std::shared_ptr<vertex_buffer> create();
	~vertex_buffer() { destroy(); }
	void destroy();

	bool set_buffer_data(const void* data, size_t n_bytes);
	const VkBuffer& get_handle() { return m_info.handle; }
private:

	buffer_info m_info{};
	size_t m_size;

};


class index_buffer {
public:
	static std::shared_ptr<index_buffer> create();
	~index_buffer() { destroy(); }
	void destroy();

	bool set_buffer_data(const uint32_t* indices, size_t n_bytes);

	const VkBuffer& get_handle() { return m_info.handle; }
	const uint32_t index_count() const { return (uint32_t)m_size / sizeof(uint32_t); }
private:
	buffer_info m_info{};
	size_t m_size;

};

#endif //ENGINE_RENDERER_BUFFER_H