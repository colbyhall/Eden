#pragma once

#include <stdint.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float	f32;
typedef double	f64;

template <typename F>
struct priv_defer {
	F f;
	priv_defer(F f) : f(f) {}
	~priv_defer() { f(); }
};

template <typename F>
priv_defer<F> defer_func(F f) {
	return priv_defer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})
