#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

uniform mat4 view_to_projection;
uniform mat4 world_to_view;

out vec4 out_color;
out vec2 out_uv;

void main() {
    gl_Position =  view_to_projection * world_to_view * vec4(position, 0.0, 1.0);
	out_color = color;
	out_uv = uv;
}
