
#include <cassert>
#include <iostream>

#include "graphics_context.hpp"

namespace element {

GraphicsContext::GraphicsContext (evg::Device& gd)
    : device (gd)
{
    // if (auto data = evg_vertex_data_new (4, false, false, false, 1, 2))
    //     sprite_buffer.reset (device.create_vertex_buffer (data, EVG_OPT_DYNAMIC));
}

GraphicsContext::~GraphicsContext()
{
    // sprite_buffer.reset();
}

void GraphicsContext::draw_sprite (const evg::Texture* tex, int width, int height)
{
}

evg::Texture* GraphicsContext::load_image_data (const uint8_t* data,
                                                egColorFormat format,
                                                int width, int height)
{
    assert (data != nullptr);
    return device.create_2d_texture (format, static_cast<uint32_t> (width), static_cast<uint32_t> (height), (const uint8_t**) &data);
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
