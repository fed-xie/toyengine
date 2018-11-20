#pragma once
#include "Types.h"
#include <limits>

namespace toy
{
	inline fvec2 operator+(const fvec2& v1, const fvec2& v2)
	{
		return fvec2(
			v1.X + v2.X,
			v2.Y + v2.Y);
	}

	inline fvec3 operator+(const fvec3& v1, const fvec3& v2)
	{
		return fvec3(
			v1.X + v2.X,
			v1.Y + v2.Y,
			v1.Z + v2.Z);
	}
	inline fvec3 operator-(const fvec3& v1, const fvec3& v2)
	{
		return fvec3(
			v1.X - v2.X,
			v1.Y - v2.Y,
			v1.Z - v2.Z);
	}
	inline fvec3 operator*(const fvec3& v, const float f)
	{
		return fvec3(
			v.X * f,
			v.Y * f,
			v.Z * f);
	}
	inline fvec3 operator*(const float f, const fvec3& v)
	{
		return fvec3(
			v.X * f,
			v.Y * f,
			v.Z * f);
	}
	inline f32 operator*(const fvec3& v1, const fvec3& v2)
	{
		return 
			v1.X * v2.X + 
			v1.Y * v2.Y + 
			v1.Z * v2.Z;
	}
	inline fvec3& fvec3::operator+=(const fvec3& v)
	{
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		return *this;
	}
	inline fvec3& fvec3::operator*=(const f32 f)
	{
		X *= f;
		Y *= f;
		Z *= f;
		return *this;
	}
	inline fvec3 cross(const fvec3& a, const fvec3& b)
	{
		return fvec3(
			a.Y * b.Z - a.Z * b.Y,
			a.Z * b.X - a.X * b.Z,
			a.X * b.Y - a.Y * b.X);
	}
	inline fvec3 fvec3::cross(const fvec3& v) { return toy::cross(*this, v); }	
	inline fvec3 fquat::rotate(const fvec3& v)
	{
		const fvec3 qv(X, Y, Z);
		const fvec3 uv(cross(qv, v));

		return v + (cross(qv, uv) + uv * W) * 2.0f;
	}
	fquat operator*(const fquat& p, const fquat& q);
	inline void fquat::operator*=(const fquat& q) { *this = *this * q; }
	inline f32 dot(const fquat& p, const fquat& q)
	{
		return p.W * q.W +
			p.X * q.X +
			p.Y * q.Y +
			p.Z * q.Z;
	}
	

	fmat4 operator*(const fmat4& v1, const fmat4& v2);

	void perspective(float fovy, float aspect, float zNear, float zFar, fmat4& out);
	void ortho(float left, float right,
		float bottom, float top,
		float zNear, float zFar,
		fmat4& out);
	void view(const fvec3& pos, const fvec3& dir, const fvec3& up,
		const fvec3& zoom, fmat4& out);

	fmat3 toMat(const fquat& q);
	fmat3 toMatT(const fquat& q);
	/* matrices MUST be aligned */
	void sqtMat(const fvec3& s, const fquat& q, const fvec3& t, fmat4& out);
	/* make a sqt matrix, and transpose it */
	void sqtMatT(const fvec3& s, const fquat& q, const fvec3& t, fmat4& out);
	fquat slerp(const fquat& q1, const fquat&q2, float factor);
	void transpose(fmat4* mats, uint matNum);
}