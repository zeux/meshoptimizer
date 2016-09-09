#pragma once

#include <cmath>

struct vec3
{
	float x, y, z;

	vec3()
	{
	}

	vec3(float x, float y, float z): x(x), y(y), z(z)
	{
	}

	explicit vec3(const float* ptr): x(ptr[0]), y(ptr[1]), z(ptr[2])
	{
	}
};

inline float dot(const vec3& lhs, const vec3& rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline vec3 cross(const vec3& lhs, const vec3& rhs)
{
	return vec3(
		lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.z * rhs.x - lhs.x * rhs.z,
		lhs.x * rhs.y - lhs.y * rhs.x);
}

inline vec3& operator+=(vec3& lhs, const vec3& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

inline vec3& operator-=(vec3& lhs, const vec3& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

inline vec3& operator*=(vec3& lhs, float rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

inline vec3& operator/=(vec3& lhs, float rhs)
{
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	return lhs;
}

inline vec3 operator+(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

inline vec3 operator-(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

inline vec3 operator*(const vec3& lhs, float rhs)
{
	return vec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

inline vec3 operator/(const vec3& lhs, float rhs)
{
	return vec3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
}

inline float length(const vec3& v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline vec3 normalize(const vec3& v)
{
	float l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	float s = l == 0 ? 0 : 1 / l;
	return vec3(v.x * s, v.y * s, v.z * s);
}