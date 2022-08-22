#include "command_buffer.h"
#include "context.h"


command_buffer::command_buffer(VkCommandBufferLevel level) : m_handle(VK_NULL_HANDLE) {
	VkCommandBufferAllocateInfo allocate_info = { };
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.commandBufferCount = 1;
	allocate_info.commandPool = context::get_command_pool();
	allocate_info.level = level;

	vkAllocateCommandBuffers(context::get_device(), &allocate_info, &m_handle);
}

bool command_buffer::start() {
	VkCommandBufferBeginInfo info = { };
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.pInheritanceInfo = NULL;
	return vkBeginCommandBuffer(m_handle, &info) == VK_SUCCESS;
}
bool command_buffer::end() {
	return vkEndCommandBuffer(m_handle) == VK_SUCCESS;
}

void command_buffer::destroy() {
	vkFreeCommandBuffers(context::get_device(), context::get_command_pool(), 1, &m_handle);
}

void command_buffer::submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal, VkPipelineStageFlags wait_stage) {
	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = wait_semaphore != VK_NULL_HANDLE;
	submit_info.pWaitSemaphores = &wait_semaphore;
	submit_info.pWaitDstStageMask = &wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_handle;
	submit_info.signalSemaphoreCount = signal != VK_NULL_HANDLE;
	submit_info.pSignalSemaphores = &signal;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}
