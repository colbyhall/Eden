#pragma once

#include "types.h"

struct lua_State;

struct Lua {

	static Lua& get();

	void init();
	void shutdown();

private:

	lua_State* lua_state;

	static Lua g_lua;
};