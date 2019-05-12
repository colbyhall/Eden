#include "parsing.h"
#include "string.h"

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
	if (is_whitespace(c) || is_number(c) || is_letter(c)) return false;

	return true;
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
    size_t len = get_count(b);
    for (size_t i = 0; i < size; i++, ++compare_to) {
        const size_t j = start + i;
        if (j < 0 || j >= len) return false;
        if (b[j] != *compare_to) return false;
    }
    return true;
}

static bool buffer_match_string(u32 *start, size_t len_in_buf, const char *str, size_t len_in_lit) {
    if (len_in_buf != len_in_lit) return false;
    for (size_t i = 0; i < len_in_buf; i++) {
        if (start[i] != str[i]) return false;
    }
    return true;
}

static bool c_starts_ident(u32 c) {
    if (c >= 128) return true;
    if (is_letter(c)) return true;
    if (c == '_') return true;
    if (c == '$') return true;
    return false;
}
static bool c_is_ident(u32 c) {
    return c_starts_ident(c) || is_number(c);
}

static bool c_is_op(u32 c) {
    switch (c) {
    case '~': case '!': case '%': case '^': case '&': case '*': case '(':
    case ')': case '-': case '=': case '+': case '[': case '{': case ']':
    case '}': case '\\': case '|': case ';': case ':': case '\'': case '"':
    case ',': case '<': case '.': case '>': case '/': case '?': return true;
    }
    return false;
}
static bool c_is_op_except_comment(u32 c) {
    switch (c) {
    case '~': case '!': case '%': case '^': case '&': case '*': case '(':
    case ')': case '-': case '=': case '+': case '[': case '{': case ']':
    case '}': case '\\': case '|': case ';': case ':': case '\'': case '"':
    case ',': case '<': case '.': case '>': case '?': return true;
    }
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

static bool c_is_eol(u32 *p) {
    return is_eol(p[0]) && p[-1] != '\\';
}

//static size_t c_scan_line(const Buffer &b, size_t len, size_t i) {
//    while (i < len && !c_is_eol(b, i)) i++;
//    return i;
//}

#define WHITESPACE() do { while (i < len && is_whitespace(b[i])) i++; } while(0)

static Buf_String *find_buf_string(const Buffer &b, Array<Buf_String> *haystack, Buf_String needle) {
    for (auto &&it : *haystack) {
        if (it.size != needle.size) continue;
        if (buffer_match_string(b, it.i, Buffer_Iterator{b, needle.i}, needle.size)) return &it;
    }
    return nullptr;
}

static bool is_type(const Buffer& b, Array<Buf_String> *types, Buf_String tok) {
    if (c_is_type(b, tok.i, tok.size)) return true;
    if (find_buf_string(b, types, tok)) return true;

    return false;
}

static bool is_macro(const Buffer& b, Array<Buf_String> *macros, Buf_String tok) {
    if (find_buf_string(b, macros, tok)) return true;

    return false;
}

//static void add_type(const Buffer& b, Array<Buf_String> *types, Buf_String tok) {
//
//}

// @Todo: static void c_advance(const Buffer &b, size_t i) {}

static bool macro_has_params(const Buffer &b, Buf_String macro) {
    size_t check = macro.i + macro.size;
    if (check < get_count(b) && b[check] == '(') return true;

    return false;
}

static void move_gap_to_end(Buffer& b) {
    u32 *buf_end = b.data + b.allocated;
    u32 *gap_end = b.gap + b.gap_size;
    size_t num_to_move = buf_end - gap_end;
    u32 *dest = b.gap;
    u32 *src = gap_end;
    memcpy(dest, src, num_to_move * sizeof(u32));
    b.gap += num_to_move;
    assert(b.gap + b.gap_size == buf_end);
}

#define BUFFER_STOP_PARSING 0xffffffff

#define PUSH(new_type)                                                         \
    do {                                                                       \
        if (type != new_type && p > begin) {                                   \
            array_add(&b.syntax,                                               \
                      Syntax_Highlight{(u64)type, (u64)(p - begin)});          \
            begin = p;                                                         \
        }                                                                      \
        type = new_type;                                                       \
    } while (0)
#define END() PUSH(SHT_NONE)

// advance in the buffer, skipping whitespace & escaped newlines, and pushing comments.
static void scan_lexeme(Buffer &b, u32 *&p, u32 *&begin, Syntax_Highlight_Type &type) {
    Syntax_Highlight_Type old_type = type;
    for (;;) switch (p[0]) {
    case BUFFER_STOP_PARSING: default: goto done; // symbols or newlines, we're done!!

    case '\\':
        // escape: if it's a newline, skip it, otherwise ignore the backslash.
        p++;
        if (p[0] == '\r' || p[0] == '\n') p++;
        break;

    case '/': {
        // slash: divide, block comment, or line comment
        bool line = false;
        switch (p[1]) {
        case '/': // line comment
            line = true;
        case '*':
            PUSH(SHT_COMMENT);
            p++;
            p++;
            for (;;) switch (p[0]) {
            case BUFFER_STOP_PARSING: goto done;
            case '\r':
            case '\n':
                if (line) {
                    if (p[-1] != '\\') goto comment_done;
                } else {
                    if (p[-1] == '*') goto comment_done;
                }
            default:
                p++;
                break;
            }
        comment_done:;
            break;
        default:
            // it was just a divide.
            goto done;
        }
        break;
    }
    case ' ':
    case '\t':
        // whitespace, scan ahead.
        p++;
        break;
    }
done:;
    PUSH(old_type);
}

#define STOP(p) (p[0] == BUFFER_STOP_PARSING)
static void parse_pp(Buffer &b, u32 *&p, u32 *&begin, Syntax_Highlight_Type &type) {
    assert(p[0] == '#');
    PUSH(SHT_DIRECTIVE);
    p++;
    scan_lexeme(b, p, begin, type);
    u32 *directive = p;
    while (is_letter(p[0])) p++;
    bool define = buffer_match_string(directive, p - directive, "define", 6);
    bool include = buffer_match_string(directive, p - directive, "include", 7);
    scan_lexeme(b, p, begin, type);
    if (!STOP(p) && p[0] != '\r' && p[0] != '\n') {
        if (define && c_starts_ident(p[0])) {
            PUSH(SHT_MACRO);
            while (c_is_ident(p[0])) p++;
            scan_lexeme(b, p, begin, type);
            PUSH(SHT_DIRECTIVE);
        } else if (include && p[0] == '"' || p[0] == '<') {
            u32 end = p[0] == '<' ? '>' : '"';
            PUSH(SHT_STRING_LITERAL);
            p++;
            while (!STOP(p) && p[0] != end && !c_is_eol(p)) p++;
        }
    }
    while (!STOP(p) && !c_is_eol(p)) {
        p++;
        scan_lexeme(b, p, begin, type);
    }
}

static void next_token(Buffer &b, u32 *&p, u32 *&begin, Syntax_Highlight_Type &type) {
    scan_lexeme(b, p, begin, type);
    if (p[0] == '#') parse_pp(b, p, begin, type);
}

static void cpp_parse_syntax(Buffer& b) {
    if (b.allocated <= 0) return;
    move_gap_to_end(b);
    u32 *p = b.data;
    u32 *end = b.gap;
    u32 final_char = end[-1];
    end[-1] = BUFFER_STOP_PARSING;

    u32 *begin = p;
    Syntax_Highlight_Type type = SHT_NONE;
    for (;; next_token(b, p, begin, type)) switch (p[0]) {
    case BUFFER_STOP_PARSING: goto done;

    case ' ':
    case '\t':
        p++;
        break;

    default:
        PUSH(SHT_NONE);
        p++;
        break;

    case '#':
        parse_pp(b, p, begin, type);
        break;
    }
done:;
    END();
    end[-1] = final_char;
}


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "os.h"

void parse_syntax(Buffer* buffer) {
	if (!buffer->language) return;

    array_empty(&buffer->syntax);
    array_empty(&buffer->macros);
    array_empty(&buffer->types);
    assert(buffer->language == "c" || buffer->language == "cpp");

    if (buffer->syntax.allocated < buffer->allocated) {
        array_reserve(&buffer->syntax, (size_t)(buffer->allocated - buffer->syntax.allocated));
        OutputDebugStringA("Reserving!!\n");
    }

    auto begin = os_get_time();

    cpp_parse_syntax(*buffer);
    
    auto end = os_get_time();

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    
    double microseconds = (end - begin);
    microseconds *= 1000000;
    char buf[512];
    sprintf(buf, "%f microseconds.\n", microseconds);
    OutputDebugStringA(buf);
}
