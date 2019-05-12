#include "string.h"
#include "parsing.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

String::String(const char* cstr) {
	count = strlen(cstr);
	allocated = count + 1;
	data = (u8*)cstr;
}

u8 String::operator[](size_t index) const {
	assert(index <= count);

	return data[index];
}

bool String::operator==(const String& right) const {
	if (count != right.count) return false;

	for (size_t i = 0; i < count; i++) {
		if (data[i] != right[i]) return false;
	}

	return true;
}

bool String::operator==(const char* cstr) const {
	return *this == String(cstr);
}

bool string_starts_with(const String& string, const String& compare, bool case_matters) {
	if (string.count < compare.count) return false;

	for (int i = 0; i < compare.count; i++) {
		const u8 a = compare[i];
		const u8 b = string[i];

		if (case_matters) {
			if (a != b) return false;
		} else {
			if (is_letter(a) && is_letter(b)) {
				if (to_lowercase(a) != to_lowercase(b)) return false;
			}
			else {
				if (a != b) return false;
			}
		}
	}

	return true;
}

void string_append(String* string, const String& addition) {
	assert(string->count + addition.count <= string->allocated);

	memcpy(string->data + string->count, addition.data, addition.count);
	string->count += addition.count;
}

void string_advance(String* string, size_t amount) {
	string->data += amount;
	string->count -= amount;
	string->allocated -= amount;
}

String string_eat_line(String* string) {
	string_eat_whitespace(string);

	String result;
	for (int i = 0; i < string->count; i++) {
		if (is_eol(string->data[i])) {
			result.data = string->data;
			result.count = i;
			result.allocated = i;
			string_advance(string, i + 1);

			return result;
		}
	}

	return result;
}

void string_eat_whitespace(String* string) {
	while (string->count > 0 && is_whitespace(string->data[0])) {
		string_advance(string, 1);
	}
}
