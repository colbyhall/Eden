#pragma once

#include "types.h"

struct Memory_Header {
	void* data;
	size_t size;

	const char* file;
	u32 line;

	Memory_Header* parent;
	Memory_Header* child;
};

struct Memory {
	size_t amount_allocated = 0;
	size_t num_allocations = 0;
	Memory_Header* first_node = nullptr;

	static void* alloc(size_t size);
	static void* realloc(void* ptr, size_t new_size);
	static void free(void* block);

#if BUILD_DEBUG
	static void* alloc_debug(size_t size, const char* file, u32 line);
	static void* realloc_debug(void* ptr, size_t new_size);
	static void free_debug(void* block);
#endif

	static const Memory& get();

private:
	static Memory g_memory;
};

#pragma warning(disable: 4595)

#define c_new new(__FILE__, __LINE__)

#define c_delete delete

#if BUILD_DEBUG
#define c_realloc(ptr, new_size) Memory::realloc_debug(ptr, new_size)
#else
#define c_realloc(ptr, new_size) Memory::realloc(ptr, new_size)
#endif

inline void* operator new(size_t size, const char* file, u32 line) {
#if BUILD_DEBUG
	return Memory::alloc_debug(size, file, line);
#else
	return Memory::alloc(size);
#endif
}

inline void* operator new[](size_t size, const char* file, u32 line) {
#if BUILD_DEBUG
	return Memory::alloc_debug(size, file, line);
#else
	return Memory::alloc(size);
#endif
}

inline void operator delete(void* block) {
#if BUILD_DEBUG
	Memory::free_debug(block);
#else
	Memory::free(block);
#endif
}

inline void operator delete[](void* block) {
#if BUILD_DEBUG
	Memory::free_debug(block);
#else
	Memory::free(block);
#endif
}

// @NOTE(Colby): These are never used but they prevent a warning
inline void operator delete(void* block, const char* file, u32 line) {
#if BUILD_DEBUG
	Memory::free_debug(block);
#else
	Memory::free(block);
#endif
}

inline void operator delete[](void* block, const char* file, u32 line) {
#if BUILD_DEBUG
	Memory::free_debug(block);
#else
	Memory::free(block);
#endif
}

#pragma warning(default : 4595)
