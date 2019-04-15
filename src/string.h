#pragma once

#include "types.h"

typedef struct String {
	u8* data;
	size_t length;
	size_t allocated;
} String;

String make_string(const char* str);
void advance_string(String* str, size_t amount);
bool string_starts_with(String* str, const char* cstr, bool case_matters);
void append_cstring(String* str, const char* app);
void append_string(String* str, String* app);
String string_eat_line(String* str);
void string_eat_whitespace(String* str);

String string_eat_until_char(String* str, u8 c);
String string_eat_word(String* str);

bool string_equals_cstr(String* str, const char* cstr);
bool string_equals(String* left, String* right);

void free_string(String* str);