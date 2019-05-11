#pragma once

#include "types.h"
#include <assert.h>


/**
 * Theory behind this allocator
 *
 * Can be passed by value
 * All allocation data is stored in header in the actual data member
 * Created by passing in some data and a function no need for derived classes
 */

struct Allocator;

using Alloc_Func = void* (*)(Allocator allocator, void* ptr, size_t size);

struct Allocator {
	u8* data;
	Alloc_Func func;
};

void* allocator_alloc(Allocator allocator, size_t size);
void* allocator_realloc(Allocator allocator, void* ptr, size_t new_size);
void allocator_free(Allocator allocator, void* ptr);

template <typename T>
T* get_allocator_header(Allocator allocator) {
	assert(allocator.data);
	return (T*)allocator.data;
}

Allocator make_heap_allocator();
void* heap_alloc(Allocator allocator, void* ptr, size_t size);

struct Arena_Allocator_Header {
	size_t allocated;
	size_t current;
	void* data;
};

Allocator make_arena_allocator(size_t size);
void destroy_arena_allocator(Allocator* allocator);
void* arena_alloc(Allocator allocator, void* ptr, size_t size);
