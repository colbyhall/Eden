#include "memory.h"

#include <assert.h>
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
    result += sizeof(Memory_Header);
	header->size = original_size;
	header->file = file;
	header->line = line;
    header->child = nullptr;
    header->parent = nullptr;

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

	return result;
}

void* memory_realloc_debug(void* ptr, size_t new_size, const char *file, u32 line) {
    if (!ptr) return memory_alloc_debug(new_size, file, line);
	u8* original_block = (u8*)ptr - sizeof(Memory_Header);
    Memory_Header *old_header = (Memory_Header *)original_block;
	// @NOTE(Colby): Order matters here
	const size_t original_size = old_header->size + sizeof(Memory_Header);
	old_header->size = new_size;
	new_size = new_size + sizeof(Memory_Header);

	amount_allocated += new_size - original_size;
	u8 *new_block = (u8*)memory_realloc(original_block, new_size);
    if (!new_block) return nullptr;
    Memory_Header *new_header = (Memory_Header *)new_block;
    
    Memory_Header *child = new_header->child;
    Memory_Header *parent = new_header->parent;
    if (child) {                   
        assert(child->parent == old_header);
        child->parent = new_header;
    }
    if (parent) {
        assert(parent->child == old_header);
        parent->child = new_header;
    } else {
        assert(old_header == first_node);
        first_node = new_header;
    }
    
    return new_block + sizeof(Memory_Header);
}

void memory_free_debug(void* block) {
    if (!block) return;
	u8* original_block = (u8*)block - sizeof(Memory_Header);
	Memory_Header* header = (Memory_Header*)original_block;

	amount_allocated -= header->size + sizeof(Memory_Header);
	num_allocations -= 1;

	Memory_Header* child = header->child;
	Memory_Header* parent = header->parent;
	if (child) {
        assert(child->parent == header);
		if (parent) {
            assert(first_node != header);
			parent->child = child;
            child->parent = parent;
		} else {
            assert(first_node == header);
			first_node = child;
            child->parent = nullptr;
		}
	} else if (parent) {
		parent->child = nullptr;
	} else {
        assert(first_node == header);
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
