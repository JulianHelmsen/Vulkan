#include "shader.h"
#include "render_api.h"
#include <stdio.h>

VkShaderModule shader::load_module_from_file(const char* filepath) {

#ifdef PLATFORM_WINDOWS
	FILE* file = NULL;
	fopen_s(&file, filepath, "rb");
#else
	FILE* file = fopen(filepath, "rb");
#endif
	if (!file)
		return VK_NULL_HANDLE;
	fseek(file, 0L, SEEK_END);
	uint32_t size = (uint32_t) ftell(file);
	
	rewind(file);
	unsigned char* data = new unsigned char[size];
	const void* read = data;
	size_t n_bytes = fread(data, 1, size, file);
	
	fclose(file);

	VkShaderModule shader_module = load_module(read, size);
	// free memory
	delete[] data;

	return shader_module;
}

VkShaderModule shader::load_module(const void* data, uint32_t size) {
	if (size % 4 != 0)
		return VK_NULL_HANDLE;
	VkShaderModuleCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.codeSize = size;
	create_info.pCode = (uint32_t*)data;

	VkShaderModule shader_module;
	if(vkCreateShaderModule(render_api::get_device(), &create_info, NULL, &shader_module) == VK_SUCCESS)
		return shader_module;
	return VK_NULL_HANDLE;
}