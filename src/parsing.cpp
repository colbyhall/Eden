#include "parsing.h"

bool is_eof(u8 c) {
	return c == EOF;
}

bool is_eol(u8 c) {
	return c == '\n' || c == '\r';
}

bool is_whitespace(u8 c) {
	return is_eol(c) || c == ' ' || c == '\t';
}

bool is_letter(u8 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_oct_digit(u8 c) {
    if (c < '0') return false;
    if (c > '7') return false;

    return true;
}

bool is_hex_digit(u8 c) {
    if (is_number(c)) return true;
    if (!is_letter(c)) return false;

    if (c >= 'a') c += ('A' - 'a');
    if (c >= 'A' && c <= 'F') return true;

    return false;
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

#include "buffer.h"

// @Cleanup: make this do some utf8 parsing or something; currently we just template on char type.
template <typename Char>
static bool buffer_match_string(const Buffer &b, size_t start, const Char *compare_to, size_t size) {
    size_t len = buffer_get_count(b);
    for (size_t i = 0; i < size; i++) {
        const size_t j = start + i;
        if (j < 0 || j >= len) return false;
        if (b[j] != compare_to[i]) return false;
    }
    return true;
}

// Doesn't include is_number.
static bool c_is_ident(u32 c) {
    if (c >= 128) return true;
    if (is_letter(c)) return true;
    if (c == '_') return true;
    if (c == '$') return true;
    return false;
}

static size_t c_scan_line(const Buffer &b, size_t len, size_t i) {
    while (i < len && (!is_eol(b[i]) || b[i - 1] == '\\')) i++;
    return i;
}

static Syntax_Highlight c_next_token(const Buffer &b, size_t i) {
    Syntax_Highlight result = {};
    result.where = i;
    size_t len = buffer_get_count(b);
    assert(i < len);
    if (b[i] == '/') {
        i++;
        if (i < len && b[i] == '/') {
            result.type = SHT_COMMENT;
            i = c_scan_line(b, len, i);
            result.size = i - result.where;
        } else if (i < len && b[i] == '*') {
            result.type = SHT_COMMENT;
            i += 2;
            while (i < len && (b[i - 1] != '*' || b[i] != '/')) i++;
            i++;
            if (i >= len) i = len; // @Hack just to make sure it's clamped.
            result.size = i - result.where;
        } else {
            result.type = SHT_OPERATOR;
            result.size = 1;
        }
    } else if (b[i] == '#') {
        result.type = SHT_DIRECTIVE;
        i = c_scan_line(b, len, i);
        result.size = i - result.where;
    } else if (b[i] == '@') {
        result.type = SHT_DIRECTIVE;
        i++;
        while (i < len && !is_whitespace(b[i])) i++;
        result.size = i - result.where;
    } else if (is_whitespace(b[i])) {
        result.type = SHT_NONE;
        i++;
        while (i < len && is_whitespace(b[i])) i++;
        result.size = i - result.where;
    } else if (b[i] == '"') {
        result.type = SHT_STRING_LITERAL;
        i++;
        while (i < len && (!is_eol(b[i]) && b[i] != '"')) {
            if (i < len && b[i] == '\\') {
                i++;
            }
            i++;
        }
        i++;
        result.size = i - result.where;
    } /*else if (b[i] == '\'') {
        result.type = SHT_STRING_LITERAL;
        while (
    }*/ else if (c_is_ident(b[i])) {
        result.type = SHT_IDENT; // @Temporary: no symbol table yet
        while (i < len && (c_is_ident(b[i]) || is_number(b[i]))) i++;
        result.size = i - result.where;
        
#define MATCHES(tok,N) (result.size == N && buffer_match_string(b, result.where, tok, N))
#define CPP_KEYWORD_ASCII(tok) \
    || MATCHES(tok, sizeof(tok) - 1)

#include "cpp_syntax.h"

        if (false CPP_KEYWORDS(CPP_KEYWORD_ASCII)) {
            result.type = SHT_KEYWORD;
        } else if (false CPP_KEYWORD_ASCII("u8")
                         CPP_KEYWORD_ASCII("u16")
                         CPP_KEYWORD_ASCII("u32")
                         CPP_KEYWORD_ASCII("u64")
                         CPP_KEYWORD_ASCII("s8")
                         CPP_KEYWORD_ASCII("s16")
                         CPP_KEYWORD_ASCII("s32")
                         CPP_KEYWORD_ASCII("s64")) { // @Temporary: crude type detector.
            result.type = SHT_TYPE;
        } else { // @Temporary: crude function detector.
            i = result.where + result.size;
            while (i < len && is_whitespace(b[i])) i++;
            if (i < len && b[i] == '(') {
                result.type = SHT_FUNCTION;
            }
        }
    } else if (is_number(b[i])) {
        result.type = SHT_NUMERIC_LITERAL;
        while (i < len && is_number(b[i])) i++;
        enum {
            NORMAL,
            HEX,
            OCT,
            FLOAT,
            ERROR,
        } type = NORMAL;
        if (b[i - 1] == '0') {
            if (b[i] == 'x') {
                i++;
                type = HEX;
            } else if (is_number(b[i])) {
                i++;
                type = OCT;
            }
        }
        if (type == OCT) {
            while (is_oct_digit(b[i])) i++;
        }
        if (type == HEX) {
            while (is_hex_digit(b[i])) i++;
        }
        if (i < len && b[i] == '.') {
            if (type == NORMAL) {
                type = FLOAT;
            } else {
                type = ERROR;
                result.size = i - result.where;
            }
            i++;
            while (i < len && is_number(b[i])) i++;
        }
        if (i < len && b[i] == 'e') {
            if (type == NORMAL) {
                type = FLOAT;
            } else if (type != FLOAT) {
                type = ERROR;
                result.size = i - result.where;
            }
            i++;
            if (i < len && (b[i] == '+' || b[i] == '-')) i++;
            if (i < len && is_number(b[i])) {
                i++;
                while (i < len && is_number(b[i])) i++;
            } else {
                type = ERROR;
                result.size = i - result.where;
            }
        }
        if (type == FLOAT && i < len && b[i] == 'f') i++;
        if (type != ERROR) {
            result.size = i - result.where;
        } else {
            //result.type = SHT_IDENT;
        }
    } else if (is_symbol(b[i])) {
        result.type = SHT_OPERATOR;
        result.size = 1;
    } else {
        assert(0);
    }

    return result;
}

void parse_syntax(Array<Syntax_Highlight> *result, const Buffer *buffer, const char *language) {
    array_empty(result);
    assert(String{"c"} == language);

    size_t buffer_count = buffer_get_count(*buffer);
    for (size_t i = 0; i < buffer_count;) {
        Syntax_Highlight tok = c_next_token(*buffer, i);
        array_add(result, tok);
        i = tok.where + tok.size;
    }
}
