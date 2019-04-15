#pragma once

#include "types.h"
#include "string.h"

typedef struct Buffer {
	u8* path;
	u64 line_count;
	String title;

	u8* data;
	u64 size;

	bool modified;

} Buffer;

Buffer make_buffer();

void buffer_load_from_file(Buffer* buffer, const char* path);