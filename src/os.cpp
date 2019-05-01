#include "os.h"

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

String os_load_file_into_memory(const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		return "";
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	u8* file_data = (u8*)c_alloc(size + 1);
	fread(file_data, size, 1, f);
	fclose(f);
	file_data[size] = 0;

	String result;
	result.data = file_data;
	result.allocated = size + 1;
	result.count = size;

	return result;
}
