
#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

#include <element/graphics.h>
#include <glad/gl.h>

namespace gl {

enum class CopyMethod {
    ARB,
    NV,
    FBO_BLIT
};

using SwapChain = egSwapChain;

struct WindowSetup {
    WindowSetup() = default;
    virtual ~WindowSetup() = default;
};

class Platform {
public:
    virtual ~Platform() = default;
    virtual bool initialize() = 0;

    //=========================================================================
    virtual std::unique_ptr<WindowSetup> create_window_setup() const = 0;
    virtual bool attach_swap_chain (egSwapChain* swap) = 0;
    virtual void load_swap_chain (const egSwapChain* swap) = 0;
    virtual void detach_swap_chain (egSwapChain* swap) = 0;

    //=========================================================================
    virtual void* context_handle() const noexcept = 0;
    virtual void enter_context() = 0;
    virtual void leave_context() = 0;
    virtual void clear_context() = 0;

    //==========================================================================
    virtual void swap_buffers() = 0;

protected:
    Platform() = default;

private:
    EL_DISABLE_COPY (Platform);
};

} // namespace gl

struct egSwapChain {
    egDevice* device { nullptr };
    egSwapSetup setup { 0 };
    std::unique_ptr<gl::WindowSetup> window;
};

struct egTexture {
    egTexture() = delete;
    virtual ~egTexture() = default;

    inline void upload (const uint8_t** data)
    {
        if (uploaded)
            return;
        assert (data != nullptr);
        uploaded = upload_data (data);
    }

    inline void fill_setup (egTextureSetup* setup) const noexcept
    {
        if (setup != nullptr)
            memcpy (setup, &_setup, sizeof (egTextureSetup));
    }
    inline egTextureType type() const noexcept { return _setup.type; }
    inline egColorFormat format() const noexcept { return _setup.format; }

    // inline uint32_t levels() const noexcept { return _setup.levels; }
    inline uint32_t width() const noexcept { return _setup.width; }
    inline uint32_t height() const noexcept { return _setup.height; }
    inline uint32_t depth() const noexcept { return _setup.depth; }
    inline uint32_t size() const noexcept { return _setup.size; }

    inline bool has_uploaded() const noexcept { return uploaded; }
    inline bool is_a (egTextureType type) const noexcept { return _setup.type == type; }
    inline bool is_dummy() const noexcept { return dummy; }
    inline bool is_dynamic() const noexcept { return dynamic; }
    inline bool is_render_target() const noexcept { return render_target; }
    inline bool use_mipmaps() const noexcept { return mipmaps; }

protected:
    explicit egTexture (egDevice& dev, const egTextureSetup& setup);
    virtual bool upload_data (const uint8_t** data) = 0;

    egDevice& device;
    egTextureSetup _setup;

    GLuint texture { 0 };
    GLenum gl_levels { 0 };
    GLenum gl_format { 0 };
    GLenum gl_format_internal { 0 };
    GLenum gl_format_type { 0 };
    GLenum gl_target { 0 };

    bool dynamic { false },
        render_target { false },
        dummy { false },
        mipmaps { false };

    void* active_sampler { nullptr };
    void* fbo { nullptr };

    bool uploaded = false;
    EL_DISABLE_COPY (egTexture);
};

struct egTexture2D : egTexture {
    explicit egTexture2D (egDevice& device, const egTextureSetup& setup)
        : egTexture (device, setup) {}

protected:
    bool upload_data (const uint8_t** data) override;

private:
    bool bind_data (const uint8_t** data);
    egTexture2D() = delete;
    EL_DISABLE_COPY (egTexture2D);

};
