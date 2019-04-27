#include "memory.h"

#include <stdlib.h>
#include <string.h>

Memory Memory::g_memory;


void* Memory::alloc(size_t size) {
	return ::malloc(size);
}

void* Memory::realloc(void* ptr, size_t new_size) {
	return ::realloc(ptr, new_size);
}

void Memory::free(void* block) {
	::free(block);
}

#if BUILD_DEBUG
void* Memory::alloc_debug(size_t size, const char* file, u32 line) {
	const size_t original_size = size;

	size += sizeof(Memory_Header);
	u8* result = (u8*)Memory::alloc(size);

	Memory_Header* header = (Memory_Header*)result;
	memset(header, 0, sizeof(Memory_Header));
	header->data = result += sizeof(Memory_Header);
	header->size = original_size;
	header->file = file;
	header->line = line;

	Memory& memory = g_memory;

	if (!memory.first_node) {
		memory.first_node = header;
	} else {
		Memory_Header* current = memory.first_node;
		while (current->child) {
			current = current->child;
		}

		current->child = header;
		header->parent = current;
	}

	memory.amount_allocated += size;
	memory.num_allocations += 1;

	return header->data;
}

void* Memory::realloc_debug(void* ptr, size_t new_size) {
	u8* original_block = (u8*)ptr - sizeof(Memory_Header);
	Memory_Header* header = (Memory_Header*)original_block;

	// @NOTE(Colby): Order matters here
	const size_t original_size = header->size + sizeof(Memory_Header);
	header->size = new_size;
	new_size = new_size + sizeof(Memory_Header);

	g_memory.amount_allocated += new_size - original_size;

	return (u8*)Memory::realloc(original_block, new_size) + sizeof(Memory_Header);
}

void Memory::free_debug(void* block) {
	u8* original_block = (u8*)block - sizeof(Memory_Header);
	Memory_Header* header = (Memory_Header*)original_block;

	Memory& memory = g_memory;

	memory.amount_allocated -= header->size + sizeof(Memory_Header);
	memory.num_allocations -= 1;

	Memory_Header* child = header->child;
	Memory_Header* parent = header->parent;
	if (child) {
		if (parent) {
			parent->child = child;
		} else {
			memory.first_node = child;
		}
	} else if (parent) {
		parent->child = nullptr;
	} else {
		memory.first_node = nullptr;
	}

	Memory::free(original_block);
}
#endif

const Memory& Memory::get() {
	return g_memory;
}
