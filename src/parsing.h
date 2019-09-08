#pragma once
#include <ch_stl\types.h>

enum Char_Type : u8 {
    WHITE,
    IDENT, // '$' '@'-'Z' '_'-'z'
    DOUBLEQUOTE, // '"'
    SINGLEQUOTE, // '\''
    DIGIT, // '0'-'9'
    BS, // '\\'
    OP, // '!' '%' '?' '^' '|' '~' '/'

    AND, // '&'
    LPAREN, // '('
    RPAREN, // ')'
    STAR, // '*'
    PLUS, // '+'
    COMMA, // ','
    MINUS, // '-'
    DOT, // '.'
    COLON, // ':'
    SEMI, // ';'
    LT,  // '<'
    EQU, // '='
    GT, // '>'
    LSQR, // '['
    RSQR, // ']'
    LCURLY, // '{'
    RCURLY, // '}'

    NUM_CHAR_TYPES,

    NUM_CHAR_TYPES_IN_LEXER = OP + 1,
};

enum Lex_Dfa : u8 {
    DFA_WHITE,
    DFA_IDENT,
    DFA_OP,
    DFA_STRINGLIT,
    DFA_STRINGLIT_BS,
    DFA_CHARLIT,
    DFA_CHARLIT_BS,
    DFA_NUMLIT,

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
    Lexer_State lex_state : 8;
};

void parse_cpp(struct Buffer* b);
