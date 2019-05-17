#include "parsing.h"
#include "string.h"

/* Ascii table:
000x_xxxx [0 --- 8]tnvfr[14 ---------- 31]
001x_xxxx  !"#$%&'()*+,-./0123456789:;<=>?
010x_xxxx @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
011x_xxxx `abcdefghijklmnopqrstuvwxyz{|}~\127
*/
// -Unprintable (0-8,14-31,127)
// -Whitespace (9-13,32)
// -Symbol (33-35,37-47,58-63,91-94,123-126)
// -Digit (48-57)
// -Idenfitier (36,64-90,95-122)

bool is_eof(u32 c) {
	return c == EOF;
}

bool is_eol(u32 c) {
	return c == '\n' || c == '\r';
}

bool is_whitespace(u32 c) {
	return c >= '\t' && c <= '\r' || c == ' ';
}

bool is_letter(u32 c) {
    c |= 0x20;
	return c >= 'a' && c <= 'z';
}

bool is_oct_digit(u32 c) {
    return c >= '0' && c <= '7';
}

bool is_hex_digit(u32 c) {
    if (c >= '0' && c <= '9') return true;
    c |= 0x20;
    if (c >= 'a' && c <= 'f') return true;

    return false;
}

bool is_digit(u32 c) {
	return c >= '0' && c <= '9';
}

bool is_symbol(u32 c) {
    if (c >= '!' && c <= '/') return true;
    if (c >= ':' && c <= '?') return true;
    c |= 0x20;
    if (c >= '{' && c <= '~') return true;

    return false;
}

bool is_lowercase(u32 c) {
	return c >= 'a' && c <= 'z';
}

bool is_uppercase(u32 c) {
	return c >= 'A' && c <= 'Z';
}

u32 to_lowercase(u32 c) {
    u32 d = c | 0x20;
    if (d >= 'a' && d <= 'z') return d;

    return c;
}

u32 to_uppercase(u32 c) {
    u32 d = c & ~0x20;
    if (d >= 'A' && d <= 'Z') return d;

    return c;
}

#include "buffer.h"

// Unprintable, bad characters that we should pretend don't exist.
static bool c_should_skip(u32 c) {
    return (c >= 0 && c <= 8) || (c >= 14 && c <= 31) || c == 127;

    return false;
}

static bool c_starts_ident(u32 c) {
    return c >= 128 || c == '$' || (c >= '@' && c <= 'Z') || (c >= '_' && c <= 'z');
}
static bool c_is_ident(u32 c) {
    return c_starts_ident(c) || is_digit(c);
}

static bool c_is_op(u32 c) {
    return c != '$' && c != '#' && is_symbol(c);
}

static bool c_ends_statement(u32 c) {
    return c == ';' || c == '}';
}

static bool c_ends_expression(u32 c) {
    return c == ')' || c_ends_statement(c);
}

#define BUF_END 0xffffffff
#define BUF_GAP 0xfffffffe

#include "cpp_syntax.h"

#define C_KW_DECL(name) C##name,
enum C_Keyword {
    NOT_KEYWORD,

    Catomic_cancel,
    Catomic_commit,

    C_KEYWORDS(C_KW_DECL)
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

        C_KEYWORDS(KW_CASE);

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

    bool next() {
        if (p[0] == BUF_GAP) skip_gap();
        if (p[0] == BUF_END) return false;
        return true;
    }

    void scan_line_comment() {
        while (next()) {
            if (p[0] == '\\') {
                p++;
                if (p[0] == BUF_GAP) skip_gap();
                if (is_eol(p[0])) continue;
                break;
            } else if (is_eol(p[0])) {
                break;
            } else {
                p++;
                continue;
            }
        }
    }

    void scan_block_comment() {
        while (next()) {
            if (p[0] == '*') {
                p++;
                if (p[0] == BUF_GAP) skip_gap();
                if (p[0] != '/') continue;
                p++;
                break;
            } else {
                p++;
                continue;
            }
        }
    }

    // returns true if a new syntax highlight was pushed
    bool scan_lexeme() {
        Syntax_Highlight* comment = nullptr;
        while (next()) {
            if (p[0] == '/') {
                const u32* q = p + 1;
                if (q[0] == BUF_GAP) q += gap_size;
                if (q[0] == '/' || q[0] == '*') {
                    if (!comment) comment = push(SHT_COMMENT);
                    index_offset += q - p - 1;
                    p = q + 1;
                    if (q[0] == '/') {
                        scan_line_comment();
                        continue;
                    } else {
                        scan_block_comment();
                        continue;
                    }
                }
                break;
            }
            if (is_eol(p[0])) break;
            if (is_whitespace(p[0])) {
                p++;
                continue;
            }
            break;
        }
        return (comment != nullptr);
    }

    void scan_include_string() {
        u32 end = '"';
        if (p[0] == '<') end = '>';
        p++;
        while (next()) {
            if (skip_escaped_newline()) continue;
            if (is_eol(p[0])) break;
            p++;
            if (p[-1] == end) break;
        }
    }

    void parse_pp() {
        auto pound = push(SHT_DIRECTIVE);
        p++;
        scan_lexeme();
        auto directive = push(SHT_DIRECTIVE);
        C_Ident i = read_ident();
        scan_lexeme();
        switch (i.chunk[0]) {
        default:
            directive->type = SHT_IDENT; // invalid directive
        case KW_CHUNK1("elif"):
        case KW_CHUNK1("else"):
        case KW_CHUNK1("endif"):
        case KW_CHUNK1("error"):
        case KW_CHUNK1("if"):
        case KW_CHUNK1("line"):
        case KW_CHUNK1("pragma"):
            break; // valid directives
        case KW_CHUNK1("define"): {
            if (c_starts_ident(p[0])) {
                push(SHT_MACRO);
                skip_ident();
            }
            break;
        }
        case KW_CHUNK1("ifdef"):
        case KW_CHUNK1("ifndef"):
        case KW_CHUNK1("undef"): {
            if (c_starts_ident(p[0])) {
                push(SHT_IDENT);
                skip_ident();
            }
            break;
        }
        case KW_CHUNK1("include"): {
            if (p[0] == '"' || p[0] == '<') {
                push(SHT_STRING_LITERAL);
                scan_include_string();
            }
            break;
        }
        }
        push(SHT_DIRECTIVE);
        while (next()) {
            if (skip_escaped_newline()) continue;
            if (is_eol(p[0])) break;
            p++;
            if (scan_lexeme()) push(SHT_DIRECTIVE);
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

    bool skip_escaped_newline() {
        if (p[0] != '\\') return false;
        const u32* q = p + 1;
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
        while (next()) {
            if (skip_escaped_newline()) continue;
            if (!c_is_ident(p[0])) break;
            
            if (p[0] >= 128) result.check_keyword[0] = 0; // No keyword is Unicode
            if (result.n < 16) result.check_keyword[result.n] = p[0];
            p++;
            result.n++;
        }
        return result;
    }

    // Returns the number of identifier characters skipped.
    size_t skip_ident() {
        size_t n = 0;
        while (next()) {
            if (skip_escaped_newline()) continue;
            if (!c_is_ident(p[0])) break;
            p++;
            n++;
        }
        return n;
    }

    void parse_braces() {
        assert(p[0] == '{');
        push(SHT_OPERATOR);
        p++;
        while (next()) {
            next_token();
            parse_stmt();
            if (p[0] == ')') p++;
            if (p[0] == ';') p++;
            if (p[0] == '}') break;
        }
    }

    void parse_params() {
        assert(p[0] == '(');
        push(SHT_OPERATOR);
        p++;
        while (next()) {
            next_token();
            parse_stmt(SHT_OPERATOR, nullptr, true);
            if (c_ends_expression(p[0])) break;
        }
        if (p[0] == ')') {
            push(SHT_OPERATOR);
            p++;
            next_token();
        }
    }

    void parse_keyword(C_Keyword kw) {
        switch (kw) {
        case Cpublic:
        case Cprivate:
        case Cprotected:
            if (p[0] == ':') {
                push(SHT_OPERATOR);
                p++;
                next_token();
            }
            // fallthrough
        case Cdo: {
            parse_stmt();
            if (c_ends_statement(p[0])) {
                p++;
                next_token();
                parse_stmt();
            }
            return;
        }
        case Ctypedef: {
            parse_stmt(SHT_TYPE);
            return;
        }
        case Celse: {
            parse_stmt();
            return;
        }
        case Cif:
        case Cfor:
        case Cwhile:
        case Cswitch:
        case Ccatch: {
            if (p[0] == '(') {
                push(SHT_OPERATOR);
                p++;
                while (next()) {
                    next_token();
                    parse_stmt();
                    if (p[0] == '}') break;
                    if (p[0] == ';') {
                        push(SHT_OPERATOR);
                        p++;
                    }
                    if (p[0] == ')') {
                        push(SHT_OPERATOR);
                        p++;
                        next_token();
                        break;
                    }
                }
            }
            parse_stmt();
            return;
        }

        case Cthrow:
        case Creturn:
        case Ccase:
        case Cgoto:
        case Cbreak:
        case Ccontinue: {
            parse_exprs();
            return;
        }
        default: {
            return;
        }
        }
    }

    void parse_expr() {
        if (c_starts_ident(p[0])) {
            auto expr = push(SHT_IDENT);
            C_Ident i = read_ident();
            auto kw = keyword_check(i);

            next_token();

            if (kw) {
                expr->type = SHT_KEYWORD;
                parse_keyword(kw);
                return;
            } else if (p[0] == '(') {
                expr->type = SHT_FUNCTION;
                parse_expr();
            } else if (p[0] == '{') {
                expr->type = SHT_TYPE;
                parse_braces();
            }
        }

        next_token();

        if (p[0] == '(' || p[0] == '[') {
            push(SHT_OPERATOR);
            u32 end = ((p[0] == '(') ? ')' : ']');
            p++;
            next_token();
            parse_exprs(end);
            if (p[0] == end) {
                p++;
            }
        } else if (p[0] == '{') {
            parse_braces();
        } else if (c_ends_expression(p[0])) {
            return;
        } else if (p[0] == '"' || p[0] == '\'') {
            push(SHT_STRING_LITERAL);
            u32 end = p[0];
            p++;
            while (next()) {
                if (skip_escaped_newline()) continue;
                if (p[0] == '\\') {
                    p++;
                    if (p[0] == BUF_GAP) skip_gap();
                    continue;
                }
                if (is_eol(p[0])) break;
                if (p[0] == end) {
                    p++;
                    break;
                }
                p++;
            }
        } else if (c_is_op(p[0])) {
            push(SHT_OPERATOR);
            p++;
            next_token();
            parse_exprs();
        } else if (is_digit(p[0])) {
            push(SHT_NUMERIC_LITERAL);
            p++;
            while (next()) {
                if (skip_escaped_newline()) continue;

                if (is_digit(p[0]) || is_letter(p[0]) || p[0] == '.' || p[0] == '+' || p[0] == '\'') {
                    p++;
                    continue;
                } else {
                    break;
                }
            }
        } else { // @Hack?
            push(SHT_IDENT);
            p++;
            return;
        }

        next_token();

        if (!c_ends_expression(p[0]) && c_is_op(p[0])) {
            parse_expr();
        }
        //if (p[0] == ')') {
        //    push(SHT_OPERATOR);
        //    p++;
        //    next_token();
        //}
    }

    void parse_exprs(u32 end_on_char = BUF_END) {
        while (next()) {
            next_token();
            parse_expr();
            if (c_ends_expression(p[0])) break;
            if (p[0] == end_on_char) break;
        }
    }

    //Syntax_Highlight* parse_decl() {
    //    Syntax_Highlight* result = nullptr;
    //    return result;
    //}
    //Syntax_Highlight* parse_type() {
    //}

    void exit_parens_as_exprs(int paren_nesting) {
        while (paren_nesting) {
            parse_exprs();
            if (p[0] == ')') {
                push(SHT_OPERATOR);
                p++;
                next_token();
                paren_nesting--;
            } else {
                // End of statement
                break;
            }
        }
    }

    const char32_t* DBG_stmt_begin; // @Temporary @Debug
    void parse_stmt(Syntax_Highlight_Type mark_vars_as = SHT_IDENT, Syntax_Highlight** const decl_type_p = nullptr, bool inside_parameter_list = false) {
        DBG_stmt_begin = (const char32_t*)p;

        if (decl_type_p) *decl_type_p = nullptr;
        if (c_ends_statement(p[0])) return;

        if (p[0] == ')') {
            return;
        }

        if (p[0] == '{') {
            parse_braces();
            if (p[0] == '}') {
                push(SHT_OPERATOR);
                p++;
            }
            return;
        }

        if (!c_starts_ident(p[0])) {
            parse_exprs();
            if (p[0] == ')') {
                push(SHT_OPERATOR);
                p++;
                next_token();
            }
            return;
        }

        Syntax_Highlight* decl_type = nullptr;
        bool any_decl_var = false;

        int paren_nesting = 0;

        while (next()) {

            //assert(paren_nesting <= 1);

            auto current = push(mark_vars_as);
            C_Ident ident = read_ident();

#define DBG_BRKON(tok) if (static bool break_on_##tok = true) if (ident.chunk[0] == KW_CHUNK1(#tok) && ident.chunk[1] == KW_CHUNK2(#tok)) __debugbreak();
            //16 chrs:xxxxxxxxxxxxxxxx
            //DBG_BRKON(stb_c_lexer_get_);

            next_token();
            
            C_Keyword kw = keyword_check(ident);
            if (kw != NOT_KEYWORD) {
                current->type = SHT_KEYWORD;
                switch (kw) {
                case Ctypename:
                case Cextern:
                case Cstatic:
                case Cvolatile:
                case Cconst: {
                    continue;
                }
                case Cdecltype: return; // @Stub.

                case Cclass:
                case Cenum:
                case Cstruct:
                case Cunion: {
                    if (c_starts_ident(p[0])) {
                        current = push(SHT_TYPE);
                        C_Ident type_name = read_ident();
                        next_token();
                        decl_type = current;
                    }
                    if (p[0] == '{') {
                        parse_braces();
                        if (p[0] == '}') {
                            push(SHT_OPERATOR);
                            p++;
                            next_token();
                        }
                    }
                    continue;
                }

#define KW_TYPE_CASE(name) case C##name:
                C_TYPE_KEYWORDS(KW_TYPE_CASE)
                    if (!paren_nesting) {
                        decl_type = current;
                    }
                    continue;
                default:
                    parse_keyword(kw);
                    return;
                }
            }

#define SET_AS_DECL_EVEN_WITHOUT_VAR()                                         \
    do {                                                                       \
        if (decl_type) {                                                       \
            if (decl_type->type != SHT_KEYWORD)                                \
                decl_type->type = SHT_TYPE;                                    \
            if (decl_type_p)                                                   \
                *decl_type_p = decl_type;                                      \
        }                                                                      \
    } while (0)
#define SET_AS_DECL()                                                          \
    do {                                                                       \
        if (any_decl_var)                                                      \
            SET_AS_DECL_EVEN_WITHOUT_VAR();                                    \
    } while (0)

            if (ident.n) {
                if (!decl_type) {
                    decl_type = current;
                    if (p[0] == '(') {
                        current->type = SHT_FUNCTION;
                        push(SHT_OPERATOR);
                        p++;
                        next_token();
                        paren_nesting++;
                    }
                } else {
                    any_decl_var = true;
                    if (p[0] == '(') {
                        current->type = SHT_FUNCTION;
                        if (!paren_nesting) {
                            parse_params();
                        } else {
                            paren_nesting++;
                        }
                    }
                }
            } else if (p[0] == '{') {
                if (paren_nesting) {
                    exit_parens_as_exprs(paren_nesting);
                    parse_exprs();
                    break;
                } else {
                    parse_braces();
                    SET_AS_DECL();
                    break;
                }
            } else if (p[0] == '(') {
                current->type = SHT_OPERATOR;
                p++;
                next_token();
                paren_nesting++;
            } else if (p[0] == ')') {
                if (!paren_nesting && inside_parameter_list) {
                    SET_AS_DECL_EVEN_WITHOUT_VAR();
                    break;
                } else {
                    if (paren_nesting) {
                        paren_nesting--;
                        current->type = SHT_OPERATOR;
                        p++;
                        next_token();
                    } else {
                        break;
                    }
                }
            } else if (p[0] == '*' || p[0] == '&') {
                current->type = SHT_OPERATOR;
                p++;
                next_token();
            } else if (p[0] == '=') {
                current->type = SHT_OPERATOR;
                p++;
                next_token();
                if (!paren_nesting) {
                    SET_AS_DECL();
                    parse_expr();
                }
            } else if (p[0] == ';') {
                SET_AS_DECL();
                break;
            } else if (p[0] == ',') {
                current->type = SHT_OPERATOR;
                p++;
                next_token();
                if (!paren_nesting && inside_parameter_list) {
                    SET_AS_DECL_EVEN_WITHOUT_VAR();
                    break;
                } else if (!any_decl_var) {
                    // Just one ident + comma == stmt is just an expr.
                    parse_exprs();
                    break;
                } else {
                    if (paren_nesting) {
                        // Var name + comma inside parens == function call.
                        exit_parens_as_exprs(paren_nesting);
                        parse_exprs(); // parses rest of stmt
                        break;
                    } else { // Multiple declaration.
                        SET_AS_DECL();
                    }
                    continue;
                }
            } else if (p[0] == '}') {
                break;
            } else if (p[0] == '[') {
                if (any_decl_var) { // for sure array decl. @Todo: or not?
                    parse_expr(); // parses []
                    continue;
                } else { // array index
                    parse_exprs(); // parses [] then rest of stmt
                    break;
                }
            } else if (c_is_op(p[0])) {
                // doesn't look like a decl... function call?
                exit_parens_as_exprs(paren_nesting);
                parse_exprs(); // parses rest of stmt
                break;
            } else if (c_is_ident(p[0])) {
                continue;
            } else {
                // @Hack?
                current->type = SHT_IDENT;
                break;
            }
        }
        return;
    }

    void parse_buffer(Buffer& b) {
        if (b.allocated <= 0) return;
        //u32 *const end = b.data + b.allocated;
        u32 *const end = &b[get_count(b) - 1] + 1;
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
        while (next()) {
            next_token();
            parse_stmt();
            assert(p < end);

            assert(p[0] != ')'); // @Temporary @Debug
            if (p[0] == ')') {
                // something's malformed, just struggle on.
                push(SHT_OPERATOR);
                p++;
            }
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
