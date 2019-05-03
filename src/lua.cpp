#include "lua.h"
#include "memory.h"
#include "os.h"

extern "C" {
#include <lua/lua.hpp>
#include <lua/lualib.h>
}

#include <assert.h>
#include <stdlib.h>

lua_State* lua_state;

// @NOTE(Colby): Based off of l_alloc from lauxlib.c
static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		if (ptr) {
			free(ptr);
		}
		return nullptr;
	} else if (ptr) {
		return realloc(ptr, nsize);
	} else {
		return malloc(nsize);
	}

	return nullptr;
}

void lua_init() {
	lua_state = lua_newstate(lua_alloc, nullptr);
	luaopen_base(lua_state);
	luaopen_table(lua_state);
	luaopen_io(lua_state);
	luaopen_string(lua_state);
	luaopen_math(lua_state);

	String startup_script = os_load_file_into_memory("config/startup.lua");
	if (luaL_loadbuffer(lua_state, (const char*)startup_script.data, startup_script.count, "startup") || lua_pcall(lua_state, 0, 0, 0)) {
		// @TODO(Colby): Implement error handling system for lua scripts
		assert(false);
	}
}

void lua_shutdown() {
	lua_close(lua_state);
}

bool lua_get_float(const char* var_name, float* result) {
	lua_getglobal(lua_state, var_name);
	if (!lua_isnumber(lua_state, -1)) {
		// @TODO(Colby): Implement error handling system for lua scripts
		return false;
	}
	*result = (float)lua_tonumber(lua_state, -1);
	return true;
}
