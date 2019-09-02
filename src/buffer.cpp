#include "buffer.h"

Buffer::Buffer() {
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();
}

Buffer::Buffer(Buffer_ID _id) : id(_id) {
	Buffer();
}

void draw_buffer(const Buffer& buffer, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index) {
	font.bind();
	immediate_begin();
	const f32 font_height = font.size;

	const ch::Gap_Buffer<u32>& gap_buffer = buffer.gap_buffer;

	const f32 original_x = x;
	const f32 original_y = y;

	for (usize i = 0; i < gap_buffer.count(); i++) {
		if (gap_buffer[i] == ch::eol) {
			y += font_height;
			x = original_x;
			continue;
		}

		if (gap_buffer[i] == '\t') {
			const Font_Glyph* space_glyph = font[' '];
			assert(space_glyph);
			x += space_glyph->advance  * 4.f;
			continue;
		}

		const Font_Glyph* glyph = font[gap_buffer[i]];

		if (glyph) {
			immediate_glyph(*glyph, font, x, y, color, z_index);

			x += glyph->advance;
		}
	}
	immediate_flush();
}
