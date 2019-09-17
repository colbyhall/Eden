#pragma once
#include <ch_stl\types.h>

struct Buffer;

namespace parsing {
enum Char_Type : u8 {
    WHITE,
    NEWLINE,     // '\r' '\n'
    IDENT,       // '$' '@'-'Z' '_'-'z'
    POUND,       // '#'
    DOUBLEQUOTE, // '"'
    SINGLEQUOTE, // '\''
    DIGIT,       // '0'-'9'
    SLASH,       // '/'
    BS,          // '\\'
    OP,          // '!' '%' '?' '^' '|' '~'

    AND,    // '&'
    LPAREN, // '('
    RPAREN, // ')'
    STAR,   // '*'
    PLUS,   // '+'
    COMMA,  // ','
    MINUS,  // '-'
    DOT,    // '.'
    COLON,  // ':'
    SEMI,   // ';'
    LT,     // '<'
    EQU,    // '='
    GT,     // '>'
    LSQR,   // '['
    RSQR,   // ']'
    LCURLY, // '{'
    RCURLY, // '}'

    NUM_CHAR_TYPES,

    NUM_CHAR_TYPES_IN_LEXER = OP + 1,
};

enum Lex_Dfa : u8 {
    DFA_STRINGLIT,
    DFA_STRINGLIT_BS,
    DFA_CHARLIT,
    DFA_CHARLIT_BS,
    DFA_PREPROC_SLASH,
    DFA_SLASH,
    DFA_LINE_COMMENT,
    DFA_WHITE,
    DFA_IDENT,
    DFA_OP,
    DFA_NUMLIT,
    DFA_PREPROC,

    // should be unreachable from any other DFA
    DFA_BLOCK_COMMENT,

    DFA_NUM_STATES
};

#pragma pack(push)
#pragma pack(1)
struct Lexeme {
    unsigned int i : 28;
    unsigned int dfa : 4;
};
#pragma pack(pop)

void parse_cpp(Buffer* b);

} // namespace parsing
