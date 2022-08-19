#include "memory.h"
#include <stdio.h>
#include <assert.h>

allocator::sub_allocation allocator::invalid_allocation = {};




void allocator::free(memory& memory, const sub_allocation& allocation) {
	if (allocation.handle == VK_NULL_HANDLE)
		return;
	assert(allocation.memory_type_index == memory.memory_type_index);

	for (auto it = memory.blocks.begin(); it != memory.blocks.end(); it++) {
		memory_block& block = *it;
		if (block.address == allocation.start_address) {
			printf("free: [%d, %d): %d\n", (int)block.start(), (int)block.end(), (int) block.size);
			memory.blocks.erase(it);
			break;
		}
	}

}


bool allocator::find_space(memory_block* space, size_t* index, const memory& memory, size_t size) {

	for (size_t i = 0; i <= memory.blocks.size(); i++) {
		

		if (i == 0) {
			space->address = 0;
		}else {
			space->address = memory.blocks[i - 1].end();
		}

		if (i == memory.blocks.size()) {
			space->size = memory.size - space->address;
		}else {
			space->size = memory.blocks[i].start() - space->address;
		}

		
		if (space->size >= size) {
			*index = i;
			return true;
		}
	}
	return false;
}

allocator::sub_allocation allocator::sub_allocate(memory& memory, size_t size) {
	if (memory.handle == VK_NULL_HANDLE)
		if (!initial_allocation(memory))
			return invalid_allocation;

	if (size > memory.size)
		return invalid_allocation;

	memory_block space;
	size_t insert_index;
	if (find_space(&space, &insert_index, memory, size)) {
		auto& blocks = memory.blocks;
		space.size = size;
		blocks.insert(blocks.begin() + insert_index, space);

		sub_allocation allocated_memory;
		allocated_memory.handle = memory.handle;
		allocated_memory.memory_type_index = memory.memory_type_index;
		allocated_memory.start_address = space.address;
		printf("allocate: [%d, %d): %d\n", (int)space.start(), (int)space.end(), (int)space.size);
		return sub_allocation{ memory.handle, memory.memory_type_index, space.address };
	}

	
	return invalid_allocation;
}


VkDeviceSize allocator::align(VkDeviceSize size) {
	VkDeviceSize mod = size % m_alignment;
	VkDeviceSize to_add = m_alignment - mod;

	assert((to_add + size) % m_alignment == 0);
	return size + to_add;
}

bool allocator::initial_allocation(memory& memory) {
	VkDeviceSize allocate_size = m_default_allocation_size;
	VkDeviceSize two_third_heap_size = memory.heap_type_info.size * 2 / 3;
	if (allocate_size > two_third_heap_size)
		allocate_size = two_third_heap_size; // 66.66% of total heap size
	allocate_size = align(allocate_size);
	assert(allocate_size % m_alignment == 0);

	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.pNext = NULL;
	info.allocationSize = allocate_size;
	info.memoryTypeIndex = memory.memory_type_index;
	if (vkAllocateMemory(m_device, &info, NULL, &memory.handle) != VK_SUCCESS) {
		memory.handle = NULL;
		memory.size = 0;
		return false;
	}
	memory.size = allocate_size;
	return true;
}

void allocator::initialize(VkPhysicalDevice physical_device, VkDevice device) {
	m_device = device;
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);
	m_memory_type_count = properties.memoryTypeCount;

	for (uint32_t i = 0; i < m_memory_type_count; i++) {
		memory& mem = m_allocated_memory_types[i];
		mem.memory_type_index = i;
		mem.memory_type_info = properties.memoryTypes[i];
		mem.heap_type_info = properties.memoryHeaps[mem.memory_type_info.heapIndex];

		// initialize later:
		mem.size = 0;
		mem.handle = VK_NULL_HANDLE;
	}

	m_initialized = true;
}



bool allocator::matches_type(const memory& memory, uint32_t memory_type_bits, access_flags flags) {
	bool matches_type = ((1 << memory.memory_type_index) & memory_type_bits) != 0;
	VkMemoryPropertyFlagBits access_bits;
	if (flags & access_flags::STATIC)
		access_bits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	else if (flags & access_flags::DYNAMIC)
		access_bits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	bool matches_access_bits = (access_bits & memory.memory_type_info.propertyFlags) == access_bits;
	return matches_type && matches_access_bits;
}

allocator::memory* allocator::find_memory_type(uint32_t memory_type_bits, access_flags access_flags) {
	if (!m_initialized)
		return NULL;

	for (uint32_t i = 0; i < m_memory_type_count; i++) {
		memory& mem = m_allocated_memory_types[i];
		if (matches_type(mem, memory_type_bits, access_flags))
			return &mem;
	}

	return NULL;
}


allocator::sub_allocation allocator::allocate(size_t size, uint32_t memory_type_bits, access_flags access_flags) {
	size = align(size);
	memory* mem = find_memory_type(memory_type_bits, access_flags);
	if (!mem)
		return invalid_allocation;

	return sub_allocate(*mem, size);
}

void allocator::free(const sub_allocation& allocation) {
	if(allocation.memory_type_index != invalid_allocation.memory_type_index)
		free(m_allocated_memory_types[allocation.memory_type_index], allocation);
}

void allocator::destroy() {
#ifdef DEBUG
	print_memory_leaks();
#endif //DEBUG
	for (uint32_t i = 0; i < m_memory_type_count; i++) {
		memory& mem_type = m_allocated_memory_types[i];
		if(mem_type.handle != VK_NULL_HANDLE) {
			vkFreeMemory(m_device, mem_type.handle, NULL);
		}
	}
}



void allocator::print_memory_leaks() {
	for (uint32_t i = 0; i < m_memory_type_count; i++) {
		memory& mem_type = m_allocated_memory_types[i];
		if (mem_type.handle != VK_NULL_HANDLE) {
			for (const memory_block& block : mem_type.blocks) {
				fprintf(stdout, "leaked memory of memory type %d: buffer[%p, %p) with size% d\n", mem_type.memory_type_index, (void*)block.start(), (void*)block.end(), (int) block.size);
			}
		}
	}
}