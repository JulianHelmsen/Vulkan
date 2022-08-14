#include "synchronization.h"
#include "render_api.h"
#include <assert.h>



VkSemaphore create_semaphore() {
	VkSemaphoreCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.flags = 0;
	create_info.pNext = NULL;


	VkSemaphore semaphore = VK_NULL_HANDLE;
	VkResult res = vkCreateSemaphore(render_api::get_device(), &create_info, NULL, &semaphore);
	assert(res == VK_SUCCESS);
	return semaphore;
}

VkFence create_fence() {
	VkFenceCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;

	VkFence fence = VK_NULL_HANDLE;

	VkResult res = vkCreateFence(render_api::get_device(), &create_info, NULL, &fence);
	assert(res == VK_SUCCESS);
	return fence;
}


VkResult waitFence(VkFence fence, uint64_t timeout) {
	return vkWaitForFences(render_api::get_device(), 1, &fence, VK_TRUE, timeout);
}