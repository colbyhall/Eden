#pragma once

#include <ch_stl/opengl.h>
#include <ch_stl/math.h>

bool draw_init();
void draw_begin();
void draw_end();

void refresh_shader_transform();
void render_right_handed();

void immediate_begin();
void immediate_flush();

void immediate_vertex(f32 x, f32 y, const ch::Color& color, ch::Vector2 uv = 0.f, f32 z_index = 9.f);

void immediate_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f);
CH_FORCEINLINE void draw_quad(f32 x0, f32 y0, f32 x1, f32 y1, const ch::Color& color, f32 z_index = 9.f) {
	immediate_begin();
	immediate_quad(x0, y0, x1, y1, color, z_index);
	immediate_flush();
}
