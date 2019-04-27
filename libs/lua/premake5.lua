project "lua-lib"
	language    "C"
	kind        "StaticLib"
	warnings    "off"

	includedirs { "**" }

	files
	{
		"**.h",
		"**.c"
	}

	excludes
	{
		"lua.c"
	}

	defines
	{

	}

	filter "system:linux or bsd or hurd or aix or solaris"
		defines     { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

	filter "system:macosx"
		defines     { "LUA_USE_MACOSX" }