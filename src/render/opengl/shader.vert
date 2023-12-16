#version 450 core

out vec3 vert_color;

vec2 verticies[] = {
	vec2(0, 0),
	vec2(1, 0),
	vec2(0, 1)
};

vec3 colors[] = {
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1)
};

void
main()
{
	vert_color = colors[gl_VertexID];
	gl_Position = vec4(verticies[gl_VertexID], 0.0, 0.0);
}
