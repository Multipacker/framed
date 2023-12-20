#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_NO_STDIO

#if COMPILER_CLANG
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wconversion"
#endif
#include "stb_image.h"
#if COMPILER_CLANG
# pragma clang diagnostic pop
#endif

internal R_TextureSlice
render_slice_from_texture(R_Texture texture, RectF32 uv)
{
	R_TextureSlice result = { uv, texture };
	return(result);
}

internal R_TextureSlice
render_slice_from_texture_region(R_Texture texture, RectU32 region)
{
#if 0
	RectF32 uv = rectf32_from_rectu32(region);
	R_TextureSlice result = { region, texture };
	return(result);
#endif
	R_TextureSlice result = { 0 };
	return(result);
}

internal R_TextureSlice
render_create_texture_slice(R_Context *renderer, Str8 path, R_ColorSpace color_space)
{
	R_TextureSlice result;
	result.texture = render_create_texture(renderer, path, color_space);
	result.region.min = v2f32(0, 0);
	result.region.max = v2f32(1, 1);
	return(result);
}

internal F32
f32_srgb_to_linear(F32 value)
{
	F32 result = 0.0f;
	if (value < 0.04045f)
	{
		result = value / 12.92f;
	}
	else
	{
		result = f32_pow((value + 0.055f) / 1.055f, 2.4f);
	}
	return(result);
}

internal Vec4F32
vec4f32_srgb_to_linear(Vec4F32 srgb)
{
	Vec4F32 result = v4f32(
		f32_srgb_to_linear(srgb.r),
		f32_srgb_to_linear(srgb.g),
		f32_srgb_to_linear(srgb.b),
		srgb.a
	);
	return(result);
}

internal F32
f32_linear_to_srgb(F32 value)
{
	F32 result = 0.0f;
	if (value < 0.0031308f)
	{
		result  = value * 12.92f;
	}
	else
	{
		result = 1.055f * f32_pow(value, 1.0f / 2.4f) - 0.055f;
	}
	return(result);
}

internal Vec4F32
vec4f32_linear_to_srgb(Vec4F32 linear)
{
	Vec4F32 result = v4f32(
		f32_linear_to_srgb(linear.r),
		f32_linear_to_srgb(linear.g),
		f32_linear_to_srgb(linear.b),
		linear.a
	);
	return(result);
}

internal R_RenderStats
render_get_stats(R_Context *renderer)
{
	return(renderer->render_stats[1]);
}