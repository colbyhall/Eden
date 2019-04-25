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

bool String::starts_with(const String& str, bool case_matters) const {
	if (str.count > count) return false;

	for (int i = 0; i < str.count; i++) {
		const u8 a = str[i];
		const u8 b = data[i];
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

void String::append(const String& str) {
	assert(count + str.count <= allocated);

	memcpy(data + count, str.data, str.count);

	count += str.count;
}

void String::advance(size_t amount) {
	data += amount;
	count -= amount;
	allocated -= amount;
}

String String::eat_line() {
	eat_whitespace();

	String result;
	for (int i = 0; i < count; i++) {
		if (is_eol(data[i])) {
			result.data = data;
			result.count = i;
			result.allocated = i;
			advance(i + 1);

			return result;
		}
	}

	return result;
}

void String::eat_whitespace() {
	while (count > 0 && is_whitespace(data[0])) {
		advance(1);
	}
}
