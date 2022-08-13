#ifndef ENGINE_RENDERER_RENDERPASS_H
#define ENGINE_RENDERER_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>


class render_pass_builder {
public:
	enum class attachment_type {
		COLOR_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT, PRESENT_ATTACHMENT
	};
	~render_pass_builder();
	VkRenderPass build();

	struct attachment_description {
		attachment_description(attachment_type a_type) : samples(1), type(a_type) {}
		attachment_description(attachment_type a_type, int samples) : samples(samples), type(a_type) {}
		attachment_type type;
		int samples;
	};


	uint32_t add_attachment(const attachment_description& attachment_descr);

	
	void begin_subpass();
	
	void write_color_attachment(uint32_t location);
	void use_input_attachment(uint32_t location);

	void end_subpass();
private:
	inline VkSubpassDescription& current_subpass() { return m_subpasses.back(); }
	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<VkSubpassDescription> m_subpasses;

	void generate_preserve_attachment_references();



	
};



#endif //ENGINE_RENDERER_RENDERPASS_H