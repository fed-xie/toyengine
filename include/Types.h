#pragma once
#include "EngineConfig.h"

#include <cstdint>

namespace toy
{
	using uint = unsigned int;
	using u8  = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;

	using c8 = char;
	using s8 = signed char;
	using s16 = std::int16_t;
	using s32 = std::int32_t;
	using s64 = std::int64_t;
	
	using f32 = float;
	using f64 = double;

	using ErrNo = s64;
	#define TOY_OK 0

#define bit(x) (1 << x)

	class f16 final
	{
	public:
		f16();
		f16(float v);
		f16& operator= (float v);
		f32 tof32();
	private:
		u16 v;
	};

	union uvec2
	{
		u32 A[2];
		struct { u32 X, Y; };

		uvec2(u32 x, u32 y) : X(x), Y(y) {}
		uvec2(u32 a[2]) : X(a[0]), Y(a[1]) {}
		uvec2(u32 v) : X(v), Y(v) {}
		uvec2() {}
	};
	union uvec3
	{
		u32 A[3];
		struct { u32 X, Y, Z; };

		uvec3(u32 x, u32 y, u32 z) : X(x), Y(y), Z(z) {}
		uvec3(u32 a[3]) : X(a[0]), Y(a[1]), Z(a[2]) {}
		uvec3(u32 v) : X(v), Y(v), Z(v) {}
		uvec3() {}
	};
	union uvec4
	{
		u32 A[4];
		struct { u32 X, Y, Z, W; };

		uvec4(u32 x, u32 y, u32 z, u32 w) : X(x), Y(y), Z(z), W(w) {}
		uvec4(u32 a[4]) : X(a[0]), Y(a[1]), Z(a[2]), W(a[3]) {}
		uvec4(u32 v) : X(v), Y(v), Z(v), W(v) {}
		uvec4() {}
	};

	union fvec2
	{
		f32 A[2];
		struct { f32 X, Y; };

		fvec2(f32 x, f32 y) : X(x), Y(y) {}
		fvec2(f32 a[2]) : X(a[0]), Y(a[1]) {}
		fvec2(f32 v) : X(v), Y(v) {}
		fvec2() {}
	};
	union fvec3
	{
		f32 A[3];
		struct { f32 X, Y, Z; };

		fvec3(f32 x, f32 y, f32 z) : X(x), Y(y), Z(z) {}
		fvec3(f32 a[3]) : X(a[0]), Y(a[1]), Z(a[2]) {}
		fvec3(f32 v) : X(v), Y(v), Z(v) {}
		fvec3() {}
		
		fvec3& operator+=(const fvec3& v);
		fvec3& operator*=(const f32 v);
		fvec3 cross(const fvec3& v);
	};
	union fvec4
	{
		f32 A[4];
		struct { f32 X, Y, Z, W; };

		fvec4(f32 x, f32 y, f32 z, f32 w) : X(x), Y(y), Z(z), W(w) {}
		fvec4(f32 a[4]) : X(a[0]), Y(a[1]), Z(a[2]), W(a[3]) {}
		fvec4(f32 v) : X(v), Y(v), Z(v), W(v) {}
		fvec4() {}
		
		fvec4& operator+=(const fvec4& v);
		fvec4& operator*=(const f32 f);
	};
	union fquat
	{
		f32 A[4];
		struct { f32 W, X, Y, Z; };

		fquat(f32 w, f32 x, f32 y, f32 z) : W(w), X(x), Y(y), Z(z) {}
		fquat(f32 a[4]) : W(a[0]), X(a[1]), Y(a[2]), Z(a[3]) {}
		fquat(f32 radian, const fvec3& axis);
		fquat() {}

		fquat conjugate() { return fquat(W, -X, -Y, -Z); }
		void operator*=(const fquat& q);
		fvec3 rotate(const fvec3& v);
		void normalize();
	};
	/* Row-majored matrix */
	union fmat2
	{
		fvec2 row[2];
		f32 a[4];
	};
	/* Row-majored matrix */
	union fmat3
	{
		fvec3 row[3];
		f32 a[9];

		fmat3() {}
	};
	/* Row-majored matrix */
	union alignas(16) fmat4
	{
		fvec4 row[4];
		f32 a[16];

		fmat4() {}
		fmat4(f32 v) :
			row{
			{v, 0, 0, 0},
			{0, v, 0, 0},
			{0, 0, v, 0},
			{0, 0, 0, v} }
		{}
		fmat4(const fmat3& m) :
			a{
			m.a[0], m.a[1], m.a[2], 0.0f,
			m.a[3], m.a[4], m.a[5], 0.0f,
			m.a[6], m.a[7], m.a[8], 0.0f,
			0.0f,   0.0f,   0.0f,   1.0f }
		{}

		fmat4& operator*=(const fmat4& v);
	};
	/* Row-majored matrix */
	union fmat4x3
	{
		fvec3 row[4];
		f32 a[12];
	};

#define ARRAYLENGTH(arr) (sizeof(arr)/sizeof(decltype(*arr)))
#define POINTOFFSET(base, ptr) ((size_t)(ptr) - (size_t)(base))

#ifdef TOY_COMPILER_VC
#elif TOY_COMPILER_GCC
#else
#endif
}