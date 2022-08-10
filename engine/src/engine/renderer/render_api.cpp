#include "render_api.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <stdio.h>


static bool createVkInstance();
static bool select_physical_device(const std::vector<const char*>& extensions);
static bool create_logical_device(const std::vector<const char*>& extensions);
static bool create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);

struct queue_family_indices {
	int graphics;
	int transfer;
	bool dedicated_transfer = false;
};

static struct {
	queue_family_indices queue_families;
	VkPhysicalDevice physical_device;
	VkSwapchainKHR swapchain;
	VkSurfaceKHR surface;
	VkInstance instance;
	VkDevice device;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkQueue transfer_queue;
	VkQueue graphics_queue;
}s_data = { 0 };

VkBool32 validation_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT                  msg_type,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* pUserData) {
	printf("validation: %s\n", data->pMessage);
	__debugbreak();

	return VK_FALSE;
}


bool render_api::init(window& window) {
	if (!createVkInstance())
		return false;

	if (!window.create_surface(s_data.instance))
		return false;
	s_data.surface = window.get_surface();

	std::vector<const char*> required_extensions = { "VK_KHR_swapchain" };
	if (!select_physical_device(required_extensions))
		return false;
	if (!create_logical_device(required_extensions))
		return false;

	if (!create_swapchain())
		return false;
	return true;
}



void render_api::shutdown() {

	vkDestroySwapchainKHR(s_data.device, s_data.swapchain, NULL);
	vkDestroySurfaceKHR(s_data.instance, s_data.surface, NULL);
	vkDestroyDevice(s_data.device, NULL);
#ifndef DISTRIBUTION
	// delete debug validation layers
	if (s_data.debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_data.instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT) {
			vkDestroyDebugUtilsMessengerEXT(s_data.instance, s_data.debug_messenger, NULL);
		}
	}

#endif
	vkDestroyInstance(s_data.instance, NULL);
}

static VkPresentModeKHR select_present_mode(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t present_mode_count;
	// VK_PRESENT_MODE_FIFO_KHR is guaranteed to be supported
	VkPresentModeKHR selected = VK_PRESENT_MODE_FIFO_KHR;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(s_data.physical_device, s_data.surface, &present_mode_count, NULL) != VK_SUCCESS)
		return selected;
	VkPresentModeKHR* present_modes = new VkPresentModeKHR[present_mode_count];
	if (!vkGetPhysicalDeviceSurfacePresentModesKHR(s_data.physical_device, s_data.surface, &present_mode_count, present_modes)) {
		delete[] present_modes;
		return selected;
	}

	for (int i = 0; i < present_mode_count; i++) {
		// prefer MAILBOX over FIFO
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			selected = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	delete[] present_modes;
	return selected;
}

static VkSurfaceFormatKHR select_surface_format(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, NULL);
	VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats);

	VkSurfaceFormatKHR selected = formats[0];
	VkSurfaceFormatKHR preferred = { VK_FORMAT_B8G8R8A8_SRGB , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	
	for (uint32_t i = 0; i < count; i++) {
		if (formats[i].colorSpace == preferred.colorSpace && formats[i].format == preferred.format) {
			selected = preferred;
			break;
		}
	}

	delete[] formats;
	return selected;
}

bool create_swapchain(VkSwapchainKHR old_swapchain) {
	
	VkSurfaceCapabilitiesKHR capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_data.physical_device, s_data.surface, &capabilities) != VK_SUCCESS)
		return false;
	VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	if (!(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
		return false;

	VkSurfaceFormatKHR surface_format = select_surface_format(s_data.physical_device, s_data.surface);

	VkSwapchainCreateInfoKHR swapchain_create_info = { };
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext = NULL;
	swapchain_create_info.flags = 0;
	swapchain_create_info.surface = s_data.surface;
	swapchain_create_info.minImageCount = capabilities.minImageCount;
	swapchain_create_info.presentMode = select_present_mode(s_data.physical_device, s_data.surface);
	swapchain_create_info.imageFormat = surface_format.format;
	swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	swapchain_create_info.imageExtent = capabilities.currentExtent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 1;
	swapchain_create_info.pQueueFamilyIndices = (const uint32_t*) &s_data.queue_families.graphics;
	swapchain_create_info.preTransform = transform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.clipped = VK_TRUE;

	
	

	return vkCreateSwapchainKHR(s_data.device, &swapchain_create_info, NULL, &s_data.swapchain) == VK_SUCCESS;
}

bool create_logical_device(const std::vector<const char*>& extensions) {
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
	
	queueCreateInfos[0].queueFamilyIndex = s_data.queue_families.graphics;
	queueCreateInfos[1].queueFamilyIndex = s_data.queue_families.transfer;



	uint32_t queue_create_info_count;
	if (s_data.queue_families.graphics == s_data.queue_families.transfer) {
		queue_create_info_count = 1;
	}else {
		queue_create_info_count = 2;
	}
	VkDeviceCreateInfo create_info = { };
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.enabledExtensionCount = (uint32_t) extensions.size();
	create_info.ppEnabledExtensionNames = extensions.size() > 0 ? &extensions[0] : NULL;
	create_info.pEnabledFeatures = NULL;
	create_info.pQueueCreateInfos = queueCreateInfos;
	create_info.queueCreateInfoCount = queue_create_info_count;
	


	if (vkCreateDevice(s_data.physical_device, &create_info, NULL, &s_data.device) != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(s_data.device, s_data.queue_families.graphics, 0, &s_data.graphics_queue);
	vkGetDeviceQueue(s_data.device, s_data.queue_families.transfer, 0, &s_data.transfer_queue);
	
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

bool select_physical_device(const std::vector<const char*>& extensions) {
	uint32_t physical_device_count;
	if (vkEnumeratePhysicalDevices(s_data.instance, &physical_device_count, NULL) != VK_SUCCESS)
		return false;
	VkPhysicalDevice* devices = new VkPhysicalDevice[physical_device_count];
	if (vkEnumeratePhysicalDevices(s_data.instance, &physical_device_count, devices) != VK_SUCCESS) {
		delete[] devices;
		return false;
	}

	queue_family_indices best = { -1, -1 , false};
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
				vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], f, s_data.surface, &supports_surface);
				if(supports_surface)
					current.graphics = f;
			}
			if (flags == VK_QUEUE_TRANSFER_BIT) {
				current.dedicated_transfer = true;
				current.transfer = f;
			}else if (flags & VK_QUEUE_TRANSFER_BIT && !current.dedicated_transfer) {
				current.transfer = f;
			}
		}


		if (current.graphics != -1 && current.transfer != -1) {
			if (best.graphics == -1 || best.dedicated_transfer || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				best = current;
				pick = devices[i];
			}
		}
		


		delete[] queue_families;
	}

	s_data.physical_device = pick;
	delete[] devices;
	return s_data.physical_device != VK_NULL_HANDLE;
}


static bool check_required_layers(const std::vector<const char*>& layers) {
	uint32_t count;
	if (vkEnumerateInstanceLayerProperties(&count, NULL) != VK_SUCCESS)
		return false;
	VkLayerProperties* props = new VkLayerProperties[count];

	if (vkEnumerateInstanceLayerProperties(&count, props) != VK_SUCCESS) {
		delete[] props;
		return false;
	}

	for (const char* required_layer : layers) {
		bool found = false;
		for (uint32_t i = 0; i < count; i++) {
			if (strcmp(required_layer, props[i].layerName) == 0) {
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


static bool check_required_extensions(const std::vector<const char*> required_extensions) {
	uint32_t count;
	if (vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS)
		return false;
	VkExtensionProperties* props = new VkExtensionProperties[count];

	if (vkEnumerateInstanceExtensionProperties(NULL, &count, props) != VK_SUCCESS) {
		delete[] props;
		return false;
	}

	for (const char* required_extension : required_extensions) {
		bool found = false;
		for (uint32_t i = 0; i < count; i++) {
			if (strcmp(required_extension, props[i].extensionName) == 0) {
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



bool createVkInstance() {
	std::vector<const char*> required_layers = {
#ifndef DISTRIBUTION
		"VK_LAYER_KHRONOS_validation"
#endif
	};
	std::vector<const char*> required_extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef PLATFORM_WINDOWS
		"VK_KHR_win32_surface",
#endif
#ifndef DISTRIBUTION
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif 
	};

	if (!check_required_extensions(required_extensions))
		return false;
	if (!check_required_layers(required_layers))
		return false;

	VkApplicationInfo app_info = { };
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pEngineName = "Engine";
	app_info.pApplicationName = "EngineApp";
	app_info.apiVersion = VK_MAKE_VERSION(1, 3, 0);
	app_info.applicationVersion = VK_MAKE_VERSION(1, 1, 1);
	app_info.engineVersion = VK_MAKE_VERSION(1, 1, 1);

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.enabledExtensionCount = (uint32_t) required_extensions.size();
	instance_create_info.enabledLayerCount = (uint32_t) required_layers.size();
	instance_create_info.ppEnabledExtensionNames = required_extensions.size() > 0 ?  &required_extensions[0] : NULL;
	instance_create_info.ppEnabledLayerNames = required_layers.size() > 0 ? &required_layers[0] : NULL;
	instance_create_info.flags = 0;
	instance_create_info.pApplicationInfo = &app_info;

	if (vkCreateInstance(&instance_create_info, NULL, &s_data.instance) != VK_SUCCESS) {
		return false;
	}

	// set up validation layers
#ifndef DISTRIBUTION
	VkDebugUtilsMessengerCreateInfoEXT debug_utils_msg_create_info = {};
	debug_utils_msg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_utils_msg_create_info.pNext = NULL;
	debug_utils_msg_create_info.flags = 0;
	debug_utils_msg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_utils_msg_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_utils_msg_create_info.pUserData = NULL;
	debug_utils_msg_create_info.pfnUserCallback = validation_callback;

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_data.instance, "vkCreateDebugUtilsMessengerEXT");

	if (vkCreateDebugUtilsMessengerEXT && vkCreateDebugUtilsMessengerEXT(s_data.instance, &debug_utils_msg_create_info, NULL, &s_data.debug_messenger) != VK_SUCCESS) {
		vkDestroyInstance(s_data.instance, NULL);
		return false;
	}
#endif

	return true;
}


const VkInstance& render_api::get_instance() {
	return s_data.instance;
}