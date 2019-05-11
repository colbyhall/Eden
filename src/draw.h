#pragma once

#include "types.h"
#include "opengl.h"
#include "math.h"
#include "font.h"

struct Buffer;
struct Buffer_View;
struct String;

void draw_init();

void bind_font(Font* font);

extern u32 draw_calls;
extern size_t verts_drawn;
extern size_t verts_culled;

void render_frame_begin();
void render_frame_end();

void immediate_begin();
void immediate_flush();

void refresh_transformation();
void render_right_handed();

void draw_rect(float x0, float y0, float x1, float y1, const Color& color);
void draw_string(const String& str, float x, float y, const Color& color, const Font& font);
void draw_buffer_view(const Buffer* buffer, float x0, float y0, float x1, float y1, const Font& font);
Vector2 get_draw_string_size(const String& str, const Font& font);

void immediate_quad(float x0, float y0, float x1, float y1, const Color& color);
void immediate_glyph(const Font_Glyph& glyph, float x, float y, const Color& color, const Font& font);
Font_Glyph immediate_char(u8 c, float x, float y, const Color& color, const Font& font);

Vector2 immediate_string(const String& str, float x, float y, const Color& color, const Font& font);
