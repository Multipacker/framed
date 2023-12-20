internal Vec2F32
v2f32(F32 x, F32 y)
{
	return((Vec2F32) { x, y });
}

internal Vec2F32
v2f32_add_v2f32(Vec2F32 a, Vec2F32 b)
{
	return((Vec2F32) { a.x + b.x, a.y + b.y });
}

internal Vec2F32
v2f32_sub_v2f32(Vec2F32 a, Vec2F32 b)
{
	return((Vec2F32) { a.x - b.x, a.y - b.y });
}

internal Vec2F32
v2f32_mul_f32(Vec2F32 a, F32 t)
{
	return((Vec2F32) { a.x * t, a.y * t });
}


internal Vec2F32
v2f32_from_v2u32(Vec2U32 v)
{
	Vec2F32 result = (Vec2F32) { (F32) v.x, (F32) v.y };
	return(result);
}


internal Vec2F32
v2f32_div_f32(Vec2F32 a, F32 t)
{
	return((Vec2F32) { a.x / t, a.y / t });
}

internal Vec3F32
v3f32(F32 x, F32 y, F32 z)
{
	return((Vec3F32) { x, y, z });
}

internal Vec3F32
v3f32_add_v3f32(Vec3F32 a, Vec3F32 b)
{
	return((Vec3F32) { a.x + b.x, a.y + b.y, a.z + b.z });
}

internal Vec3F32
v3f32_sub_v3f32(Vec3F32 a, Vec3F32 b)
{
	return((Vec3F32) { a.x - b.x, a.y - b.y, a.z - b.z });
}

internal Vec3F32
v3f32_mul_f32(Vec3F32 a, F32 t)
{
	return((Vec3F32) { a.x * t, a.y * t, a.z * t });
}

internal Vec3F32
v3f32_div_f32(Vec3F32 a, F32 t)
{
	return((Vec3F32) { a.x / t, a.y / t, a.z / t });
}

internal Vec4F32
v4f32(F32 x, F32 y, F32 z, F32 w)
{
	return((Vec4F32) { x, y, z, w });
}

internal Vec4F32
v4f32_add_v4f32(Vec4F32 a, Vec4F32 b)
{
	return((Vec4F32) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w });
}

internal Vec4F32
v4f32_sub_v4f32(Vec4F32 a, Vec4F32 b)
{
	return((Vec4F32) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w });
}

internal Vec4F32
v4f32_mul_f32(Vec4F32 a, F32 t)
{
	return((Vec4F32) { a.x * t, a.y * t, a.z * t, a.w * t });
}

internal Vec4F32
v4f32_div_f32(Vec4F32 a, F32 t)
{
	return((Vec4F32) { a.x / t, a.y / t, a.z / t, a.w / t });
}

internal Vec2F32
v2f32_add_f32(Vec2F32 a, F32 t)
{
	Vec2F32 result;
	result.x = a.x + t;
	result.y = a.y + t;
	return(result);
}

internal Vec2F32
v2f32_sub_f32(Vec2F32 a, F32 t)
{
	Vec2F32 result;
	result.x = a.x - t;
	result.y = a.y - t;
	return(result);
}

// S32

internal Vec2S32
v2s32(S32 x, S32 y)
{
	return((Vec2S32) { x, y });
}

internal Vec2S32
v2s32_add_v2s32(Vec2S32 a, Vec2S32 b)
{
	return((Vec2S32) { a.x + b.x, a.y + b.y });
}

internal Vec2S32
v2s32_sub_v2s32(Vec2S32 a, Vec2S32 b)
{
	return((Vec2S32) { a.x - b.x, a.y - b.y });
}

internal Vec2S32
v2s32_mul_s32(Vec2S32 a, S32 t)
{
	return((Vec2S32) { a.x * t, a.y * t });
}

internal Vec2S32
v2s32_div_s32(Vec2S32 a, S32 t)
{
	return((Vec2S32) { a.x / t, a.y / t });
}

internal Vec3S32
v3s32(S32 x, S32 y, S32 z)
{
	return((Vec3S32) { x, y, z });
}

internal Vec3S32
v3s32_add_v3s32(Vec3S32 a, Vec3S32 b)
{
	return((Vec3S32) { a.x + b.x, a.y + b.y, a.z + b.z });
}

internal Vec3S32
v3s32_sub_v3s32(Vec3S32 a, Vec3S32 b)
{
	return((Vec3S32) { a.x - b.x, a.y - b.y, a.z - b.z });
}

internal Vec3S32
v3s32_mul_s32(Vec3S32 a, S32 t)
{
	return((Vec3S32) { a.x * t, a.y * t, a.z * t });
}

internal Vec3S32
v3s32_div_s32(Vec3S32 a, S32 t)
{
	return((Vec3S32) { a.x / t, a.y / t, a.z / t });
}

internal Vec4S32
v4s32(S32 x, S32 y, S32 z, S32 w)
{
	return((Vec4S32) { x, y, z, w });
}

internal Vec4S32
v4s32_add_v4s32(Vec4S32 a, Vec4S32 b)
{
	return((Vec4S32) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w });
}

internal Vec4S32
v4s32_sub_v4s32(Vec4S32 a, Vec4S32 b)
{
	return((Vec4S32) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w });
}

internal Vec4S32
v4s32_mul_s32(Vec4S32 a, S32 t)
{
	return((Vec4S32) { a.x * t, a.y * t, a.z * t, a.w * t });
}

internal Vec4S32
v4s32_div_s32(Vec4S32 a, S32 t)
{
	return((Vec4S32) { a.x / t, a.y / t, a.z / t, a.w / t });
}

// U32

internal Vec2U32
v2u32(U32 x, U32 y)
{
	return((Vec2U32) { x, y });
}

internal Vec2U32
v2u32_add_v2u32(Vec2U32 a, Vec2U32 b)
{
	return((Vec2U32) { a.x + b.x, a.y + b.y });
}

internal Vec2U32
v2u32_sub_v2u32(Vec2U32 a, Vec2U32 b)
{
	return((Vec2U32) { a.x - b.x, a.y - b.y });
}

internal Vec2U32
v2u32_mul_u32(Vec2U32 a, U32 t)
{
	return((Vec2U32) { a.x * t, a.y * t });
}

internal Vec2U32
v2u32_div_u32(Vec2U32 a, U32 t)
{
	return((Vec2U32) { a.x / t, a.y / t });
}

internal Vec3U32
v3u32(U32 x, U32 y, U32 z)
{
	return((Vec3U32) { x, y, z });
}

internal Vec3U32
v3u32_add_v3u32(Vec3U32 a, Vec3U32 b)
{
	return((Vec3U32) { a.x + b.x, a.y + b.y, a.z + b.z });
}

internal Vec3U32
v3u32_sub_v3u32(Vec3U32 a, Vec3U32 b)
{
	return((Vec3U32) { a.x - b.x, a.y - b.y, a.z - b.z });
}

internal Vec3U32
v3u32_mul_u32(Vec3U32 a, U32 t)
{
	return((Vec3U32) { a.x * t, a.y * t, a.z * t });
}

internal Vec3U32
v3u32_div_u32(Vec3U32 a, U32 t)
{
	return((Vec3U32) { a.x / t, a.y / t, a.z / t });
}

internal Vec4U32
v4u32(U32 x, U32 y, U32 z, U32 w)
{
	return((Vec4U32) { x, y, z, w });
}

internal Vec4U32
v4u32_add_v4u32(Vec4U32 a, Vec4U32 b)
{
	return((Vec4U32) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w });
}

internal Vec4U32
v4u32_sub_v4u32(Vec4U32 a, Vec4U32 b)
{
	return((Vec4U32) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w });
}

internal Vec4U32
v4u32_mul_u32(Vec4U32 a, U32 t)
{
	return((Vec4U32) { a.x * t, a.y * t, a.z * t, a.w * t });
}

internal Vec4U32
v4u32_div_u32(Vec4U32 a, U32 t)
{
	return((Vec4U32) { a.x / t, a.y / t, a.z / t, a.w / t });
}

internal Mat4F32
m4f32_ortho(F32 left, F32 right, F32 bottom, F32 top, F32 n, F32 f)
{
	Mat4F32 result;

	result.m[0][0] = 2 / (right - left);
	result.m[0][1] = 0;
	result.m[0][2] = 0;
	result.m[0][3] = 0;

	result.m[1][0] = 0;
	result.m[1][1] = 2 / (top - bottom);
	result.m[1][2] = 0;
	result.m[1][3] = 0;

	result.m[2][0] = 0;
	result.m[2][1] = 0;
	result.m[2][2] = 1 / (n - f);
	result.m[2][3] = 0;

	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = n / (n - f);
	result.m[3][3] = 1;

	return result;
}

internal RectF32
rectf32(Vec2F32 min, Vec2F32 max)
{
	return((RectF32) { min, max });
}


internal RectF32
rectf32_from_rects32(RectS32 rect)
{
	RectF32 result;

	result.x0 = (F32) rect.x0;
	result.y0 = (F32) rect.y0;
	result.x1 = (F32) rect.x1;
	result.y1 = (F32) rect.y1;

	return(result);
}

internal RectF32
rectf32_from_rectu32(RectU32 rect)
{
	RectF32 result;

	result.x0 = (F32) rect.x0;
	result.y0 = (F32) rect.y0;
	result.x1 = (F32) rect.x1;
	result.y1 = (F32) rect.y1;

	return(result);
}

internal RectS32
rects32(Vec2S32 min, Vec2S32 max)
{
	return((RectS32) { min, max });
}

internal RectU32
rectu32(Vec2U32 min, Vec2U32 max)
{
	return((RectU32) { min, max });
}

internal B32
rectf32_contains_rectf32(RectF32 container, RectF32 tested)
{
	B32 contains_x = (container.min.x <= tested.min.x && tested.max.x <= container.max.x);
	B32 contains_y = (container.min.y <= tested.min.y && tested.max.y <= container.max.y);
	B32 result = (contains_x && contains_y);
	return(result);
}

internal B32
rectf32_contains_v2f32(RectF32 container, Vec2F32 tested)
{
	B32 contains_x = (container.min.x <= tested.x && tested.x <= container.max.x);
	B32 contains_y = (container.min.y <= tested.y && tested.y <= container.max.y);
	B32 result = (contains_x && contains_y);
	return(result);
}

internal B32
rectf32_overlaps(RectF32 a, RectF32 b)
{
	B32 overlaps_x = (a.min.x <= b.max.x && b.min.x <= a.max.x);
	B32 overlaps_y = (a.min.y <= b.max.y && b.min.y <= a.max.y);
	B32 result = (overlaps_x && overlaps_y);
	return(result);
}

internal U8 u8_min(U8 a, U8 b)
{
	U8 result = (a < b ? a : b);
	return(result);
}

internal U8 u8_max(U8 a, U8 b)
{
	U8 result = (a > b ? a : b);
	return(result);
}

internal U8
u8_round_down_to_power_of_2(U8 value, U8 power)
{
	U8 result = value & ~(power - 1);
	return(result);
}

internal U8
u8_round_up_to_power_of_2(U8 value, U8 power)
{
	U8 result = (value + power - 1) & ~(power - 1);
	return(result);
}

internal U8
u8_floor_to_power_of_2(U8 value)
{
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value = value ^ (value >> 1);
	return(value);
}

internal U8
u8_rotate_right(U8 x, U8 amount)
{
	return (U8) ((x >> amount) | (x << (8 - amount)));
}

internal U8
u8_rotate_left(U8 x, U8 amount)
{
	return (U8) ((x >> (8 - amount)) | (x << amount));
}

internal U8
u8_ceil_to_power_of_2(U8 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	++x;
	return x;
}

internal U8
u8_reverse(U8 x)
{
	x = (U8) (((x >> 1) & 0x55) | ((x & 0x55) << 1));
	x = (U8) (((x >> 2) & 0x33) | ((x & 0x33) << 2));
	x = (U8) (((x >> 4) & 0x0F) | ((x & 0x0F) << 4));
	return x;
}

internal U16
u16_min(U16 a, U16 b)
{
	U16 result = (a < b ? a : b);
	return result;

}

internal U16
u16_max(U16 a, U16 b)
{
	U16 result = (a > b ? a : b);
	return result;

}

internal U16
u16_round_down_to_power_of_2(U16 value, U16 power)
{
	U16 result = value & ~(power - 1);
	return result;

}

internal U16
u16_round_up_to_power_of_2(U16 value, U16 power)
{
	U16 result = (value + power - 1) & ~(power - 1);
	return result;

}

internal U16
u16_floor_to_power_of_2(U16 value)
{
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	return value ^ (value >> 1);
}

internal U16
u16_rotate_right(U16 x, U16 amount)
{
	return (U16) ((x >> amount) | (x << (16 - amount)));
}

internal U16
u16_rotate_left(U16 x, U16 amount)
{
	return (U16) ((x >> (16 - amount)) | (x << amount));
}

internal U16
u16_ceil_to_power_of_2(U16 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	++x;
	return x;
}

internal U16
u16_reverse(U16 x)
{
	x = (U16) (((x >> 1) & 0x5555) | ((x & 0x5555) << 1));
	x = (U16) (((x >> 2) & 0x3333) | ((x & 0x3333) << 2));
	x = (U16) (((x >> 4) & 0x0F0F) | ((x & 0x0F0F) << 4));
	x = (U16) (((x >> 8) & 0x00FF) | ((x & 0x00FF) << 8));
	return x;
}

internal U16
u16_big_to_local_endian(U16 x)
{
#if COMPILER_CL
	return _byteswap_ushort(x);
#elif COMPILER_CLANG || COMPILER_GCC
	return __builtin_bswap16(x);
#else
# error Your compiler does not have an implementation of u16_big_to_local_endian.
#endif
}

internal U32
u32_min(U32 a, U32 b)
{
	U32 result = (a < b ? a : b);
	return result;
}

internal U32
u32_max(U32 a, U32 b)
{
	U32 result = (a > b ? a : b);
	return result;
}

internal U32
u32_round_down_to_power_of_2(U32 value, U32 power)
{
	U32 result = value & ~(power - 1);
	return result;
}

internal U32
u32_round_up_to_power_of_2(U32 value, U32 power)
{
	U32 result = (value + power - 1) & ~(power - 1);
	return result;
}

internal U32
u32_floor_to_power_of_2(U32 value)
{
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return value ^ (value >> 1);
}

internal U32
u32_rotate_right(U32 x, U32 amount)
{
	return (x >> amount) | (x << (32 - amount));
}

internal U32
u32_rotate_left(U32 x, U32 amount)
{
	return (x >> (32 - amount)) | (x << amount);
}

internal U32
u32_ceil_to_power_of_2(U32 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;
	return x;
}

internal U32
u32_reverse(U32 x)
{
	x = ((x >>  1) & 0x55555555) | ((x & 0x55555555) <<  1);
	x = ((x >>  2) & 0x33333333) | ((x & 0x33333333) <<  2);
	x = ((x >>  4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) <<  4);
	x = ((x >>  8) & 0x00FF00FF) | ((x & 0x00FF00FF) <<  8);
	x = ((x >> 16) & 0x0000FFFF) | ((x & 0x0000FFFF) << 16);
	return x;
}

internal U32
u32_big_to_local_endian(U32 x)
{
#if COMPILER_CL
	return _byteswap_ulong(x);
#elif COMPILER_CLANG || COMPILER_GCC
	return __builtin_bswap32(x);
#else
# error Your compiler does not have an implementation of u32_big_to_local_endian.
#endif
}

internal U64
u64_min(U64 a, U64 b)
{
	U64 result = (a < b ? a : b);
	return result;
}

internal U64
u64_max(U64 a, U64 b)
{
	U64 result = (a > b ? a : b);
	return result;
}

internal U64
u64_round_down_to_power_of_2(U64 value, U64 power)
{
	U64 result = value & ~(power - 1);
	return result;
}

internal U64
u64_round_up_to_power_of_2(U64 value, U64 power)
{
	U64 result = (value + power - 1) & ~(power - 1);
	return result;
}

internal U64
u64_floor_to_power_of_2(U64 value)
{
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value |= value >> 32;
	return value ^ (value >> 1);
}

internal U64
u64_rotate_right(U64 x, U64 amount)
{
	return (x >> amount) | (x << (64 - amount));
}

internal U64
u64_rotate_left(U64 x, U64 amount)
{
	return (x >> (64 - amount)) | (x << amount);
}

internal U64
u64_ceil_to_power_of_2(U64 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	++x;
	return x;
}

internal U64
u64_reverse(U64 x)
{
	x = ((x >>  1) & 0x5555555555555555) | ((x & 0x5555555555555555) <<  1);
	x = ((x >>  2) & 0x3333333333333333) | ((x & 0x3333333333333333) <<  2);
	x = ((x >>  4) & 0x0F0F0F0F0F0F0F0F) | ((x & 0x0F0F0F0F0F0F0F0F) <<  4);
	x = ((x >>  8) & 0x00FF00FF00FF00FF) | ((x & 0x00FF00FF00FF00FF) <<  8);
	x = ((x >> 16) & 0x0000FFFF0000FFFF) | ((x & 0x0000FFFF0000FFFF) << 16);
	x = ((x >> 32) & 0x00000000FFFFFFFF) | ((x & 0x00000000FFFFFFFF) << 32);
	return x;
}

internal U64
u64_big_to_local_endian(U64 x)
{
#if COMPILER_CL
	return _byteswap_uint64(x);
#elif COMPILER_CLANG || COMPILER_GCC
	return __builtin_bswap64(x);
#else
# error Your compiler does not have an implementation of u64_big_to_local_endian.
#endif
}

internal S8
s8_min(S8 a, S8 b)
{
	S8 result = (a < b ? a : b);
	return result;
}

internal S8
s8_max(S8 a, S8 b)
{
	S8 result = (a > b ? a : b);
	return result;
}

internal S8
s8_abs(S8 x)
{
	S8 result = (x < 0 ? -x : x);
	return result;
}

internal S16
s16_min(S16 a, S16 b)
{
	S16 result = (a < b ? a : b);
	return result;
}

internal S16
s16_max(S16 a, S16 b)
{
	S16 result = (a > b ? a : b);
	return result;
}

internal S16
s16_abs(S16 x)
{
	S16 result = (x < 0 ? -x : x);
	return result;
}

internal S16
s16_big_to_local_endian(S16 x)
{
#if COMPILER_CL
	U16 swapped = _byteswap_ushort(*(U16 *) &x);
#elif COMPILER_CLANG || COMPILER_GCC
	U16 swapped = __builtin_bswap16(*(U16 *) &x);
#else
# error Your compiler does not have an implementation of s16_big_to_local_endian.
#endif
	return *(S16 *) &swapped;
}

internal S32
s32_min(S32 a, S32 b)
{
	S32 result = (a < b ? a : b);
	return result;
}

internal S32
s32_max(S32 a, S32 b)
{
	S32 result = (a > b ? a : b);
	return result;
}

internal S32
s32_abs(S32 x)
{
	S32 result = (x < 0 ? -x : x);
	return result;
}

internal S32
s32_big_to_local_endian(S32 x)
{
#if COMPILER_CL
	U32 swapped = _byteswap_ulong(*(U32 *) &x);
#elif COMPILER_CLANG || COMPILER_GCC
	U32 swapped = __builtin_bswap32(*(U32 *) &x);
#else
# error Your compiler does not have an implementation of s32_big_to_local_endian.
#endif
	return *(S32 *) &swapped;
}

internal S64
s64_min(S64 a, S64 b)
{
	S64 result = (a < b ? a : b);
	return result;
}

internal S64
s64_max(S64 a, S64 b)
{
	S64 result = (a > b ? a : b);
	return result;
}

internal S64
s64_abs(S64 x)
{
	S64 result = (x < 0 ? -x : x);
	return result;
}

internal S64
s64_big_to_local_endian(S64 x)
{
#if COMPILER_CL
	U64 swapped = _byteswap_uint64(*(U64 *) &x);
#elif COMPILER_CLANG || COMPILER_GCC
	U64 swapped = __builtin_bswap64(*(U64 *) &x);
#else
# error Your compiler does not have an implementation of s64_big_to_local_endian.
#endif

	return *(S64 *) &swapped;
}

internal F32
f32_infinity(Void)
{
	union { F32 f; U32 u; } result;
	result.u = 0x7F800000;
	return result.f;
}

internal F32
f32_negative_infinity(Void)
{
	union { F32 f; U32 u; } result;
	result.u = 0xFF800000;
	return result.f;
}

internal F64
f64_infinity(Void)
{
	union { F64 f; U64 u; } result;
	result.u = 0x7F80000000000000;
	return result.f;
}

internal F64
f64_negative_infinity(Void)
{
	union { F64 f; U64 u; } result;
	result.u = 0xFF80000000000000;
	return result.f;
}

internal F32
f32_min(F32 a, F32 b)
{
	F32 result = (a < b ? a : b);
	return result;
}

internal F32
f32_max(F32 a, F32 b)
{
	F32 result = (a > b ? a : b);
	return result;
}

internal F32
f32_sign(F32 x)
{
	union { F32 f; U32 u; } result;
	result.f = x;
	result.u &= 0x80000000;
	result.u |= 0x3F800000; // Binary representation of 1.0
	return result.f;
}

internal F32
f32_abs(F32 x)
{
	union { F32 f; U32 u; } result;
	result.f = x;
	result.u &= 0x7FFFFFFF;
	return result.f;
}

internal F32
f32_sqrt(F32 x)
{
	return sqrtf(x);
}

internal F32
f32_cbrt(F32 x)
{
	return cbrtf(x);
}

internal F32
f32_sin(F32 x)
{
	return sinf(x);
}

internal F32
f32_cos(F32 x)
{
	return cosf(x);
}

internal F32
f32_tan(F32 x)
{
	return tanf(x);
}

internal F32
f32_arctan(F32 x)
{
	return atanf(x);
}

internal F32
f32_arctan2(F32 y, F32 x)
{
	return atan2f(y, x);
}

internal F32
f32_ln(F32 x)
{
	return logf(x);
}

internal F32
f32_log2(F32 x)
{
	return log2f(x);
}

internal F32
f32_log(F32 x)
{
	return log10f(x);
}

internal F32
f32_lerp(F32 a, F32 b, F32 t)
{
	F32 x = a + (b - a) * t;
	return x;
}

internal F32
f32_unlerp(F32 a, F32 b, F32 x)
{
	F32 t = 0.0f;
	if (a != b)
	{
		t = (x - a) / (b - a);
	}
	return t;
}

internal F32
f32_pow(F32 a, F32 b)
{
	return powf(a, b);
}

internal F32
f32_floor(F32 x)
{
	return floorf(x);
}

internal F32
f32_ceil(F32 x)
{
	return ceilf(x);
}

internal U32
f32_round_to_u32(F32 x)
{
	return (U32) roundf(x);
}

internal S32
f32_round_to_s32(F32 x)
{
	return (S32) roundf(x);
}

internal F32
f32_clamp(F32 min, F32 val, F32 max)
{
	F32 result = f32_max(f32_min(val, max), min);
	return(result);
}

internal F64
f64_min(F64 a, F64 b)
{
	F64 result = (a < b ? a : b);
	return result;
}

internal F64
f64_max(F64 a, F64 b)
{
	F64 result = (a > b ? a : b);
	return result;
}

internal F64
f64_abs(F64 x)
{
	union { F64 f; U64 u; } result;
	result.f = x;
	result.u &= 0x7FFFFFFFFFFFFFFF;
	return result.f;
}

internal F64
f64_sqrt(F64 x)
{
	return sqrt(x);
}

internal F64
f64_sin(F64 x)
{
	return sin(x);
}

internal F64
f64_cos(F64 x)
{
	return cos(x);
}

internal F64
f64_tan(F64 x)
{
	return tan(x);
}

internal F64
f64_ln(F64 x)
{
	return log(x);
}

internal F64
f64_lg(F64 x)
{
	return log10(x);
}

internal F64
f64_lerp(F64 a, F64 b, F64 t)
{
	F64 x = a + (b - a) * t;
	return x;
}

internal F64
f64_unlerp(F64 a, F64 b, F64 x)
{
	F64 t = 0.0;
	if (a != b)
	{
		t = (x - a) / (b - a);
	}
	return t;
}

internal F64
f64_pow(F64 a, F64 b)
{
	return pow(a, b);
}

internal F64
f64_floor(F64 x)
{
	return floor(x);
}

internal F64
f64_ceil(F64 x)
{
	return ceil(x);
}

internal F64
f64_clamp(F64 min, F64 val, F64 max)
{
	F64 result = f64_max(f64_min(val, max), min);
	return(result);
}
