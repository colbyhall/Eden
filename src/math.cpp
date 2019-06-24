#include "math.h"

#define SMALL_NUMBER		(1.e-8f)

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

Color::Color(int color) {
	r = ((color >> 16) & 0xFF) / 255.f;
	g = ((color >> 8) & 0xFF) / 255.f;
	b = (color & 0xFF) / 255.f;
	a = 1.f;
}

Matrix4 m4() {
	Matrix4 result;
	for (int i = 0; i < 4 * 4; i++) {
		result.elems[i] = 0.f;
	}
	return result;
}
Matrix4 m4_identity() {
	Matrix4 result= m4();
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

Matrix4 m4_translate(Vector2 position) {
	Matrix4 result = m4_identity();

	result.elems[0 + 3 * 4] = position.x;
	result.elems[1 + 3 * 4] = position.y;
	result.elems[2 + 3 * 4] = 0.f;

	return result;
}

Vector2 v2() {
	return { 0.f, 0.f };
}

Vector2 v2(float scalar) {
	return { scalar, scalar };
}

Vector2 v2(float x, float y) {
	return { x, y };
}

Vector2 Vector2::operator+(const Vector2& right) const {
	return v2(x + right.x, y + right.y);
}

void Vector2::operator+=(const Vector2& right) {
	x += right.x;
	y += right.y;
}