#include "string.h"
#include "parsing.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

String make_string(const char* str) {
	String result;

	result.data = (u8*)str;
	result.length = strlen(str);
	result.allocated = result.length + 1;
	
	return result;
}

void advance_string(String* str, size_t amount) {
	if (amount > str->length) amount = str->length;

	str->data += amount;
	str->length -= amount;
	str->allocated -= amount;
}

bool string_starts_with(String* str, const char* cstr, bool case_matters) {
	size_t cstr_length = strlen(cstr);
	if (str->length> cstr_length) return false;

	for (int i = 0; i < str->length; i++) {
		const u8 a = cstr[i];
		const u8 b = str->data[i];
		if (case_matters) {
			if (a != b) return false;
		}
		else {
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

void append_cstring(String* str, const char* app) {
	size_t app_length = strlen(app);
	
	assert(str->length + app_length <= str->allocated);
	memcpy(str->data + str->length, app, app_length);
	str->length += app_length;
}

void append_string(String* str, String* app) {
	assert(str->length + app->length <= str->allocated);
	memcpy(str->data + str->length, app->data, app->length);
	str->length += app->length;
}

String string_eat_line(String* str) {
	string_eat_whitespace(str);

	String result;
	for (size_t i = 0; i < str->length; i++) {
		if (is_eol(str->data[i])) {
			result.data = str->data;
			result.length = i;
			result.allocated = i;
			advance_string(str, i + 1);

			return result;
		}
	}

	result.data = NULL;
	result.length = 0;
	result.allocated = 0;
	return result;
}

void string_eat_whitespace(String* str) {
	while (str->length > 0 && is_whitespace(str->data[0])) {
		advance_string(str, 1);
	}
}

void free_string(String* str) {
	free(str->data);

	str->allocated = 0;
	str->length = 0;
	str->data = NULL;
}