#pragma once

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 final {
	float x;
	float y;

	/// ===加法=== ///
	Vector2 operator+(const Vector2& v);
	Vector2& operator+=(const Vector2& v);
	Vector2 operator+(const float& v);
	Vector2& operator+=(const float& v);

	/// ===減法=== ///
	Vector2 operator-(const Vector2& v);
	Vector2& operator-=(const Vector2& v);
	Vector2 operator-(const float& v);
	Vector2& operator-=(const float& v);

	/// ===積=== ///
	Vector2 operator*(const Vector2& v);
	Vector2& operator*=(const Vector2& v);
	Vector2 operator*(const float& v);
	Vector2& operator*=(const float& v);

	/// ===除法=== ///
	Vector2 operator/(const Vector2& v);
	Vector2& operator/=(const Vector2& v);
	Vector2 operator/(const float& v);
	Vector2& operator/=(const float& v);
};

/// ===内積の計算=== ///
float Dot(const Vector2& v);
float Dot(const Vector2& v1, const Vector2& v2);

/// ===長さの計算=== ///
float Length(const Vector2& v);
float Length(const Vector2& v1, const Vector2& v2);

/// ===正規化=== ///
Vector2 Normalize(const Vector2& v);