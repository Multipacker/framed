struct VS_INPUT
{
	float2 min             : MIN;
	float2 max             : MAX;
	float4 colors[4]       : COLOR;
	float4 corner_radius   : CORNER_RADIUS;
	float softness         : SOFTNESS;
	float border_thickness : BORDER_THICKNESS;
	uint vertex_id         : SV_VertexID;
};

struct PS_INPUT
{
	float4 dst_pos         : SV_POSITION;
	float2 dst_half_size   : DST_HALF_SIZE;
	float2 dst_center      : DST_CENTER;
	float4 color           : COLOR;
	nointerpolation float4 corner_radius    : CORNER_RADIUS;
	float edge_softness    : SOFTNESS;
	float border_thickness : BORDER_THICKNESS;
    float vertex_id        : VERTEX_ID;
};

cbuffer cbuffer0 : register(b0)
{
	float4x4 uTransform;
}

sampler sampler0 : register(s0);

Texture2D<float4> texture0 : register(t0);

PS_INPUT vs(VS_INPUT input)
{
	float2 vertices[4];
	vertices[0] = float2(-1, -1);
	vertices[1] = float2(+1, -1);
	vertices[2] = float2(-1, +1);
	vertices[3] = float2(+1, +1);

	float2 dst_half_size = (input.max - input.min) / 2;
	float2 dst_center = (input.max + input.min) / 2;
	float2 dst_pos = (vertices[input.vertex_id] * (dst_half_size + input.softness) + dst_center);

	PS_INPUT output;
	output.dst_pos = mul(uTransform, float4(dst_pos, 0, 1));
	output.dst_half_size = dst_half_size;
	output.dst_center = dst_center;
	output.color = input.colors[input.vertex_id];
	output.corner_radius = input.corner_radius;
	output.edge_softness = input.softness;
	output.border_thickness = input.border_thickness;
	output.vertex_id = input.vertex_id;
	return output;
}

float rounded_rect_sdf(float2 sample_pos,
                     float2 rect_center,
                     float2 rect_half_size,
                     float r)
{
	float2 d2 = (abs(rect_center - sample_pos) -
               rect_half_size +
               float2(r, r));
	return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
	float corner_radius = input.corner_radius[round(input.vertex_id)];

	float2 softness_padding = float2(max(0, input.edge_softness * 2 - 1), max(0, input.edge_softness * 2 - 1));

	float2 dst_pos = float2(input.dst_pos.xy);
	float dist = rounded_rect_sdf(dst_pos,
                                  input.dst_center,
                                  input.dst_half_size-softness_padding,
                                  corner_radius);

	float sdf_factor = 1.f - smoothstep(0, 2*input.edge_softness, dist);


	float border_factor = 1.f;
	if(input.border_thickness != 0)
	{
		float2 interior_half_size = input.dst_half_size - float2(input.border_thickness, input.border_thickness);

		float interior_radius_reduce_f = min(interior_half_size.x/input.dst_half_size.x,
                                         interior_half_size.y/input.dst_half_size.y);
		float interior_corner_radius =
             (corner_radius *
              interior_radius_reduce_f *
              interior_radius_reduce_f);

		float inside_d = rounded_rect_sdf(dst_pos,
                                          input.dst_center,
                                          interior_half_size-
                                          softness_padding,
                                          interior_corner_radius);

		float inside_f = smoothstep(0, 2*input.edge_softness, inside_d);
		border_factor = inside_f;
	}

	float4 color = input.color;
	color.a *= sdf_factor * border_factor;

	return color;
}
