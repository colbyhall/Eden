#pragma once

#include "memory.h"

#include <initializer_list>
#include <stdio.h>
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
size_t array_reserve(Array<T>* arr, size_t amount) {
	arr->allocated += amount;

	if (arr->data) {
		arr->data = (T*)c_realloc(arr->data, arr->allocated * sizeof(T));
	}
	else {
		arr->data = (T*)c_alloc(sizeof(T) * arr->allocated);
	}

	return arr->allocated;
}

template <typename T>
size_t array_add(Array<T>* arr, const T& item) {
	return array_add_at_index(arr, item, arr->count);
}

template <typename T>
size_t array_add_zeroed(Array<T>* arr) {
	T to_be_added;
	return array_add(arr, to_be_added);
}

template <typename T>
void array_empty(Array<T>* arr) {
	arr->count = 0;
}

template <typename T>
size_t array_add_at_index(Array<T>* arr, const T& item, size_t index) {
	assert(index <= arr->count);

	if (arr->count == arr->allocated) {
		array_reserve(arr, 1);
	}

	if (index != arr->count) {
		memmove(arr->data + index + 1, arr->data + index, (arr->count - index) * sizeof(T));
	}

	arr->data[index] = item;
	arr->count += 1;

	return index;
}

template <typename T>
T array_remove(Array<T>* arr, size_t index) {
	T result = arr->data[index];
	memmove(arr->data + index, arr->data + index + 1, (arr->count - index) * sizeof(T));
	arr->count -= 1;
	return result;
}