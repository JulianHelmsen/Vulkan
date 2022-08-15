#include "buffer.h"
#include "context.h"
#include <assert.h>

bool memory::s_initialized_types = false;
#define INVALID_TYPE_IDX (0xFFFFFFFF)
static struct {
	VkPhysicalDeviceMemoryProperties properties;
	VkDeviceSize nonCoherentAtomSize;
}s_info;

void memory::init() {
	
	vkGetPhysicalDeviceMemoryProperties(context::get_physical_device(), &s_info.properties);
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(context::get_physical_device(), &props);
	s_info.nonCoherentAtomSize = props.limits.nonCoherentAtomSize;

	s_initialized_types = true;
}

static uint32_t get_memory_type_index(VkMemoryPropertyFlags required_flag_bits, uint32_t memory_requirment_type_bits) {
	for (uint32_t type_idx = 0; type_idx < s_info.properties.memoryTypeCount; type_idx++) {
		bool required_type = ((1 << type_idx) & memory_requirment_type_bits) != 0;
		const auto& type = s_info.properties.memoryTypes[type_idx];
		if (type.propertyFlags & required_flag_bits)
			return type_idx;

	}
	return INVALID_TYPE_IDX;
}

VkDeviceMemory memory::allocate_host_visible(size_t size, uint32_t memory_type) {
	if (!s_initialized_types)
		init();

	VkMemoryAllocateInfo allocate_info = { };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = (VkDeviceSize)size;
	allocate_info.memoryTypeIndex = get_memory_type_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_type);

	auto& type = s_info.properties.memoryTypes[allocate_info.memoryTypeIndex];
	auto& heap = s_info.properties.memoryHeaps[type.heapIndex];

	// currently not worrying about memory too much
	assert(allocate_info.allocationSize < heap.size);

	VkDeviceMemory memory;
	if (vkAllocateMemory(context::get_device(), &allocate_info, NULL, &memory) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return memory;
}

VkDeviceMemory memory::allocate_device_visible(size_t size, uint32_t memory_type_bits) {
	if (!s_initialized_types)
		init();

	VkMemoryAllocateInfo allocate_info = { };
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.allocationSize = (VkDeviceSize)size;
	allocate_info.memoryTypeIndex = get_memory_type_index(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_type_bits);

	auto& type = s_info.properties.memoryTypes[allocate_info.memoryTypeIndex];
	auto& heap = s_info.properties.memoryHeaps[type.heapIndex];

	// currently not worrying about memory too much
	assert(allocate_info.allocationSize < heap.size);

	VkDeviceMemory memory;
	if (vkAllocateMemory(context::get_device(), &allocate_info, NULL, &memory) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return memory;
}

void memory::memcpy_host_to_device(VkDeviceMemory memory, const void* data, size_t size) {
	VkDeviceSize d_size = size;
	if (size % s_info.nonCoherentAtomSize)
		d_size = VK_WHOLE_SIZE;

	void* mapped;
	vkMapMemory(context::get_device(), memory, 0, d_size, 0, &mapped);
	memcpy(mapped, data, size);
	VkMappedMemoryRange range;
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext = NULL;
	range.memory = memory;
	range.offset = 0;
	range.size = d_size;

	VkResult res = vkFlushMappedMemoryRanges(context::get_device(), 1, &range);


	vkUnmapMemory(context::get_device(), memory);
}

void memory::free(VkDeviceMemory memory) {
	vkFreeMemory(context::get_device(), memory, NULL);
}

std::shared_ptr<vertex_buffer> vertex_buffer::create(const void* data, size_t size) {
	std::shared_ptr<vertex_buffer> buffer = std::make_shared<vertex_buffer>();
	VkBufferCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.size = size;
	create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 1;
	create_info.pQueueFamilyIndices = (uint32_t*) &context::get_queue_families().graphics;

	if (vkCreateBuffer(context::get_device(), &create_info, NULL, &buffer->m_handle) != VK_SUCCESS)
		return NULL;

	vkGetBufferMemoryRequirements(context::get_device(), buffer->m_handle, &buffer->m_requirements);
	VkDeviceSize memorySize = size;
	if (size < buffer->m_requirements.size)
		memorySize = buffer->m_requirements.size;
	VkDeviceMemory memory = memory::allocate_host_visible(memorySize, buffer->m_requirements.memoryTypeBits);
	VkResult res = vkBindBufferMemory(context::get_device(), buffer->m_handle, memory, 0);
	buffer->m_memory = memory;

	memory::memcpy_host_to_device(buffer->m_memory, data, size);

	return buffer;
}
void vertex_buffer::destroy() {
	if(m_handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_handle, NULL);
	if(m_memory != VK_NULL_HANDLE)
		memory::free(m_memory);
	m_memory = VK_NULL_HANDLE;
	m_handle = VK_NULL_HANDLE;
}
