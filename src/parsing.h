#pragma once
#include <ch_stl\types.h>

struct Buffer;

// C++ lexing/parsing tools.
namespace parsing {
// This is an enum for categorizing C++ source code characters.
// The input space is split up into two different sections:
// Everything used by the lexer, and everything else.
// Since the lexer uses a table-based DFA, all of its relevant
// char types need to be numerically adjacent, so that they can
// index a contiguous cache-friendly table. That's why you see
// that characters like SLASH (used in scanning comments) and POUND
// (used in scanning preprocessor directives) are in the first
// section, but not AND or COLON (just regular old operators).
// Strictly speaking, a real lexer would also process digraphs,
// but digraphs are rarely used. Trigraphs are never used.
//
// n.b.: Backslashes get checked by their ASCII value directly in the
// core loop, so the backslash does not need to be listed here.
enum Char_Type : u8 {
    // Everything used by the lexer:
    WHITE,
    NEWLINE,     // '\r' '\n'
    IDENT,       // '$' '@'-'Z' '_'-'z'
    POUND,       // '#'
    DOUBLEQUOTE, // '"'
    SINGLEQUOTE, // '\''
    DIGIT,       // '0'-'9'
    SLASH,       // '/'
    STAR,        // '*'
    OP,          // '!' '%' '?' '^' '|' '~'

    // Everything else
    AND,    // '&'
    LPAREN, // '('
    RPAREN, // ')'
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

// This is the actual DFA state machine. It's not perfect,
// but I (phillip) can get it to run at 1.25GB/s on a 3.6 GHz machine
// with 1600 MHz DDR3 RAM and, as of writing, YEET's UTF32 gap buffer.
// It does as little as it can; it tries not to *parse* C++, only lex it.
// This means it just scans comments, string/number literals (mostly),
// and preprocessor directives.
// It doesn't even recognize keywords, it just treats all identifiers alike.
// The tradeoff is that this puts an increased burden of code processing
// onto the parser.
// There is no nesting of any kind, which is why you see
// "DFA_PREPROC_BLOCK_COMMENT": the DFA must manually keep track of the fact
// that a block comment can nest inside a preprocessor directive (exactly once).
// The ordering is also implementation-specific to speed up cache access.
// I don't yet know the optimal ordering here, if there is one; I will have to write
// some sort of randomizer/permutation-checker.
enum Lex_Dfa : u8 {
    DFA_BLOCK_COMMENT,
    DFA_BLOCK_COMMENT_STAR,
    DFA_PREPROC_BLOCK_COMMENT,
    DFA_PREPROC_BLOCK_COMMENT_STAR,
    DFA_LINE_COMMENT,
    DFA_STRINGLIT,
    DFA_CHARLIT,
    DFA_PREPROC_SLASH,
    DFA_SLASH,
    DFA_WHITE,
    DFA_IDENT,
    DFA_OP,
    DFA_NUMLIT,
    DFA_PREPROC,

    DFA_NUM_STATES
};

struct Lexeme {
    unsigned int i : 28;
    unsigned int dfa : 4;
};

void parse_cpp(Buffer* b);

} // namespace parsing
