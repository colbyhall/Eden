#pragma once

#include "types.h"

bool is_eol(u8 c);
bool is_whitespace(u8 c);
bool is_letter(u8 c);
bool is_number(u8 c);
bool is_symbol(u8 c);
bool is_lowercase(u8 c);
bool is_uppercase(u8 c);
u8 to_lowercase(u8 c);
u8 to_uppercase(u8 c);
