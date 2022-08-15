#ifndef ENGINE_RENDERER_BUFFER_H
#define ENGINE_RENDERER_BUFFER_H

#include <vulkan/vulkan.h>
#include <memory>

class memory {
public:
	static VkDeviceMemory allocate_host_visible(size_t size, uint32_t memory_type_bits = 0);
	static VkDeviceMemory allocate_device_visible(size_t size, uint32_t memory_type_bits = 0);
	static VkDeviceMemory allocate(size_t size);

	static bool memcpy_host_to_device(VkDeviceMemory memory, const void* data, size_t size);



	static void free(VkDeviceMemory memory);
private:

	static bool s_initialized_types;
	static void init();
};

class vertex_buffer {
public:
	static std::shared_ptr<vertex_buffer> create(const void* data, size_t size);
	~vertex_buffer() { destroy(); }
	void destroy();

	const VkBuffer& get_handle() { return m_handle; }
private:
	size_t m_size;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	VkMemoryRequirements m_requirements;
	VkBuffer m_handle = VK_NULL_HANDLE;

};


class index_buffer {
public:
	static std::shared_ptr<index_buffer> create(const uint32_t* data, size_t size);
	~index_buffer() { destroy(); }
	void destroy();

	const VkBuffer& get_handle() { return m_handle; }
	const uint32_t index_count() const { return (uint32_t) m_size / sizeof(uint32_t); }
private:
	size_t m_size;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	VkMemoryRequirements m_requirements;
	VkBuffer m_handle = VK_NULL_HANDLE;

};

#endif //ENGINE_RENDERER_BUFFER_H