#include "buffer.h"

Buffer::Buffer() {
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();

	eol_table.push(0);
}

Buffer::Buffer(Buffer_ID _id) : id(_id) {
	eol_table.allocator = ch::get_heap_allocator();
	gap_buffer.allocator = ch::get_heap_allocator();

	eol_table.push(0);
}

void Buffer::add_char(u32 c, usize index) {
	gap_buffer.insert(c, index);

	// @Speed(Chall): this is because we're lazy atm
	refresh_eol_table();
}

void Buffer::remove_char(usize index) {
	gap_buffer.remove_at_index(index);

	// @Speed(Chall): this is because we're lazy atm
	refresh_eol_table();
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

u64 Buffer::get_index_from_line(u64 line) {
	assert(line < eol_table.count);

	u64 result = 0;
	for (usize i = 0; i < line; i++) {
		result += eol_table[i];
	}
	return result;
}

u64 Buffer::get_line_from_index(u64 index) {
	assert(index <= gap_buffer.count());

	u64 current_index = 0;
	for (usize i = 0; i < eol_table.count; i++) {
		current_index += eol_table[i];
		if (current_index > index) return i;
	}

	return eol_table.count - 1;
}
