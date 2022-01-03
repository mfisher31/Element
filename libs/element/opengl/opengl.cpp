#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>

#include <element/graphics.hpp>
#include <element/plugin.h>

#include "helpers.hpp"
#include "opengl.hpp"
#include "program.hpp"

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define glog(msg) std::cout << msg << std::endl

namespace gl {

extern Platform* create_platform();
extern void destroy_platform (Platform* p);

//=============================================================================
Device::Device (PlatformPtr plat)
    : platform (std::move (plat))
{
    state = state_stack.get();
}

Device::~Device()
{
    platform.reset();
}

evgHandle Device::_create()
{
    std::unique_ptr<Device> device;

    if (auto plat = PlatformPtr (gl::create_platform(), gl::destroy_platform))
        device.reset (new Device (std::move (plat)));

    if (device == nullptr || device->platform == nullptr)
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

void Device::_destroy (evgHandle device)
{
    delete static_cast<Device*> (device);
}

bool Device::setup_extensions()
{
    if (! GLAD_GL_VERSION_3_3) {
        glog ("[opengl] error: requires GL version >= 3.3");
        glog ("[opengl] version: v" << (const char*) glGetString (GL_VERSION));
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
        copy_method = gl::CopyMethod::ARB;
    else if (GLAD_GL_NV_copy_image)
        copy_method = gl::CopyMethod::NV;
    else
        copy_method = gl::CopyMethod::FBO_BLIT;

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

void Device::draw (evgDrawMode _mode, uint32_t start, uint32_t nverts)
{
    const auto mode = gl::topology (_mode);
    auto vbuf = state->vbo[0];
    auto ibuf = state->ebo;
    auto program = state->program;

    float width = 500.f;
    float height = 500.f;

    if (ibuf == nullptr && vbuf == nullptr)
        goto abort;

    if (program == nullptr)
        goto noprog;

    if (state->target) {
        state->target->prepare_render();
        if (state->stencil)
            state->stencil->prepare_render();
    } else {
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
    }

    for (int i = 0; i < 8; ++i) {
        glActiveTexture (GL_TEXTURE0 + i);
        if (auto t = state->texture[i])
            t->bind();
        else
            glBindTexture (GL_TEXTURE_2D, 0);
    }

    program->load_buffers (state->vbo, ibuf);
    glUseProgram (program->object());
    program->process_uniforms();

    if (ibuf) {
        glDrawElements (mode, nverts, GL_UNSIGNED_INT, 0);
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
void Device::_enter_context (evgHandle dh)
{
    (static_cast<Device*> (dh))->platform->enter_context();
}

void Device::_leave_context (evgHandle dh)
{
    (static_cast<Device*> (dh))->platform->leave_context();
}

void Device::_clear_context (evgHandle dh)
{
    (static_cast<Device*> (dh))->platform->clear_context();
}

//=============================================================================
void Device::_save_state (evgHandle dh) {
    auto device = static_cast<Device*> (dh);
    device->state = device->state_stack.push();
}

void Device::_restore_state (evgHandle dh) {
    auto device = static_cast<Device*> (dh);
    device->state = device->state_stack.pop();
}

//=============================================================================
void Device::_enable (evgHandle dh, uint32_t enablement, bool enabled)
{
    auto device = static_cast<Device*> (dh);
    switch (enablement) {
        case EVG_FRAMEBUFFER_SRGB:
            enabled ? glEnable (GL_FRAMEBUFFER_SRGB) : glDisable (GL_FRAMEBUFFER_SRGB);
            break;
        case EVG_DEPTH_TEST:
            enabled ? glEnable (GL_DEPTH_TEST) : glDisable (GL_DEPTH_TEST);
            break;
        case EVG_STENCIL_TEST:
            enabled ? glEnable (GL_STENCIL_TEST) : glDisable (GL_STENCIL_TEST);
            break;
    }
}

//==
void Device::_clear (evgHandle device,
                     uint32_t clear_flags,
                     uint32_t color,
                     double depth,
                     int stencil)
{
    struct PixelARGB {
        uint8_t b, g, r, a;
    };

    union ConvertPixel {
        PixelARGB pixel;
        uint32_t packed;
    };

    GLbitfield bits = 0;
    if ((clear_flags & EVG_CLEAR_COLOR) != 0) {
        ConvertPixel C;
        C.packed = color;
        float r = C.pixel.r / 255.f;
        float g = C.pixel.g / 255.f;
        float b = C.pixel.b / 255.f;
        float a = C.pixel.a / 255.f;
        glClearColor (r, g, b, a);
        bits |= GL_COLOR_BUFFER_BIT;
    }

    if ((clear_flags & EVG_CLEAR_DEPTH) != 0) {
        glClearDepth (depth);
        bits |= GL_DEPTH_BUFFER_BIT;
    }

    if ((clear_flags & EVG_CLEAR_STENCIL) != 0) {
        glClearStencil ((GLint) stencil);
        bits |= GL_STENCIL_BUFFER_BIT;
    }

    glClear (bits);
}

//=============================================================================
void Device::_present (evgHandle dh)
{
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

void Device::_load_program (evgHandle dh, evgHandle ph)
{
    (static_cast<Device*> (dh))->state->program =
        static_cast<Program*> (ph);
}

void Device::_load_swap (evgHandle dh, evgHandle sh)
{
    (static_cast<Device*> (dh))->platform->load_swap (static_cast<const Swap*> (sh));
}

void Device::_load_texture (evgHandle dh, evgHandle th, int slot)
{
    (static_cast<Device*> (dh))->state->texture[slot] =
        static_cast<Texture*> (th);
}

void Device::_load_stencil (evgHandle dh, evgHandle sh)
{
    (static_cast<Device*> (dh))->state->stencil =
        static_cast<Stencil*> (sh);
}

void Device::_load_target (evgHandle dh, evgHandle th)
{
    (static_cast<Device*> (dh))->state->target =
        static_cast<Texture*> (th);
}

void Device::_load_vertex_buffer (evgHandle dh, evgHandle vbh, int slot)
{
    (static_cast<Device*> (dh))->state->vbo[slot] =
        static_cast<Buffer*> (vbh);
}

const evgDescriptor* Device::descriptor()
{
    static const evgDescriptor D = {
        .create = _create,
        .destroy = _destroy,
        .enter_context = _enter_context,
        .leave_context = _leave_context,
        .clear_context = _clear_context,
        .save_state = _save_state,
        .restore_state = _restore_state,
        .enable = _enable,
        .viewport = _viewport,
        .clear = _clear,
        .draw = _draw,
        .present = _present,
        .flush = _flush,
        .load_vertex_buffer = _load_vertex_buffer,
        .load_index_buffer = _load_index_buffer,
        .load_program = _load_program,
        .load_texture = _load_texture,
        .load_target = _load_target,
        .load_stencil = _load_stencil,
        .load_swap = _load_swap,
        .buffer = Buffer::interface(),
        .shader = Shader::interface(),
        .program = Program::interface(),
        .texture = Texture::interface(),
        .stencil = Stencil::interface(),
        .swap = Swap::interface()
    };

    return &D;
}

} // namespace gl

using gl::Device;

//=============================================================================
struct OpenGL {
    OpenGL() {}
    static elHandle create()
    {
        auto m = new OpenGL();
        return m;
    }

    static void destroy (elHandle handle)
    {
        delete (OpenGL*) handle;
    }

    static const void* extension (elHandle handle, const char* ID)
    {
        gl::unused (handle);
        if (strcmp (ID, "el.GraphicsDevice") == 0)
            return (const void*) Device::descriptor();
        return nullptr;
    }
};

const elDescriptor* element_descriptor()
{
    static const elDescriptor D = {
        .ID = "el.OpenGL",
        .create = OpenGL::create,
        .extension = OpenGL::extension,
        .load = nullptr,
        .unload = nullptr,
        .destroy = OpenGL::destroy,
    };
    return &D;
}
