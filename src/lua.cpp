#include "lua.h"
#include "memory.h"

#include <lua/lua.hpp>

lua_State* lua_state;

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

void lua_init() {
	lua_state = lua_newstate(lua_alloc, nullptr);
}

void lua_shutdown() {
	lua_close(lua_state);
}
