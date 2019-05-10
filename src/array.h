#pragma once

#include "memory.h"

#include <initializer_list>
#include <stdio.h>
#include <string.h>
#include <assert.h>

template <typename T>
struct Array {
	T* data = nullptr;
	size_t count = 0;
	size_t allocated = 0;

	T& operator[](size_t index) {
		assert(index < count);
		return data[index];
	}

	const T& operator[](size_t index) const {
		assert(index < count);
		return data[index];
	}

	operator bool() const { return data != nullptr && count > 0; }

	bool operator==(const Array<T>& right) const {
		if (count != right.count) return false;

		for (size_t i = 0; i < count; i++) {
			if (data[i] != right[i]) return false;
		}

		return true;
	}

	bool operator!=(const Array<T>& right) const {
		return !(*this == right);
	}

	Array() = default;

	explicit Array(size_t allocate_amount) {
		count = 0;
		array_reserve(this, allocate_amount);
	}

	Array(std::initializer_list<T> list) {
		count = 0;
		array_reserve(this, list.size());

		for (const T& item : list) {
			array_add(this, item);
		}
	}

	T* begin() {
		return data;
	}

	T* end() {
		return data + count;
	}

	const T* cbegin() const {
		return data;
	}

	const T* cend() const {
		return data + count - 1;
	}

};

template <typename T>
size_t array_reserve(Array<T>* a, size_t amount) {
	a->allocated += amount;

	if (a->data) {
		a->data = (T*)c_realloc(a->data, a->allocated * sizeof(T));
	}
	else {
		a->data = (T*)c_alloc(sizeof(T) * a->allocated);
	}

	return a->allocated;
}

template <typename T>
size_t array_add(Array<T>* a, const T& item) {
	return array_add_at_index(a, item, a->count);
}

template <typename T>
size_t array_add_zeroed(Array<T>* a) {
	T to_be_added;
	return array_add(a, to_be_added);
}

template <typename T>
void array_empty(Array<T>* a) {
	a->count = 0;
}

template <typename T>
size_t array_add_at_index(Array<T>* a, const T& item, size_t index) {
	assert(index <= a->count);

	if (a->count == a->allocated) {
		array_reserve(a, 1);
	}

	if (index != a->count) {
		memmove(a->data + index + 1, a->data + index, (a->count - index) * sizeof(T));
	}

	a->data[index] = item;
	a->count += 1;

	return index;
}

template <typename T>
T array_remove(Array<T>* a, size_t index) {
	T result = a->data[index];
	memmove(a->data + index, a->data + index + 1, (a->count - index) * sizeof(T));
	a->count -= 1;
	return result;
}