#pragma once
#include <ch_stl\types.h>

struct Buffer;

// C++ lexing/parsing tools.
namespace parsing {
// This is the actual DFA state machine. It's not perfect,
// but I (phillip) can get it to run at 1.25GB/s on a 3.6 GHz machine
// with 1600 MHz DDR3 RAM and, as of writing, YEET's UTF32 gap buffer.
// It does as little as it can; it tries not to *parse* C++, only lex it.
// This means it just scans comments and string/number literals (mostly).
// It doesn't even recognize keywords, it just treats all identifiers alike.
// The tradeoff is that this puts an increased burden of code processing
// onto the parser. There is no nesting of any kind.
// I don't yet know the optimal ordering here for cache-efficiency, if there is
// one; I will have to write some sort of randomizer/permutation-checker.
enum Lex_Dfa : u8 {
    DFA_BLOCK_COMMENT,
    DFA_BLOCK_COMMENT_STAR,
    DFA_LINE_COMMENT,
    DFA_WHITE,
    DFA_WHITE_BS,
    DFA_NEWLINE,
    DFA_STRINGLIT,
    DFA_STRINGLIT_BS,
    DFA_CHARLIT,
    DFA_CHARLIT_BS,
    DFA_SLASH,
    DFA_IDENT,
    DFA_OP,
    DFA_OP2,
    DFA_NUMLIT,
    
    DFA_NUM_STATES,

    DFA_PREPROC,
    DFA_MACRO,
    DFA_TYPE,
    DFA_KEYWORD,
    DFA_FUNCTION,
    DFA_PARAM,
};

// This is an enum for categorizing C++ source code characters.
// Since the lexer uses a table-based DFA, all of its relevant
// char types need to be numerically adjacent, so that they can
// index a contiguous cache-friendly table.
// Strictly speaking, a real lexer would also process digraphs,
// but digraphs are rarely used. Trigraphs are never used.
enum Char_Type : u8 {
    WHITE       = DFA_NUM_STATES * 0,
    NEWLINE     = DFA_NUM_STATES * 1, // '\r' '\n'
    IDENT       = DFA_NUM_STATES * 2, // '$' '@'-'Z' '_'-'z'
    DOUBLEQUOTE = DFA_NUM_STATES * 3, // '"'
    SINGLEQUOTE = DFA_NUM_STATES * 4, // '\''
    DIGIT       = DFA_NUM_STATES * 5, // '0'-'9'
    SLASH       = DFA_NUM_STATES * 6, // '/'
    STAR        = DFA_NUM_STATES * 7, // '*'
    BS          = DFA_NUM_STATES * 8, // '\\'
    OP          = DFA_NUM_STATES * 9,
    NUM_CHAR_TYPES,
};

#pragma pack(push)
#pragma pack(1)
struct Lexeme {
    const u8* i;
    u8 dfa;
};
#pragma pack(pop)

void parse_cpp(Buffer* b);

} // namespace parsing
