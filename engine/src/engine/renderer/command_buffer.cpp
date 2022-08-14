#include "command_buffer.h"
#include "render_api.h"


command_buffer::command_buffer(VkCommandBufferLevel level) : m_handle(VK_NULL_HANDLE) {
	VkCommandBufferAllocateInfo allocate_info = { };
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.commandBufferCount = 1;
	allocate_info.commandPool = render_api::get_command_pool();
	allocate_info.level = level;

	vkAllocateCommandBuffers(render_api::get_device(), &allocate_info, &m_handle);
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
	vkFreeCommandBuffers(render_api::get_device(), render_api::get_command_pool(), 1, &m_handle);
}