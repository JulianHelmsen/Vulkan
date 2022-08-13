#ifndef ENGINE_RENDERER_SHADER_H
#define ENGINE_RENDERER_SHADER_H

#include <vulkan/vulkan.h>

class shader {
public:
	static VkShaderModule load_module_from_file(const char* filepath);
	static VkShaderModule load_module(const void* data, uint32_t size);

};

#endif //ENGINE_RENDERER_SHADER_H