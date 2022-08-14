#ifndef ENGINE_RENDERER_SYNCHRONIZATION_H
#define ENGINE_RENDERER_SYNCHRONIZATION_H

#include <vulkan/vulkan.h>

VkSemaphore create_semaphore();

VkFence create_fence();


#endif //ENGINE_RENDERER_SYNCHRONIZATION_H