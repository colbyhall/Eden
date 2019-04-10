#pragma once

#include "types.h"
#include <math.h>

typedef struct Vector2 {
	f32 x, y;
} Vector2;

Vector2 vec2(f32 x, f32 y);
Vector2 vec2s(f32 xy);
f32 vec2_length_sq(Vector2* vec);
f32 vec2_length(Vector2* vec);

typedef struct Vector3 {
	f32 x, y, z;
} Vector3;

Vector3 vec3(f32 x, f32 y, f32 z);
Vector3 vec3s(f32 xyz);
f32 vec3_length_sq(Vector3* vec);
f32 vec3_length(Vector3* vec);

typedef struct Vector4 {
	f32 x, y, z, w;
} Vector4;

Vector4 vec4_color(int color);

typedef struct Matrix4 {
	f32 elems[4 * 4];
} Matrix4;

Matrix4 m4();
Matrix4 m4_identity();

Matrix4 m4_ortho(float size, float aspect_ratio, float far, float near);
Matrix4 m4_translate(Vector2 pos);

Matrix4 m4_multiply(Matrix4* left, Matrix4* right);
void m4_muleq(Matrix4* left, Matrix4* right);