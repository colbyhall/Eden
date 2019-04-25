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
};

Vector2 vec2(f32 x, f32 y);
Vector2 vec2s(f32 xy);
f32 vec2_length_sq(Vector2* vec);
f32 vec2_length(Vector2* vec);

struct Vector3 {
	f32 x, y, z;
};

Vector3 vec3(f32 x, f32 y, f32 z);
Vector3 vec3s(f32 xyz);
f32 vec3_length_sq(Vector3* vec);
f32 vec3_length(Vector3* vec);

struct Vector4 {
	f32 x, y, z, w;
};

Vector4 vec4_color(int color);

struct Matrix4 {
	f32 elems[4 * 4];
};

Matrix4 m4();
Matrix4 m4_identity();

Matrix4 m4_ortho(float size, float aspect_ratio, float far, float near);
Matrix4 m4_translate(Vector2 pos);

Matrix4 m4_multiply(Matrix4* left, Matrix4* right);
void m4_muleq(Matrix4* left, Matrix4* right);

float finterpto(float current, float target, float delta_time, float speed);