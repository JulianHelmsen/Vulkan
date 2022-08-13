#ifndef ENGINE_RENDERER_PIPELINE_H
#define ENGINE_RENDERER_PIPELINE_H


#include <vulkan/vulkan.h>
#include <vector>

class pipeline_builder {
public:
	pipeline_builder(VkRenderPass render_pass) 
		: m_render_pass(render_pass), m_buffer_layout_stride(0), m_culling_enabled(VK_FALSE), m_depth_test(VK_FALSE), m_stencil_test(VK_FALSE), m_blending(VK_FALSE) {
	}

	VkPipeline build();

	void buffer_layout_push_floats(uint32_t count);

	void set_viewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);

	void set_vertex_shader(VkShaderModule vertex_module);
	void set_fragment_shader(VkShaderModule fragment_module);
	void set_geometry_shader(VkShaderModule geometry_module);

	void set_culling(bool enabled) { m_culling_enabled = enabled; }
	void set_depth_test(bool enabled) { m_depth_test = enabled; }
	void set_stencil_test(bool enabled) { m_stencil_test = enabled; }
	void set_blending(bool enabled) { m_blending = enabled; }

	void set_sample_count(int samples) { m_samples = samples; }



private:
	std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;

	VkRenderPass m_render_pass;
	VkViewport m_viewport;
	int m_samples;
	

	enum class data_type {
		FLOAT, INT, UINT
	};

	struct buffer_layout_element {
		uint32_t offset;
		data_type type;
		uint32_t count;
		VkVertexInputRate input_rate;
	};

	VkBool32 m_culling_enabled;
	VkBool32 m_depth_test;
	VkBool32 m_stencil_test;
	VkBool32 m_blending;
	
	void init_vertex_input_state_create_info(VkVertexInputBindingDescription* bindings, VkVertexInputAttributeDescription* attributes);
	std::vector<buffer_layout_element> m_buffer_layout;
	uint32_t m_buffer_layout_stride;
	static VkFormat convert_to_vk_format(data_type type, uint32_t count);
};


#endif //ENGINE_RENDERER_PIPELINE_H