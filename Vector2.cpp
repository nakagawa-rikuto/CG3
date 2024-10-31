#include "Vector2.h"
#define _USE_MATH_DEFINES
#include <cmath>

// ベクトル同士の加法
Vector2 Vector2::operator+(const Vector2& v) { return Vector2(x + v.x, y + v.y); }

// ベクトル同士の加法（+=）
Vector2& Vector2::operator+=(const Vector2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

// ベクトルと float の加法
Vector2 Vector2::operator+(const float& v) { return Vector2(x + v, y + v); }

// ベクトルと float の加法（+=）
Vector2& Vector2::operator+=(const float& v) {
	x += v;
	y += v;
	return *this;
}

// ベクトル同士の減法
Vector2 Vector2::operator-(const Vector2& v) { return Vector2(x - v.x, y - v.y); }

// ベクトル同士の減法（-=）
Vector2& Vector2::operator-=(const Vector2& v) {
	x -= v.x;
	y -= v.y;
	return *this;
}

// ベクトルと float の減法
Vector2 Vector2::operator-(const float& v) { return Vector2(x - v, y - v); }

// ベクトルと float の減法（-=）
Vector2& Vector2::operator-=(const float& v) {
	x -= v;
	y -= v;
	return *this;
}

// ベクトル同士の積（要素ごとの積）
Vector2 Vector2::operator*(const Vector2& v) { return Vector2(x * v.x, y * v.y); }

// ベクトル同士の積（*=）
Vector2& Vector2::operator*=(const Vector2& v) {
	x *= v.x;
	y *= v.y;
	return *this;
}

// ベクトルと float の積
Vector2 Vector2::operator*(const float& v) { return Vector2(x * v, y * v); }

// ベクトルと float の積（*=）
Vector2& Vector2::operator*=(const float& v) {
	x *= v;
	y *= v;
	return *this;
}

// ベクトル同士の除法（要素ごとの除法）
Vector2 Vector2::operator/(const Vector2& v) { return Vector2(x / v.x, y / v.y); }

// ベクトル同士の除法（/=）
Vector2& Vector2::operator/=(const Vector2& v) {
	x /= v.x;
	y /= v.y;
	return *this;
}

// ベクトルと float の除法
Vector2 Vector2::operator/(const float& v) { return Vector2(x / v, y / v); }

// ベクトルと float の除法（/=）
Vector2& Vector2::operator/=(const float& v) {
	x /= v;
	y /= v;
	return *this;
}

///=====================================================///
///内積の計算
///=====================================================///
float Dot(const Vector2& v) { return v.x * v.x + v.y * v.y; }
float Dot(const Vector2& v1, const Vector2& v2) { return v1.x * v2.x + v1.y * v2.y; }

///=====================================================///
///長さの計算
///=====================================================///
float Length(const Vector2& v) { return std::sqrtf(Dot(v)); }
float Length(const Vector2& v1, const Vector2& v2) { return std::sqrtf(Dot(v1, v2)); }

///=====================================================///
///正規化
///=====================================================///
Vector2 Normalize(const Vector2& v) {
	float mag = Length(v);
	if (mag != 0.0f) {
		return { v.x / mag, v.y / mag };
	}
	// ゼロベクトルの場合はそのまま返す
	return v;
}
