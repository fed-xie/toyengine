#include "include/Mathlib.h"

#ifdef _DEBUG
#define GLM_FORCE_MESSAGES
#endif
#define GLM_FORCE_CXX11
#define GLM_FORCE_INLINE
#define GLM_FORCE_NO_CTOR_INIT
//#define GLM_FORCE_SWIZZLE
//#define GLM_FORCE_PURE
#define GLM_FORCE_AVX
#define GLM_FORCE_ALIGNED

#include "include/glm/glm.hpp"
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"
#include "include/glm/gtx/quaternion.hpp"

namespace toy
{
	const static glm::mat4 imat(1.0f);

	fquat operator*(const fquat& p, const fquat& q)
	{
		fquat r;
		r.W = p.W * q.W - p.X * q.X - p.Y * q.Y - p.Z * q.Z;
		r.X = p.W * q.X + p.X * q.W + p.Y * q.Z - p.Z * q.Y;
		r.Y = p.W * q.Y + p.Y * q.W + p.Z * q.X - p.X * q.Z;
		r.Z = p.W * q.Z + p.Z * q.W + p.X * q.Y - p.Y * q.X;
		return r;
	}
	fquat::fquat(f32 radian, const fvec3& axis)
	{
		radian *= 0.5f;
		auto s = std::sin(radian);
		W = std::cos(radian);
		X = axis.X * s;
		Y = axis.Y * s;
		Z = axis.Z * s;
	}
	void fquat::normalize()
	{
		auto gq = glm::normalize(glm::quat(W, X, Y, Z));
		W = gq.w;
		X = gq.x;
		Y = gq.y;
		Z = gq.z;
	}

	fmat4 operator*(const fmat4& v1, const fmat4& v2)
	{
		glm::mat4 *m1 = (glm::mat4*)&v1;
		glm::mat4 *m2 = (glm::mat4*)&v2;
		auto m = (*m2) * (*m1);//change order for transpose

		static_assert(sizeof(glm::mat4) == sizeof(fmat4), "Type error");
		return *(fmat4*)&m;
	}

	void perspective(float fovy, float aspect, float zNear, float zFar, fmat4& out)
	{
		auto mat = glm::perspective(fovy, aspect, zNear, zFar);
		mat = glm::transpose(mat);
		memcpy(&out, glm::value_ptr(mat), sizeof(fmat4));
	}

	void ortho(float left, float right,
		float bottom, float top,
		float zNear, float zFar,
		fmat4& out)
	{
		auto mat = glm::ortho(left, right, bottom, top, zNear, zFar);
		mat = glm::transpose(mat);
		memcpy(&out, glm::value_ptr(mat), sizeof(fmat4));
	}

	void view(const fvec3& pos, const fvec3& dir, const fvec3& up,
		const fvec3& zoom, fmat4& out)
	{
		auto mat = glm::scale(
			glm::lookAt(
				glm::vec3(pos.X, pos.Y, pos.Z),
				glm::vec3(dir.X, dir.Y, dir.Z),
				glm::vec3(up.X, up.Y, up.Z)),
			glm::vec3(zoom.X, zoom.Y, zoom.Z));

		mat = glm::transpose(mat);
		memcpy(&out, glm::value_ptr(mat), sizeof(fmat4));
	}
	
	fmat3 toMat(const fquat& q)
	{
		auto xx = q.X * q.X;
		auto yy = q.Y * q.Y;
		auto zz = q.Z * q.Z;
		auto xy = q.X * q.Y;
		auto xz = q.X * q.Z;
		auto yz = q.Y * q.Z;
		auto xw = q.X * q.W;
		auto yw = q.Y * q.W;
		auto zw = q.Z * q.W;

		fmat3 r;

		r.a[0] = 1.0f - 2.0f * (yy + zz);
		r.a[1] = 2.0f * (xy - zw);
		r.a[2] = 2.0f * (xz + yw);

		r.a[3] = 2.0f * (xy + zw);
		r.a[4] = 1.0f - 2.0f * (xx + zz);
		r.a[5] = 2.0f * (yz - xw);

		r.a[6] = 2.0f * (xz - yw);
		r.a[7] = 2.0f * (yz + xw);
		r.a[8] = 1.0f - 2.0f * (xx + yy);
		return r;
	}

	fmat3 toMatT(const fquat& q)
	{
		auto xx = q.X * q.X;
		auto yy = q.Y * q.Y;
		auto zz = q.Z * q.Z;
		auto xy = q.X * q.Y;
		auto xz = q.X * q.Z;
		auto yz = q.Y * q.Z;
		auto xw = q.X * q.W;
		auto yw = q.Y * q.W;
		auto zw = q.Z * q.W;

		fmat3 r;

		r.a[0] = 1.0f - 2.0f * (yy + zz);
		r.a[1] = 2.0f * (xy + zw);
		r.a[2] = 2.0f * (xz - yw);

		r.a[3] = 2.0f * (xy - zw);
		r.a[4] = 1.0f - 2.0f * (xx + zz);
		r.a[5] = 2.0f * (yz + xw);

		r.a[6] = 2.0f * (xz + yw);
		r.a[7] = 2.0f * (yz - xw);
		r.a[8] = 1.0f - 2.0f * (xx + yy);
		return r;
	}

	void sqtMat(const fvec3& s, const fquat& q, const fvec3& t, fmat4& out)
	{
		out = fmat4(toMat(q));
		out.a[0] *= s.X; out.a[1] *= s.Y; out.a[2] *= s.Z; out.a[3] = t.X;
		out.a[4] *= s.X; out.a[5] *= s.Y; out.a[6] *= s.Z; out.a[7] = t.Y;
		out.a[8] *= s.X; out.a[9] *= s.Y; out.a[10] *= s.Z; out.a[11] = t.Z;
	}

	void sqtMatT(const fvec3 & s, const fquat & q, const fvec3 & t, fmat4 & out)
	{
		out = fmat4(toMatT(q));
		out.a[0] *= s.X; out.a[1] *= s.X; out.a[2] *= s.X;
		out.a[4] *= s.Y; out.a[5] *= s.Y; out.a[6] *= s.Y;
		out.a[8] *= s.Z; out.a[9] *= s.Z; out.a[10] *= s.Z;
		out.a[12] = t.X; out.a[13] = t.Y; out.a[14] = t.Z;
	}

	fquat slerp(const fquat& q1, const fquat& q2, float factor)
	{
		glm::quat gq1(q1.W, q1.X, q1.Y, q1.Z);
		glm::quat gq2(q2.W, q2.X, q2.Y, q2.Z);
		auto gq = glm::slerp(gq1, gq2, factor);
		return fquat(gq.w, gq.x, gq.y, gq.z);
	}

	void transpose(fmat4* mats, uint matNum)
	{
		glm::mat4* gm = reinterpret_cast<glm::mat4*>(mats);
		for (uint i = 0; i != matNum; ++i)
			gm[i] = glm::transpose(gm[i]);
	}

}