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

bool memory::memcpy_host_to_device(VkDeviceMemory memory, const void* data, size_t size) {
	VkDeviceSize d_size = size;
	if (size % s_info.nonCoherentAtomSize)
		d_size = VK_WHOLE_SIZE;

	void* mapped;
	if (vkMapMemory(context::get_device(), memory, 0, d_size, 0, &mapped) != VK_SUCCESS)
		return false;
	memcpy(mapped, data, size);
	VkMappedMemoryRange range;
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext = NULL;
	range.memory = memory;
	range.offset = 0;
	range.size = d_size;

	if (vkFlushMappedMemoryRanges(context::get_device(), 1, &range) != VK_SUCCESS)
		return false;


	vkUnmapMemory(context::get_device(), memory);
	return true;
}

void memory::free(VkDeviceMemory memory) {
	vkFreeMemory(context::get_device(), memory, NULL);
}

struct create_buffer_out {
	VkDeviceMemory* memory;
	VkMemoryRequirements* requirements;
};

static VkBuffer create_buffer(create_buffer_out& out, const void* data, size_t size, VkBufferUsageFlagBits usage) {
	VkBufferCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.size = size;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 1;
	create_info.pQueueFamilyIndices = (uint32_t*)&context::get_queue_families().graphics;

	VkBuffer buffer;
	if (vkCreateBuffer(context::get_device(), &create_info, NULL, &buffer) != VK_SUCCESS)
		return NULL;

	vkGetBufferMemoryRequirements(context::get_device(), buffer, out.requirements);
	VkDeviceSize memorySize = size;
	if (size < out.requirements->size)
		memorySize = out.requirements->size;
	VkDeviceMemory memory = memory::allocate_host_visible(memorySize, out.requirements->memoryTypeBits);
	VkResult res = vkBindBufferMemory(context::get_device(), buffer, memory, 0);
	if (res != VK_SUCCESS) {
		vkDestroyBuffer(context::get_device(), buffer, NULL);
		return VK_NULL_HANDLE;
	}
	

	if (!memory::memcpy_host_to_device(memory, data, size)) {
		memory::free(memory);
		vkDestroyBuffer(context::get_device(), buffer, NULL);
		return VK_NULL_HANDLE;
	}
	*out.memory = memory;
	return buffer;
}

std::shared_ptr<vertex_buffer> vertex_buffer::create(const void* data, size_t size) {
	std::shared_ptr<vertex_buffer> buffer = std::make_shared<vertex_buffer>();
	create_buffer_out out;
	out.memory = &buffer->m_memory;
	out.requirements = &buffer->m_requirements;
	buffer->m_handle = create_buffer(out, data, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	buffer->m_size = size;
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


std::shared_ptr<index_buffer> index_buffer::create(const uint32_t* data, size_t size) {
	std::shared_ptr<index_buffer> buffer = std::make_shared<index_buffer>();
	create_buffer_out out;
	out.memory = &buffer->m_memory;
	out.requirements = &buffer->m_requirements;
	buffer->m_handle = create_buffer(out, data, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	buffer->m_size = size;
	return buffer;
}

void index_buffer::destroy() {
	if (m_handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_handle, NULL);
	if (m_memory != VK_NULL_HANDLE)
		memory::free(m_memory);
	m_memory = VK_NULL_HANDLE;
	m_handle = VK_NULL_HANDLE;
}