#ifndef ENGINE_RENDERER_BUFFER_H
#define ENGINE_RENDERER_BUFFER_H

#include <vulkan/vulkan.h>
#include <memory>
#include "memory.h"


class memory {
public:

	static bool memcpy_host_to_device(const allocator::sub_allocation& memory, const void* data, size_t size);

};

struct buffer_info {
	buffer_info() : capacity(0), memory{}, handle(VK_NULL_HANDLE) {}
	size_t capacity;
	allocator::sub_allocation memory;
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