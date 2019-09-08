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
