internal R_TextureSlice
render_slice_from_texture(R_Texture texture, RectF32 region)
{
    R_TextureSlice result = {region, texture};
    return(result);
}

internal R_TextureSlice
render_create_texture_slice(R_Context *renderer, Str8 path)
{
    R_TextureSlice result;
    result.texture = render_create_texture(renderer, path);
    result.region.min = v2f32(0, 0);
    result.region.max = v2f32(1, 1);
    return(result);
}