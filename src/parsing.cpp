#include "parsing.h"
#include "buffer.h"

static const u8 data_table[] = {
//static const u8 char_type[128] = {
    WHITE,  WHITE,  WHITE,       WHITE,  WHITE, WHITE,   WHITE, WHITE,
    WHITE,  WHITE,  WHITE,       WHITE,  WHITE, WHITE,   WHITE, WHITE,
    WHITE,  WHITE,  WHITE,       WHITE,  WHITE, WHITE,   WHITE, WHITE,
    WHITE,  WHITE,  WHITE,       WHITE,  WHITE, WHITE,   WHITE, WHITE,
    WHITE,  OP,     DOUBLEQUOTE, WHITE,  IDENT, OP,      AND,   SINGLEQUOTE,
    LPAREN, RPAREN, STAR,        PLUS,   COMMA, MINUS,   DOT,   OP,
    DIGIT,  DIGIT,  DIGIT,       DIGIT,  DIGIT, DIGIT,   DIGIT, DIGIT,
    DIGIT,  DIGIT,  COLON,       SEMI,   LT,    EQU,     GT,    OP,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       LSQR,   BS,    RSQR,    OP,    IDENT,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       IDENT,  IDENT, IDENT,   IDENT, IDENT,
    IDENT,  IDENT,  IDENT,       LCURLY, OP,    RCURLY,  OP,    WHITE,
//};
//
//static const u8 lex_table[DFA_NUM_STATES * NUM_CHAR_TYPES_IN_LEXER] = {
    // DFA_WHITE
    DFA_WHITE, // WHITE
    DFA_IDENT, // IDENT
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_NUMLIT, // DIGIT
    DFA_OP, // BS
    DFA_OP, // OP
    // DFA_IDENT
    DFA_WHITE, // WHITE
    DFA_IDENT, // IDENT
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_IDENT, // DIGIT
    DFA_OP, // BS
    DFA_OP, // OP
    // DFA_OP
    DFA_WHITE, // WHITE
    DFA_IDENT, // IDENT
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_NUMLIT, // DIGIT
    DFA_OP, // BS
    DFA_OP, // OP
    // DFA_STRINGLIT
    DFA_STRINGLIT, // WHITE
    DFA_STRINGLIT, // IDENT
    DFA_WHITE, // DOUBLEQUOTE
    DFA_STRINGLIT, // SINGLEQUOTE
    DFA_STRINGLIT, // DIGIT
    DFA_STRINGLIT_BS, // BS
    DFA_STRINGLIT, // OP
    // DFA_STRINGLIT_BS
    DFA_STRINGLIT, // WHITE
    DFA_STRINGLIT, // IDENT
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_STRINGLIT, // SINGLEQUOTE
    DFA_STRINGLIT, // DIGIT
    DFA_STRINGLIT, // BS
    DFA_STRINGLIT, // OP
    // DFA_CHARLIT
    DFA_CHARLIT, // WHITE
    DFA_CHARLIT, // IDENT
    DFA_CHARLIT, // DOUBLEQUOTE
    DFA_WHITE, // SINGLEQUOTE
    DFA_CHARLIT, // DIGIT
    DFA_CHARLIT_BS, // BS
    DFA_CHARLIT, // OP
    // DFA_CHARLIT_BS
    DFA_CHARLIT, // WHITE
    DFA_CHARLIT, // IDENT
    DFA_CHARLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_CHARLIT, // DIGIT
    DFA_CHARLIT, // BS
    DFA_CHARLIT, // OP
    // DFA_NUMLIT
    DFA_WHITE, // WHITE
    DFA_NUMLIT, // IDENT
    DFA_STRINGLIT, // DOUBLEQUOTE
    DFA_CHARLIT, // SINGLEQUOTE
    DFA_NUMLIT, // DIGIT
    DFA_OP, // BS
    DFA_OP, // OP
};

#define char_type (data_table)
#define lex_table (data_table + 128)

struct Lexer {
    int state = 0;
    Lex_Dfa dfa = {};
    const u32* last = nullptr;
    void lex(const u32* p, const u32* const end, Lexeme*& lexemes) {
        while (p < end) {
            if (p[0] == '\n') { state &= ~IN_PREPROC & ~IN_LINE_COMMENT; }
            else if (*(u64*)p == ('/' | (u64)'/' << 32)) { state |= IN_LINE_COMMENT;   dfa = DFA_WHITE; }
            else if (*(u64*)p == ('*' | (u64)'/' << 32)) { state &= ~IN_BLOCK_COMMENT; dfa = DFA_WHITE; p += 2; continue; }
            else if (p[0] == '#') { state |= IN_PREPROC; }
            else if (*(u64*)p == ('/' | (u64)'*' << 32)) { state |= IN_BLOCK_COMMENT;  dfa = DFA_WHITE; }
            else if (*(u64*)p == ('\\' | (u64)'\n' << 32)) { p += 2; continue; }
            
            int c = (int)*p;
            c &= (c - 128) >> 31;
            Char_Type ch = (Char_Type)char_type[c];
            Char_Type ch_to_lex = ch;
            if (ch_to_lex > OP) {
                ch_to_lex = OP;
            }
            Lex_Dfa new_dfa = (Lex_Dfa)lex_table[dfa * NUM_CHAR_TYPES_IN_LEXER + ch_to_lex];
            if (new_dfa != dfa) {
                lexemes->p = p;
                lexemes++;
                last = p;
                lexemes->ch = ch;
                lexemes->dfa = new_dfa;
                lexemes->lex_state = (Lexer_State)state;
                dfa = new_dfa;
            }
            p++;
        }
    }
};

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

f64 get_time() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    f64 d = (f64)li.QuadPart;
    QueryPerformanceFrequency(&li);
    d /= li.QuadPart;
    return d;
}

void parse_cpp(struct Buffer* buf) {
    ch::Gap_Buffer<u32>& b = buf->gap_buffer;
    usize buffer_count = b.count();
    if (!buffer_count) return;

    buf->lexemes = (Lexeme*)ch::realloc(buf->lexemes, (buffer_count + 1) * sizeof(Lexeme));

    {
        buf->parse_time = -get_time();
        defer(buf->parse_time += get_time());

        u32* begin = b.data;
        u32* gap = b.gap;
        u32* gap_end = gap + b.gap_size;
        u32* buffer_end = begin + b.allocated;
        if (gap_end < buffer_end) {
            gap[0] = gap_end[0];
        }
        Lexer lexer = {};
        lexer.last = begin;
        Lexeme* lex_seeker = buf->lexemes;
        lexer.lex(begin, gap, lex_seeker);
        lexer.lex(gap_end, buffer_end, lex_seeker);
    }
}
