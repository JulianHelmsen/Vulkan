
workspace "Vulkan rendering"
	configurations { "Debug", "Release", "Distribution"}
	architecture "x86_64"
	platforms {"WINDOWS"}




project "sandbox"
	kind "ConsoleApp"
	language "C++"
	location "sandbox"
	targetdir "bin/%{cfg.buildcfg}"
	cppdialect "C++17"

	files {
		"sandbox/src/**.cpp",
		"sandbox/src/**.h",
		"sandbox/src/**.hpp"
	}
	
	includedirs {
		"engine/src",
		"sandbox/src",
		"$(VULKAN_SDK)/include"
	}

	links {
		"engine"
	}
	


	filter "configurations:Debug"
		defines {"DEBUG"}
		symbols "On"

	filter "configurations:Release"
		defines {"RELEASE", "NDEBUG" }
		optimize "On"
		staticruntime "Off"
		
	filter "configurations:Distribution"
		defines {"DISTRIBUTION", "NDEBUG" }
		optimize "On"
		staticruntime "Off"


project "engine"
	kind "StaticLib"
	language "C++"
	location "engine"
	targetdir "bin/%{cfg.buildcfg}"
	cppdialect "C++17"


	files {
		"engine/src/**.cpp",
		"engine/src/**.h",
		"engine/src/**.hpp"
	}

	includedirs {
		"engine/src",
		"$(VULKAN_SDK)/include"
	}
	libdirs {
		"$(VULKAN_SDK)/Lib"
	}

	links {
		"vulkan-1.lib",
		"VkLayer_utils.lib"
	}

	defines {"BUILD_ENGINE"}

	filter "platforms:WINDOWS"
		defines {"PLATFORM_WINDOWS"}
		files {"engine/src/platform/windows"}

	filter "configurations:Debug"
		defines {"DEBUG"}
		symbols "On"

	filter "configurations:Release"
		defines {"RELEASE", "NDEBUG" }
		optimize "On"
		staticruntime "Off"
		
	filter "configurations:Distribution"
		defines {"DISTRIBUTION", "NDEBUG" }
		optimize "On"
		staticruntime "Off"

