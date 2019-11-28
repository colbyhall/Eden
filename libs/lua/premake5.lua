project "lua"
	language    "C"
	kind        "StaticLib"
	warnings    "off"

	includedirs { "./" }

	files
	{
		"**.h",
		"**.c",
		"**.hpp"
	}

	excludes
	{
		"lauxlib.c",
		"lua.c",
		"luac.c",
		"print.c",
		"**.lua",
		"etc/*.c"
	}

	filter "system:linux or bsd or hurd or aix or solaris or haiku"
		defines     { "LUA_USE_POSIX", "LUA_USE_DLOPEN" }

	filter "system:macosx"
		defines     { "LUA_USE_MACOSX" }