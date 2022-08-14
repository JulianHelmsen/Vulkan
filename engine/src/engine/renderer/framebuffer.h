#ifndef ENGINE_RENDERER_FRAMEBUFFER_H
#define ENGINE_RENDERER_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

class framebuffer {
public:
	framebuffer() : m_width(0), m_height(0) {}
	bool add_color_attachment(VkImage image, VkFormat format);
	bool create(VkRenderPass renderpass, uint32_t width, uint32_t height);


	inline VkFramebuffer get_handle() const { return m_handle; }
	inline uint32_t get_width() const { return m_width; }
	inline uint32_t get_height() const { return m_height; }

	void destroy();
private:
	VkFramebuffer m_handle;
	uint32_t m_width;
	uint32_t m_height;
	std::vector<VkImageView> m_attachments;
};

#endif //ENGINE_RENDERER_FRAMEBUFFER_H