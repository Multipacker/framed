#version 450 core

layout (location = 0)  in vec2  instance_min;
layout (location = 1)  in vec2  instance_max;
layout (location = 2)  in mat4  instance_colors;
layout (location = 6)  in vec4  instance_radies;
layout (location = 7)  in float instance_softness;
layout (location = 8)  in float instance_border_thickness;
layout (location = 9)  in float instance_omit_texture;
layout (location = 10) in float instance_is_subpixel_text;
layout (location = 11) in vec2  instance_min_uv;
layout (location = 12) in vec2  instance_max_uv;

out vec2  vert_pos;
out vec2  vert_uv;
out vec4  vert_color;
out float vert_softness;
out float vert_border_thickness;
out float vertex_id;
out flat vec4 vert_radies;
out flat float vert_omit_texture;
out flat float vert_is_subpixel_text;
// TODO(simon): See if we can avoid passing these
out flat vec2 vert_center;
out flat vec2 vert_half_size;
out flat vec2 vert_min_uv;
out flat vec2 vert_max_uv;

uniform mat4 uniform_projection;

const vec2 verticies[] = {
	vec2(-1.0, -1.0),
	vec2(+1.0, -1.0),
	vec2(-1.0, +1.0),
	vec2(+1.0, +1.0)
};

void
main()
{
	vec2 center    = 0.5 * (instance_max + instance_min);
	vec2 half_size = 0.5 * (instance_max - instance_min);
	vec2 position  = center + (half_size + instance_softness) * verticies[gl_VertexID];
	vec2 uv_center    = 0.5 * (instance_max_uv + instance_min_uv);
	vec2 uv_half_size = 0.5 * (instance_max_uv - instance_min_uv);
	vec2 uv           = uv_center + (uv_half_size * (1.0 + instance_softness / half_size)) * verticies[gl_VertexID];

	gl_Position           = uniform_projection * vec4(position, 0.0, 1.0);
	vert_pos              = position;
	vert_uv               = uv;
	vert_color            = instance_colors[gl_VertexID];
	vert_softness         = instance_softness;
	vert_border_thickness = instance_border_thickness;
	vertex_id             = gl_VertexID;
	vert_center           = center;
	vert_radies           = instance_radies;
	vert_omit_texture     = instance_omit_texture;
	vert_is_subpixel_text = instance_is_subpixel_text;
	vert_half_size        = half_size;
	vert_min_uv           = instance_min_uv;
	vert_max_uv           = instance_max_uv;
}
