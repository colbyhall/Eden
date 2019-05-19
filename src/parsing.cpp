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

enum C_Char_Type {
    CT_WHITE = 0,
    CT_OP = 1,
    CT_DIGIT = 2,
    CT_IDENT = 3,
};
C_Char_Type c_char_type(u8 c) {

    static u8 lut[256] = {
    //[ 0   - - -   8 ] t n v f r [ 1 4   - - - - - - - - - -   3 1 ]
    //  ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
    //@ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
    //` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ \127
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,3,
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,0,
      // 128 and greater: all identifier
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    };
    return (C_Char_Type)lut[c];
}
// Unprintable, bad characters that we should pretend don't exist.
//static bool c_should_skip(u32 c) {
//    return (c >= 0 && c <= 8) || (c >= 14 && c <= 31) || c == 127;
//
//    return false;
//}

#if 0
static bool c_starts_ident(u32 c) {
    //return c >= 128 || c == '$' || (c >= '@' && c <= 'Z') || (c >= '_' && c <= 'z');
    c |= 0x40;
    return c >= '`' && c <= 'z' || c == 127;
}
bool c_is_digit(u32 c) {
	return c >= '0' && c <= '9';
}
static bool c_is_ident(u32 c) {
    return c_starts_ident(c) || c_is_digit(c);
}

static bool c_is_op(u32 c) {
    return c != '$' && c != '#' && is_symbol(c);
    //return is_symbol(c);
}
bool c_is_eol(u32 c) {
	return c == '\n' || c == '\r';
}

bool c_is_whitespace(u32 c) {
	return c >= '\t' && c <= '\r' || c == ' ';
}
static bool c_ends_statement(u32 c) { return c == ';' || c == '}'; }
static bool c_ends_expression(u32 c) { return c == ')' || c_ends_statement(c); }
#else
static bool c_starts_ident(u8 c) { return c_char_type(c) == CT_IDENT; }
static bool c_is_ident(u8 c) { return c_char_type(c) >= CT_DIGIT; }
static bool c_is_digit(u8 c) { return c_char_type(c) == CT_DIGIT; }
static bool c_is_op(u8 c) { return c_char_type(c) == CT_OP; }
static bool c_is_eol(u32 c) { return c == '\n'; }
static bool c_is_whitespace(u8 c) { return c_char_type(c) == CT_WHITE; }
static bool c_ends_statement(u32 c) { return c == ';' || c == '}'; }
static bool c_ends_expression(u32 c) { return c == ')' || c_ends_statement(c); }
#endif

//#define BUF_END 0xffffffff
//#define BUF_GAP 0xfffffffe
#define BUF_END 0xff

#include "cpp_syntax.h"

#define C_KW_DECL(name) C##name,
enum C_Keyword {
    NOT_KEYWORD,

    Catomic_cancel,
    Catomic_commit,

    C_KEYWORDS(C_KW_DECL)
};

struct C_Ident {
    //const u32* p = nullptr;
    const u32* p = nullptr;
    size_t n = 0;
    union {
        char check_keyword[16];
        u64 chunk[2];
    };
};

// 512 bytes
static const u64 (chunk_masks[2])[32] = {{
    0x0000000000000000, 0x00000000000000ff, 0x000000000000ffff, 0x0000000000ffffff,
    0x00000000ffffffff, 0x000000ffffffffff, 0x0000ffffffffffff, 0x00ffffffffffffff,
    0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff,
    0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff,
    0xffffffffffffffff,},{
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x00000000000000ff, 0x000000000000ffff, 0x0000000000ffffff,
    0x00000000ffffffff, 0x000000ffffffffff, 0x0000ffffffffffff, 0x00ffffffffffffff,
    0xffffffffffffffff,}
};

static u64 get_chunk0(const C_Ident& i) {
    //return *(u64*)i.p & chunk_masks[0][i.n];
    return i.chunk[0];
}
static u64 get_chunk1(const C_Ident& i) {
    //return *(u64*)(i.p + 8) & chunk_masks[1][i.n];
    return i.chunk[1];
}

// This is THE FASTEST WAY to check if a token is a keyword.
// I wrote and profiled 2 other routines (200 lines). One was identical, the
// other slower.
C_Keyword keyword_check(const C_Ident& i) {
    if (i.n < 2 || i.n > 16) return NOT_KEYWORD;

    switch (get_chunk0(i)) {

#define KW_CHUNK_(s, offset, I)                                                \
    ((I) + offset < (sizeof(s) - 1)                                            \
         ? (u64)(s)[(I) + offset] << (u64)((I)*8ull)                           \
         : 0ull)

#define KW_CHUNK(s, offset)                                                    \
    (KW_CHUNK_(s, offset, 0) | KW_CHUNK_(s, offset, 1) |                       \
     KW_CHUNK_(s, offset, 2) | KW_CHUNK_(s, offset, 3) |                       \
     KW_CHUNK_(s, offset, 4) | KW_CHUNK_(s, offset, 5) |                       \
     KW_CHUNK_(s, offset, 6) | KW_CHUNK_(s, offset, 7))

#define KW_CHUNK0(s) KW_CHUNK(s, 0)
#define KW_CHUNK1(s) KW_CHUNK(s, 8)

#define KW_CASE(x)                                                             \
    case KW_CHUNK0(#x):                                                        \
        if ((sizeof(#x) - 1) > 8)                                              \
            switch (get_chunk1(i)) {                                           \
            case KW_CHUNK1(#x):                                                \
                return C##x;                                                   \
            default:                                                           \
                return NOT_KEYWORD;                                            \
            }                                                                  \
        else                                                                   \
            return C##x;

        C_KEYWORDS(KW_CASE);

    case KW_CHUNK0("atomic_c"):
        switch (get_chunk1(i)) {
        case KW_CHUNK1("atomic_commit"):
            return Catomic_commit;
        case KW_CHUNK1("atomic_cancel"):
            return Catomic_cancel;
        }
        break;

    default:
        return NOT_KEYWORD;
    }
    return NOT_KEYWORD;
}

struct C_Lexer {
    const u32* index_offset = nullptr;
    const u32* p = nullptr;
    const u32* buf_gap = nullptr;
    const u32* buf_end = nullptr;
    Syntax_Highlight *sh = nullptr;
    size_t gap_size = 0;

    Syntax_Highlight* push(Syntax_Highlight_Type new_type) {
        sh->type = new_type;
        sh->where = p;
        sh++;
        return sh - 1;
    }

    void skip_gap() {
        p += gap_size;
        index_offset += gap_size;
    }

    bool next() {
        if (p == buf_gap) skip_gap();
        if (p >= buf_end) return false;
        return true;
    }

    void scan_line_comment() {
        while (next()) {
            if (p[0] == '\\') {
                p++;
                if (p == buf_gap) skip_gap();
                if (c_is_eol(p[0])) {
                    p++;
                    continue;
                }
                break;
            } else if (c_is_eol(p[0])) {
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
                if (p == buf_gap) skip_gap();
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
                auto q = p + 1;
                if (q == buf_gap) q += gap_size;
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
            if (c_is_eol(p[0])) break;
            if (c_is_whitespace(p[0])) {
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
            if (c_is_eol(p[0])) break;
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
        switch (get_chunk0(i)) {
        default:
            directive->type = SHT_IDENT; // invalid directive
        case KW_CHUNK0("elif"):
        case KW_CHUNK0("else"):
        case KW_CHUNK0("endif"):
        case KW_CHUNK0("error"):
        case KW_CHUNK0("if"):
        case KW_CHUNK0("line"):
        case KW_CHUNK0("pragma"):
            break; // valid directives
        case KW_CHUNK0("define"): {
            if (c_starts_ident(p[0])) {
                push(SHT_MACRO);
                skip_ident();
            }
            break;
        }
        case KW_CHUNK0("ifdef"):
        case KW_CHUNK0("ifndef"):
        case KW_CHUNK0("undef"): {
            if (c_starts_ident(p[0])) {
                push(SHT_IDENT);
                skip_ident();
            }
            break;
        }
        case KW_CHUNK0("include"): {
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
            if (c_is_eol(p[0])) break;
            p++;
            if (scan_lexeme()) push(SHT_DIRECTIVE);
        }
    }

    void next_token() {
        scan_lexeme();
        while (c_is_eol(p[0])) {
            p++;
            scan_lexeme();
            if (p[0] == '#') parse_pp();
        }
    }

    bool skip_escaped_newline() {
        if (p[0] != '\\') return false;
        auto q = p + 1;
        if (q == buf_gap) q += gap_size;
        if (!c_is_eol(q[0])) return false;
        // Calculate the amount we skipped via gaps.
        index_offset += q - p - 1;
        p = q + 1;
        return true;
    }

    C_Ident read_ident() {
        C_Ident result; // zero the check_keyword array
        result.p = p;
        result.n = 0;
        result.chunk[0] = 0;
        result.chunk[1] = 0;
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
            if (p[0] == ')') {
                push(SHT_OPERATOR);
                p++;
            }
            if (p[0] == ';') {
                push(SHT_OPERATOR);
                p++;
            }
            if (p[0] == '}') break;
        }
    }

    void parse_params() {
        assert(p[0] == '(');
        push(SHT_OPERATOR);
        p++;
        while (next()) {
            next_token();
            parse_stmt(SHT_PARAMETER, nullptr, true);
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

    void parse_expr(bool inside_parameters = false) {
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
                push(SHT_OPERATOR);
                if (p[0] == ')') {
                    p++;
                    next_token();
                    // Check for ident (cast expression).
                    if (!c_ends_expression(p[0])) {
                        if (c_is_op(p[0]) || c_is_ident(p[0])) {
                            parse_expr();
                        }
                    }
                } else {
                    p++;
                    next_token();
                }
            }
        } else if (p[0] == '{') {
            parse_braces();
        } else if (c_ends_expression(p[0])) {
            return;
        } else if (p[0] == ',' && inside_parameters) {
            return;
        } else if (p[0] == '"' || p[0] == '\'') {
            push(SHT_STRING_LITERAL);
            u32 end = p[0];
            p++;
            while (next()) {
                if (skip_escaped_newline()) continue;
                if (p[0] == '\\') {
                    p++;
                    if (p == buf_gap) skip_gap();
                    continue;
                }
                if (c_is_eol(p[0])) break;
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
        } else if (c_is_digit(p[0])) {
            push(SHT_NUMERIC_LITERAL);
            p++;
            while (next()) {
                if (skip_escaped_newline()) continue;

                if (c_is_ident(p[0]) || p[0] == '.' || p[0] == '+' || p[0] == '\'') {
                    p++;
                    continue;
                } else {
                    break;
                }
            }
        } else if (c_is_ident(p[0])) {
            return;
        } else { // @Hack?
            push(SHT_IDENT);
            p++;
            return;
        }

        next_token();

        if (!c_ends_expression(p[0])) {
            if (c_is_op(p[0])) {
                parse_expr();
            }
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

#ifndef NDEBUG
    const char32_t* DBG_stmt_begin; // @Debug
#endif
    void parse_stmt(Syntax_Highlight_Type mark_vars_as = SHT_IDENT, Syntax_Highlight** const decl_type_p = nullptr, bool inside_parameters = false) {
#ifndef NDEBUG
        DBG_stmt_begin = (const char32_t*)p; // @Debug
#endif

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

#define DBG_BRKON(tok) if (static bool break_on_##tok = true) if (ident.chunk[0] == KW_CHUNK0(#tok) && ident.chunk[1] == KW_CHUNK1(#tok)) __debugbreak();
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
                if (!paren_nesting && inside_parameters) {
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
                    parse_expr(true);
                }
            } else if (p[0] == ';') {
                SET_AS_DECL();
                break;
            } else if (p[0] == ',') {
                current->type = SHT_OPERATOR;
                p++;
                next_token();
                if (!paren_nesting && inside_parameters) {
                    SET_AS_DECL_EVEN_WITHOUT_VAR();
                    break;
                } else if (!any_decl_var) {
                    if (inside_parameters) {
                        // We aren't a default-arg ('=' would've happened already),
                        // so we are inside a nested param list. Parse them as decls.
                        while (next()) {
                            parse_stmt(SHT_PARAMETER, nullptr, true);
                            next_token();
                            if (c_ends_expression(p[0])) {
                                push(SHT_OPERATOR);
                                p++;
                                break;
                            }
                        }
                        continue;
                    } else {
                        // Just one ident + comma == stmt is just an expr.
                        parse_exprs();
                        break;
                    }
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

enum State {
    STATE_NEWLINE = 0,
    STATE_PREPROC = 1,
    STATE_PREPROC_BACKSLASH = 2,
    STATE_SEEN_SLASH = 3,
    STATE_COMMENT_LINE = 4,
    STATE_COMMENT_LINE_BACKSLASH = 5,
    STATE_COMMENT_BLOCK = 6,
    STATE_COMMENT_BLOCK_STAR = 7,

    STATE_DECL_TYPE = 8,
    //STATE_DECL_VAR = 9,

    NUM_STATES
};
static State
parse_dfa(State initial_state,
          const u32* p,
         const u32* const pend,
          Syntax_Highlight*& sh) {
    enum Char_Type {
        IDENT = 0, // ident
        NEWLINE = 1, // '\r' '\n'
        SLASH = 2, // '/'
        BACKSLASH = 3, // '\\'
        STAR = 4, // '*'
        POUND = 5, // '#'
        WHITE = 6, // anything below 32 besides newline 
        DIGIT = 7, // '0' '1' '2' '3' '4' '5' '6' '7' '8' '9'
        QUOTE = 8, // '\'' '"'
        LPAREN = 9, // '('
        LSQUARE = 10, // '['
        LCURLY = 11, // '{'
        LTRI = 12, // '<'
        RPAREN = 13, // ')'
        RSQUARE = 14, // ']'
        RCURLY = 15, // '}'
        RTRI = 16, // '>'
        AND = 17, // '&'
        EQU = 18, // '='
        SEMI = 19, // ';'
        COMMA = 20, // ','
        MISCOP = 21, // '~' '!' '%' '^' '-' '+' '|' ':' '.' '?'
        NUM_CHAR_TYPES
    };
    static const unsigned char char_type[128] {
//[0 ----------- 8] t n v f r [14 --------------------------- 31]
  6,6,6,6,6,6,6,6,6,6,1,6,6,1,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
//   ! " # $  %  & ' (  ) *  +  ,  -  . / 0 1 2 3 4 5 6 7 8 9  :  ;  <  =  >  ?
  6,21,8,5,0,21,17,8,9,13,4,21,20,21,21,2,7,7,7,7,7,7,7,7,7,7,21,19,12,18,16,21,
//@ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z  [  \  ]  ^ _
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10, 3,14,21,0,
//` a b c d e f g h i j k l m n o p q r s t u v w x y z  {  |  }  ~ \127
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,21,15,21,
  0,//6,
//// 128 and greater: all identifier
//6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
//6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
};
static const u8 (state_table[NUM_STATES])[NUM_CHAR_TYPES] = {
    { // STATE_NEWLINE
        /* IDENT */ STATE_NEWLINE, // @Temporary
        /* NEWLINE */ STATE_NEWLINE,
        /* SLASH */ STATE_SEEN_SLASH,
        /* BACKSLASH */ STATE_NEWLINE,
        /* STAR */ STATE_NEWLINE,
        /* POUND */ STATE_PREPROC,
        /* WHITE */ STATE_NEWLINE,
        /* DIGIT */ STATE_NEWLINE, // @Temporary
        /* QUOTE */ STATE_NEWLINE, // @Temporary
        /* LPAREN */ STATE_NEWLINE, // @Temporary
        /* LSQUARE */ STATE_NEWLINE, // @Temporary
        /* LCURLY */ STATE_NEWLINE, // @Temporary
        /* LTRI */ STATE_NEWLINE, // @Temporary
        /* RPAREN */ STATE_NEWLINE, // @Temporary
        /* RSQUARE */ STATE_NEWLINE, // @Temporary
        /* RCURLY */ STATE_NEWLINE, // @Temporary
        /* RTRI */ STATE_NEWLINE, // @Temporary
        /* AND */ STATE_NEWLINE, // @Temporary
        /* EQU */ STATE_NEWLINE, // @Temporary
        /* SEMI */ STATE_NEWLINE, // @Temporary
        /* COMMA */ STATE_NEWLINE, // @Temporary
        /* MISCOP */ STATE_NEWLINE, // @Temporary
    },
    { // STATE_PREPROC
        /* IDENT */ STATE_PREPROC,
        /* NEWLINE */ STATE_NEWLINE,
        /* SLASH */ STATE_PREPROC,
        /* BACKSLASH */ STATE_PREPROC_BACKSLASH,
        /* STAR */ STATE_PREPROC,
        /* POUND */ STATE_PREPROC,
        /* WHITE */ STATE_PREPROC,
        /* DIGIT */ STATE_PREPROC,
        /* QUOTE */ STATE_PREPROC,
        /* LPAREN */ STATE_PREPROC,
        /* LSQUARE */ STATE_PREPROC,
        /* LCURLY */ STATE_PREPROC,
        /* LTRI */ STATE_PREPROC,
        /* RPAREN */ STATE_PREPROC,
        /* RSQUARE */ STATE_PREPROC,
        /* RCURLY */ STATE_PREPROC,
        /* RTRI */ STATE_PREPROC, 
        /* AND */ STATE_PREPROC, 
        /* EQU */ STATE_PREPROC, 
        /* SEMI */ STATE_PREPROC, 
        /* COMMA */ STATE_PREPROC, 
        /* MISCOP */ STATE_PREPROC, 
    },
    { // STATE_PREPROC_BACKSLASH
        /* IDENT */ STATE_PREPROC,
        /* NEWLINE */ STATE_PREPROC,
        /* SLASH */ STATE_PREPROC,
        /* BACKSLASH */ STATE_PREPROC_BACKSLASH,
        /* STAR */ STATE_PREPROC,
        /* POUND */ STATE_PREPROC,
        /* WHITE */ STATE_PREPROC,
        /* DIGIT */ STATE_PREPROC,
        /* QUOTE */ STATE_PREPROC,
        /* LPAREN */ STATE_PREPROC,
        /* LSQUARE */ STATE_PREPROC,
        /* LCURLY */ STATE_PREPROC,
        /* LTRI */ STATE_PREPROC,
        /* RPAREN */ STATE_PREPROC,
        /* RSQUARE */ STATE_PREPROC,
        /* RCURLY */ STATE_PREPROC,
        /* RTRI */ STATE_PREPROC, 
        /* AND */ STATE_PREPROC, 
        /* EQU */ STATE_PREPROC, 
        /* SEMI */ STATE_PREPROC, 
        /* COMMA */ STATE_PREPROC, 
        /* MISCOP */ STATE_PREPROC, 
    },
    { // STATE_SEEN_SLASH
        /* IDENT */ STATE_NEWLINE,
        /* NEWLINE */ STATE_NEWLINE,
        /* SLASH */ STATE_COMMENT_LINE,
        /* BACKSLASH */ STATE_NEWLINE,
        /* STAR */ STATE_COMMENT_BLOCK,
        /* POUND */ STATE_NEWLINE,
        /* WHITE */ STATE_NEWLINE,
        /* DIGIT */ STATE_NEWLINE,
        /* QUOTE */ STATE_NEWLINE,
        /* LPAREN */ STATE_NEWLINE,
        /* LSQUARE */ STATE_NEWLINE,
        /* LCURLY */ STATE_NEWLINE,
        /* LTRI */ STATE_NEWLINE,
        /* RPAREN */ STATE_NEWLINE,
        /* RSQUARE */ STATE_NEWLINE,
        /* RCURLY */ STATE_NEWLINE,
        /* RTRI */ STATE_NEWLINE, 
        /* AND */ STATE_NEWLINE, 
        /* EQU */ STATE_NEWLINE, 
        /* SEMI */ STATE_NEWLINE, 
        /* COMMA */ STATE_NEWLINE, 
        /* MISCOP */ STATE_NEWLINE,
    },
    { // STATE_COMMENT_LINE
        /* IDENT */ STATE_COMMENT_LINE,
        /* NEWLINE */ STATE_NEWLINE,
        /* SLASH */ STATE_COMMENT_LINE,
        /* BACKSLASH */ STATE_COMMENT_LINE_BACKSLASH,
        /* STAR */ STATE_COMMENT_LINE,
        /* POUND */ STATE_COMMENT_LINE,
        /* WHITE */ STATE_COMMENT_LINE,
        /* DIGIT */ STATE_COMMENT_LINE,
        /* QUOTE */ STATE_COMMENT_LINE,
        /* LPAREN */ STATE_COMMENT_LINE,
        /* LSQUARE */ STATE_COMMENT_LINE,
        /* LCURLY */ STATE_COMMENT_LINE,
        /* LTRI */ STATE_COMMENT_LINE,
        /* RPAREN */ STATE_COMMENT_LINE,
        /* RSQUARE */ STATE_COMMENT_LINE,
        /* RCURLY */ STATE_COMMENT_LINE,
        /* RTRI */ STATE_COMMENT_LINE, 
        /* AND */ STATE_COMMENT_LINE, 
        /* EQU */ STATE_COMMENT_LINE, 
        /* SEMI */ STATE_COMMENT_LINE, 
        /* COMMA */ STATE_COMMENT_LINE, 
        /* MISCOP */ STATE_COMMENT_LINE,
    },
    { // STATE_COMMENT_LINE_BACKSLASH
        /* IDENT */ STATE_COMMENT_LINE,
        /* NEWLINE */ STATE_COMMENT_LINE,
        /* SLASH */ STATE_COMMENT_LINE,
        /* BACKSLASH */ STATE_COMMENT_LINE_BACKSLASH,
        /* STAR */ STATE_COMMENT_LINE,
        /* POUND */ STATE_COMMENT_LINE,
        /* WHITE */ STATE_COMMENT_LINE,
        /* DIGIT */ STATE_COMMENT_LINE,
        /* QUOTE */ STATE_COMMENT_LINE,
        /* LPAREN */ STATE_COMMENT_LINE,
        /* LSQUARE */ STATE_COMMENT_LINE,
        /* LCURLY */ STATE_COMMENT_LINE,
        /* LTRI */ STATE_COMMENT_LINE,
        /* RPAREN */ STATE_COMMENT_LINE,
        /* RSQUARE */ STATE_COMMENT_LINE,
        /* RCURLY */ STATE_COMMENT_LINE,
        /* RTRI */ STATE_COMMENT_LINE, 
        /* AND */ STATE_COMMENT_LINE, 
        /* EQU */ STATE_COMMENT_LINE, 
        /* SEMI */ STATE_COMMENT_LINE, 
        /* COMMA */ STATE_COMMENT_LINE, 
        /* MISCOP */ STATE_COMMENT_LINE,
    },
    { // STATE_COMMENT_BLOCK
        /* IDENT */ STATE_COMMENT_BLOCK,
        /* NEWLINE */ STATE_COMMENT_BLOCK,
        /* SLASH */ STATE_COMMENT_BLOCK,
        /* BACKSLASH */ STATE_COMMENT_BLOCK,
        /* STAR */ STATE_COMMENT_BLOCK_STAR,
        /* POUND */ STATE_COMMENT_BLOCK,
        /* WHITE */ STATE_COMMENT_BLOCK,
        /* DIGIT */ STATE_COMMENT_BLOCK,
        /* QUOTE */ STATE_COMMENT_BLOCK,
        /* LPAREN */ STATE_COMMENT_BLOCK,
        /* LSQUARE */ STATE_COMMENT_BLOCK,
        /* LCURLY */ STATE_COMMENT_BLOCK,
        /* LTRI */ STATE_COMMENT_BLOCK,
        /* RPAREN */ STATE_COMMENT_BLOCK,
        /* RSQUARE */ STATE_COMMENT_BLOCK,
        /* RCURLY */ STATE_COMMENT_BLOCK,
        /* RTRI */ STATE_COMMENT_BLOCK, 
        /* AND */ STATE_COMMENT_BLOCK, 
        /* EQU */ STATE_COMMENT_BLOCK, 
        /* SEMI */ STATE_COMMENT_BLOCK, 
        /* COMMA */ STATE_COMMENT_BLOCK, 
        /* MISCOP */ STATE_COMMENT_BLOCK,
    },
    { // STATE_COMMENT_BLOCK_STAR
        /* IDENT */ STATE_COMMENT_BLOCK,
        /* NEWLINE */ STATE_COMMENT_BLOCK,
        /* SLASH */ STATE_NEWLINE,
        /* BACKSLASH */ STATE_COMMENT_BLOCK,
        /* STAR */ STATE_COMMENT_BLOCK_STAR,
        /* POUND */ STATE_COMMENT_BLOCK,
        /* WHITE */ STATE_COMMENT_BLOCK,
        /* DIGIT */ STATE_COMMENT_BLOCK,
        /* QUOTE */ STATE_COMMENT_BLOCK,
        /* LPAREN */ STATE_COMMENT_BLOCK,
        /* LSQUARE */ STATE_COMMENT_BLOCK,
        /* LCURLY */ STATE_COMMENT_BLOCK,
        /* LTRI */ STATE_COMMENT_BLOCK,
        /* RPAREN */ STATE_COMMENT_BLOCK,
        /* RSQUARE */ STATE_COMMENT_BLOCK,
        /* RCURLY */ STATE_COMMENT_BLOCK,
        /* RTRI */ STATE_COMMENT_BLOCK, 
        /* AND */ STATE_COMMENT_BLOCK, 
        /* EQU */ STATE_COMMENT_BLOCK, 
        /* SEMI */ STATE_COMMENT_BLOCK, 
        /* COMMA */ STATE_COMMENT_BLOCK, 
        /* MISCOP */ STATE_COMMENT_BLOCK,
    },
    { // STATE_DECL_TYPE
        /* IDENT */ STATE_DECL_TYPE,
        /* NEWLINE */ STATE_NEWLINE,//STATE_DECL_VAR,
        /* SLASH */ STATE_SEEN_SLASH,//STATE_DECL_VAR,
        /* BACKSLASH */ STATE_NEWLINE,
        /* STAR */ STATE_NEWLINE,//STATE_DECL_VAR,
        /* POUND */ STATE_NEWLINE,//STATE_PREPROC,
        /* WHITE */ STATE_NEWLINE,//STATE_DECL_VAR,
        /* DIGIT */ STATE_DECL_TYPE,//STATE_DECL_VAR,
        /* QUOTE */ STATE_NEWLINE, // @Temporary
        /* LPAREN */ STATE_NEWLINE, // @Temporary
        /* LSQUARE */ STATE_NEWLINE, // @Temporary
        /* LCURLY */ STATE_NEWLINE, // @Temporary
        /* LTRI */ STATE_NEWLINE, // @Temporary
        /* RPAREN */ STATE_NEWLINE, // @Temporary
        /* RSQUARE */ STATE_NEWLINE, // @Temporary
        /* RCURLY */ STATE_NEWLINE, // @Temporary
        /* RTRI */ STATE_NEWLINE, // @Temporary
        /* AND */ STATE_NEWLINE, // @Temporary
        /* EQU */ STATE_NEWLINE, // @Temporary
        /* SEMI */ STATE_NEWLINE, // @Temporary
        /* COMMA */ STATE_NEWLINE, // @Temporary
        /* MISCOP */ STATE_NEWLINE, // @Temporary
    },
};
    State state = initial_state;
    while (p < pend) {
        s32 raw = (s32)*p;
        u8 c = (raw | -(raw >> 7) >> 31);
        //u8 c;
        //if (c >= 128) c = 127;
        //else c = raw & 0x7f;
        Char_Type type = (Char_Type)char_type[(u8)(c & 0x7f)];
        State new_state = (State)(state_table[state])[(u8)type];
        if (new_state != state) {
            sh->type = (Syntax_Highlight_Type)new_state;
            sh->where = p;
            sh++;
        }
        
        state = new_state;
        p++;
    }

    return state;
}

    //__declspec(noinline) // @Temporary @Debug @Remove
    void parse_buffer(Buffer& b) {
        if (b.allocated <= 0) return;

        sh = b.syntax.data;
        p = b.data;
        buf_end = b.data + b.allocated;
        buf_gap = b.gap;
        gap_size = b.gap_size;

        push(SHT_IDENT);
#if DFA_PARSER
        {
            if (gap_size) b.gap[0] = 0; // @Debug.
            Syntax_Highlight* dfa_sh = b.syntax.data;
            State state = parse_dfa(STATE_NEWLINE, b.data, b.gap, dfa_sh);
            state = parse_dfa(state, buf_gap + gap_size, buf_end, dfa_sh);
            this->sh = dfa_sh;
            this->p = buf_end;
        }
#else
        scan_lexeme();
        if (p[0] == '#') parse_pp();
        while (next()) {
            next_token();
            parse_stmt();
            //assert(p < buf_end);

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
#endif
        push(SHT_IDENT);
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
        //array_resize(&buffer->as_ascii, get_count(*buffer));
        //if (buffer->syntax.allocated != old_cap)
        //    OutputDebugStringA("Reserving!!\n");
    }

    double begin = os_get_time();

    C_Lexer l;
    l.parse_buffer(*buffer);
    
    double end = os_get_time();

    double s = (end - begin);

    buffer->s = s;
    buffer->loc_s = buffer->eol_table.count / s;
    buffer->chars_s = get_count(*buffer) / s;
    buffer->syntax_is_dirty = false;
    //double microseconds = (end - begin);
    //microseconds *= 1000000;
    //char buf[512];
    //sprintf(buf, "%f microseconds.\n", microseconds);
    //OutputDebugStringA(buf);
}
