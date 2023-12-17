#version 450 core

in vec2  vert_pos;
in vec4  vert_color;
in float vert_softness;
in float vert_border_thickness;
in float vertex_id;
in flat vec4 vert_radies;
// TODO(simon): See if we can avoid passing these
in flat vec2 vert_center;
in flat vec2 vert_half_size;

out vec4 frag_color;

uniform mat4 uniform_projection;

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
	float dist = rounded_rect_sdf(vert_pos, vert_center, vert_half_size, vert_radies[int(round(vertex_id))]);
	float sdf_factor = 1.0 - smoothstep(0, 2 * vert_softness, dist);

	frag_color = vec4(vert_color.rgb, vert_color.a * sdf_factor);
}