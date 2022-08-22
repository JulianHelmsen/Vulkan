#ifndef ENGINE_RENDERER_COMMAND_BUFFER_H
#define ENGINE_RENDERER_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

class command_buffer {
public:

	command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	
	bool start();
	bool end();

	void destroy();

	inline const VkCommandBuffer& get_handle() const { return m_handle; }
	inline VkCommandBuffer& get_handle() { return m_handle; }

	void submit(VkQueue queue) { submit(queue, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT); }
	void submit(VkQueue queue, VkSemaphore signal) { submit(queue, VK_NULL_HANDLE, signal, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT); }
	void submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal) { submit(queue, wait_semaphore, signal, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT); }
	void submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal, VkPipelineStageFlags wait_stage);
private:
	VkCommandBuffer m_handle;
};

#endif //ENGINE_RENDERER_COMMAND_BUFFER_H