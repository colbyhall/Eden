#pragma once

#include "types.h"

struct lua_State;

void lua_init();
void lua_shutdown();

bool lua_get_float(const char* var_name, float* result);