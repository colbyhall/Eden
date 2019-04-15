#include "buffer.h"
#include "parsing.h"
#include "os.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Buffer make_buffer() {
	Buffer result;
	memset(&result, 0, sizeof(result));
	return result;
}

void buffer_load_from_file(Buffer* buffer, const char* path) {
	buffer->path = (u8*)path;
	String file_data = os_load_file_into_memory(path);

	buffer->data = file_data.data;
	buffer->size = file_data.length;
	u64 line_count = 1;
	for (size_t i = 0; buffer->data[i] != 0; i++) {
		if (is_eol(buffer->data[i])) {
			line_count += 1;
		}
	}
	buffer->line_count = line_count;
}
