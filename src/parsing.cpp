#include "parsing.h"
#include "buffer.h"
#include <ch_stl\time.h>

namespace parsing {
// This is a column-reduction table to map 128 ASCII values to a 11-input space.
static const u8 char_type[] = {
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, NEWLINE,
    WHITE, WHITE, NEWLINE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    OP, DOUBLEQUOTE, OP, IDENT, OP, OP, SINGLEQUOTE, OP, OP, STAR,
    OP, OP, OP, OP, SLASH, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT,
    DIGIT, DIGIT, DIGIT, DIGIT, OP, OP, OP, OP, OP, OP, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, OP, BS, OP, OP, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, OP, OP, OP, OP, WHITE,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
    IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
};
// This is a state transition table for the deterministic finite
// automaton (DFA) lexer. Overtop this DFA runs a block-comment scanner.
static const u8 lex_table[] = {
    // WHITE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_WHITE,         // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_NEWLINE,       // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE,         // DFA_SLASH
    DFA_WHITE,         // DFA_IDENT
    DFA_WHITE,         // DFA_OP
    DFA_WHITE,         // DFA_OP2
    DFA_WHITE,         // DFA_NUMLIT
    // NEWLINE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_NEWLINE,       // DFA_LINE_COMMENT
    DFA_NEWLINE,       // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_NEWLINE,       // DFA_NEWLINE
    DFA_NEWLINE,       // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_NEWLINE,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NEWLINE,       // DFA_SLASH
    DFA_NEWLINE,       // DFA_IDENT
    DFA_NEWLINE,       // DFA_OP
    DFA_NEWLINE,       // DFA_OP2
    DFA_NEWLINE,       // DFA_NUMLIT
    // IDENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_IDENT,         // DFA_WHITE
    DFA_IDENT,         // DFA_WHITE_BS
    DFA_IDENT,         // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_IDENT,         // DFA_SLASH
    DFA_IDENT,         // DFA_IDENT
    DFA_IDENT,         // DFA_OP
    DFA_IDENT,         // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DOUBLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_WHITE
    DFA_STRINGLIT,     // DFA_WHITE_BS
    DFA_STRINGLIT,     // DFA_NEWLINE
    DFA_WHITE,         // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_STRINGLIT,     // DFA_SLASH
    DFA_STRINGLIT,     // DFA_IDENT
    DFA_STRINGLIT,     // DFA_OP
    DFA_STRINGLIT,     // DFA_OP2
    DFA_STRINGLIT,     // DFA_NUMLIT
    // SINGLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_CHARLIT,       // DFA_WHITE
    DFA_CHARLIT,       // DFA_WHITE_BS
    DFA_CHARLIT,       // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_WHITE,         // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_CHARLIT,       // DFA_SLASH
    DFA_CHARLIT,       // DFA_IDENT
    DFA_CHARLIT,       // DFA_OP
    DFA_CHARLIT,       // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DIGIT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_NUMLIT,        // DFA_WHITE
    DFA_NUMLIT,        // DFA_WHITE_BS
    DFA_NUMLIT,        // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NUMLIT,        // DFA_SLASH
    DFA_IDENT,         // DFA_IDENT
    DFA_NUMLIT,        // DFA_OP
    DFA_NUMLIT,        // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // SLASH
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_WHITE,         // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_SLASH,         // DFA_WHITE
    DFA_SLASH,         // DFA_WHITE_BS
    DFA_SLASH,         // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_LINE_COMMENT,  // DFA_SLASH
    DFA_SLASH,         // DFA_IDENT
    DFA_SLASH,         // DFA_OP
    DFA_SLASH,         // DFA_OP2
    DFA_SLASH,         // DFA_NUMLIT
    // STAR
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,       // DFA_LINE_COMMENT
    DFA_OP,                 // DFA_WHITE
    DFA_OP,                 // DFA_WHITE_BS
    DFA_OP,                 // DFA_NEWLINE
    DFA_STRINGLIT,          // DFA_STRINGLIT
    DFA_STRINGLIT,          // DFA_STRINGLIT_BS
    DFA_CHARLIT,            // DFA_CHARLIT
    DFA_CHARLIT,            // DFA_CHARLIT_BS
    DFA_BLOCK_COMMENT,      // DFA_SLASH
    DFA_OP,                 // DFA_IDENT
    DFA_OP2,                // DFA_OP
    DFA_OP,                 // DFA_OP2
    DFA_OP,                 // DFA_NUMLIT
    // BS
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_WHITE_BS,      // DFA_WHITE
    DFA_WHITE_BS,      // DFA_WHITE_BS
    DFA_WHITE_BS,      // DFA_NEWLINE
    DFA_STRINGLIT_BS,  // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT_BS,    // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE_BS,      // DFA_SLASH
    DFA_WHITE_BS,      // DFA_IDENT
    DFA_WHITE_BS,      // DFA_OP
    DFA_WHITE_BS,      // DFA_OP2
    DFA_WHITE_BS,      // DFA_NUMLIT
    // OP
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_OP,            // DFA_WHITE
    DFA_OP,            // DFA_WHITE_BS
    DFA_OP,            // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_OP,            // DFA_SLASH
    DFA_OP,            // DFA_IDENT
    DFA_OP2,           // DFA_OP
    DFA_OP,            // DFA_OP2
    DFA_OP,            // DFA_NUMLIT
};

u8 lex(u8 dfa, const u8* p, const u8* const end, Lexeme*& lexemes) {
    while (p < end) {
        u8 new_dfa = lex_table[dfa + char_type[*p]];
        if (new_dfa != dfa) {
            lexemes->i = p;
            lexemes->dfa = (Lex_Dfa)new_dfa;
            lexemes->cached_first = *p;
            lexemes++;
            dfa = new_dfa;
        }
        p++;
    }
    return dfa;
}

const u8* temp_parser_gap;
u64 temp_parser_gap_size;

static const u8 lexeme_sentinel_buffer[16];
u64 toklen(Lexeme* l) {
    const u8* next_i = l[1].i;
    if (next_i == lexeme_sentinel_buffer) next_i = l[2].i;
    if (next_i == temp_parser_gap + temp_parser_gap_size) next_i = temp_parser_gap;
    return next_i - l->i;
}

bool nested = false; // JUST for debugging

void parse(Lexeme* l, Lexeme* end);

static const u64 chunk_mask[] = {
    0x0000000000000000ull, 0x00000000000000ffull, 0x000000000000ffffull, 0x0000000000ffffffull,
    0x00000000ffffffffull, 0x000000ffffffffffull, 0x0000ffffffffffffull, 0x00ffffffffffffffull,
    0xffffffffffffffffull,
};

#define C_KEYWORDS(X) \
    X(include) \
    X(define) \
    X(return) \
    X(switch) \
    X(if) \
    X(else) \
    X(while) \
    X(for) \
    X(static) \
    X(const) \
    X(extern) \
    X(typedef) \
    X(struct) \
    X(union) \
    X(sizeof) \

#define C_KW_DECL(name) C##name,
enum C_Keyword {
    NOT_KEYWORD,
    C_KEYWORDS(C_KW_DECL)
};

C_Keyword match_token(Lexeme* l) {
    u64 len = toklen(l);
    if (len < 2 || len > 8) return NOT_KEYWORD;
    switch (*(u64*)l->i & chunk_mask[len]) {
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
        return C##x;

        C_KEYWORDS(KW_CASE);
    }
    return NOT_KEYWORD;
}

static Lexeme* skip_comments_in_line(Lexeme* l, Lexeme* end) {
    while (l < end && (l->dfa < DFA_NEWLINE || l->dfa == DFA_SLASH && l[1].dfa <= DFA_LINE_COMMENT)) l++;
    return l;
}
static Lexeme* parse_preproc(Lexeme* l, Lexeme* end) {
    l->dfa = DFA_PREPROC;
    l++;
    l = skip_comments_in_line(l, end);
    if (l->dfa == DFA_IDENT) {
        Lexeme* directive = l;
        C_Keyword kw = match_token(directive);
        l->dfa = DFA_PREPROC;
        l++;
        l = skip_comments_in_line(l, end);
        if (kw == Cinclude && l->c() == '<') {
            l->dfa = DFA_STRINGLIT;
            while (l < end && l->dfa != DFA_NEWLINE) {
                l->dfa = DFA_STRINGLIT;
                l++;
                if (l->c() == '>') {
                    l->dfa = DFA_STRINGLIT;
                    l++;
                    break;
                }
            }
        } else if (kw == Cdefine && l->dfa == DFA_IDENT) {
            l->dfa = DFA_MACRO;
            l++;
            if (l->c() == '(') {
                while (l < end && l->dfa != DFA_NEWLINE) {
                    l++;
                    if (l->c() == ')') {
                        l++;
                        break;
                    }
                }
            }
        }
    }
    Lexeme* preproc_begin = l;
    while (l < end && l->dfa != DFA_NEWLINE) {
        l++;
    }
    nested = true;
    parse(preproc_begin, l);
    nested = false;
    return l;
}
static Lexeme* next_token(Lexeme* l, Lexeme* end) {
    while (true) {
        l = skip_comments_in_line(l, end);
        if (l < end && l->dfa == DFA_NEWLINE) {
            l++;
            if (l->c() == '#') {
                assert(!nested);
                l = parse_preproc(l, end);
            }
            continue;
        }
        break;
    }
    return l;
}

static bool at_token(Lexeme* l, Lexeme* end) {
    Lexeme* r = skip_comments_in_line(l, end);
    return r == l && (!(r < end) || r->c() != '#');
}

// Brace nesting precedence: {for(;[;{;];)}}
// 1. {} - Curlies: Most important
// 2. () - STATEMENT parentheses: for(;;) etc.
//    ;  - Semicolon supersedes everything below
// 3. () - EXPRESSION/PARAMETER parentheses: call(x, y); int func(int a); etc.
// 4. [] - Square brackets.
//    ,  - Comma supersedes almost nothing.
// 5. <> - Greater than/less than: least important.

static Lexeme* parse_stmt(Lexeme* l, Lexeme* end, Lex_Dfa var_name = DFA_IDENT);
static Lexeme* parse_braces(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '{');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_stmt(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')') {
            l++;
        }
        if (l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_stmt_parens(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_stmt(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_expr(Lexeme* l, Lexeme* end);
static Lexeme* parse_exprs_til_semi(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    while (l < end) {
        l = parse_expr(l, end);
        if (l->c() == ',' ||
            l->c() == ']') {
            l++;
        }
        if (l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_exprs_til_comma(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    while (l < end) {
        l = parse_expr(l, end);
        if (l->c() == ',' ||
            l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
        l = next_token(l, end);
    }
    return l;
}
static Lexeme* parse_expr_parens(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_exprs_til_comma(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}
static Lexeme* parse_expr_sqr(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '[');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_exprs_til_comma(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_param(Lexeme* l, Lexeme* end);
static Lexeme* parse_params(Lexeme* l, Lexeme* end) {
    assert(l < end);
    assert(l->c() == '(');
    l++;
    l = next_token(l, end);
    while (l < end) {
        l = parse_param(l, end);
        if (l->c() == ',') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            break;
        }
    }
    return l;
}

static Lexeme* parse_expr(Lexeme* l, Lexeme* end) {
    //assert(l < end);
    if (!(l < end)) return l;
    if (l->c() == '#') {
        if (l[1].i[0] == '#') {
            l->dfa = DFA_IDENT;
            l++;
            l->dfa = DFA_IDENT;
            l++;
            l = next_token(l, end);
            if (l->dfa == DFA_NUMLIT) {
                l->dfa = DFA_IDENT;
                l++;
            }
        } else {
            l->dfa = DFA_PREPROC;
            l++;
        }
        l = next_token(l, end);
    }
    switch (match_token(l)) {
    case Csizeof:
        l++;
        l = next_token(l, end);
        l = parse_expr(l, end);
    }
    if (l->c() == '~' ||
        l->c() == '!' ||
        l->c() == '&' ||
        l->c() == '*' ||
        l->c() == '-' ||
        l->c() == '+') {
        l++;
        l = next_token(l, end);
        return parse_expr(l, end);
    }
    if (l->c() == '{') {
        l = parse_braces(l, end);
        if (l->c() == '}') {
            l++;
            l = next_token(l, end);
        }
    } else if (l->c() == '(') {
        l = parse_expr_parens(l, end);
        if (l->c() == ')') {
            l++;
            l = next_token(l, end);
        }
    } else if (l->c() == '[') {
        l = parse_expr_sqr(l, end);
        if (l->c() == ']') {
            l++;
            l = next_token(l, end);
        }
        if (l->c() == '(') {
            l = parse_params(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
            if (l->c() == '{') {
                l = parse_braces(l, end);
                if (l->c() == '}') {
                    l++;
                    l = next_token(l, end);
                }
            }
        }
    } else if (l->dfa == DFA_IDENT) {
        Lexeme* ident = l;
        l++;
        l = next_token(l, end);
        if (l->c() == '(') {
            ident->dfa = DFA_FUNCTION;
            l = parse_expr_parens(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
        } else if (l->c() == '{') {
            ident->dfa = DFA_TYPE;
            l = parse_braces(l, end);
            if (l->c() == '}') {
                l++;
                l = next_token(l, end);
            }
        }
    } else if (l->dfa == DFA_NUMLIT) {
        l++;
        l = next_token(l, end);
    } else if (l->dfa == DFA_STRINGLIT) {
        while (l < end && (l->dfa == DFA_STRINGLIT || l->dfa == DFA_STRINGLIT_BS)) {
            l++;
            l = next_token(l, end);
            // lets us properly parse this:
            // char*x = "string literal but there's "
            //          #pragma once
            //          "more more more";
        }
    } else if (l->dfa == DFA_CHARLIT) {
        while (l < end && (l->dfa == DFA_CHARLIT || l->dfa == DFA_CHARLIT_BS)) {
            l++;
            l = next_token(l, end);
            // lets us properly parse this:
            // char*x = "string literal but there's "
            //          #pragma once
            //          "more more more";
        }
    }
    if (l->c() == '[' ||
        l->c() == '(') {
        l = parse_expr(l, end);
        l = next_token(l, end);
    }
    if (l->c() == '%' ||
        l->c() == '^' ||
        l->c() == '&' ||
        l->c() == '*' ||
        l->c() == '-' ||
        l->c() == '=' ||
        l->c() == '+' ||
        l->c() == '|' ||
        l->c() == ':' ||
        l->c() == '<' ||
        l->c() == '.' ||
        l->c() == '>' ||
        l->c() == '/' ||
        l->c() == '?') {
        l++;
        l = next_token(l, end);
        l = parse_expr(l, end);
    }
    return l;
}

static Lexeme* parse_param(Lexeme* l, Lexeme* end) { return parse_stmt(l, end, DFA_PARAM); }

static Lexeme* parse_stmt(Lexeme* l, Lexeme* end, Lex_Dfa var_name_type) {
    switch (match_token(l)) {
    case Ctypedef:
        l++;
        l = next_token(l, end);
        return parse_stmt(l, end, DFA_TYPE);
    case Creturn:
        l++;
        l = next_token(l, end);
        return parse_exprs_til_semi(l, end);
    case Cif:
    case Cswitch:
    case Cwhile:
    case Cfor:
        l++;
        l = next_token(l, end);
        if (l->c() == '(') {
            l = parse_stmt_parens(l, end);
            if (l->c() == ')') {
                l++;
                l = next_token(l, end);
            }
        }
        return parse_stmt(l, end);
    case Celse:
        l++;
        l = next_token(l, end);
        return parse_stmt(l, end);
    case Cstruct:
    case Cunion:
        l++;
        l = next_token(l, end);
        if (l->dfa == DFA_IDENT) {
            l->dfa = DFA_TYPE;
            l++;
            l = next_token(l, end);
        }
        if (l->c() == '{') {
            l = parse_braces(l, end);
            if (l->c() == '}') {
                l++;
                l = next_token(l, end);
            }
        }
        break;
    default:
        if (l->dfa == DFA_IDENT) {
            l->dfa = DFA_TYPE;
            l++;
            l = next_token(l, end);
        } else {
            return parse_exprs_til_semi(l, end);
        }
    }
    bool is_likely_function = false;
more_decls:
    bool seen_closing_paren = false;
    int paren_nesting = 0;
    Lexeme* last = nullptr;
    Lexeme* func = nullptr;
    while (l < end &&
           (l->dfa == DFA_IDENT ||
            l->c() == '*' ||
            l->c() == '&' ||
            l->c() == '(' ||
            l->c() == ')' ||
            paren_nesting)) {
        if (l->c() == '(') {
            if (seen_closing_paren || func) {
                if (func) {
                    func->dfa = DFA_FUNCTION;
                }
                is_likely_function = true;
                l = parse_params(l, end);
                if (l->c() == ')') {
                    l++;
                    l = next_token(l, end);
                }
                continue;
            } else {
                paren_nesting++;
            }
            func = nullptr;
        } else if (l->c() == ')') {
            paren_nesting--;
            if (paren_nesting < 0) break;
            seen_closing_paren = true;
            func = nullptr;
        } else if (l->c() == '[') {
            l = parse_expr_sqr(l, end);
            if (l->c() == ']') {
                l++;
                l = next_token(l, end);
            }
            continue;
        } else if (l->c() == '*' ||
            l->c() == '&') {
            l->dfa = DFA_TYPE;
        } else if (l->dfa == DFA_IDENT) {
            last = l;
            func = l;
            l->dfa = DFA_TYPE;
        }
        l++;
        l = next_token(l, end);
    }
    if (last && last->dfa != DFA_FUNCTION) last->dfa = var_name_type;
    if (l < end) {
        if (var_name_type != DFA_PARAM) {
            //assert(at_token(l, end));
            l = next_token(l, end);
            if (is_likely_function && l->c() == '{') {
                l = parse_braces(l, end);
                if (l->c() == '}') {
                    l++;
                    l = next_token(l, end);
                }
                return l;
            } else {
                l = parse_exprs_til_comma(l, end);
                if (l->c() == ',') {
                    l++;
                    l = next_token(l, end);
                    goto more_decls;
                }
            }
        } else {
            l = parse_exprs_til_comma(l, end);
        }
    }

    return l;
}

static void parse(Lexeme* l, Lexeme* end) {
    while (l < end) {
        l = next_token(l, end);
        l = parse_stmt(l, end);
        assert(l->c() != ',');
        if (l->c() == ']' ||
            l->c() == ';' ||
            l->c() == ')' ||
            l->c() == '}') {
            l++;
        }
    }
}

void parse_cpp(Buffer* buf) {
    if (!buf->syntax_dirty || buf->disable_parse) return;
    buf->syntax_dirty = false;
    ch::Gap_Buffer<u8>& b = buf->gap_buffer;
    usize buffer_count = b.count();

    // Three extra lexemes:
    // One extra lexeme at the front.
    // One at the back to indicate buffer end to the parser, pointing to safe
    // scratch data.
    // One after that pointing to the the real position of the (unreadable) buffer
    // end, so that identifier lengths can be correctly computed.
    buf->lexemes.reserve((1 + buffer_count + 1 + 1) - buf->lexemes.allocated);
    buf->lexemes.count = 1;
    if (buffer_count) {
        u8 lexer = DFA_NEWLINE;
        Lexeme* lex_seeker = buf->lexemes.begin();
        {
            lex_seeker->i = b.data;
            lex_seeker->dfa = (Lex_Dfa)lexer;
            lex_seeker++;
        }
        
        f64 lex_time = -ch::get_time_in_seconds();
        lexer = lex(lexer, b.data, b.gap, lex_seeker);
        Lexeme* lexeme_at_gap = lex_seeker - 1;
        lexer = lex(lexer, b.gap + b.gap_size, b.data + b.allocated, lex_seeker);
        lex_time += ch::get_time_in_seconds();

        if (lexeme_at_gap > buf->lexemes.begin() && lexeme_at_gap < lex_seeker) {
            assert(lexeme_at_gap->i < b.gap);
            if (lexeme_at_gap + 1 < lex_seeker) {
                assert(lexeme_at_gap[1].i >= b.gap + b.gap_size);
            }
            b.move_gap_to_index(lexeme_at_gap->i - b.data);
            assert(lexeme_at_gap->i == b.gap);
            lexeme_at_gap->i += b.gap_size;
        }

        {
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->dfa = DFA_NUM_STATES;
            lex_seeker->i = lexeme_sentinel_buffer; // So the parser can safely read from here.
            lex_seeker++;
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->dfa = DFA_NUM_STATES;
            lex_seeker->i = b.data + b.allocated; // So the parser knows the real end position.
            lex_seeker++;
        }
        buf->lexemes.count = lex_seeker - buf->lexemes.begin();

        temp_parser_gap = b.gap;
        temp_parser_gap_size = b.gap_size;

        f64 parse_time = -ch::get_time_in_seconds();
        parse(buf->lexemes.begin(), buf->lexemes.end() - 2);
        parse_time += ch::get_time_in_seconds();
        buf->lexemes.end()[-2] = buf->lexemes.end()[-1];
        buf->lex_time = lex_time;
        buf->parse_time = parse_time;
    }
}
} // namespace parsing
