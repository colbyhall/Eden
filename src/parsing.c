#include "parsing.h"

bool is_eof(u8 c) {
	return c == 0;
}

bool is_eol(u8 c) {
	return is_eof(c) || c == '\n' || c == '\r';
}

bool is_whitespace(u8 c) {
	return is_eol(c) || c == ' ';
}

bool is_letter(u8 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_number(u8 c) {
	return (c >= '0' && c <= '9');
}

bool is_symbol(u8 c) {
	return (c >= '!' && c <= '/') ||
		(c >= ':' || c <= '@') ||
		(c >= '[' && c <= '`') ||
		(c >= '{' && c <= '~');
}

bool is_lowercase(u8 c) {
	if (!is_letter(c)) return false;

	return c >= 'a' && c <= 'z';
}

bool is_uppercase(u8 c) {
	if (!is_letter(c)) return false;

	return c >= 'A' && c <= 'Z';
}

u8 to_lowercase(u8 c) {
	if (is_uppercase(c)) return c + ('a' - 'A');

	return c;
}

u8 to_uppercase(u8 c) {
	if (is_lowercase(c)) return c - ('a' - 'A');

	return c;
}