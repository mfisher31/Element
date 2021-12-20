#pragma once

#include <element/evg/matrix.h>
#include <element/evg/device.hpp>

namespace element {

template<typename Val>
class Rectangle {
public:
    using Type = Val;
    
    Val x, y, width, height;

    Rectangle() = default;
    Rectangle (Val _x, Val _y, Val _w, Val _h)
        : x(_x), y(_y), width(_w), height(_h) {}

    bool empty() const noexcept { return x == 0 && y == 0 && width == 0 && height == 0; }
};

class SpriteBuffer;
class GraphicsContext {
public:
    explicit GraphicsContext (evg::Device&);
    ~GraphicsContext();
    
    evg::Device& get_device() noexcept { return device; }

    //=========================================================================
    void draw_sprite (const evg::Texture& texture);
    void draw_sprite (int width, int height);

    evg::Texture* load_image_data (const uint8_t*, evgColorFormat format, int width, int height);
    evg::Shader*  reserve_vertex_shader();
    evg::Shader*  reserve_fragment_shader();
    evg::Program* reserve_program();

private:
    evg::Device& device;
    std::unique_ptr<SpriteBuffer> sprite_buffer;
};

}
