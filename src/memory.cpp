#include "memory.h"

#include <stdlib.h>
#include <string.h>

void* memory_alloc(size_t size) {
	return malloc(size);
}

void* memory_realloc(void* ptr, size_t new_size) {
	return realloc(ptr, new_size);
}

void memory_free(void* block) {
	free(block);
}

#if BUILD_DEBUG
struct Memory_Header {
	void* data;
	size_t size;

	const char* file;
	u32 line;

	Memory_Header* parent;
	Memory_Header* child;
};

size_t amount_allocated = 0;
size_t num_allocations = 0;
Memory_Header* first_node = nullptr;

void* memory_alloc_debug(size_t size, const char* file, u32 line) {
	const size_t original_size = size;

	size += sizeof(Memory_Header);
	u8* result = (u8*)memory_alloc(size);

	Memory_Header* header = (Memory_Header*)result;
	memset(header, 0, sizeof(Memory_Header));
	header->data = result += sizeof(Memory_Header);
	header->size = original_size;
	header->file = file;
	header->line = line;

	if (!first_node) {
		first_node = header;
	} else {
		Memory_Header* current = first_node;
		while (current->child) {
			current = current->child;
		}

		current->child = header;
		header->parent = current;
	}

	amount_allocated += size;
	num_allocations += 1;

	return header->data;
}

void* memory_realloc_debug(void* ptr, size_t new_size) {
	u8* original_block = (u8*)ptr - sizeof(Memory_Header);
	Memory_Header* header = (Memory_Header*)original_block;

	// @NOTE(Colby): Order matters here
	const size_t original_size = header->size + sizeof(Memory_Header);
	header->size = new_size;
	new_size = new_size + sizeof(Memory_Header);

	amount_allocated += new_size - original_size;

	return (u8*)memory_realloc(original_block, new_size) + sizeof(Memory_Header);
}

void memory_free_debug(void* block) {
	u8* original_block = (u8*)block - sizeof(Memory_Header);
	Memory_Header* header = (Memory_Header*)original_block;

	amount_allocated -= header->size + sizeof(Memory_Header);
	num_allocations -= 1;

	Memory_Header* child = header->child;
	Memory_Header* parent = header->parent;
	if (child) {
		if (parent) {
			parent->child = child;
		} else {
			first_node = child;
		}
	} else if (parent) {
		parent->child = nullptr;
	} else {
		first_node = nullptr;
	}

	memory_free(original_block);
}

size_t memory_num_allocations() {
	return num_allocations;
}

size_t memory_amount_allocated() {
	return amount_allocated;
}

#endif
