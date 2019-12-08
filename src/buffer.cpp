#include "buffer.h"

#include "config.h"

#include <ch_stl/hash_table.h>
#include <vadefs.h>
#include <stdio.h>
#include <stdarg.h>

static u32 get_char_column_size(u32 c) {
	if (c == '\t') return get_config().tab_width;
	return 1;
}

Buffer::Buffer(Buffer_ID _id) : id(_id) {
    line_column_table.allocator = ch::get_heap_allocator();
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();

	eol_table.push(0);
	line_column_table.push(0);
}

bool Buffer::load_file_into_buffer(const ch::Path& path) {
	if (gap_buffer) return false;


	ch::File f;
	if (!f.open(path, ch::FO_Read | ch::FO_Binary)) return false;
	defer(f.close());

	const usize f_size = f.size();
	gap_buffer.resize(f_size + ch::default_gap_size);

	f.read(gap_buffer.data, f_size);
	gap_buffer.gap = gap_buffer.data + f_size;
	gap_buffer.gap_size = ch::default_gap_size;

	eol_table.count = 0;
	line_column_table.count = 0;

	u32 num_nix = 0;
	u32 num_clrf = 0;
	u32 last_eol = 0;
	u32 col_count = 0;
	for (ch::UTF8_Iterator<ch::Gap_Buffer<u8>> it(gap_buffer, gap_buffer.count()); it.can_advance(); it.advance()) {
		const u32 c = it.get();

		col_count += get_char_column_size(c);

		if (c == '\r' || c == '\n') {
			if (c == '\r' && it.can_advance()) {
				const u32 peek_c = it.peek();
				if (peek_c == '\n') {
					it.advance();
					col_count += get_char_column_size(peek_c);
				}
				num_clrf += 1;
			}
			else
			{
				num_nix += 1;
			}

			eol_table.push((u32)it.index - last_eol + 1);
			last_eol = (u32)it.index + 1;

			line_column_table.push(col_count);
			col_count = 0;
		}
	}
	eol_table.push((u32)f_size - last_eol);
	line_column_table.push(col_count);

	if (!num_nix && num_clrf) {
		line_ending = LE_CLRF;
	}

	return true;
}

void Buffer::empty() {
    gap_buffer.gap = gap_buffer.data;
    gap_buffer.gap_size = gap_buffer.allocated;
    eol_table.count = 0;
    line_column_table.count = 0;
    syntax_dirty = true;
    lexemes.count = 0;
}

void Buffer::free() {
	gap_buffer.free();
	eol_table.free();
	line_column_table.free();
	lexemes.free();
}

void Buffer::add_char(u32 c, usize index) {
	gap_buffer.insert(c, index);

	refresh_line_tables();
}

void Buffer::remove_char(usize index) {
	const u32 c = gap_buffer[index];

	const usize next = find_next_char(index);
	for (u8 i = 0; i < (u8)(next - index); i += 1) {
		gap_buffer.remove_at_index(index);	
	}

	refresh_line_tables();
}

void Buffer::print_to(const char* fmt, ...) {
	char write_buffer[1024];

	va_list args;
	va_start(args, fmt);
	const usize size = vsprintf(write_buffer, fmt, args);
	va_end(args);

	for (usize i = 0; i < size; i += 1) {
		gap_buffer.push(write_buffer[i]);
	}

	refresh_line_tables();
}

void Buffer::refresh_line_tables() {
	eol_table.count = 0;
	line_column_table.count = 0;

	u32 last_eol = 0;
	u32 col_count = 0;
	for (ch::UTF8_Iterator<ch::Gap_Buffer<u8>> it(gap_buffer, gap_buffer.count()); it.can_advance(); it.advance()) {
		const u32 c = it.get();

		col_count += get_char_column_size(c);

		if (c == '\r' || c == '\n') {
			if (c == '\r') {
				const u32 peek_c = it.peek();
				if (peek_c == '\n') {
					it.advance();
					col_count += get_char_column_size(peek_c);
				}
			}

			eol_table.push((u32)it.index - last_eol + 1);
			last_eol = (u32)it.index + 1;

			line_column_table.push(col_count);
			col_count = 0;
		}
	}
	eol_table.push((u32)gap_buffer.count() - last_eol);
	line_column_table.push(col_count);
}

usize Buffer::find_next_char(usize index) {
	assert(index < gap_buffer.count());

	static const u8 utf8_size_table[] = { 0, 0, 0, 0, 2, 2, 3, 4 };
	const u8 key = (gap_buffer[index] >> 4);

	u8 offset = 1;
	if (key > 7) {
		offset = utf8_size_table[key - 8];
	}
	// @NOTE(CHall): if offset is 0 index is in the middle of a utf8 char
	assert(offset);

	return index + (usize)offset;
}

usize Buffer::find_prev_char(usize index) {
	assert(index <= gap_buffer.count() && index > 0);

	for (u8 i = 1; i < 5 && index - i >= 0; i += 1) {
		const u8 key = (gap_buffer[index - i] >> 4);
		if (key < 8 || key > 11) return index - i;
	}

	return 0;
}

u32 Buffer::get_char(usize index) {
	assert(index < gap_buffer.count());

	u32 codepoint = 0;
	u32 decoder_state = ch::utf8_accept;

	for (; index < gap_buffer.count(); index += 1) {
		const u8 c = gap_buffer[index];
		ch::utf8_decode(&decoder_state, &codepoint, c);

		if (decoder_state == ch::utf8_reject) return '?';

		if (decoder_state != ch::utf8_accept) continue;

		return codepoint;
	}

	return '?';
}

u64 Buffer::get_index_from_line(u64 line) const {
	assert(line < eol_table.count);

	u64 result = 0;
	for (usize i = 0; i < line; i++) {
		result += eol_table[i];
	}
	return result;
}

u64 Buffer::get_line_from_index(u64 index) const {
	assert(index <= gap_buffer.count());

	u64 current_index = 0;
	for (usize i = 0; i < eol_table.count; i++) {
		current_index += eol_table[i];
		if (current_index > index) return i;
	}

	return eol_table.count - 1;
}

u64 Buffer::get_wrapped_line_from_index(u64 index, u64 max_line_width) const {
    assert(max_line_width > 0);
	assert(index <= gap_buffer.count());

    u64 num_lines = 0;

	u64 current_index = 0;
	for (usize i = 0; i < eol_table.count; i++) {
		current_index += eol_table[i];
        num_lines += line_column_table[i] / max_line_width + 1;
		if (current_index > index) break;
	}

	return num_lines;
}

static ch::Hash_Table<Buffer_ID, Buffer> the_buffers;
static Buffer_ID last_buffer_id = 0;

Buffer_ID create_buffer() {
	last_buffer_id += 1;
	
	the_buffers.push(last_buffer_id, Buffer(last_buffer_id));

	return last_buffer_id;
}

Buffer* find_buffer(Buffer_ID id) {
	return the_buffers.find(id);
}

bool remove_buffer(Buffer_ID id) {
	Buffer* const buffer = find_buffer(id);
	if (buffer) {
		buffer->free();
		the_buffers.remove(id);
		return true;
	}
	
	return false;
}