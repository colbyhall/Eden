#pragma once
#include <ch_stl\types.h>

struct Buffer;

// C++ lexing/parsing tools.
namespace parsing {
// This is an enum for categorizing C++ source code characters.
// Since the lexer uses a table-based DFA, all of its relevant
// char types need to be numerically adjacent, so that they can
// index a contiguous cache-friendly table.
// Strictly speaking, a real lexer would also process digraphs,
// but digraphs are rarely used. Trigraphs are never used.
enum Char_Type : u8 {
    WHITE,
    NEWLINE,     // '\r' '\n'
    IDENT,       // '$' '@'-'Z' '_'-'z'
    POUND,       // '#'
    DOUBLEQUOTE, // '"'
    SINGLEQUOTE, // '\''
    DIGIT,       // '0'-'9'
    SLASH,       // '/'
    STAR,        // '*'
    BS,          // '\\'
    OP,
    NUM_CHAR_TYPES,
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
    DFA_STRINGLIT_BS,
    DFA_CHARLIT,
    DFA_CHARLIT_BS,
    DFA_PREPROC_SLASH,
    DFA_SLASH,
    DFA_WHITE,
    DFA_IDENT,
    DFA_OP,
    DFA_NUMLIT,
    DFA_PREPROC,

    DFA_NUM_STATES,

    // This is an informational bitmask that may be used in the future.
    // At the moment it is 0, so that it has no effect when bitwise-ORed.
    ADD = 0,
};

#pragma pack(push)
#pragma pack(1)
struct Lexeme {
    const u32* i;
    u8 dfa;
};
#pragma pack(pop)

void parse_cpp(Buffer* b);

} // namespace parsing
