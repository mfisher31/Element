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

//=============================================================================

using PlatformPtr = std::unique_ptr<gl::Platform, void (*) (gl::Platform*)>;

struct egDevice final {
    explicit egDevice (PlatformPtr plat)
        : platform (std::move (plat))
    {
    }

    ~egDevice()
    {
        platform.reset();
    }

    bool have_platform() const { return platform != nullptr; }

    bool setup_extensions()
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
            copy_method = gl::CopyMethod::ARB;
        else if (GLAD_GL_NV_copy_image)
            copy_method = gl::CopyMethod::NV;
        else
            copy_method = gl::CopyMethod::FBO_BLIT;

        return true;
    }

    PlatformPtr platform;
    gl::CopyMethod copy_method;
};

static egDevice* gl_create_device()
{
    std::unique_ptr<egDevice> device;

    if (auto plat = PlatformPtr (gl::glx_platform_new(), gl::glx_platform_delete))
        device.reset (new egDevice (std::move (plat)));

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

static void gl_destroy_device (egDevice* device)
{
    delete device;
}

//=============================================================================
static void gl_enter_context (egDevice* device)
{
    device->platform->enter_context();
}

static void gl_leave_context (egDevice* device)
{
    device->platform->leave_context();
}

//=============================================================================
static egSwapChain* gl_swap_chain_new (egDevice* device, const egSwapSetup* setup)
{
    if (device == nullptr || setup == nullptr)
        return nullptr;

    if (auto swap = std::make_unique<egSwapChain>()) {
        auto& platform = *device->platform;
        swap->device = device;
        memcpy (&swap->setup, setup, sizeof (egSwapSetup));
        swap->window = platform.create_window_setup();
        if (swap->window != nullptr && platform.attach_swap_chain (swap.get()))
            return swap.release();
    }

    return nullptr;
}

static void gl_swap_chain_load (egDevice* d, const egSwapChain* s)
{
    if (d && d->platform)
        d->platform->load_swap_chain (s);
}

static void gl_swap_chain_free (egSwapChain* swap)
{
    if (nullptr == swap)
        return;
    if (swap->device && swap->device->have_platform())
        swap->device->platform->detach_swap_chain (swap);
    if (swap->window)
        swap->window.reset();
    delete swap;
}

//=============================================================================
static egTexture* gl_texture_create(egDevice* device, const egTextureSetup* setup, const uint8_t** data)
{
    if (nullptr == device)
        return nullptr;
    
    std::unique_ptr<egTexture> tex;

    switch (setup->type) {
        case EL_TEXTURE_2D:
            tex = std::make_unique<egTexture2D> (*device, *setup);
            break;
        case EL_TEXTURE_3D:
            // tex = std::make_unique<egTexture3D> (*device, *setup);
            break;
        case EL_TEXTURE_CUBE:
            // tex = std::make_unique<egTextureCube> (*device, *setup);
            break;
    }
    
    if (tex == nullptr)
        return nullptr;

    tex->upload (data);
    return tex->has_uploaded() ? tex.release() : nullptr;
}

static void gl_texture_destroy(egTexture* t)
{
    std::unique_ptr<egTexture> tex (t);
    if (tex == nullptr)
        return;

    switch (tex->type()) {
        case EL_TEXTURE_2D:
            break;
        case EL_TEXTURE_3D:
            break;
        case EL_TEXTURE_CUBE:
            break;
    }

    tex.reset();
}

static void gl_texture_fill_setup (egTexture* tex, egTextureSetup* setup)
{
    if (tex != nullptr && setup != nullptr)
        tex->fill_setup (setup);
}

static void gl_texture_load (egDevice* device, egTexture* tex, int unit)
{
    std::clog << "gl_texture_load\n";
    gl::unused (device, tex, unit);
}

//=============================================================================
static void gl_draw (egDevice* device, egDrawMode mode, uint32_t start, uint32_t nverts)
{
    std::clog << "gl_draw\n";
    gl::unused (device, mode, start, nverts);
}

//=============================================================================
struct OpenGL {
    egDeviceDescriptor vg;

    OpenGL()
    {
        vg = {
            .create = gl_create_device,
            .destroy = gl_destroy_device,

            .enter_context = gl_enter_context,
            .leave_context = gl_leave_context,

            .swap_create = gl_swap_chain_new,
            .swap_destroy = gl_swap_chain_free,
            .swap_load = gl_swap_chain_load,
            
            .texture_create = gl_texture_create,
            .texture_destroy = gl_texture_destroy,
            .texture_fill_setup = gl_texture_fill_setup
        };
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
        return &gl->vg;
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
