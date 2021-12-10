#pragma once

#include <element/graphics.h>
#include <element/evg/device.hpp>

namespace element {

class GraphicsContext {
public:
    explicit GraphicsContext (evg::Device&);
    ~GraphicsContext();
    evg::Device& get_device() noexcept { return device; }
    
    void draw_sprite (const evg::Texture* tex, int width, int height);
    evg::Texture* load_image_data (const uint8_t*, egColorFormat format, int width, int height);
    evg::Shader*  reserve_vertex_shader();
    evg::Shader*  reserve_fragment_shader();
    evg::Program* reserve_program();

private:
    evg::Device& device;
    // std::unique_ptr<evg::VertexBuffer> sprite_buffer;
};

}
