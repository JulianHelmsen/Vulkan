#ifndef ENGINE_RENDERER_COMMAND_BUFFER_H
#define ENGINE_RENDERER_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

class command_buffer {
public:

	command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	
	bool start();
	bool end();

	void destroy();

	inline VkCommandBuffer get_handle() const { return m_handle; }
private:
	VkCommandBuffer m_handle;
};

#endif //ENGINE_RENDERER_COMMAND_BUFFER_H