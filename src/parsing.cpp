#include "parsing.h"
#include "buffer.h"
#include <ch_stl\time.h>

namespace parsing {
// This is a column-reduction table to map 128 ASCII values to a 11-input space.
static const u8 char_type[] = {
    IDENT, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, NEWLINE,
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
};
// This is a state transition table for the deterministic finite
// automaton (DFA) lexer. Overtop this DFA runs a block-comment scanner.
static const u8 lex_table[] = {
    // WHITE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE,         // DFA_SLASH
    DFA_WHITE,         // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_WHITE,         // DFA_NEWLINE
    DFA_WHITE,         // DFA_IDENT
    DFA_WHITE,         // DFA_OP
    DFA_WHITE,         // DFA_OP2
    DFA_WHITE,         // DFA_NUMLIT
    // NEWLINE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_NEWLINE,       // DFA_LINE_COMMENT
    DFA_NEWLINE,       // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_NEWLINE,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NEWLINE,       // DFA_SLASH
    DFA_NEWLINE,       // DFA_WHITE
    DFA_WHITE,         // DFA_WHITE_BS
    DFA_NEWLINE,       // DFA_NEWLINE
    DFA_NEWLINE,       // DFA_IDENT
    DFA_NEWLINE,       // DFA_OP
    DFA_NEWLINE,       // DFA_OP2
    DFA_NEWLINE,       // DFA_NUMLIT
    // IDENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_IDENT,         // DFA_SLASH
    DFA_IDENT,         // DFA_WHITE
    DFA_IDENT,         // DFA_WHITE_BS
    DFA_IDENT,         // DFA_NEWLINE
    DFA_IDENT,         // DFA_IDENT
    DFA_IDENT,         // DFA_OP
    DFA_IDENT,         // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DOUBLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_WHITE,         // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_STRINGLIT,     // DFA_SLASH
    DFA_STRINGLIT,     // DFA_WHITE
    DFA_STRINGLIT,     // DFA_WHITE_BS
    DFA_STRINGLIT,     // DFA_NEWLINE
    DFA_STRINGLIT,     // DFA_IDENT
    DFA_STRINGLIT,     // DFA_OP
    DFA_STRINGLIT,     // DFA_OP2
    DFA_STRINGLIT,     // DFA_NUMLIT
    // SINGLEQUOTE
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_WHITE,         // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_CHARLIT,       // DFA_SLASH
    DFA_CHARLIT,       // DFA_WHITE
    DFA_CHARLIT,       // DFA_WHITE_BS
    DFA_CHARLIT,       // DFA_NEWLINE
    DFA_CHARLIT,       // DFA_IDENT
    DFA_CHARLIT,       // DFA_OP
    DFA_CHARLIT,       // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // DIGIT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_NUMLIT,        // DFA_SLASH
    DFA_NUMLIT,        // DFA_WHITE
    DFA_NUMLIT,        // DFA_WHITE_BS
    DFA_NUMLIT,        // DFA_NEWLINE
    DFA_IDENT,         // DFA_IDENT
    DFA_NUMLIT,        // DFA_OP
    DFA_NUMLIT,        // DFA_OP2
    DFA_NUMLIT,        // DFA_NUMLIT
    // SLASH
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_WHITE,         // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_LINE_COMMENT,  // DFA_SLASH
    DFA_SLASH,         // DFA_WHITE
    DFA_SLASH,         // DFA_WHITE_BS
    DFA_SLASH,         // DFA_NEWLINE
    DFA_SLASH,         // DFA_IDENT
    DFA_SLASH,         // DFA_OP
    DFA_SLASH,         // DFA_OP2
    DFA_SLASH,         // DFA_NUMLIT
    // STAR
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT_STAR, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,       // DFA_LINE_COMMENT
    DFA_STRINGLIT,          // DFA_STRINGLIT
    DFA_STRINGLIT,          // DFA_STRINGLIT_BS
    DFA_CHARLIT,            // DFA_CHARLIT
    DFA_CHARLIT,            // DFA_CHARLIT_BS
    DFA_BLOCK_COMMENT,      // DFA_SLASH
    DFA_OP,                 // DFA_WHITE
    DFA_OP,                 // DFA_WHITE_BS
    DFA_OP,                 // DFA_NEWLINE
    DFA_OP,                 // DFA_IDENT
    DFA_OP2,                // DFA_OP
    DFA_OP,                 // DFA_OP2
    DFA_OP,                 // DFA_NUMLIT
    // BS
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT_BS,  // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT_BS,    // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_WHITE_BS,      // DFA_SLASH
    DFA_WHITE_BS,      // DFA_WHITE
    DFA_WHITE_BS,      // DFA_WHITE_BS
    DFA_WHITE_BS,      // DFA_NEWLINE
    DFA_WHITE_BS,      // DFA_IDENT
    DFA_WHITE_BS,      // DFA_OP
    DFA_WHITE_BS,      // DFA_OP2
    DFA_WHITE_BS,      // DFA_NUMLIT
    // OP
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT
    DFA_BLOCK_COMMENT, // DFA_BLOCK_COMMENT_STAR
    DFA_LINE_COMMENT,  // DFA_LINE_COMMENT
    DFA_STRINGLIT,     // DFA_STRINGLIT
    DFA_STRINGLIT,     // DFA_STRINGLIT_BS
    DFA_CHARLIT,       // DFA_CHARLIT
    DFA_CHARLIT,       // DFA_CHARLIT_BS
    DFA_OP,            // DFA_SLASH
    DFA_OP,            // DFA_WHITE
    DFA_OP,            // DFA_WHITE_BS
    DFA_OP,            // DFA_NEWLINE
    DFA_OP,            // DFA_IDENT
    DFA_OP2,           // DFA_OP
    DFA_OP,            // DFA_OP2
    DFA_OP,            // DFA_NUMLIT
};

struct Lexer_Data {
    u8 dfa;
    const u32* idx_base;
};

static const u32 *lex(Lexer_Data& l, const u32* p, const u32* const end,
                      Lexeme*& lexemes) {
    for (; p < end; p++) {
        u8 new_dfa = lex_table[l.dfa + char_type[(int)*p & (((int)*p - 128) >> 31)]];
        if (new_dfa != l.dfa) {
            lexemes->i = p;
            lexemes->dfa = new_dfa;
            lexemes++;
            l.dfa = new_dfa;
        }
    }
    return p;
}

Lexeme* skip_comments_in_line(Lexeme* l, Lexeme* end) {
    while (l < end &&
           (l->dfa <= DFA_LINE_COMMENT ||
            (l->dfa >= DFA_WHITE && l->dfa < DFA_NEWLINE) ||
            (l->dfa == DFA_SLASH && l[1].dfa <= DFA_LINE_COMMENT))) {
        l++;
    }
    return l;
}
Lexeme* parse_preproc(Lexeme* l, Lexeme* end) {
    l->dfa = DFA_PREPROC;
    l++;
    l = skip_comments_in_line(l, end);
    if (l->dfa == DFA_IDENT) {
        l->dfa = DFA_PREPROC;
        l++;
        l = skip_comments_in_line(l, end);
        if (l < end && l->i[0] == '<') {
            l->dfa = DFA_STRINGLIT;
            while (l < end && l->dfa != DFA_NEWLINE) {
                l->dfa = DFA_STRINGLIT;
                l++;
                if (l < end && l->i[0] == '>') {
                    l->dfa = DFA_STRINGLIT;
                    l++;
                    break;
                }
            }
        } else if (l->dfa == DFA_IDENT) {
            l->dfa = DFA_MACRO;
        }
    }
    while (l < end && l->dfa != DFA_NEWLINE) {
        l++;
    }
    return l;
}
Lexeme* next_token(Lexeme* l, Lexeme* end) {
    l = skip_comments_in_line(l, end);
    if (l->dfa == DFA_NEWLINE) {
        l++;
        l = skip_comments_in_line(l, end);
        if (l < end && l->i[0] == '#') parse_preproc(l, end);
    }
    return l;
}

void parse(Lexeme* l, Lexeme* end, const u32* gap, const u32* gap_end) {
    if (l < end && l->i[0] == '#') parse_preproc(l, end);
    while (l < end) {
        l = next_token(l, end);
        l++;
        // @Todo: more here
    }
}

void parse_cpp(Buffer* buf) {
    if (!buf->syntax_dirty) return;
    buf->syntax_dirty = false;
    ch::Gap_Buffer<u32>& b = buf->gap_buffer;
    usize buffer_count = b.count();

    buf->lexemes.reserve((buffer_count + 1) - buf->lexemes.allocated);
    buf->lexemes.count = 1;
    if (buffer_count) {

        u32* gap_end = b.gap + b.gap_size;
        u32* buffer_end = b.data + b.allocated;
        Lexer_Data lexer = {};
        lexer.dfa = DFA_WHITE;
        lexer.idx_base = b.data;
        Lexeme* lex_seeker = buf->lexemes.begin();
        
        f64 lex_time = -ch::get_time_in_seconds();
        const u32* const finished_on = lex(lexer, b.data, b.gap, lex_seeker);
        const usize skip_ahead = finished_on - b.gap;
        lexer.idx_base = b.data + b.gap_size;
        lex(lexer, gap_end + skip_ahead, buffer_end, lex_seeker);
        lex_time += ch::get_time_in_seconds();
        {
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->i = buffer_end;
            lex_seeker->dfa = DFA_WHITE;
            lex_seeker++;
        }
        buf->lexemes.count = lex_seeker - buf->lexemes.begin();

        f64 parse_time = -ch::get_time_in_seconds();
        parse(buf->lexemes.begin(), buf->lexemes.end(), b.gap, gap_end);
        parse_time += ch::get_time_in_seconds();
        buf->lex_time = lex_time;
        buf->parse_time = parse_time;
    }
}
} // namespace parsing
