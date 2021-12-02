
#include <cassert>
#include <iostream>
#include "graphics_context.hpp"
#include "graphics_device.hpp"

namespace element {

GraphicsContext::GraphicsContext (GraphicsDevice& gd)
    : device (gd)
{
}

GraphicsContext::~GraphicsContext()
{
}

void GraphicsContext::draw_sprite (const egTexture* tex, int width, int height)
{
    
}

egTexture* GraphicsContext::load_image_data (const uint8_t* data,
                                             egColorFormat format,
                                             int width, int height)
{
    assert (data != nullptr);
    return device.create_2d_texture (format,
                                     static_cast<uint32_t> (width),
                                     static_cast<uint32_t> (height),
                                     (const uint8_t**) &data);
}

} // namespace element
