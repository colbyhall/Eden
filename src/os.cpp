#include "os.h"

#include <ch_stl/ch_memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <ch_stl/ch_stl.h>

ch::String os_load_file_into_memory(const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
        return {};
	}
	fseek(f, 0, SEEK_END);
	usize size = ftell(f);
	fseek(f, 0, SEEK_SET);

	tchar* file_data = (tchar*)ch::malloc(size + 1);
	usize num_chars_read = fread(file_data, 1, size, f);
	fclose(f);
    assert(num_chars_read <= size);
	file_data[num_chars_read] = 0;

	ch::String result;
	result.data = file_data;
	result.allocated = size + 1;
	result.count = num_chars_read;

	return result;
}
