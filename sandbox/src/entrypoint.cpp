#include <stdlib.h>
#include <engine/engine.h>
#include <stdio.h>
#include <assert.h>




int main(const int argc, const char** argv) {
	

	window window{"Title", 1020, 720};

	if (!render_api::init(window)) {
		window.destroy();
		return -1;
	}

	render_pass_builder render_pass_builder;
	uint32_t output_location = render_pass_builder.add_attachment(render_pass_builder::attachment_description{ render_pass_builder::attachment_type::PRESENT_ATTACHMENT });
	render_pass_builder.begin_subpass();
	render_pass_builder.write_color_attachment(output_location);
	render_pass_builder.end_subpass();
	VkRenderPass render_pass = render_pass_builder.build();

	pipeline_builder pipeline_builder{render_pass};
	pipeline_builder.buffer_layout_push_floats(3);
	VkShaderModule vertex = shader::load_module_from_file("res/vertex.spv");
	VkShaderModule fragment = shader::load_module_from_file("res/fragment.spv");
	pipeline_builder.set_vertex_shader(vertex);
	pipeline_builder.set_fragment_shader(fragment);
	VkPipeline pipeline = pipeline_builder.build();
	
	vkDestroyShaderModule(render_api::get_device(), vertex, NULL);
	vkDestroyShaderModule(render_api::get_device(), fragment, NULL);

	framebuffer framebuffers[2];
	for (int i = 0; i < sizeof(framebuffers) / sizeof(framebuffers[0]); i++) {
		if (!framebuffers[i].add_color_attachment(render_api::get_swapchain_image(i), render_api::get_surface_format().format))
			return -1;
		if (!framebuffers[i].create(render_pass, window.get_width(), window.get_height()))
			return -1;
	}



	while (!window.is_closed_requsted()) {
		window.wait_events();
	}



	for(int i = 0; i < sizeof(framebuffers) / sizeof(framebuffers[0]); i++)
		framebuffers[i].destroy();
	vkDestroyPipeline(render_api::get_device(), pipeline, NULL);
	vkDestroyRenderPass(render_api::get_device(), render_pass, NULL);
	render_api::shutdown();
	window.destroy();
	return 0;
}