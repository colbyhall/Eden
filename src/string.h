#pragma once

#include "types.h"

struct String {
	u8* data = nullptr;
	size_t count = 0;
	size_t allocated = 0;

	String() = default;
	String(const char* cstr);

	operator bool() const { return count > 0 && data != nullptr; }
	operator const char*() const { return (const char*)data; }
	operator char*() const { return (char*)data; }

	u8 operator[](size_t index) const;
};

bool string_starts_with(const String& string, const String& compare, bool case_matters = true);
void string_append(String* string, const String& addition);
void string_advance(String* string, size_t amount);
String string_eat_line(String* string);
void string_eat_whitespace(String* string);

