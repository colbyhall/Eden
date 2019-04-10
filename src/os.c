#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

String os_load_file_into_memory(const char* path) {
	FILE* f;
	fopen_s(&f, path, "r");
	assert(f != NULL);

	fseek(f, 0, SEEK_END);
	size_t fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	String fd;
	fd.data = (u8*)malloc(fsize + 1);
	fd.allocated = fsize + 1;
	fd.length = fsize;
	fread(fd.data, 1, fsize, f);
	fclose(f);

	fd.data[fsize] = 0;

	return fd;
}