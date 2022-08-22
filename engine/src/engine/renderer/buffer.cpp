#include "buffer.h"
#include "context.h"
#include <assert.h>




bool create_buffer(buffer_info& info, size_t capacity, VkBufferUsageFlags usage, bool host_visible) {
	// initialize buffer info
	info.capacity = 0;
	info.handle = VK_NULL_HANDLE;
	info.memory = allocator::invalid_allocation;

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
	allocator& allocator = context::get_memory_allocator();
	allocator::access_flags access = host_visible ? allocator::access_flags::DYNAMIC : allocator::access_flags::STATIC;
	allocator::sub_allocation sub_allocation = allocator.allocate(capacity, info.memory_requirements.memoryTypeBits, access);


	// check for allocation errors
	if (!sub_allocation) {
		// failed to allocate memory for this buffer
		vkDestroyBuffer(context::get_device(), info.handle, NULL);
		info.handle = VK_NULL_HANDLE;
		info.capacity = 0;
		return false;
	}

	// bind the allocated memory to the buffer
	if (vkBindBufferMemory(context::get_device(), info.handle, sub_allocation.handle, sub_allocation.start_address) != VK_SUCCESS) {
		// failed to bind memory to buffer
		allocator.free(sub_allocation);
		vkDestroyBuffer(context::get_device(), info.handle, NULL);
		info.memory = allocator::invalid_allocation;
		info.handle = VK_NULL_HANDLE;
		info.capacity = 0;
		return false;
	}

	// set buffer size
	info.capacity = capacity;
	info.memory = sub_allocation;
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
	allocator& allocator = context::get_memory_allocator();
	if (m_info.memory)
		allocator.free(m_info.memory);
	m_info.memory = allocator::invalid_allocation;
	m_info.handle = VK_NULL_HANDLE;
	m_info.capacity = 0;
}

void index_buffer::destroy() {
	if (m_info.handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_info.handle, NULL);
	allocator& allocator = context::get_memory_allocator();
	if (m_info.memory)
		allocator.free(m_info.memory);
	m_info.memory = allocator::invalid_allocation;
	m_info.handle = VK_NULL_HANDLE;
	m_info.capacity = 0;
}



bool vertex_buffer::set_buffer_data(command_buffer& cmd_buf, std::shared_ptr<staging_buffer> staging, const void* data, size_t n_bytes) {
	// (re)allocate buffer
	if(m_info.capacity < n_bytes) {
		// clean up old memory
		if (m_info.handle)
			destroy();

		// allocate new buffer
		if (!create_buffer(m_info, n_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false))
			return false;
	}

	// memcpy host to device
	staging->cpy(cmd_buf, m_info.handle, 0, data, n_bytes);
	m_size = n_bytes;
	return true;
}
bool index_buffer::set_buffer_data(command_buffer& cmd_buf, std::shared_ptr<staging_buffer> staging, const uint32_t* indices, size_t n_bytes) {
	// (re)allocate buffer
	if (m_info.capacity < n_bytes) {
		// clean up old memory
		if (m_info.handle)
			destroy();

		// allocate new buffer
		if (!create_buffer(m_info, n_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false))
			return false;
	}


	// memcpy host to device
	staging->cpy(cmd_buf, m_info.handle, 0, indices, n_bytes);
	m_size = n_bytes;
	return true;
}

std::shared_ptr<staging_buffer> staging_buffer::create() {

	std::shared_ptr<staging_buffer> staging = std::make_shared<staging_buffer>();
	size_t n_bytes = 1 << 20;
	if (!create_buffer(staging->m_info, n_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true))
		return NULL;

	staging->m_size = n_bytes;

	return staging;
}

void staging_buffer::destroy() {
	if (m_info.handle != VK_NULL_HANDLE)
		vkDestroyBuffer(context::get_device(), m_info.handle, NULL);
	allocator& allocator = context::get_memory_allocator();
	if (m_info.memory)
		allocator.free(m_info.memory);
	m_info.memory = allocator::invalid_allocation;
	m_info.handle = VK_NULL_HANDLE;
	m_info.capacity = 0;
}

void staging_buffer::cpy(command_buffer& cmd_buf, VkBuffer dest, VkDeviceAddress offset, const void* data, VkDeviceSize size) {
	if (size == 0)
		return;
	memory::memcpy_host_to_device(m_info.memory, data, size);


	VkBufferCopy cpy {};
	cpy.srcOffset = 0;
	cpy.dstOffset = 0;
	cpy.size = size;

	vkCmdCopyBuffer(cmd_buf.get_handle(), m_info.handle, dest, 1, &cpy);
}