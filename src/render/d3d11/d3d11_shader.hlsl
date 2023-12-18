struct VS_INPUT
{
	float2 min             : MIN;
	float2 max             : MAX;
	float2 min_uv          : MIN_UV;
	float2 max_uv          : MAX_UV;
	float4 colors[4]       : COLOR;
	float4 corner_radius   : CORNER_RADIUS;
	float softness         : SOFTNESS;
	float border_thickness : BORDER_THICKNESS;
	float emit_texture     : EMIT_TEXTURE;
	float is_subpixel_text : IS_SUBPIXEL_TEXT;
	uint vertex_id         : SV_VertexID;
};

struct PS_INPUT
{
	float4 dst_pos         : SV_POSITION;
	float2 dst_half_size   : DST_HALF_SIZE;
	float2 dst_center      : DST_CENTER;
	float2 uv              : UV;
	float4 color           : COLOR;
	nointerpolation float4 corner_radius    : CORNER_RADIUS;
	float edge_softness    : SOFTNESS;
	float border_thickness : BORDER_THICKNESS;
	float emit_texture     : EMIT_TEXTURE;
	float is_subpixel_text : IS_SUBPIXEL_TEXT;
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

	float2 uv_half_size = (input.max_uv - input.min_uv) / 2;
	float2 uv_center = (input.max_uv + input.min_uv) / 2;
	float2 uv_pos = vertices[input.vertex_id] * (uv_half_size * (1 + input.softness / (dst_half_size*2).x)) + uv_center;

	PS_INPUT output;
	output.dst_pos = mul(uTransform, float4(dst_pos, 0, 1));
	output.dst_half_size = dst_half_size;
	output.dst_center = dst_center;
	output.uv = uv_pos;
	output.color = input.colors[input.vertex_id];
	output.corner_radius = input.corner_radius;
	output.edge_softness = input.softness;
	output.border_thickness = input.border_thickness;
	output.emit_texture = input.emit_texture;
	output.vertex_id = input.vertex_id;
	output.is_subpixel_text = input.is_subpixel_text;
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

struct ps_out
{
	float4 color0 : SV_TARGET0;
	float4 color1 : SV_TARGET1;
};

ps_out ps(PS_INPUT input)
{
	input.uv.x = clamp(input.uv.x, 0, 1);
	input.uv.y = clamp(input.uv.y, 0, 1);
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

	float4 sample_color = float4(1, 1, 1, 1);
	if (input.emit_texture < 1)
	{
		sample_color = texture0.Sample(sampler0, input.uv);
	}

	ps_out output;

	if (input.is_subpixel_text < 1)
	{
		float4 color = input.color * sample_color;
		color.a *= sdf_factor * border_factor;
		output.color0 = color;
		output.color1 = float4(color.a, color.a, color.a, color.a);
	}
	else
	{
		output.color0 = float4(input.color.rgb * input.color.a, input.color.a);
		output.color1 = float4(sample_color.rgb, input.color.a);
	}

	return output;
}
