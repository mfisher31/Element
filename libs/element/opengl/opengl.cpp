#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>

#include <element/graphics.h>
#include <element/plugin.h>

#include "helpers.hpp"
#include "opengl.hpp"
#include "x11_glx.hpp"

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define glog(msg) std::clog << msg << std::endl

namespace gl {
//=============================================================================

Device::Device (PlatformPtr plat)
    : platform (std::move (plat))
{
    evg_mat4_reset (&active_projection);
}

evgHandle Device::create()
{
    std::unique_ptr<Device> device;

    if (auto plat = PlatformPtr (gl::glx_platform_new(), gl::glx_platform_delete))
        device.reset (new Device (std::move (plat)));

    if (device == nullptr || ! device->have_platform())
        return nullptr;

    if (! device->setup_extensions()) {
        return nullptr;
    }

    glog ("[opengl] device: " << (const char*) glGetString (GL_VENDOR));
    glog ("[opengl] renderer: " << (const char*) glGetString (GL_RENDERER));
    glog ("[opengl] version: v" << (const char*) glGetString (GL_VERSION));
    glog ("[opengl] shader: v" << (const char*) glGetString (GL_SHADING_LANGUAGE_VERSION));

    return device.release();
}

void Device::destroy (evgHandle device)
{
    delete static_cast<Device*> (device);
}

bool Device::setup_extensions()
{
    if (! GLAD_GL_VERSION_3_3) {
        glog ("[opengl] error: requires GL version >= 3.3");
        return false;
    }

    // gl_enable_debug();

    if (! GLAD_GL_EXT_texture_sRGB_decode) {
        glog ("[opengl] error: sRGB not supported");
        return false;
    }

    glEnable (GL_TEXTURE_CUBE_MAP_SEAMLESS);
    auto code = glGetError();
    if (code != GL_NO_ERROR) {
        glog ("[opengl] error: " << gl::error_string (code));
        return false;
    }

    if (GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_copy_image)
        m_copy_method = gl::CopyMethod::ARB;
    else if (GLAD_GL_NV_copy_image)
        m_copy_method = gl::CopyMethod::NV;
    else
        m_copy_method = gl::CopyMethod::FBO_BLIT;

    return true;
}

void Device::viewport (int x, int y, int width, int height)
{
    int baseheight = 0;
    int flip_y = y;

    if (baseheight > 0)
        flip_y = baseheight - y - height;

    glViewport (x, flip_y, width, height);
}

void Device::ortho (float left, float right, float top, float bottom, float near, float far)
{
#if 0
    evgMatrix4 *dst = &active_projection;
	evg_mat4_reset (dst);

	float rml = right - left;
	float bmt = bottom - top;
	float fmn = far - near;

	dst->x.x = 2.0f / rml;
	dst->t.x = (left + right) / -rml;

	dst->y.y = 2.0f / -bmt;
	dst->t.y = (bottom + top) / bmt;

	dst->z.z = -2.0f / fmn;
	dst->t.z = (far + near) / -fmn;

	dst->t.w = 1.0f;
#else
    glOrtho (left, right, bottom, top, near, far);
#endif
}

void Device::draw (egDrawMode _mode, uint32_t start, uint32_t nverts)
{
    glClearColor (0.0, 0.0, 0.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);

    const auto mode = gl::topology (_mode);
    auto vbuf = active_vertex_buffer;
    auto ibuf = active_index_buffer;
    auto program = active_program;

    if (ibuf == nullptr && vbuf == nullptr)
        goto abort;
    
    if (program == nullptr)
        goto noprog;

    glUseProgram (program->object());
    program->load_buffers (vbuf, ibuf);
    
    if (ibuf) {
        glDrawElements (mode, nverts, ibuf->element_type(), ibuf->draw_offset (start));
    } else {
        glDrawArrays (mode, start, nverts);
    }

    goto done;
noprog:
    std::cerr << "[opengl] draw: program not loaded\n";
abort:
    std::cerr << "[opengl] draw failed.\n";
done:
    return;
}

//=============================================================================
void Device::_present (evgHandle dh) {
    auto device = static_cast<Device*> (dh);
    device->platform->swap_buffers();
}

void Device::_flush (evgHandle dh)
{
#ifdef __APPLE__
    if ((static_cast<Device*> (dh))->active_swap == nullptr)
        glFlush();
#else
    gl::unused (dh);
    glFlush();
#endif
}

} // namespace gl

using namespace gl;

//=============================================================================
static evgHandle gl_texture_create (evgHandle dh, const evgTextureSetup* setup, const uint8_t** data)
{
    if (nullptr == dh)
        return nullptr;

    auto device = static_cast<Device*> (dh);
    std::unique_ptr<Texture> tex;

    switch (setup->type) {
        case EVG_TEXTURE_2D:
            tex = std::make_unique<Texture2D> (*device, *setup);
            break;
        case EVG_TEXTURE_3D:
            // tex = std::make_unique<evgTexture3D> (*device, *setup);
            break;
        case EVG_TEXTURE_CUBE:
            // tex = std::make_unique<evgTextureCube> (*device, *setup);
            break;
    }

    if (tex == nullptr)
        return nullptr;

    tex->upload (data);
    return tex->has_uploaded() ? tex.release() : nullptr;
}

static void gl_texture_destroy (evgHandle t)
{
    std::unique_ptr<Texture> tex ((Texture*) t);
    if (tex == nullptr)
        return;

    switch (tex->type()) {
        case EVG_TEXTURE_2D:
            break;
        case EVG_TEXTURE_3D:
            break;
        case EVG_TEXTURE_CUBE:
            break;
    }

    tex.reset();
}

static void gl_texture_fill_setup (evgHandle tex, evgTextureSetup* setup)
{
    if (tex != nullptr && setup != nullptr)
        ((Texture*) tex)->fill_setup (setup);
}

//=============================================================================
static evgHandle gl_vertex_buffer_create (evgHandle device,
                                          evgVertexData* data,
                                          uint32_t flags)
{
    auto vbuf = std::make_unique<VertexBuffer> (data, flags);
    if (! vbuf->create_buffers()) {
        glog ("[opengl] failed to create vertex buffer");
        return nullptr;
    }
    return vbuf.release();
    return nullptr;
}

static void gl_vertex_buffer_destroy (evgHandle v)
{
    if (auto vbuf = static_cast<VertexBuffer*> (v)) {
        vbuf->destroy_buffers();
        delete vbuf;
    }
}

static evgVertexData* gl_vertex_buffer_data (evgHandle v)
{
    auto vbuf = (VertexBuffer*) v;
    return vbuf != nullptr ? vbuf->get_data() : nullptr;
}

static void gl_vertex_buffer_flush (evgHandle v)
{
    auto vbuf = (VertexBuffer*) v;
    vbuf != nullptr ? vbuf->flush() : void();
}

//=============================================================================
static evgHandle gl_shader_create (evgHandle dh, evgShaderType type)
{
    auto device = static_cast<Device*> (dh);
    return new Shader (*device, type);
}

static bool gl_shader_parse (evgHandle sh, const char* text)
{
    return (static_cast<Shader*> (sh))->parse (text);
}

static void gl_shader_destroy (evgHandle sh)
{
    delete static_cast<Shader*> (sh);
}

//=============================================================================

struct OpenGL {
    using Descriptor = evg::Descriptor<gl::Device>;
    Descriptor vgdesc;

    OpenGL()
        : vgdesc (Device::create, Device::destroy)
    {
        vgdesc.viewport = Device::_viewport;
        vgdesc.ortho = Device::_ortho;
        vgdesc.draw = Device::_draw;
        vgdesc.present = Device::_present;

        vgdesc.clear_context = Device::_clear_context;
        vgdesc.load_program = Device::_load_program;
        vgdesc.load_index_buffer = Device::_load_index_buffer;
        vgdesc.load_shader = Device::_load_shader;
        vgdesc.load_swap = Device::_load_swap;
        vgdesc.load_texture = Device::_load_texture;
        vgdesc.load_vertex_buffer = Device::_load_vertex_buffer;

        vgdesc.texture_create = gl_texture_create;
        vgdesc.texture_destroy = gl_texture_destroy;
        vgdesc.texture_fill_setup = gl_texture_fill_setup;
        vgdesc.texture_map = nullptr;

        vgdesc.vertex_buffer_create = gl_vertex_buffer_create;
        vgdesc.vertex_buffer_destroy = gl_vertex_buffer_destroy;
        vgdesc.vertex_buffer_data = gl_vertex_buffer_data;
        vgdesc.vertex_buffer_flush = gl_vertex_buffer_flush;

        vgdesc.shader_create = gl_shader_create;
        vgdesc.shader_destroy = gl_shader_destroy;
        vgdesc.shader_parse = gl_shader_parse;
        vgdesc.shader_add_attribute = Shader::add_attribute;
        vgdesc.shader_add_uniform = Shader::add_uniform;

        vgdesc.program = Program::interface();
        vgdesc.index_buffer = IndexBuffer::interface();
        vgdesc.swap = SwapChain::interface();
    }
};

elHandle gl_create()
{
    auto m = new OpenGL();
    return m;
}

void gl_destroy (elHandle handle)
{
    delete (OpenGL*) handle;
}

const void* gl_extension (elHandle handle, const char* ID)
{
    auto gl = (OpenGL*) handle;
    if (strcmp (ID, "el.GraphicsDevice") == 0) {
        return &gl->vgdesc;
    }
    std::clog << "[opengl] check: " << ID << std::endl;
    return nullptr;
}

const elDescriptor* element_descriptor()
{
    static const elDescriptor D = {
        .ID = "el.OpenGL",
        .create = gl_create,
        .extension = gl_extension,
        .load = nullptr,
        .unload = nullptr,
        .destroy = gl_destroy,
    };
    return &D;
}
