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
    DFA_PREPROC_BS,

    DFA_NUM_STATES
};

enum Lexer_State : u8 {
    IN_NO_COMMENT = 0,
    IN_BLOCK_COMMENT = 1,
    IN_LINE_COMMENT = 2,
    IN_PREPROC = 4,
};

struct Lexeme {
    const u32* p;
    Char_Type ch : 8;
    Lex_Dfa dfa : 8;
    int lex_state : 8;
};

void parse_cpp(Buffer* b);

} // namespace parsing
