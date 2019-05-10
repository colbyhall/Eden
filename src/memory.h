#pragma once

#include "types.h"

void* memory_alloc(size_t size);
void* memory_realloc(void* ptr, size_t new_size);
void memory_free(void* block);

#if BUILD_DEBUG
void* memory_alloc_debug(size_t size, const char* file, u32 line);
void* memory_realloc_debug(void* ptr, size_t new_size, const char *file, u32 line);
void memory_free_debug(void* block);

size_t memory_num_allocations();
size_t memory_amount_allocated();
#endif

#pragma warning(disable: 4595)

#if BUILD_DEBUG
#define c_alloc(size) memory_alloc_debug(size, __FILE__, __LINE__)
#define c_free(block) memory_free_debug(block)
#define c_realloc(ptr, new_size) memory_realloc_debug(ptr, new_size, __FILE__, __LINE__)
#else
#define c_alloc(size) memory_alloc(size);
#define c_free(block) memory_free(block)
#define c_realloc(ptr, new_size) memory_realloc(ptr, new_size)
#endif

#pragma warning(default : 4595)
