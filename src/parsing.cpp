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

bool is_whitespace_not_eol(u8 c) {
    return is_whitespace(c) && !is_eol(c);
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

struct Buffer_Iterator {
    const Buffer &b;
    size_t i;
    u32 operator*() { return b[i]; }
    void operator++() { ++i; }
};

// @Cleanup: make this do some utf8 parsing or something; currently we just template on char type.
template <typename Char_Iterator>
static bool buffer_match_string(const Buffer &b, size_t start, Char_Iterator compare_to, size_t size) {
    size_t len = buffer_get_count(b);
    for (size_t i = 0; i < size; i++, ++compare_to) {
        const size_t j = start + i;
        if (j < 0 || j >= len) return false;
        if (b[j] != *compare_to) return false;
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

#define MATCHES(tok,N) (toklen == N && buffer_match_string(b, where, tok, N))
#define CPP_MATCH_ASCII(tok) || MATCHES(tok, sizeof(tok) - 1)
#include "cpp_syntax.h"
static bool c_is_keyword(const Buffer &b, size_t where, size_t toklen) {
    if (false CPP_KEYWORDS(CPP_MATCH_ASCII)) {
        return true;
    }
    return false;
}

static bool c_is_type(const Buffer& b, size_t where, size_t toklen) {
    if (false CPP_TYPES(CPP_MATCH_ASCII)) {
        return true;
    }
    return false;
}

static bool c_is_eol(const Buffer &b, size_t i) {
    assert(i - 1 >= 0);
    return is_eol(b[i]) && b[i - 1] != '\\';
}

static size_t c_scan_line(const Buffer &b, size_t len, size_t i) {
    while (i < len && !c_is_eol(b, i)) i++;
    return i;
}

#define WHITESPACE() do { while (i < len && is_whitespace(b[i])) i++; } while(0)

struct Buf_String {
    size_t i;
    size_t size;
};

static Buf_String *find_buf_string(const Buffer &b, Array<Buf_String> *haystack, Buf_String needle) {
    for (auto &&it : *haystack) {
        if (it.size != needle.size) continue;
        if (buffer_match_string(b, it.i, Buffer_Iterator{b, needle.i}, needle.size)) return &it;
    }
    return nullptr;
}

static Syntax_Highlight c_next_token(Array<Buf_String> *macros, Array<Buf_String> *types, const Buffer &b, size_t i) {
    Syntax_Highlight result = {};
    result.where = i;
    size_t len = buffer_get_count(b);
    assert(i < len);
    if (b[i] == '/') {
        i++;
        if (i < len && b[i] == '/') {
            result.type = SHT_COMMENT;
            i = c_scan_line(b, len, i);
            WHITESPACE();
            result.size = i - result.where;
        } else if (i < len && b[i] == '*') {
            result.type = SHT_COMMENT;
            i += 2;
            while (i < len && (b[i - 1] != '*' || b[i] != '/')) i++;
            i++;
            if (i >= len) i = len; // @Hack just to make sure it's clamped.

            WHITESPACE();
            result.size = i - result.where;
        } else {
            result.type = SHT_OPERATOR;
            WHITESPACE();
            result.size = i - result.where;
        }
    } else if (b[i] == '#') {
        result.type = SHT_DIRECTIVE;
        i++;
        // @Todo: skip backslash-escaped newlines
        while (i < len && is_whitespace_not_eol(b[i])) i++;
        size_t tokstart = i;
        while (i < len && is_letter(b[i])) i++;
        size_t toklen = i - tokstart;

        while (i < len && is_whitespace_not_eol(b[i])) i++;
        result.size = i - result.where;

        bool define = (toklen == 6 && buffer_match_string(b, tokstart, "define", toklen));
        bool elif = (toklen == 4 && buffer_match_string(b, tokstart, "elif", toklen));
        bool else_ = (toklen == 4 && buffer_match_string(b, tokstart, "else", toklen));
        bool endif = (toklen == 5 && buffer_match_string(b, tokstart, "endif", toklen));
        bool error = (toklen == 5 && buffer_match_string(b, tokstart, "error", toklen));
        bool if_ = (toklen == 2 && buffer_match_string(b, tokstart, "if", toklen));
        bool ifdef = (toklen == 5 && buffer_match_string(b, tokstart, "ifdef", toklen));
        bool ifndef = (toklen == 6 && buffer_match_string(b, tokstart, "ifndef", toklen));
        bool include = (toklen == 7 && buffer_match_string(b, tokstart, "include", toklen));
        bool line = (toklen == 4 && buffer_match_string(b, tokstart, "line", toklen));
        bool pragma = (toklen == 6 && buffer_match_string(b, tokstart, "pragma", toklen));
        bool undef = (toklen == 5 && buffer_match_string(b, tokstart, "undef", toklen));
        if (define || undef) {
            Buf_String macro = {};
            macro.i = i;
            while (i < len && (c_is_ident(b[i]) || is_number(b[i]))) i++;
            macro.size = i - macro.i;
            if (macro.size) {
                if (define) {
                    if (!find_buf_string(b, macros, macro)) array_add(macros, macro);
                } else {
                    assert(undef);
                    Buf_String *which = find_buf_string(b, macros, macro);
                    if (which) array_remove_index(macros, which - macros->data);
                }
            }
        } else if (include) {
            // speculatively check for #include <asdf>, if not, backtrack.
            if (i < len && b[i] == '<') {
                while (i < len && !is_eol(b[i]) && b[i] != '>') i++;
                if (!is_eol(b[i])) {
                    i++;
                    WHITESPACE();
                    result.size = i - result.where;
                }
            }
        } else if (error || line || pragma) {
            i = c_scan_line(b, len, i);
            WHITESPACE();
            result.size = i - result.where;
        } else if (elif  || else_ || endif || if_ || ifdef || ifndef) {
        } else {
            // Fake directive; only highlight the '#' and backtrack to the ident.
            result.size = tokstart - result.where;
            i = tokstart;
        }
    } else if (b[i] == '@') {
        result.type = SHT_ANNOTATION;
        i++;
        while (i < len && !is_whitespace(b[i])) i++;
        WHITESPACE();
        result.size = i - result.where;
    } else if (is_whitespace(b[i])) {
        result.type = SHT_NONE;
        i++;
        WHITESPACE();
        result.size = i - result.where;
    } else if (b[i] == '"' || b[i] == '\'') {
        result.type = SHT_STRING_LITERAL;
        u32 end_char = b[i];
        i++;
        while (i < len && (!is_eol(b[i]) && b[i] != end_char)) {
            if (i < len && b[i] == '\\') {
                i++;
            }
            i++;
        }
        i++;
        WHITESPACE();
        result.size = i - result.where;
    } else if (c_is_ident(b[i])) {
        result.type = SHT_IDENT;
        i++;
        while (i < len && (c_is_ident(b[i]) || is_number(b[i]))) i++;
        size_t toklen = i - result.where;

        WHITESPACE();
        result.size = i - result.where;

        if (find_buf_string(b, macros, Buf_String{result.where, toklen})) {
            result.type = SHT_MACRO;
        } else if (c_is_keyword(b, result.where, toklen)) {
            result.type = SHT_KEYWORD;
            // @Speed: duplicate checks
            if (toklen == 5 && buffer_match_string(b, result.where, "using", toklen) ||
                toklen == 6 && buffer_match_string(b, result.where, "struct", toklen) ||
                toklen == 5 && buffer_match_string(b, result.where, "class", toklen) ||
                toklen == 4 && buffer_match_string(b, result.where, "enum", toklen) ||
                toklen == 8 && buffer_match_string(b, result.where, "typename", toklen)) {
                if (c_is_ident(b[i])) {
                    Buf_String type = {};
                    type.i = i;
                    i++; 
                    while (i < len && (c_is_ident(b[i]) || is_number(b[i]))) i++;
                    type.size = i - type.i;
                    if (c_is_keyword(b, type.i, type.size)) {
                        // Already a keyword.
                    } else if (!find_buf_string(b, types, type)) {
                        array_add(types, type);
                    }
                }
            } else if (toklen == 7 && buffer_match_string(b, result.where, "typedef", toklen)) {
                // @Todo.
            }
        } else if (toklen == 7 && buffer_match_string(b, result.where, "defined", toklen)) {
            result.type = SHT_DIRECTIVE;
        } else if (c_is_type(b, result.where, toklen)) {
            result.type = SHT_TYPE;
        } else { // @Temporary: crude function detector.
            i = result.where + result.size;
            while (i < len && is_whitespace(b[i])) i++;
            if (i < len && b[i] == '(') {
                result.type = SHT_FUNCTION;
            } else if (i < len) {
                // @Temporary: crude ident detector.
                switch (u32 c = b[i]) {
                    /*
                case '~':
                case '!':
                case '%':
                case '^':
                case '-':
                case '=':
                case '+':
                case '[':
                case ']':
                case '|':
                case '\'':
                case '"':
                case ',':
                case '.':
                case '/':
                case '?':
                case '}':
                    result.type = SHT_IDENT;
                    break;
                    */
                case ';':
                case ':':
                case '&':
                case '*':
                case '<':
                case '>':
                case ')':
                case '{':
                    if (find_buf_string(b, types, Buf_String{result.where, toklen})) {
                        result.type = SHT_TYPE;
                    } else {
                        result.type = SHT_IDENT;
                    }
                    break;
                    result.type = SHT_TYPE;
                    break;
                default:
                    if (c_is_ident(c)) {
                        result.type = SHT_TYPE;
                    }
                    break;
                }
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
                type = OCT; // @Todo: broken.
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
                WHITESPACE();
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
                WHITESPACE();
                result.size = i - result.where;
            }
            i++;
            if (i < len && (b[i] == '+' || b[i] == '-')) i++;
            if (i < len && is_number(b[i])) {
                i++;
                while (i < len && is_number(b[i])) i++;
            } else {
                type = ERROR;
                WHITESPACE();
                result.size = i - result.where;
            }
        }
        if (type == FLOAT && i < len && b[i] == 'f') i++;
        if (type != ERROR) {
            WHITESPACE();
            result.size = i - result.where;
        }
    } else if (is_symbol(b[i])) {
        result.type = SHT_OPERATOR;
        i++;
        WHITESPACE();
        result.size = i - result.where;
    } else {
        assert(0);
    }
    if (result.size < 1) {
        assert(0);
        result.size = 1;
    }

    return result;
}

void parse_syntax(Array<Syntax_Highlight> *result, const Buffer *buffer, const char *language) {
    array_empty(result);
    assert(String{"c"} == language);

    Array<Buf_String> macros = {};
    Array<Buf_String> types = {};

    size_t buffer_count = buffer_get_count(*buffer);
    for (size_t i = 0; i < buffer_count;) {
        Syntax_Highlight tok = c_next_token(&macros, &types, *buffer, i);
        array_add(result, tok);
        i = tok.where + tok.size;
    }

    array_destroy(&macros);
    array_destroy(&types);
}
