
#include <cassert>
#include <iostream>

#include "graphics_context.hpp"

namespace element {

GraphicsContext::GraphicsContext (evg::Device& gd)
    : device (gd)
{
	uint32_t bufsize = sizeof(evgVec3) * 4;
	bufsize += sizeof (evgVec2) * 4;
    sprite_buffer.reset (device.create_vertex_buffer (bufsize, EVG_OPT_DYNAMIC));
}

GraphicsContext::~GraphicsContext()
{
    sprite_buffer.reset();
}

static inline void sprite_uv_limits (float *start, float *end, float size, bool flip)
{
	if (! flip) {
		*start = 0.0f;
		*end = size;
	} else {
		*start = size;
		*end = 0.0f;
	}
}

#include <element/evg/vector.h>

static void build_sprite (evg::Buffer& buffer, float width, float height,
			 float start_u, float end_u, float start_v, float end_v)
{
	evgVec2 *tvarray = 0; //(evgVec2*) data->texdata[0].array;
    auto points = (evgVec3*) buffer.data();
	evg_vec3_reset (points);
	evg_vec3_set (points + 1, width, 0.0f, 0.0f);
	evg_vec3_set (points + 2, 0.0f, height, 0.0f);
	evg_vec3_set (points + 3, width, height, 0.0f);

	evg_vec2_set (tvarray, start_u, start_v);
	evg_vec2_set (tvarray + 1, end_u, start_v);
	evg_vec2_set (tvarray + 2, start_u, end_v);
	evg_vec2_set (tvarray + 3, end_u, end_v);
}

static inline void build_sprite_norm (evg::Buffer& buffer, float width, float height, uint32_t flip)
{
	float u1, u2;
	float v1, v2;
	sprite_uv_limits (&u1, &u2, 1.f, (flip & EVG_FLIP_U) != 0);
	sprite_uv_limits (&v1, &v1, 1.f, (flip & EVG_FLIP_V) != 0);
	build_sprite (buffer, width, height, u1, u2, v1, v2);
}

static inline void build_subsprite_norm (evg::Buffer& buffer, float fsub_x,
					float fsub_y, float fsub_cx,
					float fsub_cy, float width, float height,
					uint32_t flip)
{
	float start_u, end_u;
	float start_v, end_v;

	if ((flip & EVG_FLIP_U) == 0) {
		start_u = fsub_x / width;
		end_u = (fsub_x + fsub_cx) / width;
	} else {
		start_u = (fsub_x + fsub_cx) / width;
		end_u = fsub_x / width;
	}

	if ((flip & EVG_FLIP_V) == 0) {
		start_v = fsub_y / height;
		end_v = (fsub_y + fsub_cy) / height;
	} else {
		start_v = (fsub_y + fsub_cy) / height;
		end_v = fsub_y / height;
	}

	build_sprite (buffer, fsub_cx, fsub_cy, start_u, end_u, start_v, end_v);
}

static inline void build_sprite_rect (evg::Buffer& buffer, evg::Texture *tex,
				                      float width, float height, uint32_t flip)
{
	float u1, u2, v1, v2;
	sprite_uv_limits (&u1, &u2, (float) tex->width(), (flip & EVG_FLIP_U) != 0);
	sprite_uv_limits (&v1, &v2, (float) tex->height(), (flip & EVG_FLIP_V) != 0);
	build_sprite (buffer, width, height, u1, u2, v1, v2);
}

void GraphicsContext::draw_texture (const evg::Texture& texture)
{
#if 0
	float width, height;
	struct evgVertexData *data;

	data = gs_vertexbuffer_get_data(graphics->sprite_buffer);
	if (tex && gs_texture_is_rect(tex))
		build_sprite_rect(data, tex, width, height, flip);
	else
		build_sprite_norm(data, width, height, flip);
#endif
    
    build_sprite_norm (*sprite_buffer, (float)texture.width(), (float)texture.height(), 0);
    
    sprite_buffer->flush();
	device.load_vertex_buffer (sprite_buffer.get());
	device.load_index_buffer (nullptr);
	device.draw (EVG_TRIANGLE_STRIP, 0, 0);
}

evg::Texture* GraphicsContext::load_image_data (const uint8_t* data,
                                                evgColorFormat format,
                                                int width, int height)
{
    assert (data != nullptr);
    auto tex = device.create_2d_texture (format,
        static_cast<uint32_t> (width),
        static_cast<uint32_t> (height));
    tex->update (data);
    return tex;
}

evg::Shader* GraphicsContext::reserve_vertex_shader()
{
    return device.create_shader (EVG_SHADER_VERTEX);
}

evg::Shader* GraphicsContext::reserve_fragment_shader()
{
    return device.create_shader (EVG_SHADER_FRAGMENT);
}

evg::Program* GraphicsContext::reserve_program()
{
    return device.create_program();
}

} // namespace element
