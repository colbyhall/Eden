#include "buffer_view.h"
#include "input.h"
#include "editor.h"

#include <ch_stl/math.h>

Buffer_View* focused_view;
Buffer_View* hovered_view;
ch::Array<Buffer_View*> views;
static const f32 scroll_speed = 5.f;

void Buffer_View::on_char_entered(u32 c) {
	Buffer* buffer = find_buffer(the_buffer);
	assert(buffer);

	if (c == '\r') c = ch::eol;

	reset_cursor_timer();
	switch (c) {
	case CH_KEY_BACKSPACE:
		if (cursor > -1) {
			buffer->remove_char(cursor);
			cursor -= 1;
		}
		break;
	default:
		buffer->add_char(c, cursor + 1);
		cursor += 1;
		break;
	}
}

void Buffer_View::on_action_entered(const Action_Bind& action) {

}

void tick_views(f32 dt) {
	for (Buffer_View* view : views) {
		view->current_scroll_y = ch::interp_to(view->current_scroll_y, view->target_scroll_y, dt, scroll_speed);

		u32 blink_time;
		if (!ch::get_caret_blink_time(&blink_time)) {
			view->cursor_blink_time = 0.f;
			view->show_cursor = true;
		}
		view->cursor_blink_time += dt;
		if (view->cursor_blink_time > (f32)blink_time / 1000.f) {
			view->show_cursor = !view->show_cursor;
			view->cursor_blink_time = 0.f;
		}
	}
}

static const ch::Color background_color = 0x052329FF;
static const ch::Color foreground_color = 0xd6b58dFF;
static const ch::Color cursor_color = 0x81E38EFF;
static const ch::Color selection_color = 0x000EFFFF;
static const ch::Color selected_text_color = ch::white;

static void draw_buffer_view(const Buffer_View& view, f32 x0, f32 y0, f32 x1, f32 y1) {
	const Buffer* buffer = find_buffer(view.the_buffer);
	assert(buffer);

	the_font.bind();
	immediate_begin();
	const f32 font_height = the_font.size;
	const ch::Gap_Buffer<u32>& gap_buffer = buffer->gap_buffer;

	immediate_quad(x0, y0, x1, y1, background_color);

	// @NOTE(CHall): draw gap buffer 
	{
		auto draw_rect_at_char = [](f32 x, f32 y, const Font_Glyph& g, const ch::Color& color) {
			y += the_font.ascent;
			const f32 x0 = x;
			const f32 y0 = y - the_font.ascent;
			const f32 x1 = x0 + g.advance;
			const f32 y1 = y - the_font.descent;
			immediate_quad(x0, y0, x1, y1, color);
		};

		const bool show_cursor = view.show_cursor;

		const f32 original_x = x0;
		const f32 original_y = y0;

		f32 x = original_x;
		f32 y = original_y;
		for (usize i = 0; i < gap_buffer.count(); i++) {
			if (gap_buffer[i] == ch::eol) {
				y += font_height;
				x = original_x;
				continue;
			}

			if (gap_buffer[i] == '\t') {
				const Font_Glyph* space_glyph = the_font[' '];
				assert(space_glyph);
				x += space_glyph->advance  * 4.f;
				continue;
			}

			if (gap_buffer[i] == ch::eol) {
				x = original_x;
				y += font_height;
				continue;
			}

			const bool is_in_cursor = view.cursor + 1 == i && show_cursor;

			ch::Color color = foreground_color;
			const Font_Glyph* glyph = the_font[gap_buffer[i]];
			if (!glyph) {
				glyph = the_font['?'];
				color = ch::magenta;
			}

			if (is_in_cursor) {
				draw_rect_at_char(x, y, *glyph, cursor_color);
			}
			immediate_glyph(*glyph, the_font, x, y, color);

			x += glyph->advance;
		}

		if (view.cursor + 1 == gap_buffer.count() && show_cursor) draw_rect_at_char(x, y, *the_font[' '], cursor_color);
	}
	// @NOTE(CHall): draw info bar
	{
		const ch::Vector2 padding = 1.f;
		const f32 bar_height = font_height + padding.x;
		immediate_quad(x0, y1 - bar_height, x1, y1, foreground_color);

		tchar text_buffer[1024];
		ch::sprintf(text_buffer, CH_TEXT("%llu"), buffer->eol_table.count);
		immediate_string(text_buffer, the_font, x0 + (padding.x / 2.f), (y1 - bar_height) + (padding.y / 2.f), background_color);
	}
	immediate_flush();
}

void draw_views() {
	const ch::Vector2 viewport_size = the_window.get_viewport_size();
	const f32 viewport_width = (f32)viewport_size.ux;
	const f32 viewport_height = (f32)viewport_size.uy;

	f32 x = 0.f;
	for (usize i = 0; i < views.count; i++) {
		const Buffer_View* view = views[i];

		const f32 x0 = x;
		const f32 y0 = 0.f;
		const f32 x1 = (i == views.count - 1) ? viewport_width - x : viewport_width * view->width_ratio;
		const f32 y1 = viewport_height;
		draw_buffer_view(*view, x0, y0, x1, y1);

		x += x1 - x0;
	}
}

usize push_view(Buffer_ID the_buffer) {
	Buffer_View* view = ch_new Buffer_View;
	view->the_buffer = the_buffer;
	if (!focused_view) focused_view = view;
	return views.push(view);
}

usize insert_view(Buffer_ID the_buffer, usize index) {
	Buffer_View* view = ch_new Buffer_View;
	view->the_buffer = the_buffer;
	views.insert(view, index);
	return index;
}

bool remove_view(usize view_index) {
	assert(view_index < views.count);
	Buffer_View* view = views[view_index];
	ch_delete view;
	views.remove(view_index);
	return true;
}

Buffer_View* get_view(usize index) {
	return views[index];
}

ssize get_view_index(Buffer_View* view) {
	return views.find(view);
}
