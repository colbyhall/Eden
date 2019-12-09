#include "gui.h"
#include "editor.h"
#include "buffer.h"
#include "draw.h"
#include "input.h"
#include "config.h"

#include "parsing.h"

#include <ch_stl/time.h>

/* STYLE */

const f32 border_width = 5.f;

/* GUI */

void gui_label(const ch::String& s, const ch::Color& color, f32 x, f32 y) {
	const ch::Vector2 draw_size = get_string_draw_size(s, the_font);
	const ch::Vector2 padding = 5.f;

	const f32 x0 = x;
	const f32 y0 = y;
	const f32 x1 = x0 + draw_size.x + padding.x;
	const f32 y1 = y0 + draw_size.y + padding.y;
	imm_quad(x0, y0, x1, y1, ch::white);
	imm_string(s, the_font, x + padding.x / 2.f, y + padding.y / 2.f, color);
}

bool gui_button(f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;

	const bool lmb_went_down = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const bool is_hovered = is_point_in_rect(current_mouse_position, x0, y0, x1, y1);

	imm_quad(x0, y0, x1, y1, get_config().foreground_color);

	return result;
}

// @TODO: Finish
bool gui_button_label(const ch::String& s, f32 x0, f32 y0, f32 x1, f32 y1) {
	bool result = false;

	const bool lmb_went_down = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const bool is_hovered = is_point_in_rect(current_mouse_position, x0, y0, x1, y1);

	imm_quad(x0, y0, x1, y1, get_config().foreground_color);

	return result;
}

static bool is_point_in_glyph(ch::Vector2 p, const Font_Glyph* g, f32 x, f32 y) {
	const f32 x0 = x;
	const f32 y0 = y;
	const f32 x1 = x0 + g->advance;
	const f32 y1 = y0 + (f32)the_font.size;

	return is_point_in_rect(p, x0, y0, x1, y1);
}

static const f32 line_number_padding = 3.f;

static void imm_line_number(u64 current_line_number, u64 max_line_number, f32* x, f32 y) {
	const Font_Glyph* space_glyph = the_font[' '];

	const u8 spaces_needed = (ch::get_num_digits(max_line_number) + 1) - ch::get_num_digits(current_line_number);

	char temp_buffer[16];
	ch::sprintf(temp_buffer, "%llu", current_line_number);

	*x += space_glyph->advance * spaces_needed; 
	const ch::Vector2 text_draw_size = imm_string(temp_buffer, the_font, *x, y, get_config().line_number_text_color);
	*x += text_draw_size.x + line_number_padding;
}

static void imm_cursor(bool edit_mode, const Font_Glyph* g, float x, float y, const ch::Color& color) {
	const f32 font_height = the_font.size;
	
	if (edit_mode) {
		imm_quad(x, y, x + g->advance, y + font_height + the_font.line_gap, color);
	} else {
		imm_border_quad(x, y, x + g->advance, y + font_height + the_font.line_gap, 1.f, color);
	}
}

#define PARSE_SPEED_DEBUG 0
#define LINE_SIZE_DEBUG 0
#define EOL_DEBUG 0

bool gui_buffer(const Buffer& buffer, usize* cursor, usize* selection, bool show_cursor, bool show_line_numbers, bool edit_mode, f32 scroll_y, f32 x0, f32 y0, f32 x1, f32 y1) {
	CH_SCOPED_TIMER(GUI_BUFFER);
	const ch::Vector2 mouse_pos = current_mouse_position;
	const bool was_lmb_pressed = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool is_lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const Config& config = get_config();

	imm_quad(x0, y0, x1, y1, config.background_color);

	const f32 font_height = the_font.size;
	const Font_Glyph* space_glyph = the_font[' '];
	const Font_Glyph* unknown_glyph = the_font['?'];

	const usize orig_cursor = *cursor;
	const usize orig_selection = *selection;

	const usize num_lines = buffer.eol_table.count;
	const ch::Gap_Buffer<u8>& gap_buffer = buffer.gap_buffer;

	const f32 starting_x = x0;
	const f32 starting_y = y0 - scroll_y;
	f32 x = starting_x;
	f32 y = starting_y;
	u64 line_number = 1;

	const f32 width = x1 - x0;
    assert(width > 0);

	f32 line_number_quad_width = 0.f;

    const u32 line_number_columns = ch::get_num_digits(num_lines) + 1;

    if (line_number_columns * space_glyph->advance >= width) {
        show_line_numbers = false;
    }

	if (show_line_numbers) {
		line_number_quad_width = (line_number_columns * space_glyph->advance) + line_number_padding;
		const f32 ln_x0 = x0;
		const f32 ln_y0 = y0;
		const f32 ln_x1 = ln_x0 + line_number_quad_width;
		const f32 ln_y1 = y1;
		imm_quad(ln_x0, ln_y0, ln_x1, ln_y1, config.line_number_background_color);
	}

	usize starting_index = 0;
	for (usize i = 0; i < buffer.line_column_table.count; i += 1) {
		if (y > -font_height) {
			starting_index = buffer.get_index_from_line(i);
			line_number = i + 1;
			break;
		}

		f32 line_size_x = buffer.line_column_table[i] * space_glyph->advance;

		while (line_size_x + space_glyph->advance * 2 > width - line_number_quad_width) {
            // width needs to be more than zero
			line_size_x -= width;
			y += font_height;
		}

		y += font_height;
	}

	if (*cursor > gap_buffer.count()) {
		*cursor = gap_buffer.count();
		*selection = *cursor;
	}

    // Some bookkeeping variables are needed to identify the current syntax highlight.
    const parsing::Lexeme* const lexemes_begin = buffer.lexemes.cbegin();
    const parsing::Lexeme* const lexemes_end = buffer.lexemes.cend();
    const parsing::Lexeme* lexeme = lexemes_begin;

	if (show_line_numbers) imm_line_number(line_number, num_lines, &x, y);
	
	for (ch::UTF8_Iterator<const ch::Gap_Buffer<u8>> it(gap_buffer, gap_buffer.count(), starting_index); it.can_advance(); it.advance()) {
		const u32 c = it.get();
		ch::Color color = config.foreground_color;

		const f32 old_x = x;
		const f32 old_y = y;

		if (!buffer.syntax_dirty && !buffer.disable_parse)
        {
            // Obtain the current lexeme based on the current index

            while (lexeme + 1 < lexemes_end &&
                   // @TEMPORARY @HACK @@@
                   ((ch::Gap_Buffer<u8>&)gap_buffer).get_index_as_cursor(it.index + 1) - 1 >= (const u8*)lexeme[1].i
                   ) {
                lexeme++;
                //push_glyph(the_font['_'], x, y, ch::magenta); // @Debug
            }

            assert(lexeme < lexemes_end);
            // @Temporary method to determine the colour of the current lexeme.
            // More nuanced parsing and configurable colours are on the roadmap. -phillip
            ch::Color stringlit = {1.0f, 1.0f, 0.2f, 1.0f};
            ch::Color comment = {0.3f, 0.3f, 0.3f, 1.0f};
            ch::Color preproc = {0.1f, 1.0f, 0.6f, 1.0f};
            ch::Color op = {0.7f, 0.7f, 0.7f, 1.0f};
            ch::Color numlit = {0.5f, 0.5f, 1.0f, 1.0f};
            ch::Color type = {0.0f, 0.7f, 0.9f, 1.0f};
            ch::Color keyword = {1.0f, 1.0f, 1.0f, 1.0f};
            ch::Color param = {1.0f, 0.6f, 0.125f, 1.0f};
            ch::Color label = op;
            switch (lexeme->dfa) {
            case parsing::DFA_FUNCTION:
                if (parsing::is_keyword(lexeme)) {
                    color = keyword;
                } else {
                    color = preproc;
                }
                break;
            case parsing::DFA_PARAM:
                if (parsing::is_keyword(lexeme)) {
                    color = keyword;
                } else {
                    color = param;
                }
                break;
            case parsing::DFA_KEYWORD:
                color = keyword;
                break;
            case parsing::DFA_PREPROC:
                color = preproc;
                break;
            case parsing::DFA_MACRO:
                color = numlit;
                break;
            case parsing::DFA_STRINGLIT:
            case parsing::DFA_STRINGLIT_BS:
            case parsing::DFA_CHARLIT:
            case parsing::DFA_CHARLIT_BS:
                color = stringlit;
                break;
            case parsing::DFA_BLOCK_COMMENT:
            case parsing::DFA_BLOCK_COMMENT_STAR:
            case parsing::DFA_LINE_COMMENT:
                color = comment;
                break;
            case parsing::DFA_WHITE_BS:
            case parsing::DFA_WHITE:
                if (lexeme > lexemes_begin && lexeme[-1].dfa == parsing::DFA_STRINGLIT) {
                    color = stringlit;
                }
                if (lexeme > lexemes_begin && lexeme[-1].dfa == parsing::DFA_CHARLIT) {
                    color = stringlit;
                }
                if (lexeme > lexemes_begin && lexeme[-1].dfa <= parsing::DFA_LINE_COMMENT) {
                    color = comment;
                }
                break;
            case parsing::DFA_IDENT:
                if (parsing::is_keyword(lexeme)) {
                    color = keyword;
                } else {
                    
                }
                break;
            case parsing::DFA_OP:
            case parsing::DFA_OP2:
                color = op;
                break;
            case parsing::DFA_NEWLINE:
                break;
            case parsing::DFA_NUMLIT:
                color = numlit;
                break;
            case parsing::DFA_SLASH:
                if (lexeme + 1 < lexemes_end && lexeme[1].dfa <= parsing::DFA_LINE_COMMENT) {
                    color = comment;
                } else {
                    color = op;
                }
                break;
            case parsing::DFA_TYPE:
                if (parsing::is_keyword(lexeme)) {
                    color = keyword;
                } else {
                    color = type;
                }
                break;
            case parsing::DFA_LABEL:
                color = label;
                break;
            default: ch_debug_trap;
            }
        }

		const Font_Glyph* g = the_font[c];
		if (!g) {
			color = ch::magenta;
			g = unknown_glyph;
		}

		if (c == '\t') {
			x += space_glyph->advance * config.tab_width;
		} else if (c == '\n' || c == '\r') {
			x += space_glyph->advance;			
			g = space_glyph;
		} else {
			x += g->advance;
		}

		const usize i = it.index;

		const bool mouse_on_line = ((c == '\n' || c == '\r' || it.is_on_last()) && mouse_pos.y >= old_y && mouse_pos.y <= old_y + font_height + the_font.line_gap);
		const bool mouse_past_eol = mouse_pos.x >= old_x;
		if (is_point_in_rect(mouse_pos, old_x, old_y, x, old_y + font_height + the_font.line_gap) || (mouse_on_line && mouse_past_eol)) {
			const usize new_cursor = it.is_on_last() ? it.index + 1 : it.index;
			if (was_lmb_pressed) {
				*cursor = new_cursor;
				*selection = *cursor;
			} else if (is_lmb_down) {
				*cursor = new_cursor;
			}
		}

		const bool is_in_selection = (orig_cursor > orig_selection && i >= orig_selection && i < orig_cursor) || (orig_cursor < orig_selection && i < orig_selection && i >= orig_cursor);
		if (is_in_selection && edit_mode) {
			imm_quad(old_x, old_y, x, old_y + font_height + the_font.line_gap, config.selection_color);
		}

		const bool is_in_cursor = *cursor == i;
		if (is_in_cursor) {
			if (show_cursor || !edit_mode) {
				imm_cursor(edit_mode, g, old_x, old_y, config.cursor_color);
			}

			if (is_in_cursor && show_cursor) {
				color = config.background_color;
			}
		}

		if (c == '\r' || c == '\n') {
			if (c == '\r' && it.can_advance()) {
				const u32 peek_c = it.peek();
				if (peek_c == '\n') {
					it.advance();
#if EOL_DEBUG
					const ch::Vector2 nl_size = imm_string("\\r\\n ", the_font, old_x, old_y, ch::magenta);
					x += nl_size.x;
#endif
				}
#if EOL_DEBUG
				else {
					const ch::Vector2 cr_size = imm_string("\\r ", the_font, old_x, old_y, ch::magenta);
					x += cr_size.x;
				}
#endif
			} 
#if EOL_DEBUG
			else {
				const ch::Vector2 nl_size = imm_string("\\n ", the_font, old_x, old_y, ch::magenta);
				x += nl_size.x;
			}
#endif

#if LINE_SIZE_DEBUG
			char temp[100];
			ch::sprintf(temp, "col: %lu, bytes: %lu", buffer.line_column_table[line_number - 1], buffer.eol_table[line_number - 1]);
			imm_string(temp, the_font, x, y, ch::magenta);
#endif;

			x = starting_x;
			y += font_height + the_font.line_gap;
			line_number += 1;
			if (show_line_numbers) {
				imm_line_number(line_number, num_lines, &x, y);
			}
			continue;
		}

		if (x + space_glyph->advance * 2 > x1) {
			const Font_Glyph* return_g = the_font[0x21B5];
			imm_glyph(g, the_font, old_x, old_y, color);

			x = starting_x;
			y += font_height + the_font.line_gap;

			if (show_line_numbers) {
                x += space_glyph->advance * (ch::get_num_digits(num_lines) + 1) + line_number_padding;
            }
		}

		if (is_in_selection && (!is_in_cursor || !show_cursor)) color = config.selected_text_color;
			
		if (!ch::is_whitespace(c) && old_y + font_height > y0) {
			imm_glyph(g, the_font, old_x, old_y, color);
		}

		if (y > y1) break;
		
#if LINE_SIZE_DEBUG || EOL_DEBUG
		if (it.is_on_last()) {
#if EOL_DEBUG
			ch::Vector2 eos_size = imm_string("0 ", the_font, x, y, ch::magenta);
			x += eos_size.x;
#endif
#if LINE_SIZE_DEBUG
			char temp[100];
			ch::sprintf(temp, "col: %lu, bytes: %lu", buffer.line_column_table[line_number - 1], buffer.eol_table[line_number - 1]);
			imm_string(temp, the_font, x, y, ch::magenta);
#endif
		}
#endif
	}

#if PARSE_SPEED_DEBUG
    if (!buffer.syntax_dirty && !buffer.disable_parse) {
        char temp[1024];

        u64 num_chars = buffer.gap_buffer.count();
        u64 num_lexemes = buffer.lexemes.count;
        u64 num_lines = buffer.eol_table.count;

        f64 gibi = 1024 * 1024 * 1024;
        f64 million = 1000 * 1000;

        f64 lex_time = buffer.lex_time / buffer.lex_parse_count;
        f64 parse_time = buffer.parse_time / buffer.lex_parse_count;
        f64 total_time = lex_time + parse_time;

	    ch::sprintf(temp,
            "Lex: %.3f GB/s (%dms: %.2f mloc/s)\n"
            "Parse: %llu million lexemes/s (%dms: %.2f mloc/s)\n"
            "Total: %.3f GB/s (%dms: %.2f mloc/s)",
            num_chars / lex_time / gibi,
            (int)(0.5f + 1000 * lex_time),
            num_lines / lex_time / million,

            (u64)(num_lexemes / parse_time / million),
            (int)(0.5f + 1000 * parse_time),
            num_lines / parse_time / million,

            num_chars / total_time / gibi,
            (int)(0.5f + 1000 * total_time),
            num_lines / total_time / million
        );
	    imm_string(temp, the_font, x0, y0, ch::magenta);
    }
#endif

	if (*cursor == gap_buffer.count() && (show_cursor || !edit_mode)) imm_cursor(edit_mode, space_glyph, x, y, config.cursor_color);

	return *cursor != orig_cursor || *selection != orig_selection || (was_lmb_pressed && is_point_in_rect(mouse_pos, x0, y0, x1, y1));
}

void Vertical_Layout::row() {
	at_y += row_height;
}