#include "buffer.h"

Buffer::Buffer() {
	eol_table.allocator = ch::get_heap_allocator();
}

Buffer::Buffer(Buffer_ID _id) : id(_id) {
	Buffer();
}

Buffer Buffer::copy() const {
	Buffer r = *this;
	r.data = ch_new u32[allocated];
	r.gap = r.data + (gap - data);
	r.eol_table = eol_table.copy();
	return r;
}

void Buffer::free() {
	ch_delete[] data;
	eol_table.free();
}

u32& Buffer::operator[](usize index) {
	assert(index < count());

	if (data + index < gap) {
		return data[index];
	} else {
		return data[index + gap_size];
	}
}

u32 Buffer::operator[](usize index) const {
	assert(index < count());

	if (data + index < gap) {
		return data[index];
	} else {
		return data[index + gap_size];
	}
}

bool Buffer::load_from_path(const ch::Path& path) {
	ch::File f;
	defer(f.close());
	if (!f.open(path, ch::FO_Read | ch::FO_Binary)) return false;
	f.get_full_path(&full_path);
	assert(full_path);

	const usize fs = f.size();

	allocated = fs + DEFAULT_GAP_SIZE;
	data = ch_new u32[allocated];

	{
		usize chars_read = 0;
		usize extra = 0;
		u32* current_char = data;
		usize line_size = 0;
		while (current_char != data + fs) {
			u32 c = 0;
			f.read(&c, 1);

			if (!c) break;

			chars_read += 1;

			if (c == '\r') {
				extra += 1;
				continue;
			}

			*current_char = c;
			current_char += 1;
			line_size += 1;

			if (c == '\n') {
				eol_table.push(line_size);
				line_size = 0;
			}
		}

		gap = data + fs - extra;
		gap_size = DEFAULT_GAP_SIZE + extra;
	}

	return true;
}

bool Buffer::init_from_size(usize size) {
	if (size < DEFAULT_GAP_SIZE) size = DEFAULT_GAP_SIZE;

	allocated = size;
	data = ch_new u32[allocated];
	if (!data) return false;

	gap = data;
	gap_size = size;
	eol_table.push((usize)0);

	return true;
}

void Buffer::resize(usize new_gap_size) {
	// @NOTE(Colby): Check if we have been initialized
	if (!data) {
		init_from_size(new_gap_size);
		return;
	}
	assert(data);
	assert(gap_size == 0);

	const usize old_size = allocated;
	const usize new_size = old_size + new_gap_size;

	u32* old_data = data;
	u32* new_data = (u32 *)ch::realloc(old_data, new_size * sizeof(u32));
	assert(!new_data);

	data = new_data;
	allocated = new_size;
	gap = (data + old_size);
	gap_size = new_gap_size;
}

void Buffer::move_gap_to_index(usize index) {
	assert(index < count());

	u32* index_ptr = &(*this)[index];

	if (index_ptr < gap) {
		const usize amount_to_move = gap - index_ptr;

		ch::mem_copy(gap + gap_size - amount_to_move, index_ptr, amount_to_move * sizeof(u32));

		gap = index_ptr;
	} else {
		const usize amount_to_move = index_ptr - (gap + gap_size);

		memcpy(gap, gap + gap_size, amount_to_move * sizeof(u32));

		gap += amount_to_move;
	}
}

u32* Buffer::get_index_as_cursor(usize index) {
	if (data + index <= gap) {
		return data + index;
	}

	return data + gap_size + index;
}

void Buffer::add_char(u32 c, usize index) {
	if (gap_size <= 0) {
		resize(DEFAULT_GAP_SIZE);
	}

	if (count() > 0) {
		move_gap_to_index(index);
	}

	u32* cursor = get_index_as_cursor(index);
	*cursor = c;
	gap += 1;
	gap_size -= 1;

	usize index_line = 0;
	usize index_column = 0;

	usize current_index = 0;
	for (usize i = 0; i < eol_table.count; i++) {
		usize line_size = eol_table[i];

		if (index < current_index + line_size) {
			index_column = index - current_index;
			break;
		}

		current_index += line_size;
		index_line += 1;
	}

	if (c == ch::eol) {
		const usize line_size = eol_table[index_line];
		eol_table[index_line] = index_column + 1;
		eol_table.insert(line_size - index_column, index_line + 1);
	} else if (index_line < eol_table.count) { // @Hack
		eol_table[index_line] += 1;
	}
}

void Buffer::remove_at_index(usize index) {
	const usize buffer_count = count();
	assert(index < buffer_count);

	u32* cursor = get_index_as_cursor(index);
	if (cursor == data) {
		return;
	}

	move_gap_to_index(index);

	usize index_line = 0;
	usize current_index = 0;
	for (usize i = 0; i < eol_table.count; i++) {
		usize line_size = eol_table[i];

		if (index < current_index + line_size) {
			break;
		}

		current_index += line_size;
		index_line += 1;
	}

	eol_table[index_line] -= 1;

	const u32 c = (*this)[index - 1];
	if (c == ch::eol) {
		const usize amount_on_line = eol_table[index_line];
		eol_table.remove(index_line);
		eol_table[index_line - 1] += amount_on_line;
	}

	gap -= 1;
	gap_size += 1;
}

void Buffer::remove_between(usize index, usize count) {
	// @SPEED(CHall): this is slow
	for (usize i = index; i < index + count; i++) {
		remove_at_index(i);
	}
}

usize Buffer::get_line_index(usize index) const {
	assert(index < eol_table.count);

	usize result = 0;
	for (usize i = 0; i < index; i++) {
		result += eol_table[i];
	}

	return result;
}

usize Buffer::get_line_from_index(usize index) const {
	assert(index < count());

	usize line = 0;

	usize i = 0;
	for (; line < eol_table.count; line++) {
		const usize new_index = i + eol_table[line];
		if (new_index > index) {
			break;
		}
		i = new_index;
	}

	return line;
}

void draw_buffer(const Buffer& buffer, const Font& font, f32 x, f32 y, const ch::Color& color, f32 z_index) {
	font.bind();
	immediate_begin();
	const f32 font_height = font.size;

	const f32 original_x = x;
	const f32 original_y = y;

	for (usize i = 0; i < buffer.count(); i++) {
		if (buffer[i] == ch::eol) {
			y += font_height;
			x = original_x;
			continue;
		}

		if (buffer[i] == '\t') {
			const Font_Glyph* space_glyph = font[' '];
			assert(space_glyph);
			x += space_glyph->advance  * 4.f;
			continue;
		}

		const Font_Glyph* glyph = font[buffer[i]];

		if (glyph) {
			immediate_glyph(*glyph, font, x, y, color, z_index);

			x += glyph->advance;
		}
	}
	immediate_flush();
}
