#include "os.h"

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

String OS::load_file_into_memory(const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		return "";
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	u8* file_data = c_new u8[size];
	fread(file_data, size, 1, f);

	fclose(f);

	return String((const char*)file_data);
}
