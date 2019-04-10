#version 330 core

out vec4 frag_color;

in vec4 out_color;
in vec2 out_uv;

uniform sampler2D ftex;

void main() {
	frag_color = out_color;
}
