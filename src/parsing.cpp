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
    DFA_OP,            // DFA_STRINGLIT
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
    DFA_OP,            // DFA_CHARLIT
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

struct Lexer_Data {
    u8 dfa;
    const u32* idx_base;
};

void lex(Lexer_Data& l, const u32* p, const u32* const end, Lexeme*& lexemes) {
    for (; p < end; p++) {
        int c = (int)*p;
        c &= (c - 128) >> 31;
        u8 new_dfa = lex_table[l.dfa + char_type[c]];
        if (new_dfa != l.dfa) {
            lexemes->i = p;
            lexemes->dfa = new_dfa;
            lexemes++;
            l.dfa = new_dfa;
        }
    }
}

const u32* temp_parser_gap;
u64 temp_parser_gap_size;
u64 toklen(Lexeme* l) {
    const u32* next_i;
    if (l[1].dfa == DFA_NUM_STATES) {
        next_i = l[2].i;
    } else {
        next_i = l[1].i;
    }
    if (l->i < temp_parser_gap && next_i > temp_parser_gap) {
        return next_i - l->i - temp_parser_gap_size;
    } else {
        return next_i - l->i;
    }
}

bool nested = false; // JUST for debugging

void parse(Lexeme* l, Lexeme* end);

Lexeme* skip_comments_in_line(Lexeme* l, Lexeme* end) {
    while (l < end && (l->dfa < DFA_NEWLINE || l->dfa == DFA_SLASH && l[1].dfa <= DFA_LINE_COMMENT)) {
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
        if (l->i[0] == '<') {
            l->dfa = DFA_STRINGLIT;
            while (l < end && l->dfa != DFA_NEWLINE) {
                l->dfa = DFA_STRINGLIT;
                l++;
                if (l->i[0] == '>') {
                    l->dfa = DFA_STRINGLIT;
                    l++;
                    break;
                }
            }
        } else if (l->dfa == DFA_IDENT) {
            l->dfa = DFA_MACRO;
            l++;
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
Lexeme* next_token(Lexeme* l, Lexeme* end) {
skip_more_comments:;
    l = skip_comments_in_line(l, end);
    if (l < end && l->dfa == DFA_NEWLINE) {
        l++;
        if (l->i[0] == '#') {
            assert(!nested);
            l = parse_preproc(l, end);
        }
        goto skip_more_comments;
    }
    return l;
}

Lexeme* parse_expr(Lexeme* l, Lexeme* end) {
    if (l->dfa == DFA_IDENT) {
        Lexeme* ident = l;
        l++;
        l = next_token(l, end);
        if (l->i[0] == '(') {
            ident->dfa = DFA_FUNCTION;
        }
    }
    return l;
}

Lexeme* parse_stmt(Lexeme* l, Lexeme* end) {

    if (l->dfa == DFA_IDENT) {
        Lexeme* last_ident = l;
        while (l < end) {
            if (l->i[0] == ':' && l[1].i[0] == ':') {
                l++;
                l++;
                l = next_token(l, end);
            }
            if (l->dfa == DFA_IDENT) {
                last_ident = l;
            } else if (l->i[0] != '*' && l->i[0] != '&') {
                break;
            }
            l->dfa = DFA_TYPE;
            l++;
            l = next_token(l, end);
        }
        last_ident->dfa = DFA_IDENT;
        if (l->i[0] == '(') {
            last_ident->dfa = DFA_FUNCTION;
        }
    }
    while (l < end && l->i[0] != ';'
                   && l->i[0] != ':'
                   && l->i[0] != '{'
                   && l->i[0] != '}'
                   && l->i[0] != ']'
                   && l->i[0] != ')') {
        l++;
        l = next_token(l, end);
        l = parse_expr(l, end);
    }
    return l;
}

void parse(Lexeme* l, Lexeme* end) {
    while (l < end) {
        l = next_token(l, end);
        l = parse_stmt(l, end);
        l++;
    }
}

void parse_cpp(Buffer* buf) {
    if (!buf->syntax_dirty || buf->disable_parse) return;
    buf->syntax_dirty = false;
    ch::Gap_Buffer<u32>& b = buf->gap_buffer;
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
        Lexer_Data lexer = {};
        lexer.dfa = DFA_NEWLINE;
        lexer.idx_base = b.data;
        Lexeme* lex_seeker = buf->lexemes.begin();
        {
            lex_seeker->i = b.data;
            lex_seeker->dfa = lexer.dfa;
            lex_seeker++;
        }
        
        f64 lex_time = -ch::get_time_in_seconds();
        lex(lexer, b.data, b.gap, lex_seeker);
        Lexeme* lexeme_at_gap = lex_seeker - 1;
        lex(lexer, b.gap + b.gap_size, b.data + b.allocated, lex_seeker);
        lex_time += ch::get_time_in_seconds();

        if (lexeme_at_gap > buf->lexemes.begin() && lexeme_at_gap < lex_seeker) {
            assert(lexeme_at_gap->i < b.gap);
            if (lexeme_at_gap + 1 < lex_seeker) {
                assert(lexeme_at_gap[1].i >= b.gap + b.gap_size);
            }
            b.move_gap_to_index(lexeme_at_gap->i - b.data);
            assert(lexeme_at_gap->i == b.gap);
        }

        {
            assert(lex_seeker < buf->lexemes.begin() + buf->lexemes.allocated);
            lex_seeker->dfa = DFA_NUM_STATES;
            static const u32 dummy_buffer[16];
            lex_seeker->i = &dummy_buffer[0]; // So the parser can safely read from here.
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
