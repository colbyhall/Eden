workspace "YEET"
    architecture "x64"
    startproject "YEET"
    
    configurations
    {
        "Debug",
        "Release"
    }

project "YEET"
    kind "WindowedApp"
    language "C"

    targetdir ("bin")
    objdir ("bin")
	characterset("ASCII")

    files
    {
        "src/*.h",
        "src/*.c",
		"libs/**.h"
    }

    includedirs
    {
        "src/**",
        "libs/"
    }

    links
    {
        "opengl32",
        "user32",
        "kernel32",
		"shlwapi"
    }

    filter "configurations:Debug"
		defines 
		{
			"BUILD_DEBUG#1",
			"BUILD_RELEASE#0"
		}
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines 
		{
			"BUILD_RELEASE#1",
			"BUILD_DEBUG#0"
		}
		runtime "Release"
        optimize "On"
        
    filter "system:windows"
        cdialect  "Default"
		systemversion "latest"
		architecture "x64"

		defines
		{
			"PLATFORM_WINDOWS#1",
        }

		files 
		{
			"src/win32/**.h",
			"src/win32/**.c"
		}