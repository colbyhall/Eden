#pragma once

#include "types.h"

struct String {
	u8* data = nullptr;
	size_t count = 0;
	size_t allocated = 0;

	String() = default;
	String(const char* cstr);

	operator bool() const { return count > 0 && data != nullptr; }

	u8 operator[](size_t index) const;

	operator const char*() const { return (const char*)data; }

	bool starts_with(const String& str, bool case_matters = true) const;
	void append(const String& str);

	void advance(size_t amount);
	String eat_line();
	void eat_whitespace();
};