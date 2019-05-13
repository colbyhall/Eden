#include "parsing.h"
#include "string.h"

bool is_eof(u32 c) {
	return c == EOF;
}

bool is_eol(u32 c) {
	return c == '\n' || c == '\r';
}

bool is_whitespace(u32 c) {
	return is_eol(c) || c == ' ' || c == '\t';
}

bool is_whitespace_not_eol(u32 c) {
    return is_whitespace(c) && !is_eol(c);
}

bool is_letter(u32 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_oct_digit(u32 c) {
    if (c < '0') return false;
    if (c > '7') return false;

    return true;
}

bool is_hex_digit(u32 c) {
    if (is_number(c)) return true;
    if (!is_letter(c)) return false;

    if (c >= 'a') c += ('A' - 'a');
    if (c >= 'A' && c <= 'F') return true;

    return false;
}

bool is_number(u32 c) {
	return (c >= '0' && c <= '9');
}

bool is_symbol(u32 c) {
	if (is_whitespace(c) || is_number(c) || is_letter(c)) return false;

	return true;
}

bool is_lowercase(u32 c) {
	if (!is_letter(c)) return false;

	return c >= 'a' && c <= 'z';
}

bool is_uppercase(u32 c) {
	if (!is_letter(c)) return false;

	return c >= 'A' && c <= 'Z';
}

u32 to_lowercase(u32 c) {
	if (is_uppercase(c)) return c + ('a' - 'A');

	return c;
}

u32 to_uppercase(u32 c) {
	if (is_lowercase(c)) return c - ('a' - 'A');

	return c;
}

#include "buffer.h"

static bool buffer_match_string(u32 *start, size_t len_in_buf, const char *str, size_t len_in_lit) {
    if (len_in_buf != len_in_lit) return false;
    for (size_t i = 0; i < len_in_buf; i++) {
        if (start[i] != str[i]) return false;
    }
    return true;
}
#define BUFMATCH_LIT(start, len, str) buffer_match_string(start, len, str, sizeof(str) - 1)

// Caution: this takes signed 32, because REASONS.
// (It's so that the function performs a signed compare
// in order to make sure that it doesn't match against
// the buffer end character, which has the high bit
// [and every other bit] set.) phil 2019-05-12
static bool c_starts_ident(s32 c) {
    if (c >= 128) return true;
    if (is_letter(c)) return true;
    if (c == '_') return true;
    if (c == '$') return true;
    return false;
}
static bool c_is_ident(u32 c) { return c_starts_ident(c) || is_number(c); }

static bool c_is_op(u32 c) {
    switch (c) {
    case '~': case '!': case '%': case '^': case '&': case '*': case '(':
    case ')': case '-': case '=': case '+': case '[': case '{': case ']':
    case '}': case '\\': case '|': case ';': case ':': case '\'': case '"':
    case ',': case '<': case '.': case '>': case '/': case '?': return true;
    }
    return false;
}

static bool c_is_eol(u32 *p) {
    return is_eol(p[0]) && p[-1] != '\\';
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

struct Cpp_Lexer {
    u32 *buffer_start = nullptr;
    u32 *p = nullptr;
    Syntax_Highlight *sh = nullptr;

    bool more() { return p[0] != BUFFER_STOP_PARSING; }
    bool eol() { return c_is_eol(p); }

#define CPP_MATCH_ASCII(tok) || BUFMATCH_LIT(start, n, tok)
#include "cpp_syntax.h"
    static bool is_keyword(u32 *start, size_t n) {
        if (n < 2 || n > 16) return false;
        // @Todo: switch on length instead of linear search.
        if (false CPP_KEYWORDS(CPP_MATCH_ASCII)) return true;

        return false;
    }

    static bool is_type_keyword(u32 *start, size_t n) {
        if (n < 3 || n > 8) return false;
        if (false CPP_TYPE_KEYWORDS(CPP_MATCH_ASCII)) return true;

        return false;
    }

    static bool is_type(u32 *start, size_t n) {
        if (n < 6 || n > 14) return false; // @Temporary: only for builtins.
        if (false CPP_TYPES(CPP_MATCH_ASCII)) return true;
        
        return false;
    }

    Syntax_Highlight* push(Syntax_Highlight_Type new_type) {
        sh->type = new_type;
        sh->where = (size_t)(p - buffer_start);
        sh++;
        return sh - 1;
    }

    // advance in the buffer, skipping whitespace & escaped newlines, and pushing comments.
    void scan_lexeme() {
        Syntax_Highlight_Type old_type = (Syntax_Highlight_Type)sh->type;
        for (;;) switch (p[0]) {
        case BUFFER_STOP_PARSING: default: goto done; // symbols or newlines, we're done!!
    
        case '\\':
            // escape: if it's a newline, skip it, otherwise it's over.
            switch (p[1]) {
            default: goto done;
            case '\r': case '\n': p++; p++;
            }
            break;
    
        case '/': {
            // slash: divide, block comment, or line comment
            bool line = false;
            auto s = push(SHT_OPERATOR);
            p++;
            switch (p[0]) {
            case '/': // line comment
                line = true;
            case '*':
                s->type = SHT_COMMENT;
                p++;
                for (;;) switch (p[0]) {
                case BUFFER_STOP_PARSING: goto done;
                case '\r':
                case '\n':
                    if (line && p[-1] != '\\') goto comment_done;
                case '/':
                    if (!line && p[-1] == '*') goto comment_done;
                default:
                    p++;
                    break;
                }
            comment_done:;
                p++;
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
        if ((Syntax_Highlight_Type)sh->type != old_type) push(old_type);
    }

    void parse_pp() {
        assert(p[0] == '#');
        push(SHT_DIRECTIVE);
        p++;
        scan_lexeme();
        u32 *directive = p;
        while (is_letter(p[0])) p++;
        bool define = buffer_match_string(directive, p - directive, "define", 6);
        bool include = buffer_match_string(directive, p - directive, "include", 7);
        scan_lexeme();
        if (more() && !is_eol(p[0])) {
            if (define && c_starts_ident(p[0])) {
                push(SHT_MACRO);
                while (c_is_ident(p[0])) p++;
                scan_lexeme();
            } else if (include && p[0] == '"' || p[0] == '<') {
                u32 end;
                if (p[0] == '<') end = '>'; else end = '"';
                push(SHT_STRING_LITERAL);
                p++;
                while (more() && !eol() && p[0] != end) p++;
                p++;
                scan_lexeme();
            }
            if (more() && !eol()) {
                push(SHT_DIRECTIVE);
                p++;
            }
        }
        while (more() && !eol()) {
            p++;
            scan_lexeme();
        }
    }

    void next_token() {
        scan_lexeme();
        while (is_eol(p[0])) {
            p++;
            scan_lexeme();
            if (p[0] == '#') {
                parse_pp();
                push(SHT_IDENT); // @Hack.
            }
        }
    }

    struct Ident {
        u32 *p = nullptr;
        size_t n = 0;
    };
    Ident read_ident() {
        Ident result;
        result.p = p;
        while (c_is_ident(p[0])) p++;
        result.n = p - result.p;
        return result;
    }

    void parse_braces() { // @Stub.
        assert(p[0] == '{');
        push(SHT_OPERATOR);
        p++; // Eat operator
        while (more() && p[0] != '}') {
            next_token();
            if (should_bail()) return;
            parse_stmt();
            if (p[0] == ';') { push(SHT_OPERATOR); p++; }
        }
    }

    void parse_params() { // @Stub.
        assert(p[0] == '(');
        push(SHT_OPERATOR);
        p++; // Eat operator
        while (more() && p[0] != ')') {
            next_token();
            if (should_bail()) return;
            parse_stmt(true);
            //if (p[0] == ';') { push(SHT_OPERATOR); p++; }
        }
    }
    bool should_bail() {
        if (p[0] == ']' || p[0] == '}' || p[0] == ';') return true;
        return false;
    }
    void parse_expr() { // @Temporary @Stub
        if (p[0] == '(') {
            push(SHT_OPERATOR);
            p++; // Eat operator
            while (more() && p[0] != ')') {
                next_token();
                if (should_bail()) return;
                parse_expr();
            }
        } else if (p[0] == '{') {
            parse_braces();
        } else if (p[0] == '[') {
            push(SHT_OPERATOR);
            p++;
            while (more() && p[0] != ']') {
                next_token();
                if (should_bail()) return;
                parse_expr();
            }
            if (p[0] == ']') { push(SHT_OPERATOR); p++; }
        } else if (p[0] == ')') {
            push(SHT_OPERATOR);
            p++;
            // We have been passed a closing paren as the expr.
            // This is weird and shouldn't happen in valid c++.
            return;
        } else if (should_bail()) {
            return;
        } else if (c_starts_ident(p[0])) {
            auto w = push(SHT_IDENT);
            Ident i = read_ident();
            if (is_keyword(i.p, i.n)) {
                w->type = SHT_KEYWORD;
            }
            next_token();
            if (p[0] == '(') w->type = SHT_FUNCTION;
        } else if (p[0] == '"' || p[0] == '\'') {
            u32 end = p[0];
            push(SHT_STRING_LITERAL);
            p++;
            while (more() && p[0] != end && !c_is_eol(p)) {
                p++;
                if (p[0] == '\\') p++;
            }
            if (p[0] == end) p++;
        } else if (c_is_op(p[0])) {
            push(SHT_OPERATOR);
            p++; // Eat operator
        } else if (is_number(p[0])) { // @Todo: real C++ number parsing
            push(SHT_NUMERIC_LITERAL);
            p++;
            while (more() && is_number(p[0])) p++;
        }
    }

    //void parse_expr() {
    //    assert(p[0] == '(');
    //}

    void parse_stmt(bool is_params = false) {
        if (c_starts_ident(p[0])) {
            auto prev = push(SHT_IDENT);
            Ident ident = read_ident();
            assert(ident.n);
            // @Todo: if (is_macro(ident)) {
            // } else
            if (BUFMATCH_LIT(ident.p, ident.n, "break") ||
                BUFMATCH_LIT(ident.p, ident.n, "continue")) {
                prev->type = SHT_KEYWORD;
                next_token();
                parse_stmt();
                return;
            } else if (BUFMATCH_LIT(ident.p, ident.n, "return")) {
                prev->type = SHT_KEYWORD;
                while (more() && p[0] != ';') {
                    next_token();
                    if (should_bail()) return;
                    parse_expr();
                }
                return;
            } else {
                if (is_keyword(ident.p, ident.n)) {
                    prev->type = SHT_KEYWORD;
                //} else if (is_type(ident.p, ident.n)) {
                //    prev->type = SHT_TYPE;
                }
                if (BUFMATCH_LIT(ident.p, ident.n, "if") ||
                    BUFMATCH_LIT(ident.p, ident.n, "while") ||
                    BUFMATCH_LIT(ident.p, ident.n, "switch")) {
                    next_token();
                    if (p[0] == '(') {
                        push(SHT_OPERATOR);
                        p++;
                        next_token();
                        parse_stmt(true);
                        if (p[0] == ')') { push(SHT_OPERATOR); p++; }
                    }
                    next_token();
                    parse_stmt();
                    return;
                }
                if (BUFMATCH_LIT(ident.p, ident.n, "for")) {
                    next_token();
                    if (p[0] == '(') {
                        push(SHT_OPERATOR);
                        p++;
                        next_token();
                        parse_stmt(true);
                        if (p[0] == ';') { push(SHT_OPERATOR); p++; }
                        next_token();
                        while (more() && p[0] != ';') {
                            next_token();
                            if (should_bail()) return;
                            parse_expr();
                        }
                        if (p[0] == ';') { push(SHT_OPERATOR); p++; }
                        next_token();
                        while (more() && p[0] != ')') {
                            next_token();
                            if (should_bail()) return;
                            parse_expr();
                        }
                        if (p[0] == ')') { push(SHT_OPERATOR); p++; }
                    }
                    next_token();
                    parse_stmt();
                    return;
                }
                bool is_declaration = false;
                if (is_params) is_declaration = true;

                Syntax_Highlight *maybe_type = nullptr;
                bool cant_move_type_forward = false;

                int paren_nesting = 0;

                while (more()) {
                    next_token();
                    auto next = push(SHT_IDENT);
                    if (c_starts_ident(p[0])) {
                        ident = read_ident();
                    } else {
                        ident.n = 0;
                    }
                    if (ident.n) {
                        if (!paren_nesting) {
                            // Type lags behind by 1 ident, and only outside parens
                            maybe_type = prev;
                        }
                        if (is_keyword(ident.p, ident.n)) {//if (is_type_keyword(ident.p, ident.n)) {
                            next->type = SHT_KEYWORD;
                            
                        } else {
                            //if (is_type(ident.p, ident.n)) {
                            //    next->type = SHT_TYPE;
                            //}
                        }
                        prev = next;
                    } else if (p[0] == '*' || p[0] == '&') {
                        next->type = SHT_OPERATOR;
                        p++; // Eat operator
                        while (p[0] == '*' || p[0] == '&') {
                            p++; // Eat operator
                            next_token();
                        }
                        cant_move_type_forward = true;
                    } else if (p[0] == '(') {
                        next->type = SHT_OPERATOR;
                        if (maybe_type && !paren_nesting) {
                            is_declaration = true;
                            if (prev->type != SHT_KEYWORD) prev->type = SHT_FUNCTION;
                            parse_params();
                            if (p[0] == ')') { push(SHT_OPERATOR); p++; } // @Hack?
                        } else {
                            p++; // Eat paren.
                            paren_nesting++;
                        }
                    } else if (p[0] == ')') {
                        next->type = SHT_OPERATOR;
                        if (paren_nesting) {
                            p++; // Eat paren.
                            paren_nesting--;
                        } else {
                            if (is_params) {
                                // We're the last parameter.
                                is_declaration = true;
                                break;
                            } else {
                                // Something's weird, eat paren and bail.
                                p++;
                                break;
                            }
                        }
                    } else if (p[0] == ';' || p[0] == '}' ||
                               (is_params && p[0] == ',' && !paren_nesting)) {
                        push(SHT_OPERATOR);
                        // Done.
                        if (!paren_nesting) {
                            if (maybe_type) is_declaration = true;
                        }
                        break;
                    } else if (p[0] == ',') {
                        push(SHT_OPERATOR);
                        p++; // Eat the comma
                        if (!paren_nesting) {
                            // Either a multiple declaration or a comma expression.
                            // We reassign prev back to whatever might be the type.
                            if (maybe_type) {
                                prev = maybe_type;
                            } else {
                                //prev = first;
                            }
                        }
                    } else if (p[0] == '{') { // This is a variable or function, hard to tell
                        push(SHT_OPERATOR);
                        parse_braces();
                        if (p[0] == '}') {
                            p++;
                            if (prev->type == SHT_FUNCTION) { // Don't expect a semicolon, just leave.
                                // (Only possible when is_declaration == true.)
                                break;
                            }
                            next_token();
                        }
                    } else if (p[0] == '[') {
                        push(SHT_OPERATOR);
                        p++; // Eat bracket
                        while (more() && p[0] != ';' && p[0] != '}'
                               && p[0] != ']') {
                            next_token();
                            parse_expr();
                        }
                        if (!paren_nesting) {
                            if (maybe_type) is_declaration = true;
                        }
                    } else if (p[0] == ']') {
                        push(SHT_OPERATOR);
                        p++; // Eat bracket
                    } else if (p[0] == '=') { // This is a variable for sure(?)
                        push(SHT_OPERATOR);
                        p++; // Eat operator
                        while (more()) {
                            next_token();
                            if (should_bail()) break;
                            parse_expr();
                        }
                        if (!paren_nesting) {
                            if (maybe_type) is_declaration = true;
                        }
                    } else if (p[0] == '#') {
                        // '#' inside decl/expr
                        while (p[0] == '#') {
                            p++;
                            scan_lexeme();
                        }
                    } else if (c_is_op(p[0])) {
                        push(SHT_OPERATOR);
                        p++;
                        while (more()) {
                            next_token();
                            if (should_bail()) break;
                            parse_expr();
                        }
                        is_declaration = false;
                    } else {
                        push(SHT_COMMENT);
                        while (more()) {
                            next_token();
                            if (should_bail()) break;
                            parse_stmt();
                        }
                        if (!paren_nesting) {
                            if (maybe_type) is_declaration = true;
                        }
                        break;
                    }
                }
                if (is_declaration) {
                    if (maybe_type) {
                        if (maybe_type->type != SHT_KEYWORD) {
                            maybe_type->type = SHT_TYPE;
                        }
                    } else {
                        // We only found a single ident. We'll treat it
                        // as the type name then, especially in params.
                        prev->type = SHT_TYPE;
                    }
                }
            }
        } else if (p[0] == ';' || p[0] == '}') {
            return;
        } else {
            parse_expr();
        }
    }
    void parse_buffer(Buffer& b) {
        if (b.allocated <= 0) return;
        move_gap_to_end(b); // @Temporary @Remove
        u32 *end = b.gap;
        u32 final_char = end[-1];
        end[-1] = BUFFER_STOP_PARSING;
    
        assert(b.syntax.count >= get_count(b));
        buffer_start = b.data;
        p = b.data;
        sh = b.syntax.data;

        next_token();
        for (;; next_token()) switch (p[0]) {
        case BUFFER_STOP_PARSING: goto done;
    
        default:
            assert(!is_whitespace(p[0]));
            //if (is_whitespace(p[0])) next_token();
            parse_stmt();
            // If we hit a semicolon/weirdness, skip it.
            if (should_bail()) { push(SHT_OPERATOR); p++; }
            break;
    
        case '#':
            parse_pp();
            break;
        }
    done:;
        end[-1] = final_char;
        p++;
        push(SHT_IDENT);
        b.syntax.count = sh - b.syntax.data;
    }
};


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "os.h"

void parse_syntax(Buffer* buffer) {
	if (!buffer->language) return;

    //array_empty(&buffer->syntax);
    array_empty(&buffer->macros);
    array_empty(&buffer->types);
    assert(buffer->language == "c" || buffer->language == "cpp");

    if (buffer->syntax.count < get_count(*buffer)) {
        auto old_cap = buffer->syntax.allocated;
        array_resize(&buffer->syntax, get_count(*buffer));
        if (buffer->syntax.allocated != old_cap)
            OutputDebugStringA("Reserving!!\n");
    }

    auto begin = os_get_time();

    Cpp_Lexer l;
    l.parse_buffer(*buffer);
    
    auto end = os_get_time();

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    
    double microseconds = (end - begin);
    microseconds *= 1000000;
    char buf[512];
    sprintf(buf, "%f microseconds.\n", microseconds);
    OutputDebugStringA(buf);
}
