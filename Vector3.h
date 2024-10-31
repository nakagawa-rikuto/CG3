#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;

	/// ===加法=== ///
	Vector3 operator+(const Vector3& v);
	Vector3& operator+=(const Vector3& v);
	Vector3 operator+(const float& v);
	Vector3& operator+=(const float& v);

	/// ===減法=== ///
	Vector3 operator-(const Vector3& v);
	Vector3& operator-=(const Vector3& v);
	Vector3 operator-(const float& v);
	Vector3& operator-=(const float& v);

	/// ===積=== ///
	Vector3 operator*(const Vector3& v);
	Vector3& operator*=(const Vector3& v);
	Vector3 operator*(const float& v);
	Vector3& operator*=(const float& v);

	/// ===除法=== ///
	Vector3 operator/(const Vector3& v);
	Vector3& operator/=(const Vector3& v);
	Vector3 operator/(const float& v);
	Vector3& operator/=(const float& v);
};

/// ===内積の計算=== ///
float Dot(const Vector3& v);
float Dot(const Vector3& v1, const Vector3& v2);

/// ===長さの計算=== ///
float Length(const Vector3& v);
float Length(const Vector3& v1, const Vector3& v2);

/// ===正規化=== ///
Vector3 Normalize(const Vector3& v);