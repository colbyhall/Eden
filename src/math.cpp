#include "math.h"

#include <stdlib.h>
#include <string.h>

Vector2 vec2(f32 x, f32 y) {
	Vector2 result;
	result.x = x;
	result.y = y;

	return result;
}

Vector2 vec2s(f32 xy) {
	return vec2(xy, xy);
}

f32 vec2_length_sq(Vector2* vec) {
	return vec->x * vec->x + vec->y * vec->y;
}

f32 vec2_length(Vector2* vec) {
	f32 length_sq = vec2_length_sq(vec);
	if (length_sq > 0.f) {
		return sqrtf(length_sq);
	}

	return 0.f;
}

Vector3 vec3(f32 x, f32 y, f32 z) {
	Vector3 result;
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

Vector3 vec3s(f32 xyz) {
	return vec3(xyz, xyz, xyz);
}

f32 vec3_length_sq(Vector3* vec) {
	return vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
}

f32 vec3_length(Vector3* vec) {
	f32 length_sq = vec3_length_sq(vec);
	if (length_sq > 0.f) {
		return sqrtf(length_sq);
	}

	return 0.f;
}

Vector4 vec4_color(int color) {
	Vector4 result;
	result.x = ((color >> 16) & 0xFF) / 255.f;
	result.y = ((color >> 8) & 0xFF) / 255.f;
	result.z = (color & 0xFF) / 255.f;
	result.w = 1.f;

	return result;
}

Matrix4 m4() {
	Matrix4 result;
	memset(&result, 0, sizeof(result));
	return result;
}
Matrix4 m4_identity() {
	Matrix4 result = m4();
	for (s32 i = 0; i < 4; i++) {
		result.elems[i + i * 4] = 1.f;
	}
	return result;
}

static Matrix4 m4__ortho(float left, float right, float top, float bottom, float far, float near) {
	Matrix4 result = m4_identity();

	result.elems[0 + 0 * 4] = 2.f / (right - left);
	result.elems[1 + 1 * 4] = 2.f / (top - bottom);
	result.elems[2 + 2 * 4] = -2.f / (far - near);

	result.elems[0 + 3 * 4] = -((right + left) / (right - left));
	result.elems[1 + 3 * 4] = -((top + bottom) / (top - bottom));
	result.elems[2 + 3 * 4] = -((far + near) / (far - near));

	return result;
}

Matrix4 m4_ortho(float size, float aspect_ratio, float far, float near) {
	const float right = size * aspect_ratio;
	const float left = -right;

	const float top = size;
	const float bottom = -top;

	return m4__ortho(left, right, top, bottom, far, near);
}

Matrix4 m4_translate(Vector2 pos) {
	Matrix4 result = m4_identity();

	result.elems[0 + 3 * 4] = pos.x;
	result.elems[1 + 3 * 4] = pos.y;
	result.elems[2 + 3 * 4] = 0.f;

	return result;
}

Matrix4 m4_multiply(Matrix4* left, Matrix4* right) {
	Matrix4 result;

	for (s32 y = 0; y < 4; y++) {
		for (s32 x = 0; x < 4; x++) {
			float sum = 0.0f;
			for (s32 e = 0; e < 4; e++) {
				sum += left->elems[x + e * 4] * right->elems[e + y * 4];
			}
			result.elems[x + y * 4] = sum;
		}
	}

	return result;
}

void m4_muleq(Matrix4* left, Matrix4* right) {
	Matrix4 result = m4_multiply(left, right);
	*left = result;
}

#define SMALL_NUMBER		(1.e-8f)

float fclamp(float value, float min, float max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

float finterpto(float current, float target, float delta_time, float speed) {

	if (speed <= 0.f) {
		return target;
	}

	const float distance = target - current;

	if (distance * distance < SMALL_NUMBER) {
		return target;
	}

	const float delta_move = distance * fclamp(delta_time * speed, 0.f, 1.f);

	return current + delta_move;
}
