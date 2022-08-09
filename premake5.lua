
workspace "Vulkan rendering"
	configurations { "Debug", "Release" }
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
		"sandbox/src"
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
		"engine/src"
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
