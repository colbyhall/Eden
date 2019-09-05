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

	ssize index_line = -1;
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

void Buffer::remove_char(usize index) {
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

	const u32 c = gap_buffer[index];
	if (c == ch::eol) {
		const size_t amount_on_line = eol_table[index_line];
		eol_table.remove(index_line);
		eol_table[index_line - 1] += amount_on_line;
	}

	gap_buffer.remove_at_index(index);
}
