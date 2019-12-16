#include "buffer_view.h"
#include "input.h"
#include "editor.h"
#include "config.h"
#include "gui.h"

static ch::Array<Buffer_View> views;
static usize focused_view;

static void ensure_all_cursors_visible() {
    for (usize i = 0; i < views.count; i++) {
        views[i].ensure_cursor_in_view();
    }
}

static f32 get_view_width(f32 viewport_width, usize i) {
    if (i < views.count - 1) {
        return viewport_width * views[i].width_ratio;
    } else {
        f32 sum_of_widths = 0;
        for (usize i = 0; i < views.count - 1; i++) {
            sum_of_widths += views[i].width_ratio;
        }
        return viewport_width * (1 - sum_of_widths);
    }
}

static u64 get_view_columns(f32 viewport_width, usize i) {
    Buffer_View* view = &views[i];
    Buffer* buffer = find_buffer(view->the_buffer);
	assert(buffer);
    u64 result = (u64)(get_view_width(viewport_width, i) / the_font[' ']->advance);
    result -= (ch::get_num_digits(buffer->eol_table.count) + 1);
    //if (result > (ch::get_num_digits(buffer->eol_table.count) + 1)) {
    //    result -= (ch::get_num_digits(buffer->eol_table.count) + 1);
    //}
    return result;
}


static bool is_point_in_glyph(ch::Vector2 p, const Font_Glyph* g, f32 x, f32 y) {
	const f32 x0 = x;
	const f32 y0 = y;
	const f32 x1 = x0 + g->advance;
	const f32 y1 = y0 + (f32)the_font.size;

	return is_point_in_rect(p, x0, y0, x1, y1);
}

static void imm_line_number(u64 current_line_number, u64 max_line_number, f32* x, f32 y, bool on_cursor_line) {
	const Font_Glyph* space_glyph = the_font[' '];

	const u8 spaces_needed = ch::get_num_digits(max_line_number) - ch::get_num_digits(current_line_number) + 1;

	char temp_buffer[16];
	ch::sprintf(temp_buffer, "%llu", current_line_number);
	
	if (!on_cursor_line) {
		*x += space_glyph->advance * spaces_needed;
	}
	const ch::Vector2 text_draw_size = imm_string(temp_buffer, the_font, *x, y, on_cursor_line ? get_config().foreground_color : get_config().line_number_text_color);
	*x += text_draw_size.x;

	if (on_cursor_line) {
		*x += space_glyph->advance * spaces_needed;
	}

	*x += space_glyph->advance;
}

static void imm_cursor(bool edit_mode, const Font_Glyph* g, float x, float y, const ch::Color& color) {
	const f32 font_height = the_font.size;

	if (edit_mode) {
		imm_quad(x, y, x + g->advance, y + font_height + the_font.line_gap, color);
	}
	else {
		imm_border_quad(x, y, x + g->advance, y + font_height + the_font.line_gap, 1.f, color);
	}
}

#define PARSE_SPEED_DEBUG 0
#define LINE_SIZE_DEBUG 0
#define EOL_DEBUG 0

// TODO: Finish up to fit gui system
static void gui_buffer_view(UI_ID id, Buffer_View* view, f32 x0, f32 y0, f32 x1, f32 y1) {
	const ch::Vector2 mouse_pos = current_mouse_position;
	const bool was_lmb_pressed = was_mouse_button_pressed(CH_MOUSE_LEFT);
	const bool is_lmb_down = is_mouse_button_down(CH_MOUSE_LEFT);

	const Buffer* const buffer = find_buffer(view->the_buffer);

	const Config& config = get_config();

	imm_quad(x0, y0, x1, y1, config.background_color);

	const f32 font_height = the_font.size;
	const Font_Glyph* space_glyph = the_font[' '];
	const Font_Glyph* unknown_glyph = the_font['?'];

	// TODO: Remove this
	bool edit_mode = true;
	bool show_cursor = true;
	usize* cursor = &view->cursor;
	usize* selection = &view->selection;

	const usize orig_cursor = *cursor;
	const usize orig_selection = *selection;

	const usize num_lines = buffer->eol_table.count;
	const ch::Gap_Buffer<u8>& gap_buffer = buffer->gap_buffer;

	const f32 starting_x = x0;
	const f32 starting_y = y0 - view->current_scroll_y;
	f32 x = starting_x;
	f32 y = starting_y;
	u64 line_number = 1;

	const f32 width = x1 - x0;
	assert(width > 0);

	f32 line_number_quad_width = 0.f;

	const u32 line_number_columns = ch::get_num_digits(num_lines) + 2;

	bool show_line_numbers = config.show_line_numbers;

	if (line_number_columns * space_glyph->advance >= width) {
		show_line_numbers = false;
	}

	if (show_line_numbers) {
		line_number_quad_width = (line_number_columns * space_glyph->advance);
		const f32 ln_x0 = x0;
		const f32 ln_y0 = y0;
		const f32 ln_x1 = ln_x0 + line_number_quad_width;
		const f32 ln_y1 = y1;
		imm_quad(ln_x0, ln_y0, ln_x1, ln_y1, config.line_number_background_color);
	}

	usize starting_index = 0;
	for (usize i = 0; i < buffer->line_column_table.count; i += 1) {
		if (y > -font_height) {
			starting_index = buffer->get_index_from_line(i);
			line_number = i + 1;
			break;
		}

		f32 line_size_x = buffer->line_column_table[i] * space_glyph->advance;

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
	const parsing::Lexeme* const lexemes_begin = buffer->lexemes.cbegin();
	const parsing::Lexeme* const lexemes_end = buffer->lexemes.cend();
	const parsing::Lexeme* lexeme = lexemes_begin;

	if (show_line_numbers) imm_line_number(line_number, num_lines, &x, y, view->current_line == 0);
	if (view->current_line == 0) {
		imm_quad(x, y, x1, y + font_height + the_font.line_gap, config.line_number_background_color);
	}

	// @HACK: We have to do this until we move away from wait for events
	bool found_new_cursor_pos = false;
	const bool mouse_over = is_point_in_rect(mouse_pos, x0, y0, x1, y1);

	for (ch::UTF8_Iterator<const ch::Gap_Buffer<u8>> it(gap_buffer, gap_buffer.count(), starting_index); it.can_advance(); it.advance()) {
		const u32 c = it.get();
		ch::Color color = config.foreground_color;

		const f32 old_x = x;
		const f32 old_y = y;

		if (!buffer->syntax_dirty && !buffer->disable_parse)
		{
			while (lexeme + 1 < lexemes_end && ((ch::Gap_Buffer<u8>&)gap_buffer).get_index_as_cursor(it.index + 1) - 1 >= (const u8*)lexeme[1].i) {
				lexeme += 1;
			}

			assert(lexeme < lexemes_end);
			// @Temporary method to determine the colour of the current lexeme.
			// More nuanced parsing and configurable colours are on the roadmap. -phillip
			ch::Color stringlit = { 1.0f, 1.0f, 0.2f, 1.0f };
			ch::Color comment = { 0.3f, 0.3f, 0.3f, 1.0f };
			ch::Color preproc = { 0.1f, 1.0f, 0.6f, 1.0f };
			ch::Color op = { 0.7f, 0.7f, 0.7f, 1.0f };
			ch::Color numlit = { 0.5f, 0.5f, 1.0f, 1.0f };
			ch::Color type = { 0.0f, 0.7f, 0.9f, 1.0f };
			ch::Color keyword = { 1.0f, 1.0f, 1.0f, 1.0f };
			ch::Color param = { 1.0f, 0.6f, 0.125f, 1.0f };
			ch::Color label = op;
			switch (lexeme->dfa) {
			case parsing::DFA_FUNCTION:
				if (parsing::is_keyword(lexeme)) {
					color = keyword;
				}
				else {
					color = preproc;
				}
				break;
			case parsing::DFA_PARAM:
				if (parsing::is_keyword(lexeme)) {
					color = keyword;
				}
				else {
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
				}
				else {

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
				}
				else {
					color = op;
				}
				break;
			case parsing::DFA_TYPE:
				if (parsing::is_keyword(lexeme)) {
					color = keyword;
				}
				else {
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
		}
		else if (c == '\n' || c == '\r') {
			x += space_glyph->advance;
			g = space_glyph;
		}
		else {
			x += g->advance;
		}

		const usize i = it.index;

		const bool mouse_on_line = ((c == '\n' || c == '\r' || it.is_on_last()) && mouse_pos.y >= old_y && mouse_pos.y <= old_y + font_height + the_font.line_gap);
		const bool mouse_past_eol = mouse_pos.x >= old_x;
		if ((is_point_in_rect(mouse_pos, old_x, old_y, x, old_y + font_height + the_font.line_gap) || (mouse_on_line && mouse_past_eol)) && mouse_over) {
			const usize new_cursor = it.is_on_last() ? it.index + 1 : it.index;
			if (was_lmb_pressed) {
				*cursor = new_cursor;
				*selection = *cursor;
				found_new_cursor_pos = true;
			}
			else if (is_lmb_down) {
				*cursor = new_cursor;
			}
		}

		const bool should_draw_selection_or_cursor = ((mouse_over && was_lmb_pressed && found_new_cursor_pos) || !was_lmb_pressed || (was_lmb_pressed && !mouse_over));

		const bool is_in_selection = ((orig_cursor > orig_selection && i >= orig_selection && i < orig_cursor) || (orig_cursor < orig_selection && i < orig_selection && i >= orig_cursor)) && should_draw_selection_or_cursor;
		if (is_in_selection && edit_mode) {
			imm_quad(old_x, old_y, x, old_y + font_height + the_font.line_gap, config.selection_color);
		}

		const bool is_in_cursor = *cursor == i && should_draw_selection_or_cursor;
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

			const bool on_cursor_line = view->current_line == line_number - 1;
			if (show_line_numbers) {
				imm_line_number(line_number, num_lines, &x, y, on_cursor_line);
			}

			if (on_cursor_line) {
				imm_quad(x, y, x1, y + font_height + the_font.line_gap, config.line_number_background_color);
			}

			continue;
		}

		if (x + space_glyph->advance * 2 > x1) {
			const Font_Glyph* return_g = the_font[0x21B5];
			imm_glyph(g, the_font, old_x, old_y, color);

			x = starting_x;
			y += font_height + the_font.line_gap;

			if (show_line_numbers) {
				x += space_glyph->advance * (ch::get_num_digits(num_lines) + 2);
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

	if (*cursor != orig_cursor || *selection != orig_cursor) {
		view->update_column_info(true);
	}
}

void Buffer_View::remove_selection() {
	if (!has_selection()) return;

	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	if (cursor > selection) {
		for (usize i = selection; i < cursor; i = buffer->find_next_char(i)) {
			buffer->gap_buffer.remove_at_index(selection);
		}
		cursor = selection;
	} else {
		for (usize i = cursor; i <= buffer->find_prev_char(selection); i = buffer->find_next_char(i)) {
			if (i < buffer->gap_buffer.count()) {
				buffer->gap_buffer.remove_at_index(cursor);
			}
		}
		selection = cursor;
	}

	buffer->refresh_line_tables();
	update_column_info(true);
	buffer->mark_file_dirty();
}

void Buffer_View::update_column_info(bool update_desired_col) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	current_line = buffer->get_line_from_index(cursor);
	const u64 line_index = buffer->get_index_from_line(current_line);

	u64 column_count = 0;
	for (u64 i = line_index; i < cursor; i = buffer->find_next_char(i)) {
		const u32 c = buffer->get_char(i);

		column_count += get_char_column_size(c);
	}

	current_column = column_count;
	if (update_desired_col) desired_column = column_count;
}

void Buffer_View::ensure_cursor_in_view() {
    Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	return;

    u64 max_line_width = get_view_columns((f32)the_window.get_viewport_size().ux, this - views.begin()); // @Hack: this "self-contained view acting only on itself" abstraction is bad

    f32 view_height = (f32)the_window.get_viewport_size().uy;
    f32 font_height = (f32)the_font.size;

    f32 cursor_y = (buffer->get_wrapped_line_from_index(cursor, max_line_width)) * font_height; // @Todo: need to compute the exact value for wrapped line scrolling
    // print_to_messages("%llu\n", max_line_width);

    if (target_scroll_y > cursor_y - font_height * 2) {
	    target_scroll_y = cursor_y - font_height * 2;
    } else if (target_scroll_y < cursor_y + font_height * 2 - view_height) {
	    target_scroll_y = cursor_y + font_height * 2 - view_height;
    }
}

void Buffer_View::on_char_entered(u32 c) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	// @NOTE(CHall): Needs to ensure we're doing the correct encoding with push

	remove_selection();
	buffer->add_char(c, cursor);
	cursor += 1;
	selection = cursor;

	update_column_info(true);
	reset_cursor_timer();
    buffer->syntax_dirty = true;

	buffer->mark_file_dirty();
}

void tick_views(f32 dt) {
	f32 x = 0.f;
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;
	const ch::Vector2 mouse_pos = current_mouse_position;

    if (!viewport_width || !viewport_height) return;

	const Config& config = get_config();
	u32 blink_time;
	ch::get_caret_blink_time(&blink_time);

	for (usize i = 0; i < views.count; i += 1) {
		Buffer_View* const view = &views[i];
		Buffer* const the_buffer = find_buffer(view->the_buffer);
		assert(the_buffer);

		if (!blink_time) {
			view->cursor_blink_time = 0.f;
			view->show_cursor = true;
		} else {
			view->cursor_blink_time += dt;
			if (view->cursor_blink_time > (f32)blink_time / 1000.f) {
				view->show_cursor = !view->show_cursor;
				view->cursor_blink_time = 0.f;
			}
		}

		parsing::parse_cpp(the_buffer);

		const float powerline_padding = 2.f;
		const float powerline_height = (float)the_font.size + the_font.line_gap;

		// @NOTE(CHall): Draw buffer
		{
			const f32 x0 = x;
			const f32 y0 = 0.f;
			const f32 x1 = x0 + get_view_width(viewport_width, i);
			const f32 y1 = viewport_height - (powerline_height + powerline_padding * 2.f);

			if (is_point_in_rect(mouse_pos, x0, y0, x1, y1) && !(view->target_scroll_y == 0.f && current_mouse_scroll_y > 0.f)) view->target_scroll_y -= current_mouse_scroll_y;

			view->current_scroll_y = ch::interp_to(view->current_scroll_y, view->target_scroll_y, dt, config.scroll_speed);

			gui_button(the_buffer, 0.f, 0.f, 100.f, 100.f);

			gui_buffer_view(view, view, x0, y0, x1, y1);
		}

		// @NOTE(CHall): Draw powerline
		{
			const f32 x0 = x;
			const f32 y0 = viewport_height - powerline_height - powerline_padding;
			const f32 x1 = x0 + get_view_width(viewport_width, i);
			const f32 y1 = y0 + powerline_height + powerline_padding;

			imm_quad(x0, y0, x1, y1, config.foreground_color);

			{
				const f32 text_y = y0 + powerline_padding;

				const f32 horz_padding = 10.f;

				const usize current_column = view->current_column + 1;
				const usize current_line = view->current_line + 1;
				const usize num_lines = the_buffer->eol_table.count;

				u64 total_col = 0;
				u64 cursor_col = 0;
				for (usize i = 0; i < the_buffer->line_column_table.count; i += 1) {
					const u64 col = the_buffer->line_column_table[i];

					if (i == view->current_line) {
						cursor_col = total_col + view->current_column;
					}

					total_col += col;
				}
				const f32 percent_through_file = total_col ? ((f32)cursor_col / (f32)total_col) * 100.f : 0.f;

				const char* line_ending = get_line_ending_display(the_buffer->line_ending);
				const char* encoding = get_buffer_encoding_display(the_buffer->encoding);

				const bool is_read_only = (the_buffer->flags & BF_ReadOnly) == BF_ReadOnly;
				char buffer[512];
				ch::sprintf(buffer, "%s | %s | %llu:%llu | %.0f%% | %llu lines%s", line_ending, encoding, current_line, current_column, percent_through_file, num_lines, is_read_only ? " | read-only" : "");

				const ch::Vector2 fi_size = get_string_draw_size(buffer, the_font);
				imm_string(buffer, the_font, x1 - fi_size.x - horz_padding, text_y, config.background_color);

				ch::sprintf(buffer, "%.*s%s", the_buffer->name.count, the_buffer->name.data, the_buffer->is_dirty ? "*" : "");
				imm_string(buffer, the_font, x0 + horz_padding, text_y, config.background_color);
			}
		}

		// @TODO(CHall): Calculate max buffer size

		if (view->target_scroll_y < 0.f) {
			view->target_scroll_y = 0.f;
		}

		x += get_view_width(viewport_width, i);
	}
}

Buffer_View* get_focused_view() {
	if (views.count > 0) {
		assert(focused_view < views.count);
		return &views[focused_view];
	}
	return nullptr;
}

usize push_view(Buffer_ID the_buffer) {
	Buffer_View view = {};
	view.the_buffer = the_buffer;
    usize result = views.push(view);
	return result;
}

usize insert_view(Buffer_ID the_buffer, usize index) {
	Buffer_View view = {};
	view.the_buffer = the_buffer;
	views.insert(view, index);
	return index;
}

bool remove_view(usize view_index) {
	assert(view_index < views.count);
	views.remove(view_index);
	return true;
}

Buffer_View* get_view(usize index) {
	return &views[index];
}