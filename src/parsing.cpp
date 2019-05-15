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

// Caution: this takes signed 32, because REASONS.
// (It's so that the function performs a signed compare
// in order to make sure that it doesn't match against
// the buffer end character, which has the high bit
// [and every other bit] set.) phil 2019-05-12
static bool c_starts_ident(s32 c) {
    if (c == '#') return true; // @Remove.
    if (c == '$') return true;
    if (c < '@') return false;
    if (c > 'Z' && c < '_') return false;
    if (c > 'z' && c < 256) return false;

    return true;
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

static bool c_ends_statement(u32 c) {
    if (c == ';') return true;
    if (c == '}') return true;

    return false;
}

#define BUF_END 0xffffffff
#define BUF_GAP 0xfffffffe

#include "cpp_syntax.h"

#define C_KW_DECL(name) C##name,
enum C_Keyword {
    NOT_KEYWORD,

    Catomic_cancel,
    Catomic_commit,

    CPP_KEYWORDS(C_KW_DECL)
};

struct C_Ident {
    const u32* p = nullptr;
    size_t n = 0;
    union {
        char check_keyword[16];
        u64 chunk[2];
    };
};

// This is THE FASTEST WAY to check if a token is a keyword.
// I wrote and profiled 2 other routines (200 lines). One was identical, the
// other slower.
C_Keyword keyword_check(const C_Ident& i) {
    if (i.n < 2 || i.n > 16) return NOT_KEYWORD;

    switch (i.chunk[0]) {

#define KW_CHUNK_(s, offset, I)                                                \
    ((I) + offset < (sizeof(s) - 1)                                            \
         ? (u64)(s)[(I) + offset] << (u64)((I)*8ull)                           \
         : 0ull)

#define KW_CHUNK(s, offset)                                                    \
    (KW_CHUNK_(s, offset, 0) | KW_CHUNK_(s, offset, 1) |                       \
     KW_CHUNK_(s, offset, 2) | KW_CHUNK_(s, offset, 3) |                       \
     KW_CHUNK_(s, offset, 4) | KW_CHUNK_(s, offset, 5) |                       \
     KW_CHUNK_(s, offset, 6) | KW_CHUNK_(s, offset, 7))

#define KW_CHUNK1(s) KW_CHUNK(s, 0)
#define KW_CHUNK2(s) KW_CHUNK(s, 8)

#define KW_CASE(x)                                                             \
    case KW_CHUNK1(#x):                                                        \
        if ((sizeof(#x) - 1) > 8)                                              \
            switch (i.chunk[1]) {                                              \
            case KW_CHUNK2(#x):                                                \
                return C##x;                                                   \
            default:                                                           \
                return NOT_KEYWORD;                                            \
            }                                                                  \
        else                                                                   \
            return C##x;

        CPP_KEYWORDS(KW_CASE);

    case KW_CHUNK1("atomic_commit"):
        switch (i.chunk[1]) {
        case KW_CHUNK2("atomic_commit"):
            return Catomic_commit;
        case KW_CHUNK2("atomic_cancel"):
            return Catomic_cancel;
        }

    default:
        return NOT_KEYWORD;
    }
}

struct C_Lexer {
    const u32 *index_offset = nullptr;
    const u32 *p = nullptr;
    Syntax_Highlight *sh = nullptr;
    size_t gap_size = 0;

    bool more() { return p[0] != BUF_END; }

    Syntax_Highlight* push(Syntax_Highlight_Type new_type) {
        sh->type = new_type;
        sh->where = (size_t)(p - index_offset);
        sh++;
        return sh - 1;
    }

    void skip_gap() {
        p += gap_size;
        index_offset += gap_size;
    }

    void scan_line_comment() {
        auto comment = push(SHT_COMMENT);
        comment->where -= 1; // Bit of a @Hack.
        u32 previous_char = p[0];
        for (;;) {
            switch (p[0]) {
            case BUF_GAP: skip_gap(); continue;
            case BUF_END: return;

            default:
                previous_char = p[0];
                p++;
                continue;

            case '\r':
            case '\n':
                if (previous_char == '\\') {
                    previous_char = p[0];
                    p++;
                    continue;
                } else {
                    return;
                }
            }
        }
    }
    void scan_block_comment() {
        auto comment = push(SHT_COMMENT);
        comment->where -= 1; // Bit of a @Hack.
        u32 previous_char = p[0];
        for (;;) {
            switch (p[0]) {
            case BUF_GAP: skip_gap(); continue;
            case BUF_END: return;

            default:
                previous_char = p[0];
                p++;
                continue;

            case '/':
                if (previous_char == '*') {
                    p++;
                    return;
                } else {
                    previous_char = p[0];
                    p++;
                    continue;
                }
            }
        }
    }

    // advance in the buffer, skipping whitespace & escaped newlines, and pushing comments.
    void scan_lexeme() {
        u32 previous_char = p[0];
        for (;;) {
            switch (p[0]) {
            case BUF_GAP: skip_gap(); continue;
            case BUF_END: default: return;
    
            case '/': { // divide, block comment, or line comment
                auto old_p = p;
                auto old_index_offset = index_offset;
                p++;

            retry_slash_lookahead:;
                switch (p[0]) {
                case BUF_GAP: skip_gap(); goto retry_slash_lookahead;
                case BUF_END: default: {
                    p = old_p;
                    index_offset = old_index_offset;
                    return;
                }
                case '/':
                    scan_line_comment();
                    continue;
                case '*':
                    scan_block_comment();
                    continue;
                }
            }
            case '\\': {
                auto old_p = p;
                auto old_index_offset = index_offset;
                p++;

            retry_escaped_newline_lookahead:;
                switch (p[0]) {
                case BUF_GAP: skip_gap(); goto retry_escaped_newline_lookahead;
                case BUF_END: default: {
                    p = old_p;
                    index_offset = old_index_offset;
                    return;
                }
                case '\r':
                case '\n':
                    p++;
                    continue;
                }
            }

            case ' ':
            case '\t':
                previous_char = p[0];
                p++;
                continue;
            }
        }
    }

    void parse_pp() {
        assert(p[0] == '#');
        push(SHT_DIRECTIVE);

        p++;
        scan_lexeme();

        push(SHT_DIRECTIVE);

        C_Ident i = read_ident();

        bool define = (i.chunk[0] == KW_CHUNK1("define"));
        bool include = (i.chunk[0] == KW_CHUNK1("include"));

        scan_lexeme();
        if (define) {
            if (c_starts_ident(p[0])) {
                push(SHT_MACRO);
                C_Ident macro = read_ident();
                scan_lexeme();
            }
        } else if (include) {
            if (p[0] == '"' || p[0] == '<') {
                u32 end = '"';
                if (p[0] == '<') end = '>';

                push(SHT_STRING_LITERAL);
                p++;
                scan_lexeme();

                for (;;) {
                    if (p[0] == BUF_END) {
                        return;

                    } else if (p[0] == BUF_GAP) {
                        skip_gap();
                        continue;

                    } else if (eat_escaped_newline_if_present()) {
                        continue;

                    } else if (is_eol(p[0])) {
                        break;

                    } else if (p[0] == end) {
                        p++;
                        break;

                    } else {
                        p++;
                        continue;

                    }
                }
            }
        }

        if (!is_eol(p[0])) {
            if (!more()) return;

            push(SHT_DIRECTIVE);
            while (!is_eol(p[0])) {
                if (!more()) return;

                p++;
                scan_lexeme();
            }
        }
    }

    void next_token() {
        scan_lexeme();
        while (is_eol(p[0])) {
            p++;
            scan_lexeme();
            if (p[0] == '#') parse_pp();
        }
    }

    bool eat_escaped_newline_if_present() {
        if (p[0] != '\\') return false;

        const u32 *q = p + 1;
        if (q[0] == BUF_GAP) q += gap_size;

        if (!is_eol(q[0])) return false;

        // Calculate the amount we skipped via gaps.
        index_offset += q - p - 1;
        p = q + 1;
        return true;
    }

    C_Ident read_ident() {
        C_Ident result = {0}; // zero the check_keyword array
        result.p = p;
        for (;;) {
            if (p[0] == BUF_GAP) {
                skip_gap();
                continue;

            } else if (p[0] == BUF_END) {
                break;

            } else if (p[0] >= 128) { // Unicode: we're good, but it's not a keyword.
                result.check_keyword[0] = 0;

                p++;
                result.n++;
                continue;

            } else if (c_is_ident(p[0])) {
                if (result.n < sizeof(result.check_keyword)) {
                    result.check_keyword[result.n] = p[0];
                }
                
                p++;
                result.n++;
                continue;

            } else if (eat_escaped_newline_if_present()) {
                continue;

            } else {
                break;

            }
        }
        return result;
    }

    void parse_braces() {
        assert(p[0] == '{');
        push(SHT_OPERATOR);
        p++;
        while (p[0] != '}') {
            if (!more()) return;

            next_token();
            parse_stmt();

            if (p[0] == ';') { push(SHT_OPERATOR); p++; }
        }
    }

    // Returns true if these look like valid params.
    bool parse_params() {
        assert(p[0] == '(');
        push(SHT_OPERATOR);
        p++;
        bool valid = true;
        while (p[0] != ')') {
            if (!more()) return true;

            next_token();
            parse_stmt(true, &valid);

            if (p[0] == ',') { push(SHT_OPERATOR); p++; }

            if (c_ends_statement(p[0])) return false;
        }
        return valid;
    }

    void parse_expr() { // @Temporary @Stub
        if (p[0] == BUF_END) {
            return;

        } else if (p[0] == BUF_GAP) {
            skip_gap();
            parse_expr();
            return;

        } else if (c_ends_statement(p[0])) {
            return;

        } else if (p[0] == '(') {
            push(SHT_OPERATOR);
            p++;

            while (p[0] != ')') {
                if (!more()) return;

                next_token();
                parse_expr();

                if (c_ends_statement(p[0])) return;
            }
            if (p[0] == ')') { push(SHT_OPERATOR); p++; }
            return;

        } else if (p[0] == '[') {
            push(SHT_OPERATOR);
            p++;

            while (p[0] != ']') {
                if (!more()) return;

                next_token();
                parse_expr();

                if (c_ends_statement(p[0])) return;
            }
            if (p[0] == ']') { push(SHT_OPERATOR); p++; }
            return;

        } else if (p[0] == '{') {
            parse_braces();
            return;

        } else if (p[0] == ')' || p[0] == ']') {
            push(SHT_OPERATOR);
            p++;
            return;

        } else if (p[0] == '"' || p[0] == '\'') {
            u32 end = p[0];
            push(SHT_STRING_LITERAL);
            p++;

            for (;;) {
                if (p[0] == BUF_END) {
                    return;

                } else if (p[0] == BUF_GAP) {
                    skip_gap();
                    continue;

                } else if (p[0] == '\\') {
                    // This naturally handles escaped newlines.
                    p++;
                    if (p[0] == BUF_GAP) skip_gap();
                    if (p[0] == BUF_END) return;
                    p++;
                    continue;
                    
                } else if (is_eol(p[0])) {
                    break;

                } else if (p[0] == end) {
                    break;

                } else {
                    p++;
                    continue;

                }
            }
            if (p[0] == end) p++;
            return;

        } else if (p[0] == '#') {
            push(SHT_IDENT);
            p++;

            while (p[0] == '#') {
                if (!more()) return;

                p++;
                next_token();
            }
            return;

        } else if (c_is_op(p[0])) {
            push(SHT_OPERATOR);
            p++;
            return;

        } else if (is_number(p[0])) { // @Todo: real C++ number parsing
            push(SHT_NUMERIC_LITERAL);
            p++;

            for (;;) {
                if (p[0] == BUF_END) {
                    return;

                } else if (p[0] == BUF_GAP) {
                    skip_gap();
                    continue;

                } else if (c_is_ident(p[0])) {
                    p++;
                    continue;

                } else {
                    break;

                }

            }
            return;

        } else if (c_starts_ident(p[0])) {
            auto w = push(SHT_IDENT);
            C_Ident i = read_ident();

            next_token();
            if (keyword_check(i) != NOT_KEYWORD) {
                w->type = SHT_KEYWORD;
            } else if (p[0] == '(') {
                w->type = SHT_FUNCTION;
                parse_expr();
            } else if (p[0] == '{') {
                w->type = SHT_TYPE;
            } else if (p[0] == '*' || p[0] == '&') { // Check pointer cast.
                push(SHT_OPERATOR);
                while (p[0] == '*' || p[0] == '&') {
                    p++;
                    next_token();
                }
                if (p[0] == ')') {
                    w->type = SHT_TYPE;
                }
            }
            return;

        } else {
            push(SHT_IDENT);
            // um... bump pointer?
            p++;
            return;

        }
    }

    void parse_stmt(bool is_params = false, bool *is_valid_decl = nullptr) {
        if (c_ends_statement(p[0])) return;
        if (!c_starts_ident(p[0])) {
            if (is_valid_decl) *is_valid_decl = false;
            parse_expr();
            return;
        }

        auto first = push(SHT_IDENT);
        auto prev = first;
        C_Ident ident = read_ident();
            
        switch (keyword_check(ident)) {
        case Creturn:
        case Cbreak:
        case Ccontinue: {
            if (is_valid_decl) *is_valid_decl = false;
            prev->type = SHT_KEYWORD;
            next_token();

            while (!c_ends_statement(p[0])) {
                if (!more()) return;

                next_token();
                parse_expr();
            }
            return;

        }
        case Cif:
        case Cwhile:
        case Cswitch: {
            if (is_valid_decl) *is_valid_decl = false;
            prev->type = SHT_KEYWORD;
            next_token();

            if (p[0] == '(') {
                push(SHT_OPERATOR);
                p++;
                next_token();

                parse_stmt(true);

                if (p[0] != ')') return;

                push(SHT_OPERATOR);
                p++;
                next_token();

                parse_stmt();
            }
            return;

        }
        case Cfor: {
            if (is_valid_decl) *is_valid_decl = false;
            prev->type = SHT_KEYWORD;
            next_token();

            if (p[0] == '(') {
                push(SHT_OPERATOR);
                p++;
                next_token();

                parse_stmt(true);

                if (p[0] != ';') return;
                
                push(SHT_OPERATOR);
                p++;
                next_token();

                while (!c_ends_statement(p[0])) {
                    if (!more()) return;

                    next_token();
                    parse_expr();
                }

                if (p[0] != ';') return;
                
                push(SHT_OPERATOR);
                p++;
                next_token();

                while (p[0] != ')') {
                    if (!more()) return;

                    next_token();
                    parse_expr();

                    if (c_ends_statement(p[0])) return;
                }
                
                push(SHT_OPERATOR);
                p++;
            }
            return;

        }
        case Ccase: {
            prev->type = SHT_KEYWORD;
            next_token();

            parse_expr();

            if (p[0] == ':') {
                push(SHT_OPERATOR);
                p++;
                next_token();
                parse_stmt();
            }
            return;

        }
        case Cdefault: {
            prev->type = SHT_KEYWORD;
            next_token();

            if (p[0] == ':') {
                push(SHT_OPERATOR);
                p++;
                next_token();
                parse_stmt();
            }
            return;

        }
        default:
            prev->type = SHT_KEYWORD;
        case NOT_KEYWORD:
        // If both of these are false, the answer is "maybe".
        bool is_declaration = false;
        bool not_declaration = false;
        //if (is_params) is_declaration = true;

        Syntax_Highlight *maybe_type = nullptr;
        bool cant_move_type_forward = false;

        Syntax_Highlight *maybe_function_call = nullptr;

        int paren_nesting = 0;

        next_token();
        if (p[0] == ':') { // goto label
            prev->type = SHT_OPERATOR;
            p++;
            next_token();
            parse_stmt();
            return;
        }

        for (;;) {
            if (!more()) return;

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
                if (keyword_check(ident) != NOT_KEYWORD) {//if (is_type_keyword(ident.p, ident.n)) {
                    next->type = SHT_KEYWORD;
                            
                } else {
                    //if (is_type(ident.p, ident.n)) {
                    //    next->type = SHT_TYPE;
                    //}
                }
                prev = next;
            } else if (p[0] == '*' || p[0] == '&') {
                next->type = SHT_OPERATOR;
                p++;
                cant_move_type_forward = true;
            } else if (p[0] == '(') {
                next->type = SHT_OPERATOR;
                if (maybe_type) {
                    if (!paren_nesting) {
                        if (maybe_type == prev) {
                            if (prev->type != SHT_KEYWORD) prev->type = SHT_FUNCTION;
                            parse_expr();
                            not_declaration = true;
                            maybe_function_call = maybe_type; // @Hack
                        } else {
                            if (prev->type != SHT_KEYWORD) prev->type = SHT_FUNCTION;
                            is_declaration = true;
                            if (!parse_params()) {
                                not_declaration = true;
                            }
                            if (p[0] == ')') { push(SHT_OPERATOR); p++; } // @Hack?
                        }
                    } else {
                        p++;
                        paren_nesting++;
                    }
                } else {
                    p++; // Eat paren.
                    paren_nesting++;
                        
                    maybe_function_call = prev;
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
                if (p[0] == '}') {
                    not_declaration = true;
                }
                break;
            } else if (p[0] == ',') {
                push(SHT_OPERATOR);
                p++;
                if (!paren_nesting) {
                    // Either a multiple declaration or a comma expression.
                    // We reassign prev back to whatever might be the type.
                    if (maybe_type) {
                        prev = maybe_type;
                    } else {
                        //prev = first;
                    }
                } else {
                    assert(maybe_function_call);
                    if (paren_nesting == 1) {
                        not_declaration = true;
                        //maybe_function_call = maybe_type;
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
                p++;
                while (p[0] != ']') {
                    if (!more()) return;

                    next_token();
                    parse_expr();

                    if (c_ends_statement(p[0])) break;
                }
                if (!paren_nesting) {
                    if (maybe_type) is_declaration = true;
                }
            } else if (p[0] == ']') {
                push(SHT_OPERATOR);
                p++;
            } else if (p[0] == '=') { // This is a variable for sure(?)
                push(SHT_OPERATOR);
                p++;
                while (!c_ends_statement(p[0])) {
                    if (!more()) return;

                    next_token();
                    parse_expr();
                }
                if (!paren_nesting) {
                    if (maybe_type) is_declaration = true;
                }
            } else if (p[0] == '#') {
                while (p[0] == '#') {
                    if (!more()) return;

                    p++;
                    scan_lexeme();
                }
            } else if (c_is_op(p[0])) {
                while (!c_ends_statement(p[0])) {
                    if (!more()) return;

                    next_token();
                    parse_expr();
                }
                not_declaration = true;
            } else if (is_number(p[0])) {
                parse_expr();
                if (!paren_nesting) {
                    not_declaration = true;
                }
            } else {
                // something weird like ascii 0x01: ignore it
                p++;
            }
        }
        if (!not_declaration && is_declaration) {
            if (maybe_type) {
                if (maybe_type->type != SHT_KEYWORD) {
                    maybe_type->type = SHT_TYPE;
                }
            } else {
                // We only found a single ident. We'll treat it
                // as the type name then, especially in params.
                prev->type = SHT_TYPE;
            }
        } else {
            if (is_valid_decl) *is_valid_decl = false;
            if (maybe_function_call) {
                first->type = SHT_FUNCTION;
            }
        }
        }
    }
    void parse_buffer(Buffer& b) {
        if (b.allocated <= 0) return;
        //move_gap_to_end(b); // @Temporary @Remove
        u32 *const end = &b[get_count(b) - 1] + 1; // will skip gap
        const u32 final_char = end[-1];
        end[-1] = BUF_END;
    
        assert(b.syntax.count >= get_count(b));
        index_offset = b.data;
        p = b.data;
        sh = b.syntax.data;
        gap_size = b.gap_size;
        if (gap_size) {
            b.gap[0] = BUF_GAP;
        }

        scan_lexeme();
        if (p[0] == '#') parse_pp();
        while (more()) {
            next_token();
            parse_stmt();

            if (c_ends_statement(p[0])) {
                push(SHT_OPERATOR);
                p++;
            }
        }

        end[-1] = final_char;
        p = end;
        push(SHT_IDENT);
        p++;
        push(SHT_IDENT); // @Hack: why is this necessary?
        b.syntax.count = sh - b.syntax.data;
    }
};

#include "os.h"
void parse_syntax(Buffer* buffer) {
	if (!buffer->language) return;

    assert(buffer->language == "c" ||
           buffer->language == "h" ||
           buffer->language == "cpp" ||
           buffer->language == "hpp" ||
           buffer->language == "cc" ||
           buffer->language == "hh" ||
           buffer->language == "cxx" ||
           buffer->language == "hxx");

    if (buffer->syntax.count < get_count(*buffer)) {
        auto old_cap = buffer->syntax.allocated;
        array_resize(&buffer->syntax, get_count(*buffer));
        //if (buffer->syntax.allocated != old_cap)
        //    OutputDebugStringA("Reserving!!\n");
    }

    double begin = os_get_time();

    C_Lexer l;
    l.parse_buffer(*buffer);
    
    double end = os_get_time();

    buffer->loc_s = buffer->eol_table.count / (end - begin);
    buffer->chars_s = get_count(*buffer) / (end - begin);
    buffer->syntax_is_dirty = false;
    //double microseconds = (end - begin);
    //microseconds *= 1000000;
    //char buf[512];
    //sprintf(buf, "%f microseconds.\n", microseconds);
    //OutputDebugStringA(buf);
}
