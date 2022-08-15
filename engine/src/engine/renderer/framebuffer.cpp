#include "framebuffer.h"
#include "context.h"


bool framebuffer::add_color_attachment(VkImage image, VkFormat format) {
	VkImageViewCreateInfo view_create_info = { };
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.pNext = NULL;
	view_create_info.flags = 0;
	view_create_info.image = image;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = format;
	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;

	VkImageView view;
	if (vkCreateImageView(context::get_device(), &view_create_info, NULL, &view) != VK_SUCCESS)
		return false;
	m_attachments.push_back(view);
	return true;
}

bool framebuffer::create(VkRenderPass renderpass, uint32_t width, uint32_t height) {
	m_width = width;
	m_height = height;

	VkFramebufferCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.renderPass = renderpass;
	create_info.attachmentCount = (uint32_t)m_attachments.size();
	create_info.pAttachments = m_attachments.data();
	create_info.width = m_width;
	create_info.height = m_height;
	create_info.layers = 1;

	return VK_SUCCESS == vkCreateFramebuffer(context::get_device(), &create_info, NULL, &m_handle);
}


void framebuffer::destroy() {
	for (VkImageView view : m_attachments)
		vkDestroyImageView(context::get_device(), view, NULL);
	vkDestroyFramebuffer(context::get_device(), m_handle, NULL);
}