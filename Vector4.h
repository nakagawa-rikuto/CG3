#pragma once

/// <summary>
/// 4次元ベクトル
/// </summary>
struct Vector4 final {
	float x;
	float y;
	float z;
	float w;

	/// ===加法=== ///
	Vector4 operator+(const Vector4& v);
	Vector4& operator+=(const Vector4& v);
	Vector4 operator+(const float& v);
	Vector4& operator+=(const float& v);

	/// ===減法=== ///
	Vector4 operator-(const Vector4& v);
	Vector4& operator-=(const Vector4& v);
	Vector4 operator-(const float& v);
	Vector4& operator-=(const float& v);

	/// ===積=== ///
	Vector4 operator*(const Vector4& v);
	Vector4& operator*=(const Vector4& v);
	Vector4 operator*(const float& v);
	Vector4& operator*=(const float& v);

	/// ===除法=== ///
	Vector4 operator/(const Vector4& v);
	Vector4& operator/=(const Vector4& v);
	Vector4 operator/(const float& v);
	Vector4& operator/=(const float& v);
};