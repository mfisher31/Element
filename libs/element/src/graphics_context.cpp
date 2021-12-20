
#include <cassert>
#include <iostream>

#include "graphics_context.hpp"

namespace element {

class SpriteBuffer {
public:
    void prepare (evg::Device& dev)
    {
        if (p_points == nullptr)
            p_points.reset (dev.create_vertex_buffer (sizeof (evgVec3) * 4, EVG_OPT_DYNAMIC));
        if (p_uv == nullptr)
            p_uv.reset (dev.create_vertex_buffer (sizeof (evgVec2) * 4, EVG_OPT_DYNAMIC));
    }

    void load (evg::Device& dev)
    {
        if (points_changed)
            p_points->flush();
        if (uv_changed)
            p_uv->flush();
        dev.load_vertex_buffer (p_points.get(), 0);
        dev.load_vertex_buffer (p_uv.get(), 1);
        dev.load_index_buffer (nullptr);
        uv_changed = points_changed = false;
    }

    inline void resize (uint32_t width, uint32_t height)
    {
        if (m_width != width || m_height != height) {
            m_width = width;
            m_height = height;
            assign_rectangle ((evgVec3*) p_points->data(),
                              static_cast<float> (m_width),
                              static_cast<float> (m_height));
            assign_rectangle ((evgVec2*) p_uv->data(), 0.0, 1.0, 0.0, 1.0);
            points_changed = uv_changed = true;
        }
    }

private:
    uint32_t m_width, m_height;
    std::unique_ptr<evg::Buffer> p_points;
    std::unique_ptr<evg::Buffer> p_uv;
    bool points_changed = false, uv_changed = false;

    void assign_rectangle (evgVec3* points, float w, float h)
    {
        evg_vec3_reset (points);
        evg_vec3_set (points + 1, w, 0.0, 0.0f);
        evg_vec3_set (points + 2, 0.0, h, 0.0f);
        evg_vec3_set (points + 3, w, h, 0.0f);
    }

    void assign_rectangle (evgVec2* tc, float u0, float u1, float v0, float v1)
    {
        evg_vec2_set (tc + 0, u0, v0);
        evg_vec2_set (tc + 1, u1, v0);
        evg_vec2_set (tc + 2, u0, v1);
        evg_vec2_set (tc + 3, u1, v1);
    }
};

GraphicsContext::GraphicsContext (evg::Device& dev)
    : device (dev)
{
    dev.enter_context();
    sprite_buffer = std::make_unique<SpriteBuffer>();
    sprite_buffer->prepare (dev);
    dev.leave_context();
}

GraphicsContext::~GraphicsContext()
{
    sprite_buffer.reset();
}

void GraphicsContext::draw_sprite (int width, int height)
{
    sprite_buffer->resize (static_cast<uint32_t> (width),
                           static_cast<uint32_t> (height));
    sprite_buffer->load (device);
    device.draw (EVG_TRIANGLE_STRIP, 0, 4);
}

void GraphicsContext::draw_sprite (const evg::Texture& texture)
{
    sprite_buffer->resize (texture.width(), texture.height());
    sprite_buffer->load (device);
    device.draw (EVG_TRIANGLE_STRIP, 0, 4);
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
