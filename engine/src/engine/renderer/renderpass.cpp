#include "renderpass.h"
#include "render_api.h"
#include <assert.h>

static void make_present_attachment(VkAttachmentDescription& descr, const render_pass_builder::attachment_description& attachment_descr) {
	descr.flags = 0;
	descr.format = render_api::get_surface_format().format;
	descr.samples = (VkSampleCountFlagBits) attachment_descr.samples; // maps perfectly to integer values
	descr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	descr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	descr.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	descr.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	descr.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

/*
* TODO: This is slow because it causes a lot of memory allocations, deletions and copies.
* It's not that bad because the renderpass builder should not be used frequently.
*/
static VkAttachmentReference* append_reference(const VkAttachmentReference* references, uint32_t count, VkAttachmentReference ref) {
	VkAttachmentReference* new_references = new VkAttachmentReference[count + 1];
	if(references) {
		memcpy(new_references, references, sizeof(VkAttachmentReference) * count);
		delete[] references;
	}
	new_references[count] = ref;
	return new_references;
}

uint32_t render_pass_builder::add_attachment(const attachment_description& attachment_descr) {
	uint32_t attachment_location = (uint32_t) m_attachments.size();
	VkAttachmentDescription& descr = m_attachments.emplace_back();
	
	assert(attachment_descr.samples == 1 && "Multi sampling currently not supported");

	if (attachment_descr.type == attachment_type::PRESENT_ATTACHMENT)
		make_present_attachment(descr, attachment_descr);
	else
		assert(0 && "currently unsupported attachment type");
	return attachment_location;
}

VkRenderPass render_pass_builder::build() {


	VkAttachmentReference color_attachment_ref;
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;



	VkRenderPassCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.attachmentCount = (uint32_t) m_attachments.size();
	create_info.pAttachments = m_attachments.data();
	create_info.subpassCount = (uint32_t)m_subpasses.size();
	create_info.pSubpasses = m_subpasses.data();
	create_info.dependencyCount = 0;
	create_info.pDependencies = NULL;


	VkRenderPass render_pass;
	if(vkCreateRenderPass(render_api::get_device(), &create_info, NULL, &render_pass) == VK_SUCCESS)
		return render_pass;
	return VK_NULL_HANDLE;
}


void render_pass_builder::write_color_attachment(uint32_t location) {
	VkSubpassDescription& descr = current_subpass();
	VkAttachmentReference reference;
	
	reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	reference.attachment = location;

	descr.pColorAttachments = append_reference(descr.pColorAttachments, descr.colorAttachmentCount, reference);
	descr.colorAttachmentCount++;
}


void render_pass_builder::use_input_attachment(uint32_t location) {
	VkSubpassDescription& descr = current_subpass();
	VkAttachmentReference reference;

	// see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageLayout.html
	// images that are references by this should be created with the usage bits:
	// VK_IMAGE_USAGE_SAMPLED_BIT or VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
	reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	reference.attachment = location;

	descr.pInputAttachments = append_reference(descr.pInputAttachments, descr.inputAttachmentCount, reference);
	descr.inputAttachmentCount++;
}

void render_pass_builder::begin_subpass() {
	VkSubpassDescription descr = m_subpasses.emplace_back();
	descr.flags = 0;
	descr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	descr.inputAttachmentCount = 0;
	descr.colorAttachmentCount = 0;
	descr.preserveAttachmentCount = 0;

	descr.pInputAttachments = NULL;
	descr.pColorAttachments = NULL;
	descr.pPreserveAttachments = NULL;
	descr.pResolveAttachments = NULL;
	descr.pDepthStencilAttachment = NULL;
	
}

render_pass_builder::~render_pass_builder() {
	for (VkSubpassDescription& subpass : m_subpasses) {
		delete[] subpass.pColorAttachments;
		delete[] subpass.pResolveAttachments;
		delete[] subpass.pPreserveAttachments;
		delete subpass.pDepthStencilAttachment;
	}
}

void render_pass_builder::end_subpass() {
	// I think all unused attachments can be added to the pPreserve attachments of the subpass
	// I should look into this at some point
	generate_preserve_attachment_references();
}

void render_pass_builder::generate_preserve_attachment_references() {

}