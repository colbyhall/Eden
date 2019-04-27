#include "lua.h"
#include "memory.h"

#include <lua/lua.hpp>

Lua Lua::g_lua;

Lua& Lua::get() {
	return g_lua;
}

// @NOTE(Colby): Based off of l_alloc from lauxlib.c
static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		if (ptr) {
			c_free(ptr);
		}
		return nullptr;
	} else if (ptr) {
		return c_realloc(ptr, nsize);
	} else {
		return c_alloc(nsize);
	}

	return nullptr;
}

void Lua::init() {
	lua_state = lua_newstate(lua_alloc, nullptr);
}

void Lua::shutdown() {
	lua_close(lua_state);
}
