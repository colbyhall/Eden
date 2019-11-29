#include "buffer.h"

#include "config.h"

#include <ch_stl/hash_table.h>
#include <vadefs.h>
#include <stdio.h>
#include <stdarg.h>

static usize get_char_column_size(u32 c) {
	if (c == '\t') return get_config().tab_width;
	return 1;
}

Buffer::Buffer() : id{0} {
    line_column_table.allocator = ch::get_heap_allocator();
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();

	eol_table.push(0);
}

Buffer::Buffer(Buffer_ID _id) : id(_id) {
    line_column_table.allocator = ch::get_heap_allocator();
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();

	eol_table.push(0);
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

	const usize current_line = get_line_from_index(index);

	refresh_eol_table();
	refresh_line_column_table();
}

void Buffer::remove_char(usize index) {
	const u32 c = gap_buffer[index];

	gap_buffer.remove_at_index(index);

	const usize current_line = get_line_from_index(index);
	refresh_eol_table();
	refresh_line_column_table();
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

	refresh_eol_table();
	refresh_line_column_table();
}

void Buffer::refresh_eol_table() {
	eol_table.count = 0;
	eol_table.push(0);

	usize char_count = 0;
	for (usize i = 0; i < gap_buffer.count(); i++) {
		const u32 c = gap_buffer[i];
		char_count += 1;
		if (c == ch::eol) {
			eol_table[eol_table.count - 1] = char_count;

			eol_table.push(0);
			char_count = 0;
		}
	}

	eol_table[eol_table.count - 1] = char_count;
}

void Buffer::refresh_line_column_table() {
	line_column_table.count = 0;
	line_column_table.push(0);

	const Config& config = get_config();

	usize column_count = 0;
	for (usize i = 0; i < gap_buffer.count(); i++) {
		const u32 c = gap_buffer[i];

		column_count += get_char_column_size(c);

		if (c == ch::eol) {
			line_column_table[line_column_table.count - 1] = column_count;

			line_column_table.push(0);
			column_count = 0;
		}
	}

	line_column_table[line_column_table.count - 1] = column_count;
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