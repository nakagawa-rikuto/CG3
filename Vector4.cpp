#include "Vector4.h"

// ベクトル同士の加法
Vector4 Vector4::operator+(const Vector4& v) { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }

// ベクトル同士の加法（+=）
Vector4& Vector4::operator+=(const Vector4& v) {
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}

// ベクトルと float の加法
Vector4 Vector4::operator+(const float& v) { return Vector4(x + v, y + v, z + v, w + v); }

// ベクトルと float の加法（+=）
Vector4& Vector4::operator+=(const float& v) {
	x += v;
	y += v;
	z += v;
	w += v;
	return *this;
}

// ベクトル同士の減法
Vector4 Vector4::operator-(const Vector4& v) { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }

// ベクトル同士の減法（-=）
Vector4& Vector4::operator-=(const Vector4& v) {
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return *this;
}

// ベクトルと float の減法
Vector4 Vector4::operator-(const float& v) { return Vector4(x - v, y - v, z - v, w - v); }

// ベクトルと float の減法（-=）
Vector4& Vector4::operator-=(const float& v) {
	x -= v;
	y -= v;
	z -= v;
	w -= v;
	return *this;
}

// ベクトル同士の積（要素ごとの積）
Vector4 Vector4::operator*(const Vector4& v) { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }

// ベクトル同士の積（*=）
Vector4& Vector4::operator*=(const Vector4& v) {
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
	return *this;
}

// ベクトルと float の積
Vector4 Vector4::operator*(const float& v) { return Vector4(x * v, y * v, z * v, w * v); }

// ベクトルと float の積（*=）
Vector4& Vector4::operator*=(const float& v) {
	x *= v;
	y *= v;
	z *= v;
	w *= v;
	return *this;
}

// ベクトル同士の除法（要素ごとの除法）
Vector4 Vector4::operator/(const Vector4& v) { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }

// ベクトル同士の除法（/=）
Vector4& Vector4::operator/=(const Vector4& v) {
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
	return *this;
}

// ベクトルと float の除法
Vector4 Vector4::operator/(const float& v) { return Vector4(x / v, y / v, z / v, w / v); }

// ベクトルと float の除法（/=）
Vector4& Vector4::operator/=(const float& v) {
	x /= v;
	y /= v;
	z /= v;
	w /= v;
	return *this;
}