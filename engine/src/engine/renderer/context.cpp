#include "context.h"
#include "render_api.h"
#include <vector>



context* context::s_current = NULL;


context* context::create_context(window_handle_t handle) {
	context* ctx = new context;
	if (!ctx->init(handle)) {
		delete ctx;
		return NULL;
	}
	return ctx;
}

bool context::init(window_handle_t handle) {
	if (!create_surface(handle))
		return false;

	std::vector<const char*> required_extensions = { "VK_KHR_swapchain" };
	if (!select_physical_device(required_extensions))
		return false;
	if (!create_logical_device(required_extensions))
		return false;
	if (!create_swapchain())
		return false;
	if (!create_command_pool())
		return false;

	return true;
}

static bool physical_device_supports(VkPhysicalDevice device, const std::vector<const char*>& required_extensions) {
	uint32_t count;
	if (vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL) != VK_SUCCESS)
		return false;
	VkExtensionProperties* props = new VkExtensionProperties[count];
	if (vkEnumerateDeviceExtensionProperties(device, NULL, &count, props) != VK_SUCCESS) {
		delete[] props;
		return false;
	}

	for (const char* required : required_extensions) {
		bool found = false;
		for (uint32_t i = 0; i < count; i++) {
			if (strcmp(required, props[i].extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			delete[] props;
			return false;
		}
	}
	delete[] props;
	return true;
}

bool context::select_physical_device(const std::vector<const char*>& extensions) {
	uint32_t physical_device_count;
	if (vkEnumeratePhysicalDevices(render_api::get_instance(), &physical_device_count, NULL) != VK_SUCCESS)
		return false;
	VkPhysicalDevice* devices = new VkPhysicalDevice[physical_device_count];
	if (vkEnumeratePhysicalDevices(render_api::get_instance(), &physical_device_count, devices) != VK_SUCCESS) {
		delete[] devices;
		return false;
	}
	m_queue_family_indices = { -1, -1, false };
	
	VkPhysicalDevice pick = VK_NULL_HANDLE;
	for (uint32_t i = 0; i < physical_device_count; i++) {
		if (!physical_device_supports(devices[i], extensions))
			continue;
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);
		uint32_t family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, NULL);
		VkQueueFamilyProperties* queue_families = new VkQueueFamilyProperties[family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, queue_families);

		queue_family_indices current = { -1, -1, false };
		bool dedicated_transfer = false;
		bool can_present = false;
		for (uint32_t f = 0; f < family_count; f++) {
			VkQueueFlags flags = queue_families[f].queueFlags;
			if (flags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 supports_surface;
				vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], f, m_surface.surface, &supports_surface);
				if (supports_surface)
					current.graphics = f;
			}
			if (flags == VK_QUEUE_TRANSFER_BIT) {
				current.dedicated_transfer = true;
				current.transfer = f;
			}
			else if (flags & VK_QUEUE_TRANSFER_BIT && !current.dedicated_transfer) {
				current.transfer = f;
			}
		}


		if (current.graphics != -1 && current.transfer != -1) {
			if (m_queue_family_indices.graphics == -1 || m_queue_family_indices.dedicated_transfer || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				m_queue_family_indices = current;
				pick = devices[i];
			}
		}



		delete[] queue_families;
	}

	m_physical_device = pick;
	delete[] devices;
	return m_physical_device != VK_NULL_HANDLE;
}

bool context::create_logical_device(const std::vector<const char*>& extensions) {
	float priorities = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfos[2] = { };
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = NULL;
	queueCreateInfos[1].pNext = NULL;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[1].flags = 0;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &priorities;
	queueCreateInfos[1].pQueuePriorities = &priorities;

	queueCreateInfos[0].queueFamilyIndex = m_queue_family_indices.graphics;
	queueCreateInfos[1].queueFamilyIndex = m_queue_family_indices.transfer;



	uint32_t queue_create_info_count;
	if (m_queue_family_indices.graphics == m_queue_family_indices.transfer) {
		queue_create_info_count = 1;
	}else {
		queue_create_info_count = 2;
	}
	VkDeviceCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.enabledExtensionCount = (uint32_t)extensions.size();
	create_info.ppEnabledExtensionNames = extensions.size() > 0 ? &extensions[0] : NULL;
	create_info.pEnabledFeatures = NULL;
	create_info.pQueueCreateInfos = queueCreateInfos;
	create_info.queueCreateInfoCount = queue_create_info_count;



	if (vkCreateDevice(m_physical_device, &create_info, NULL, &m_device) != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(m_device, m_queue_family_indices.graphics, 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, m_queue_family_indices.transfer, 0, &m_transfer_queue);

	return true;
}



static VkSurfaceFormatKHR select_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, NULL);
	VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats);

	VkSurfaceFormatKHR selected = formats[0];
	VkSurfaceFormatKHR preferred = { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (uint32_t i = 0; i < count; i++) {
		if (formats[i].colorSpace == preferred.colorSpace && formats[i].format == preferred.format) {
			selected = preferred;
			break;
		}
	}

	delete[] formats;
	return selected;
}


static VkPresentModeKHR select_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t present_mode_count;
	// VK_PRESENT_MODE_FIFO_KHR is guaranteed to be supported
	VkPresentModeKHR selected = VK_PRESENT_MODE_FIFO_KHR;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL) != VK_SUCCESS)
		return selected;
	VkPresentModeKHR* present_modes = new VkPresentModeKHR[present_mode_count];
	if (!vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes)) {
		delete[] present_modes;
		return selected;
	}

	for (uint32_t i = 0; i < present_mode_count; i++) {
		// prefer MAILBOX over FIFO
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			selected = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	delete[] present_modes;
	return selected;
}

bool context::create_swapchain() {
	VkSurfaceCapabilitiesKHR capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface.surface, &capabilities) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	if (!(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
		return VK_NULL_HANDLE;

	m_surface.surface_format = select_surface_format(m_physical_device, m_surface.surface);

	VkSwapchainCreateInfoKHR swapchain_create_info = { };
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext = NULL;
	swapchain_create_info.flags = 0;
	swapchain_create_info.surface = m_surface.surface;
	swapchain_create_info.minImageCount = capabilities.minImageCount;
	swapchain_create_info.presentMode = select_present_mode(m_physical_device, m_surface.surface);
	swapchain_create_info.imageFormat = m_surface.surface_format.format;
	swapchain_create_info.imageColorSpace = m_surface.surface_format.colorSpace;
	swapchain_create_info.imageExtent = capabilities.currentExtent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 1;
	swapchain_create_info.pQueueFamilyIndices = (const uint32_t*)&m_queue_family_indices.graphics;
	swapchain_create_info.preTransform = transform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = m_swapchain.swapchain;

	m_swapchain.extent = capabilities.currentExtent;

	printf("capabilities::currentExtent: %dx%d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);

	if (vkCreateSwapchainKHR(m_device, &swapchain_create_info, NULL, &m_swapchain.swapchain) != VK_SUCCESS)
		return false;

	if (vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &m_swapchain.image_count, NULL) != VK_SUCCESS) {
		vkDestroySwapchainKHR(m_device, m_swapchain.swapchain, NULL);
		return VK_NULL_HANDLE;
	}
	m_swapchain.images = new VkImage[m_swapchain.image_count];

	if (vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &m_swapchain.image_count, m_swapchain.images) != VK_SUCCESS) {
		vkDestroySwapchainKHR(m_device, m_swapchain.swapchain, NULL);
		delete[] m_swapchain.images;
		return false;
	}

	return true;
}


bool context::create_command_pool() {

	VkCommandPoolCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = m_queue_family_indices.graphics;

	return vkCreateCommandPool(m_device, &create_info, NULL, &m_command_pool) == VK_SUCCESS;
}

void context::make_context_current() {
	s_current = this;
}

context::~context() {
	if (this == s_current)
		s_current = NULL;
	
	if (m_swapchain.swapchain)
		vkDestroySwapchainKHR(m_device, m_swapchain.swapchain, NULL);
	if (m_command_pool != VK_NULL_HANDLE)
		vkDestroyCommandPool(m_device, m_command_pool, NULL);

	if (m_surface.surface)
		vkDestroySurfaceKHR(render_api::get_instance(), m_surface.surface, NULL);

	if (m_device)
		vkDestroyDevice(m_device, NULL);

		
}
