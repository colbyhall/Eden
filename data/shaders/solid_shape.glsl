#version 330 core

#if VERT
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 3) in vec2 uv;

uniform mat4 view_to_projection;
uniform mat4 world_to_view;

out vec4 out_color;
out vec2 out_uv;

void main() {
    gl_Position =  view_to_projection * world_to_view * vec4(position, 0.0, 1.0);
	out_color = color;
	out_uv = uv;
}
#elif FRAG
out vec4 frag_color;

in vec4 out_color;
in vec2 out_uv;

uniform sampler2D ftex;

void main() {
	vec4 result = texture(ftex, out_uv);

	result.w = 1.0;

	frag_color = out_color;
}

#endif