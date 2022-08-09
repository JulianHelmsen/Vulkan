#include "render_api.h"
#include <vulkan/vulkan.h>
#include <vector>


static bool createVkInstance();
static bool select_physical_device();

static struct {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
}s_data = { 0 };

VkBool32 validation_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT                  msg_type,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* pUserData) {
	printf("validation: %s\n", data->pMessage);
	__debugbreak();

	return VK_FALSE;
}


bool render_api::init() {
	if (!createVkInstance())
		return false;
	if (!select_physical_device())
		return false;
}


bool select_physical_device() {

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
#ifndef DISTRIBUTION
		"VK_EXT_debug_utils"
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
	debug_utils_msg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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


void render_api::shutdown() {
#ifndef DISTRIBUTION
	// delete debug validation layers
	if(s_data.debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(s_data.instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT) {
			vkDestroyDebugUtilsMessengerEXT(s_data.instance, s_data.debug_messenger, NULL);
		}
	}

#endif
	vkDestroyInstance(s_data.instance, NULL);
}

const VkInstance& render_api::get_instance() {
	return s_data.instance;
}