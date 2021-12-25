
#include "element/evg/context.hpp"
#include "element/evg/device.hpp"
#include <cassert>

namespace evg {

class DefaultProgram {
public:
    DefaultProgram() = default;

    operator Program*() const noexcept { return program.get(); }

    void set_projection (const evgMatrix4& mat4) noexcept
    {
        program->set_value (proj_key, sizeof (evgMatrix4), &mat4);
    }

    bool compile (Device& d)
    {
        if (program_linked)
            return true;

        {
            std::unique_ptr<Shader> vshader, fshader;
            vshader.reset (d.create_shader (EVG_SHADER_VERTEX));
            vshader->add_attribute ("pos", EVG_VALUE_VEC3);
            vshader->add_attribute ("uv", EVG_VALUE_VEC2);
            vshader->add_uniform ("projection", EVG_VALUE_MAT4X4);
            vshader->add_uniform ("scale", EVG_VALUE_FLOAT);
            vshader->add_uniform ("tx", EVG_VALUE_FLOAT);
            vshader->add_uniform ("ty", EVG_VALUE_FLOAT);

            fshader.reset (d.create_shader (EVG_SHADER_FRAGMENT));
            fshader->add_texture ("image");

            program.reset (d.create_program());

            if (vshader && vshader->parse (vsource))
                if (fshader && fshader->parse (fsource))
                    program_linked = program->link (vshader.get(), fshader.get());
        }

        if (! program_linked) {
            program.reset();
        } else {
            image_key = program->map_symbol ("image");
            proj_key = program->map_symbol ("projection");
        }

        return program_linked;
    }

private:
    friend class Context;
    bool program_linked = false;
    std::unique_ptr<Program> program;
    int image_key = -1;
    int proj_key = -1;

    static constexpr const char* vsource = R"(
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

out VertInOut {
    vec2 uv;
} vout;

uniform mat4 projection;
uniform float scale = 1.0;
uniform float tx = 0.0;
uniform float ty = 0.0;

void main()
{
    mat4 tr = mat4 (scale, 0.0, 0.0, tx,
                    0.0, scale, 0.0, ty,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0);
    gl_Position = projection * vec4(vec4(pos, 1.0) * tr);
    vout.uv = uv;
}
)";

    static constexpr const char* fsource = R"(
#version 330 core

out vec4 FragColor;
in VertInOut {
    vec2 uv;
} vin;

uniform sampler2D image;

void main()
{
	FragColor = texture(image, vin.uv);
}
)";
};

class SpriteBuffer {
public:
    void prepare (Device& dev)
    {
        if (p_points == nullptr)
            p_points.reset (dev.create_vertex_buffer (sizeof (evgVec3) * 4, EVG_DYNAMIC));
        if (p_uv == nullptr)
            p_uv.reset (dev.create_vertex_buffer (sizeof (evgVec2) * 4, EVG_DYNAMIC));
    }

    void load (Device& dev)
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

    void resize (uint32_t width, uint32_t height)
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
    uint8_t m_flip = 0;
    std::unique_ptr<Buffer> p_points;
    std::unique_ptr<Buffer> p_uv;
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
        tc[0].x = u0;
        tc[0].y = v0;
        tc[1].x = u1;
        tc[1].y = v0;
        tc[2].x = u0;
        tc[2].y = v1;
        tc[3].x = u1;
        tc[3].y = v1;
    }
};

Context::Context (Device& dev)
    : device (dev)
{
    dev.enter_context();
    sprite_buffer = std::make_unique<SpriteBuffer>();
    sprite_buffer->prepare (dev);

    program.reset (new DefaultProgram());
    if (! program->compile (dev))
        throw "compile error";

    dev.leave_context();
}

Context::~Context()
{
    sprite_buffer.reset();
}

void Context::save_state()
{
    save_if_pending();
    save_pending = true;
}

void Context::restore_state()
{
    if (save_pending)
        save_pending = false;
    else
        device.restore_state();
}

void Context::ortho (float left, float right, float bottom, float top, float near, float far)
{
    evg_mat4_ortho (&projection, left, right, bottom, top, near, far);
}

void Context::save_if_pending()
{
    if (save_pending) {
        save_pending = false;
        device.save_state();
    }
}

Program& Context::default_program() const noexcept
{
    return **program;
}

void Context::draw_sprite (int width, int height)
{
    save_if_pending();
    program->set_projection (projection);
    device.load_program (*program);
    sprite_buffer->resize (static_cast<uint32_t> (width),
                           static_cast<uint32_t> (height));
    sprite_buffer->load (device);
    device.draw (EVG_TRIANGLE_STRIP, 0, 4);
}

void Context::draw_sprite (const Texture& texture)
{
    save_if_pending();
    program->set_projection (projection);
    device.load_program (*program);
    device.load_texture (const_cast<Texture*> (&texture), 0);
    sprite_buffer->resize (texture.width(), texture.height());
    sprite_buffer->load (device);
    device.draw (EVG_TRIANGLE_STRIP, 0, 4);
}

Texture* Context::load_image_data (const uint8_t* data,
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

Shader* Context::reserve_vertex_shader()
{
    return device.create_shader (EVG_SHADER_VERTEX);
}

Shader* Context::reserve_fragment_shader()
{
    return device.create_shader (EVG_SHADER_FRAGMENT);
}

Program* Context::reserve_program()
{
    return device.create_program();
}

} // namespace evg
