#pragma once

#include "types.h"
#include <math.h>

struct Vector2 {
	union {
		struct { f32 x, y; };
		struct { f32 u, v; };
		f32 xy[2];
		f32 uv[2];
	};

	Vector2() : x(0.f), y(0.f) {}
	Vector2(float xy) : x(xy), y(xy) {}
	Vector2(float x, float y) : x(x), y(y) {}

	Vector2 operator+(const Vector2& right) const;
	void operator+=(const Vector2& right);
};

struct Vector4 {
	union {
		struct { f32 x, y, z, w; };
		struct { f32 r, g, b, a; };
	};

	Vector4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
	Vector4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
	Vector4(int color);
};

struct Matrix4 {
	union {
		f32 row_col[4][4];
		f32 elems[4 * 4];
	};

	Matrix4();
	static Matrix4 identity();
	static Matrix4 ortho(float size, float aspect_ratio, float far, float near);
	static Matrix4 translate(Vector2 pos);

};

namespace Math {
	float finterpto(float current, float target, float delta_time, float speed);
}
