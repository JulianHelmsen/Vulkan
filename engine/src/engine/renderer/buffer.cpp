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
		if ((type.propertyFlags & required_flag_bits) == required_flag_bits)
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
	allocate_info.memoryTypeIndex = get_memory_type_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, memory_type);
	if(allocate_info.memoryTypeIndex == INVALID_TYPE_IDX)
		allocate_info.memoryTypeIndex = get_memory_type_index(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_type);
	if (allocate_info.memoryTypeIndex == INVALID_TYPE_IDX)
		return VK_NULL_HANDLE;


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


bool create_buffer(buffer_info& info, size_t capacity, VkBufferUsageFlags usage, bool host_visible) {
	// initialize buffer info
	info.capacity = 0;
	info.handle = VK_NULL_HANDLE;
	info.memory = VK_NULL_HANDLE;

	// create the buffer handle
	VkBufferCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.size = capacity;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 1;
	create_info.pQueueFamilyIndices = (uint32_t*)&context::get_queue_families().graphics;

	if (vkCreateBuffer(context::get_device(), &create_info, NULL, &info.handle) != VK_SUCCESS)
		return false;
	// query buffer memory requirements
	vkGetBufferMemoryRequirements(context::get_device(), info.handle, &info.memory_requirements);
	
	// make sure to allocate atleast the required amount of memory
	if (capacity < info.memory_requirements.size)
		capacity = info.memory_requirements.size;

	// allocate memory
	if(host_visible)
		info.memory = memory::allocate_host_visible((VkDeviceSize)capacity, info.memory_requirements.memoryTypeBits);
	else
		info.memory = memory::allocate_device_visible((VkDeviceSize)capacity, info.memory_requirements.memoryTypeBits);

	// check for allocation errors
	if (info.memory == VK_NULL_HANDLE) {
		// failed to allocate memory for this buffer
		vkDestroyBuffer(context::get_device(), info.handle, NULL);
		info.handle = VK_NULL_HANDLE;
		info.capacity = 0;
		return false;
	}

	// bind the allocated memory to the buffer
	if (vkBindBufferMemory(context::get_device(), info.handle, info.memory, 0) != VK_SUCCESS) {
		// failed to bind memory to buffer
		vkFreeMemory(context::get_device(), info.memory, NULL);
		vkDestroyBuffer(context::get_device(), info.handle, NULL);
		info.memory = VK_NULL_HANDLE;
		info.handle = VK_NULL_HANDLE;
		info.capacity = 0;
		return false;
	}

	// set buffer size
	info.capacity = capacity;
	// buffer creation, memory allocation and binding was successfull
	return true;
}


std::shared_ptr<index_buffer> index_buffer::create() {
	return std::make_shared<index_buffer>();
}

std::shared_ptr<vertex_buffer> vertex_buffer::create() {
	return std::make_shared<vertex_buffer>();
}

void vertex_buffer::destroy() {
	if(m_info.handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_info.handle, NULL);
	if(m_info.memory != VK_NULL_HANDLE)
		memory::free(m_info.memory);
	m_info.memory = VK_NULL_HANDLE;
	m_info.handle = VK_NULL_HANDLE;
	m_info.capacity = 0;
}

void index_buffer::destroy() {
	if (m_info.handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_info.handle, NULL);
	if (m_info.memory != VK_NULL_HANDLE)
		memory::free(m_info.memory);
	m_info.memory = VK_NULL_HANDLE;
	m_info.handle = VK_NULL_HANDLE;
	m_info.capacity = 0;
}



bool vertex_buffer::set_buffer_data(const void* data, size_t n_bytes) {
	// (re)allocate buffer
	if(m_info.capacity < n_bytes) {
		// clean up old memory
		if (m_info.handle != VK_NULL_HANDLE)
			destroy();

		// allocate new buffer
		if (!create_buffer(m_info, n_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, true))
			return false;
	}

	// memcpy host to device
	if (!memory::memcpy_host_to_device(m_info.memory, data, n_bytes)) {
		destroy();
		return false;
	}
	m_size = n_bytes;
	return true;
}
bool index_buffer::set_buffer_data(const uint32_t* indices, size_t n_bytes) {
	// (re)allocate buffer
	if (m_info.capacity < n_bytes) {
		// clean up old memory
		if (m_info.handle != VK_NULL_HANDLE)
			destroy();

		// allocate new buffer
		if (!create_buffer(m_info, n_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, true))
			return false;
	}


	// memcpy host to device
	if (!memory::memcpy_host_to_device(m_info.memory, indices, n_bytes)) {
		destroy();
		return false;
	}
	m_size = n_bytes;
	return true;
}

