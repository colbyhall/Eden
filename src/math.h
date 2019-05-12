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

	Vector2 operator+(const Vector2& right) const;
	void operator+=(const Vector2& right);
};

Vector2 v2();
Vector2 v2(float scalar);
Vector2 v2(float x, float y);

struct Color {
	f32 r, g, b, a;

	Color(int color);
	Color() : r(0.f), g(0.f), b(0.f), a(0.f) {}
};

union Matrix4 {
	f32 row_col[4][4];
	f32 elems[4 * 4];
};

Matrix4 m4();
Matrix4 m4_identity();
Matrix4 m4_ortho(float size, float aspect_ratio, float far, float near);
Matrix4 m4_translate(Vector2 position);

float finterpto(float current, float target, float delta_time, float speed);

inline float fclamp(float value, float min, float max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

inline bool point_in_rect(Vector2 point, float x0, float y0, float x1, float y1) {
	return point.x >= x0 && point.x <= x1 && point.y >= y0 && point.y <= y1;
}
