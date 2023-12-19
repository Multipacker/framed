#version 450 core

in vec2  vert_pos;
in vec2  vert_uv;
in vec4  vert_color;
in float vert_softness;
in float vert_border_thickness;
in float vertex_id;
in flat vec4 vert_radies;
in flat float vert_omit_texture;
// TODO(simon): See if we can avoid passing these
in flat vec2 vert_center;
in flat vec2 vert_half_size;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;

float
rounded_rect_sdf(vec2 sample_pos, vec2 rect_center, vec2 rect_half_size, float radius)
{
	vec2 d2 = abs(rect_center - sample_pos) - rect_half_size + radius;
	return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - radius;
}

// Converts a color from linear light gamma to sRGB gamma
vec4
fromLinear(vec4 linearRGB)
{
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3  higher = vec3(1.055) * pow(linearRGB.rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3  lower  = linearRGB.rgb * vec3(12.92);

    return vec4(mix(higher, lower, cutoff), linearRGB.a);
}

// Converts a color from sRGB gamma to linear light gamma
vec4
toLinear(vec4 sRGB)
{
    bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));
    vec3  higher = pow((sRGB.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3  lower  = sRGB.rgb / vec3(12.92);

    return vec4(mix(higher, lower, cutoff), sRGB.a);
}

void
main()
{
	float radius = vert_radies[int(round(vertex_id))];
	float softness_padding = max(0, vert_softness * 2 - 1);
	float dist = rounded_rect_sdf(vert_pos, vert_center, vert_half_size - softness_padding, radius);
	float sdf_factor = 1.0 - smoothstep(0, 2 * vert_softness, dist);

	float border_factor = 1.f;
	if (vert_border_thickness != 0)
	{
		vec2 interior_half_size = vert_half_size - vert_border_thickness;

		float interior_radius_reduce_f = min(interior_half_size.x / vert_half_size.x, interior_half_size.y / vert_half_size.y);
		float interior_corner_radius = radius * interior_radius_reduce_f * interior_radius_reduce_f;

		float inside_dist = rounded_rect_sdf(vert_pos, vert_center, interior_half_size - softness_padding, interior_corner_radius);

		float inside_factor = smoothstep(0, 2 * vert_softness, inside_dist);
		border_factor = inside_factor;
	}

	vec4 sample_color = vec4(1.0);
	if (vert_omit_texture < 1)
	{
		sample_color = texture(uniform_sampler, vert_uv);
	}

	vec4 color = sample_color * vert_color;

	frag_color = vec4(color.rgb, color.a * sdf_factor * border_factor);
}
