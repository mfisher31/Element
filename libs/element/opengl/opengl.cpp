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

void Device::draw (evgDrawMode _mode, uint32_t start, uint32_t nverts)
{
    const auto mode = gl::topology (_mode);
    auto vbuf = active_vertex_buffer;
    auto ibuf = active_index_buffer;
    auto program = active_program;

    if (ibuf == nullptr && vbuf == nullptr)
        goto abort;
    
    if (program == nullptr)
        goto noprog;

    if (auto tex1 = active_texture[0]) {
        std::cerr << "activate texture\n";
        glActiveTexture (GL_TEXTURE0);
        tex1->bind();
    }

    program->load_buffers (vbuf, ibuf);
    glUseProgram (program->object());
    
    if (auto tex1 = active_texture[0]) {
        glUniform1i (glGetUniformLocation (program->object(), "texture1"), 0);
        check_ok ("uniform 1i");
    }
    
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


void Device::_load_program (evgHandle dh, evgHandle ph)
{
    (static_cast<Device*> (dh))->active_program =
        static_cast<Program*> (ph);
}

void Device::_load_texture (evgHandle dh, evgHandle th, int unit)
{
    auto device  = static_cast<Device*> (dh);
    auto texture = static_cast<Texture*> (th);
    auto current = device->active_texture[unit];

    // if (current == texture)
    //     return;

    glActiveTexture (GL_TEXTURE0 + unit);
    if (! gl::check_ok (""))
        return;
    
    device->active_texture[unit] = texture;

    if (texture == nullptr)
        return;
    if (! texture->bind()) {
        std::clog << "could not bind texture\n";
    } else {
        std::clog << "bound texture\n";
    }
     // if (!gl_tex_param_i(tex->gl_target, GL_TEXTURE_SRGB_DECODE_EXT, decode))
	// 	goto fail;
}

} // namespace gl

using namespace gl;

//=============================================================================
struct OpenGL {
    using Descriptor = evg::Descriptor<gl::Device>;
    Descriptor vgdesc;

    OpenGL()
        : vgdesc (Device::create, Device::destroy)
    {
        vgdesc.viewport = Device::_viewport;
        vgdesc.clear = Device::_clear;
        
        vgdesc.ortho = Device::_ortho;
        vgdesc.draw = Device::_draw;
        vgdesc.present = Device::_present;
        vgdesc.flush = Device::_flush;

        vgdesc.clear_context = Device::_clear_context;
        vgdesc.load_program = Device::_load_program;
        vgdesc.load_index_buffer = Device::_load_index_buffer;
        vgdesc.load_shader = Device::_load_shader;
        vgdesc.load_swap = Device::_load_swap;
        vgdesc.load_texture = Device::_load_texture;
        vgdesc.load_vertex_buffer = Device::_load_vertex_buffer;

        vgdesc.texture = Texture::interface();
        vgdesc.buffer = Buffer::interface();
        vgdesc.shader = Shader::interface();
        vgdesc.program = Program::interface();
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
