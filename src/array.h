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
		reserve(allocate_amount);
	}

	Array(std::initializer_list<T> list) {
		count = 0;
		reserve(list.size());

		for (const T& item : list) {
			add(item);
		}
	}

	~Array() {
		if (data) {
			c_delete[] data;
		}
	}

	size_t reserve(size_t amount) {
		allocated += amount;

		if (data) {
			data = (T*)c_realloc(data, allocated * sizeof(T));
		}
		else {
			data = c_new T[allocated];
		}

		return allocated;
	}

	size_t add(const T& item) {
		return add_at_index(item, count);
	}

	size_t add_zeroed() {
		T to_be_added;
		return add(to_be_added);
	}

	size_t add_at_index(const T& item, size_t index) {
		assert(index >= count);

		if (count == allocated) {
			reserve(1);
		}

		if (index != count) {
			memmove(data + index, data + index + 1, count - (index + 1));
		}

		data[index] = item;
		count += 1;

		return index;
	}

	T& remove(size_t index) {
		T& result = data[index];
		memmove(data + index, data + index - 1, count - (index + 1));
		count -= 1;
		return result;
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