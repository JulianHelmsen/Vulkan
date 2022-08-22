#ifndef ENGINE_RENDERER_MEMORY_H
#define ENGINE_RENDERER_MEMORY_H

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

#define ALIGNMENT (256)
VkDeviceSize align(VkDeviceSize size);

class allocator {
public:

	allocator() : m_initialized(false) { }

	enum access_flags {
		STATIC = 1,
		DYNAMIC = 2
	};


	// Contains all the data needed to bind to a buffer
	struct sub_allocation {
		sub_allocation() : memory_type_index(0xFFFFFFFF), handle(VK_NULL_HANDLE), start_address(0) {}
		sub_allocation(VkDeviceMemory memory, uint32_t memory_type, VkDeviceAddress address) 
				: memory_type_index(memory_type), handle(memory), start_address(address){}
		uint32_t memory_type_index;
		VkDeviceMemory handle;
		VkDeviceAddress start_address;
		operator bool() const { return memory_type_index != 0xFFFFFFFF; };
	};
	static sub_allocation invalid_allocation;

	// allocates memory from a large memory buffer
	sub_allocation allocate(size_t size, uint32_t memory_type_bits, access_flags access_flags);

	// deallocates memory 
	void free(const sub_allocation& allocation);

	void destroy(); // cleans up all the resources asociated with this allocator


	void initialize(VkPhysicalDevice physicalDevice, VkDevice device);

private:
	VkDevice m_device;
	VkDeviceSize m_default_allocation_size = 1 << 28;
	bool m_initialized;

	struct memory_block {
		VkDeviceAddress address;
		VkDeviceSize size;

		inline VkDeviceAddress start() const { return address; }
		inline VkDeviceAddress end() const { return address + size; }
	};

	struct memory {
		uint32_t memory_type_index;
		VkMemoryType memory_type_info;
		VkMemoryHeap heap_type_info;

		VkDeviceMemory handle;
		size_t size;

		std::vector<memory_block> blocks;
	};

	sub_allocation sub_allocate(memory& memory, size_t size);
	void free(memory& memory, const sub_allocation& allocation);
	void free(memory& memory);
	memory* find_memory_type(uint32_t memory_type_bits, access_flags access_flags);
	static bool matches_type(const memory& memory, uint32_t memory_type_bits, access_flags flags);
	bool initial_allocation(memory& memory);
	
	uint32_t m_memory_type_count;
	std::array<memory, VK_MAX_MEMORY_TYPES> m_allocated_memory_types;
	

	bool find_space(memory_block* space, size_t* index, const memory& memory, size_t size);

	void print_memory_leaks();

	
};

class memory {
public:

	static bool memcpy_host_to_device(const allocator::sub_allocation& memory, const void* data, size_t size);

};

#endif //ENGINE_RENDERER_MEMORY_H