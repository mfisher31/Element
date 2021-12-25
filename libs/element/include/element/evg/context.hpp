
#pragma once

#include "element/evg/device.hpp"
#include "element/evg/matrix.h"

namespace evg {

class DefaultProgram;
class SpriteBuffer;

class Context {
public:
    explicit Context (Device&);
    ~Context();
    
    Device& get_device() noexcept { return device; }
    
    //=========================================================================
    void save_state();
    void restore_state();

    //=========================================================================
    void ortho (float left, float right, float bottom, float top, float near, float far);

    //=========================================================================
    void draw_sprite (const Texture& texture);
    void draw_sprite (int width, int height);

    Program& default_program() const noexcept;
    Texture* load_image_data (const uint8_t*, evgColorFormat format, int width, int height);
    Shader*  reserve_vertex_shader();
    Shader*  reserve_fragment_shader();
    Program* reserve_program();

private:
    Device& device;
    std::unique_ptr<SpriteBuffer> sprite_buffer;
    std::unique_ptr<DefaultProgram> program;
    evgMatrix4 projection;
    bool save_pending = false;
    void save_if_pending();
};
}
