#ifndef ENGINE_RENDERER_SYNCHRONIZATION_H
#define ENGINE_RENDERER_SYNCHRONIZATION_H

#include <vulkan/vulkan.h>

VkSemaphore create_semaphore();

VkFence create_fence();

VkResult waitFence(VkFence fence, uint64_t timeout = 1e9);



#endif //ENGINE_RENDERER_SYNCHRONIZATION_H