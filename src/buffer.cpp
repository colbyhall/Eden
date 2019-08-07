#include "buffer.h"

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
	}
	else {
		return data[index + gap_size];
	}
}

u32 Buffer::operator[](usize index) const {
	assert(index < count());

	if (data + index < gap) {
		return data[index];
	}
	else {
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
