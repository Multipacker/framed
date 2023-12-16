#ifndef BASE_MATH_H
#define BASE_MATH_H

typedef union Vec2F32 Vec2F32;
union Vec2F32
{
	struct
	{
		F32 x, y;
	};
	struct
	{
		F32 width, height;
	};
	F32 v[2];
};

typedef union Vec3F32 Vec3F32;
union Vec3F32
{
	struct
	{
		F32 x, y, z;
	};

	struct
	{
		F32 r, g, b;
	};
	F32 v[3];
};

typedef union Vec4F32 Vec4F32;
union Vec4F32
{
	struct
	{
		F32 x, y, z, w;
	};

	struct
	{
		F32 r, g, b, a;
	};
	F32 v[4];
};

typedef union Vec2S32 Vec2S32;
union Vec2S32
{
	struct
	{
		S32 x, y;
	};

	struct
	{
		S32 width, height;
	};
	S32 v[2];
};

typedef union Vec3S32 Vec3S32;
union Vec3S32
{
	struct
	{
		S32 x, y, z;
	};

	struct
	{
		S32 r, g, b;
	};
	S32 v[3];
};

typedef union Vec4S32 Vec4S32;
union Vec4S32
{
	struct
	{
		S32 x, y, z, w;
	};

	struct
	{
		S32 r, g, b, a;
	};

	S32 v[4];
};

typedef union Vec2U32 Vec2U32;
union Vec2U32
{
	struct
	{
		U32 x, y;
	};

	struct
	{
		U32 width, height;
	};

	U32 v[2];
};

typedef union Vec3U32 Vec3U32;
union Vec3U32
{
	struct
	{
		U32 x, y, z;
	};

	struct
	{
		U32 r, g, b;
	};

	U32 v[3];
};

typedef union Vec4U32 Vec4U32;
union Vec4U32
{
	struct
	{
		U32 x, y, z, w;
	};

	struct
	{
		U32 r, g, b, a;
	};

	U32 v[4];
};

typedef struct Mat4F32 Mat4F32;
struct Mat4F32
{
    F32 m[4][4];
};

typedef union RectF32 RectF32;
union RectF32
{
	struct
	{
		Vec2F32 min, max;
	};

	struct
	{
		F32 x0, y0;
		F32 x1, y1;
	};

	Vec2F32 v[2];
};

typedef union RectS32 RectS32;
union RectS32
{
	struct
	{
		Vec2S32 min, max;
	};

	struct
	{
		S32 x0, y0;
		S32 x1, y1;
	};

	Vec2S32 v[2];
};

typedef union RectU32 RectU32;
union RectU32
{
	struct
	{
		Vec2U32 min, max;
	};

	struct
	{
		U32 x0, y0;
		U32 x1, y1;
	};

	Vec2U32 v[2];
};

// F32

internal Vec2F32 v2f32(F32 x, F32 y);
internal Vec2F32 v2f32_add_v2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 v2f32_sub_v2f32(Vec2F32 a, Vec2F32 b);
internal Vec2F32 v2f32_mul_f32(Vec2F32 a, F32 t);
internal Vec2F32 v2f32_div_f32(Vec2F32 a, F32 t);

internal Vec3F32 v3f32(F32 x, F32 y, F32 z);
internal Vec3F32 v3f32_add_v3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 v3f32_sub_v3f32(Vec3F32 a, Vec3F32 b);
internal Vec3F32 v3f32_mul_f32(Vec3F32 a, F32 t);
internal Vec3F32 v3f32_div_f32(Vec3F32 a, F32 t);

internal Vec4F32 v4f32(F32 x, F32 y, F32 z, F32 w);
internal Vec4F32 v4f32_add_v4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 v4f32_sub_v4f32(Vec4F32 a, Vec4F32 b);
internal Vec4F32 v4f32_mul_f32(Vec4F32 a, F32 t);
internal Vec4F32 v4f32_div_f32(Vec4F32 a, F32 t);

// S32

internal Vec2S32 v2s32(S32 x, S32 y);
internal Vec2S32 v2s32_add_v2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 v2s32_sub_v2s32(Vec2S32 a, Vec2S32 b);
internal Vec2S32 v2s32_mul_s32(Vec2S32 a, S32 t);
internal Vec2S32 v2s32_div_s32(Vec2S32 a, S32 t);

internal Vec3S32 v3s32(S32 x, S32 y, S32 z);
internal Vec3S32 v3s32_add_v3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 v3s32_sub_v3s32(Vec3S32 a, Vec3S32 b);
internal Vec3S32 v3s32_mul_s32(Vec3S32 a, S32 t);
internal Vec3S32 v3s32_div_s32(Vec3S32 a, S32 t);

internal Vec4S32 v4s32(S32 x, S32 y, S32 z, S32 w);
internal Vec4S32 v4s32_add_v4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 v4s32_sub_v4s32(Vec4S32 a, Vec4S32 b);
internal Vec4S32 v4s32_mul_s32(Vec4S32 a, S32 t);
internal Vec4S32 v4s32_div_s32(Vec4S32 a, S32 t);

// U32

internal Vec2U32 v2u32(U32 x, U32 y);
internal Vec2U32 v2u32_add_v2u32(Vec2U32 a, Vec2U32 b);
internal Vec2U32 v2u32_sub_v2u32(Vec2U32 a, Vec2U32 b);
internal Vec2U32 v2u32_mul_u32(Vec2U32 a, U32 t);
internal Vec2U32 v2u32_div_u32(Vec2U32 a, U32 t);

internal Vec3U32 v3u32(U32 x, U32 y, U32 z);
internal Vec3U32 v3u32_add_v3u32(Vec3U32 a, Vec3U32 b);
internal Vec3U32 v3u32_sub_v3u32(Vec3U32 a, Vec3U32 b);
internal Vec3U32 v3u32_mul_u32(Vec3U32 a, U32 t);
internal Vec3U32 v3u32_div_u32(Vec3U32 a, U32 t);

internal Vec4U32 v4u32(U32 x, U32 y, U32 z, U32 w);
internal Vec4U32 v4u32_add_v4u32(Vec4U32 a, Vec4U32 b);
internal Vec4U32 v4u32_sub_v4u32(Vec4U32 a, Vec4U32 b);
internal Vec4U32 v4u32_mul_u32(Vec4U32 a, U32 t);
internal Vec4U32 v4u32_div_u32(Vec4U32 a, U32 t);

// Mat4

internal Mat4F32 m4f32_ortho(F32 left, F32 right, F32 bottom, F32 top, F32 _near, F32 _far);

// Rect

internal RectF32 rectf32(Vec2F32 min, Vec2F32 max);
internal RectF32 rectf32_from_rects32(RectS32 rect);
internal RectS32 rects32(Vec2S32 min, Vec2S32 max);
internal RectU32 retu32(Vec2U32 min, Vec2U32 max);

global S8  S8_MIN  = (S8) 0x80;
global S16 S16_MIN = (S16) 0x8000;
global S32 S32_MIN = (S32) 0x80000000;
global S64 S64_MIN = (S64) 0x8000000000000000ll;

global S8  S8_MAX  = 0x7F;
global S16 S16_MAX = 0x7FFF;
global S32 S32_MAX = 0x7FFFFFFF;
global S64 S64_MAX = 0x7FFFFFFFFFFFFFFFll;

global U8  U8_MAX  = 0xFF;
global U16 U16_MAX = 0xFFFF;
global U32 U32_MAX = 0xFFFFFFFF;
global U64 U64_MAX = 0xFFFFFFFFFFFFFFFFllu;

// TODO(simon): Update constants to more precise values.
global F32 F32_EPSILON = 1.1920929e-7f;
global F32 F32_PI      = 3.14159265359f;
global F32 F32_TAU     = 6.28318530718f;
global F32 F32_E       = 2.71828182846f;
global U32 F32_BIAS    = 127;
global F32 F32_MAX     = 3.402823466e+38F;
global F32 F32_MIN     = 1.175494351e-38F;

// TODO(simon): Update constants to more precise values.
global F64 F64_EPSILON = 2.220446e-16;
global F64 F64_PI      = 3.141592653589793;
global F64 F64_TAU     = 6.28318530718;
global F64 F64_E       = 2.71828182846;
global U32 F64_BIAS    = 1023;

internal U8 u8_min(U8 a, U8 b);
internal U8 u8_max(U8 a, U8 b);
internal U8 u8_round_down_to_power_of_2(U8 value, U8 power);
internal U8 u8_round_up_to_power_of_2(U8 value, U8 power);
internal U8 u8_floor_to_power_of_2(U8 value);
internal U8 u8_rotate_right(U8 x, U8 amount);
internal U8 u8_rotate_left(U8 x, U8 amount);
internal U8 u8_ceil_to_power_of_2(U8 x);
internal U8 u8_reverse(U8 x);

internal U16 u16_min(U16 a, U16 b);
internal U16 u16_max(U16 a, U16 b);
internal U16 u16_round_down_to_power_of_2(U16 value, U16 power);
internal U16 u16_round_up_to_power_of_2(U16 value, U16 power);
internal U16 u16_floor_to_power_of_2(U16 value);
internal U16 u16_rotate_right(U16 x, U16 amount);
internal U16 u16_rotate_left(U16 x, U16 amount);
internal U16 u16_ceil_to_power_of_2(U16 x);
internal U16 u16_reverse(U16 x);
internal U16 u16_big_to_local_endian(U16 x);

internal U32 u32_min(U32 a, U32 b);
internal U32 u32_max(U32 a, U32 b);
internal U32 u32_round_down_to_power_of_2(U32 value, U32 power);
internal U32 u32_round_up_to_power_of_2(U32 value, U32 power);
internal U32 u32_floor_to_power_of_2(U32 value);
internal U32 u32_rotate_right(U32 x, U32 amount);
internal U32 u32_rotate_left(U32 x, U32 amount);
internal U32 u32_ceil_to_power_of_2(U32 x);
internal U32 u32_reverse(U32 x);
internal U32 u32_big_to_local_endian(U32 x);

internal U64 u64_min(U64 a, U64 b);
internal U64 u64_max(U64 a, U64 b);
internal U64 u64_round_down_to_power_of_2(U64 value, U64 power);
internal U64 u64_round_up_to_power_of_2(U64 value, U64 power);
internal U64 u64_floor_to_power_of_2(U64 value);
internal U64 u64_rotate_right(U64 x, U64 amount);
internal U64 u64_rotate_left(U64 x, U64 amount);
internal U64 u64_ceil_to_power_of_2(U64 x);
internal U64 u64_reverse(U64 x);
internal U64 u64_big_to_local_endian(U64 x);

internal S8 s8_min(S8 a, S8 B);
internal S8 s8_max(S8 a, S8 B);
internal S8 s8_abs(S8 x);

internal S16 s16_min(S16 a, S16 b);
internal S16 s16_max(S16 a, S16 b);
internal S16 s16_abs(S16 x);
internal S16 s16_big_to_local_endian(S16 x);

internal S32 s32_min(S32 a, S32 b);
internal S32 s32_max(S32 a, S32 b);
internal S32 s32_abs(S32 x);
internal S32 s32_big_to_local_endian(S32 x);

internal S64 s64_min(S64 a, S64 b);
internal S64 s64_max(S64 a, S64 b);
internal S64 s64_abs(S64 x);
internal S64 s64_big_to_local_endian(S64 x);

internal F32 f32_infinity(Void);
internal F32 f32_negative_infinity(Void);

internal F64 f64_infinity(Void);
internal F64 f64_negative_infinity(Void);

internal F32 f32_min(F32 a, F32 b);
internal F32 f32_max(F32 a, F32 b);
internal F32 f32_sign(F32 x);
internal F32 f32_abs(F32 x);
internal F32 f32_sqrt(F32 x);
internal F32 f32_cbrt(F32 x);
internal F32 f32_sin(F32 x);
internal F32 f32_cos(F32 x);
internal F32 f32_tan(F32 x);
internal F32 f32_arctan(F32 x);
internal F32 f32_arctan2(F32 y, F32 x);
internal F32 f32_ln(F32 x);
internal F32 f32_log(F32 x);
internal F32 f32_log2(F32 x);
internal F32 f32_lerp(F32 a, F32 b, F32 t);
internal F32 f32_unlerp(F32 a, F32 b, F32 x);
internal F32 f32_pow(F32 a, F32 b);
internal F32 f32_floor(F32 x);
internal F32 f32_ceil(F32 x);
internal U32 f32_round_to_u32(F32 x);
internal S32 f32_round_to_s32(F32 x);

internal F64 f64_min(F64 a, F64 b);
internal F64 f64_max(F64 a, F64 b);
internal F64 f64_abs(F64 x);
internal F64 f64_sqrt(F64 x);
internal F64 f64_sin(F64 x);
internal F64 f64_cos(F64 x);
internal F64 f64_tan(F64 x);
internal F64 f64_ln(F64 x);
internal F64 f64_lg(F64 x);
internal F64 f64_lerp(F64 a, F64 b, F64 t);
internal F64 f64_unlerp(F64 a, F64 b, F64 x);
internal F64 f64_pow(F64 a, F64 b);
internal F64 f64_floor(F64 x);
internal F64 f64_ceil(F64 x);

#endif // BASE_MATH_H
