
#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include <glad/gl.h>

#include <element/element.h>
#include <element/evg/device.hpp>
#include <element/evg/matrix.h>

namespace gl {

enum class CopyMethod {
    ARB,
    NV,
    FBO_BLIT
};

using TextureType = evgTextureType;
using TextureSetup = evgTextureInfo;
using SwapSetup = evgSwapSetup;

class Program;
class Swap;
class Swap;
class Buffer;
class Texture;
class Shader;
class Stencil;

class Platform {
public:
    virtual ~Platform() = default;
    virtual bool initialize() = 0;

    //=========================================================================
    virtual Swap* create_swap (const evgSwapSetup* setup) = 0;
    virtual void load_swap (const Swap* swap) = 0;
    virtual void swap_buffers() = 0;

    //=========================================================================
    virtual void* context_handle() const noexcept = 0;
    virtual void enter_context() = 0;
    virtual void leave_context() = 0;
    virtual void clear_context() = 0;

protected:
    Platform() = default;

private:
    EL_DISABLE_COPY (Platform);
};

using PlatformPtr = std::unique_ptr<Platform, void (*) (Platform*)>;

class Device final {
public:
    PlatformPtr platform;
    
    ~Device();
    void viewport (int x, int y, int width, int height);
    void ortho (float left, float right, float top, float bottom, float near, float far);
    void draw (evgDrawMode mode, uint32_t start, uint32_t nverts);

    static const evgDescriptor* descriptor();

private:
    Device() = delete;
    Device (PlatformPtr plat);

    //=========================================================================
    CopyMethod copy_method;
    
    //=========================================================================
    Buffer* active_index_buffer { nullptr };
    Buffer* active_vertex_buffer[8] { nullptr };
    Program* active_program { nullptr };
    Swap* active_swap { nullptr };
    Stencil* active_stencil { nullptr };
    Texture* active_target { nullptr };
    Texture* active_texture[8];

    //=========================================================================
    bool setup_extensions();

    //=========================================================================
    static evgHandle _create();
    static void _destroy (evgHandle device);

    //=========================================================================
    static void _enter_context (evgHandle dh);
    static void _leave_context (evgHandle dh);
    static void _clear_context (evgHandle dh);

    //=========================================================================
    static void _enable (evgHandle device, uint32_t enablement, bool enabled);

    //=========================================================================
    inline static void _viewport (evgHandle dh, int x, int y, int w, int h)
    {
        (static_cast<Device*> (dh))->viewport (x, y, w, h);
    }

    static void _clear (evgHandle device, uint32_t clear_flags, uint32_t color, double depth, int stencil);

    inline static void _draw (evgHandle dh, evgDrawMode mode, uint32_t start, uint32_t nverts)
    {
        (static_cast<Device*> (dh))->draw (mode, start, nverts);
    }

    static void _present (evgHandle dh);
    static void _flush (evgHandle dh);

    //=========================================================================


    inline static void _load_index_buffer (evgHandle dh, evgHandle ibh)
    {
        (static_cast<Device*> (dh))->active_index_buffer =
            static_cast<Buffer*> (ibh);
    }

    static void _load_program (evgHandle dh, evgHandle ph);

    inline static void _load_shader (evgHandle dh, evgHandle sh)
    {
        // (static_cast<Device*> (dh))->load_shader (
        //     static_cast<Shader*> (sh));
    }

    inline static void _load_swap (evgHandle dh, evgHandle sh)
    {
        (static_cast<Device*> (dh))->platform->load_swap (static_cast<const Swap*> (sh));
    }

    static void _load_texture (evgHandle dh, evgHandle th, int unit);
    static void _load_stencil (evgHandle dh, evgHandle sh);
    static void _load_target (evgHandle dh, evgHandle th);

    inline static void _load_vertex_buffer (evgHandle dh, evgHandle vbh, int location)
    {
        (static_cast<Device*> (dh))->active_vertex_buffer[location] =
            static_cast<Buffer*> (vbh);
    }

    EL_DISABLE_COPY (Device);
};

class Swap {
public:
    virtual ~Swap() {}

    SwapSetup setup { 0 };

    static const evgSwapInterface* interface()
    {
        static const evgSwapInterface I = {
            .create = _create,
            .destroy = _destroy
        };
        return &I;
    }

private:
    static evgHandle _create (evgHandle dh, const evgSwapSetup* setup)
    {
        return (static_cast<Device*> (dh))->platform->create_swap (setup);
    }

    static void _destroy (evgHandle sh)
    {
        delete static_cast<Swap*> (sh);
    }
};

class Texture {
public:
    Texture() = delete;
    virtual ~Texture() = default;

    inline void upload (const uint8_t** data)
    {
        assert (data != nullptr);
        uploaded = upload_data (data);
    }

    inline bool bind() const noexcept
    {
        glBindTexture (gl_target, texture);
        return glGetError() == GL_NO_ERROR;
    }

    inline evgTextureType type() const noexcept { return _setup.type; }
    inline evgColorFormat format() const noexcept { return _setup.format; }

    // inline uint32_t levels() const noexcept { return _setup.levels; }
    inline uint32_t width() const noexcept { return _setup.width; }
    inline uint32_t height() const noexcept { return _setup.height; }
    inline uint32_t depth() const noexcept { return _setup.depth; }
    inline uint32_t size() const noexcept { return _setup.size; }

    inline bool has_uploaded() const noexcept { return uploaded; }
    inline bool is_a (TextureType type) const noexcept { return _setup.type == type; }
    inline bool is_dummy() const noexcept { return dummy; }
    inline bool is_dynamic() const noexcept { return dynamic; }
    inline bool is_render_target() const noexcept { return render_target; }
    inline bool use_mipmaps() const noexcept { return mipmaps; }

    void enable_fbo() noexcept;
    void prepare_render (int side = 0) noexcept;

    //=========================================================================
    inline static const evgTextureInterface* interface()
    {
        static const evgTextureInterface I = {
            .create = _create,
            .destroy = _destroy,
            .fill_info = _fill_info,
            .update = _update
        };
        return &I;
    }

protected:
    explicit Texture (Device& dev, const TextureSetup& setup);
    virtual bool upload_data (const uint8_t** data) = 0;

    Device& device;
    TextureSetup _setup;

    GLuint texture { 0 };
    GLenum gl_levels { 0 };
    GLenum gl_format { 0 };
    GLenum gl_format_internal { 0 };
    GLenum gl_format_type { 0 };
    GLenum gl_target { 0 };
    GLuint fbo { 0 };

    bool dynamic { false },
        render_target { false },
        dummy { false },
        mipmaps { false };

    bool uploaded = false;

    static evgHandle _create (evgHandle dh, const evgTextureInfo* setup);
    static void _destroy (evgHandle t);
    static void _fill_info (evgHandle tex, evgTextureInfo* setup);
    static void _update (evgHandle tex, const uint8_t* data);

    EL_DISABLE_COPY (Texture);
};

struct Texture2D : public Texture {
    explicit Texture2D (Device& device, const TextureSetup& setup)
        : Texture (device, setup)
    {
    }

protected:
    bool upload_data (const uint8_t** data);

private:
    bool bind_data (const uint8_t** data);
    Texture2D() = delete;
    EL_DISABLE_COPY (Texture2D);
};

struct Buffer {
    ~Buffer();

    void update (uint32_t size, const void* data);
    bool create_buffers();
    bool destroy_buffers();
    inline bool is_dynamic() const noexcept { return dynamic; }
    inline GLuint object() const noexcept { return buffer; }

    //=========================================================================
    inline static const evgBufferInterface* interface()
    {
        static const evgBufferInterface I = {
            .create = _create,
            .destroy = _destroy,
            .fill_info = _fill_info,
            .update = _update
        };
        return &I;
    }

private:
    explicit Buffer (evgBufferType type, uint32_t size, uint32_t flags);
    evgBufferInfo info;
    bool dynamic = false;
    GLuint buffer { 0 },
           target { 0 };

    inline static evgHandle _create (evgHandle device, evgBufferType type, uint32_t capacity, uint32_t flags)
    {
        if (auto buffer = std::unique_ptr<Buffer> (new Buffer (type, capacity, flags)))
            if (buffer->create_buffers())
                return buffer.release();
        return nullptr;
    }

    inline static void _destroy (evgHandle bh)
    {
        auto buffer = static_cast<Buffer*> (bh);
        buffer->destroy_buffers();
        delete buffer;
    }

    inline static void _fill_info (evgHandle bh, evgBufferInfo* info)
    {
        auto buffer = static_cast<Buffer*> (bh);
        memcpy (info, &buffer->info, sizeof (evgBufferInfo));
    }

    inline static void _update (evgHandle bh, uint32_t size, const void* data)
    {
        auto buffer = static_cast<Buffer*> (bh);
        buffer->update (size, data);
    }
};

class Shader final {
public:
    Shader (Device& d, evgShaderType t)
        : device (d), type (t)
    {
        res.reserve (8);
    }

    ~Shader() {}

    struct Resource {
        std::string symbol;
        evgResource resource;
    };

    evgShaderType get_type() const noexcept { return type; }
    GLuint object() const noexcept { return gl_shader; }
    bool parse (const char* text);

    inline const std::vector<Resource>& resources() const noexcept { return res; }

    //=========================================================================
    inline static const evgShaderInterface* interface()
    {
        static const evgShaderInterface I = {
            .create = _create,
            .destroy = _destroy,
            .parse = _parse,
            // .add_attribute = _add_attribute,
            // .add_uniform = _add_uniform,
            .add_resource = _add_resource
        };
        return &I;
    }

    inline static evgHandle _create (evgHandle dh, evgShaderType type)
    {
        auto device = static_cast<Device*> (dh);
        return new Shader (*device, type);
    }

    inline static void _destroy (evgHandle sh)
    {
        delete static_cast<Shader*> (sh);
    }

    inline static bool _parse (evgHandle sh, const char* text)
    {
        return (static_cast<Shader*> (sh))->parse (text);
    }

    inline static void _add_resource (evgHandle sh, const char* symbol,
                                      evgResourceType resource,
                                      evgValueType value_type)
    {
        auto self = static_cast<Shader*> (sh);
        self->res.push_back (Resource());
        auto& item = self->res.back();
        item.symbol = symbol;
        item.resource.symbol = item.symbol.c_str();
        item.resource.type = resource;
        item.resource.value_type = value_type;
        item.resource.key = self->res.size() - 1;
    }

private:
    Device& device;
    evgShaderType type;
    GLuint gl_shader = 0;
    std::vector<Resource> res;
};

class Stencil final {
public:
    ~Stencil() = default;

    inline static const evgStencilInterface* interface()
    {
        const static evgStencilInterface I = {
            .create = _create,
            .destroy = _destroy
        };
        return &I;
    }

    void prepare_render() const noexcept;

private:
    Stencil() = default;
    evgStencilFormat format;
    GLuint width, height;
    GLuint gl_object { 0 };
    GLenum gl_format { 0 };
    GLenum gl_attachment { 0 };
    bool create_buffer();
    static evgHandle _create (evgHandle device, uint32_t width, uint32_t height, evgStencilFormat format);
    static void _destroy (evgHandle sh);
};

} // namespace gl
