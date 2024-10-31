#include "Vector3.h"
#define _USE_MATH_DEFINES
#include <cmath>

// ベクトル同士の加法
Vector3 Vector3::operator+(const Vector3& v) { return Vector3(x + v.x, y + v.y, z + v.z); }

// ベクトル同士の加法（+=）
Vector3& Vector3::operator+=(const Vector3& v) {
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

// ベクトルと float の加法
Vector3 Vector3::operator+(const float& v) { return Vector3(x + v, y + v, z + v); }

// ベクトルと float の加法（+=）
Vector3& Vector3::operator+=(const float& v) {
	x += v;
	y += v;
	z += v;
	return *this;
}

// ベクトル同士の減法
Vector3 Vector3::operator-(const Vector3& v) { return Vector3(x - v.x, y - v.y, z - v.z); }

// ベクトル同士の減法（-=）
Vector3& Vector3::operator-=(const Vector3& v) {
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

// ベクトルと float の減法
Vector3 Vector3::operator-(const float& v) { return Vector3(x - v, y - v, z - v); }

// ベクトルと float の減法（-=）
Vector3& Vector3::operator-=(const float& v) {
	x -= v;
	y -= v;
	z -= v;
	return *this;
}

// ベクトル同士の積（要素ごとの積）
Vector3 Vector3::operator*(const Vector3& v) { return Vector3(x * v.x, y * v.y, z * v.z); }

// ベクトル同士の積（*=）
Vector3& Vector3::operator*=(const Vector3& v) {
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

// ベクトルと float の積
Vector3 Vector3::operator*(const float& v) { return Vector3(x * v, y * v, z * v); }

// ベクトルと float の積（*=）
Vector3& Vector3::operator*=(const float& v) {
	x *= v;
	y *= v;
	z *= v;
	return *this;
}

// ベクトル同士の除法（要素ごとの除法）
Vector3 Vector3::operator/(const Vector3& v) { return Vector3(x / v.x, y / v.y, z / v.z); }

// ベクトル同士の除法（/=）
Vector3& Vector3::operator/=(const Vector3& v) {
	x /= v.x;
	y /= v.y;
	z /= v.z;
	return *this;
}

// ベクトルと float の除法
Vector3 Vector3::operator/(const float& v) { return Vector3(x / v, y / v, z / v); }

// ベクトルと float の除法（/=）
Vector3& Vector3::operator/=(const float& v) {
	x /= v;
	y /= v;
	z /= v;
	return *this;
}

///=====================================================///
/// 内積の計算
///=====================================================///
float Dot(const Vector3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
float Dot(const Vector3& v1, const Vector3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

///=====================================================///
/// 長さの計算
///=====================================================///
float Length(const Vector3& v) { return std::sqrtf(Dot(v)); }
float Length(const Vector3& v1, const Vector3& v2) { return std::sqrtf(Dot(v1, v2)); }

///=====================================================///
/// 正規化
///=====================================================///
Vector3 Normalize(const Vector3& v) {
	float mag = Length(v);
	if (mag != 0.0f) {
		return { v.x / mag, v.y / mag, v.z / mag };
	}
	// ゼロベクトルの場合はそのまま返す
	return v;
}