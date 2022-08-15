#include "engine/renderer/context.h"
#include <Windows.h>
#include "engine/renderer/render_api.h"
#include <vulkan/vulkan_win32.h>


bool context::create_surface(window_handle_t handle) {
	VkWin32SurfaceCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.pNext = NULL;
	info.flags = 0;
	info.hinstance = GetModuleHandle(NULL);
	info.hwnd = (HWND)handle;
	return vkCreateWin32SurfaceKHR(render_api::get_instance(), &info, NULL, &m_surface) == VK_SUCCESS;
}