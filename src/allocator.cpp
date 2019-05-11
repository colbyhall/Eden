#include "allocator.h"
#include "memory.h"

#include <stdlib.h>

void* allocator_alloc(Allocator allocator, size_t size) {
	assert(allocator.func);
	return allocator.func(allocator, nullptr, size);
}

void* allocator_realloc(Allocator allocator, void* ptr, size_t new_size) {
	assert(allocator.func);
	return allocator.func(allocator, ptr, new_size);
}

void allocator_free(Allocator allocator, void* ptr) {
	assert(allocator.func);
	allocator.func(allocator, ptr, 0);
}

Allocator make_heap_allocator() {
	Allocator result;
	result.data = nullptr;
	result.func = heap_alloc;
	return result;
}

void* heap_alloc(Allocator allocator, void* ptr, size_t size) {
	void* result = nullptr;
	if (size) {
        if (!ptr) {
    		result = (u8*)c_alloc(size);
    	} else {
            result = (u8*)c_realloc(ptr, size);
        }
	} else if (ptr) {
		result = nullptr;
		c_free(ptr);
	}

	return result;
}

Allocator make_arena_allocator(size_t size) {
	u8* data = (u8*)c_alloc(size + sizeof(Arena_Allocator_Header));
	Arena_Allocator_Header* header = (Arena_Allocator_Header*)data;
	*header = { 0 };
	header->data = data + sizeof(Arena_Allocator_Header);
	header->allocated = size;

	return { data, arena_alloc };
}

void destroy_arena_allocator(Allocator* allocator) {
	Arena_Allocator_Header* header = (Arena_Allocator_Header*)allocator->data;
	assert(header->current == 0);
	c_free(allocator->data);
	allocator->data = nullptr;
}

void* arena_alloc(Allocator allocator, void* ptr, size_t size) {
	Arena_Allocator_Header* header = (Arena_Allocator_Header*)allocator.data;
	// @NOTE(Colby): With our arena if we realloc we essentially just want a new alloc
	if (!ptr || (ptr && size > 0)) {
		// @NOTE(Colby): this will assert if we're trying to free with a nullptr
		assert(size > 0);
		assert(header->current + size <= header->allocated);
		void* result = (u8*)header->data + header->current;
		header->current += size;
		return result;
	}
	else {
		header->current = 0;
		return nullptr;
	}
}


