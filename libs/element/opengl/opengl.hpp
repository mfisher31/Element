
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
class SwapChain;
class Buffer;
class Texture;
class Shader;

class Platform {
public:
    virtual ~Platform() = default;
    virtual bool initialize() = 0;

    //=========================================================================
    virtual SwapChain* create_swap (const evgSwapSetup* setup) = 0;
    virtual void load_swap (const SwapChain* swap) = 0;
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
    ~Device()
    {
        platform.reset();
    }

    Device() = delete;
    Device (PlatformPtr plat);

    inline bool have_platform() const noexcept { return platform != nullptr; }
    inline void enter_context() { platform->enter_context(); }
    inline void leave_context() { platform->leave_context(); }
    
    void viewport (int x, int y, int width, int height);
    void ortho (float left, float right, float top, float bottom, float near, float far);
    void draw (evgDrawMode mode, uint32_t start, uint32_t nverts);

    CopyMethod copy_method() const noexcept { return m_copy_method; }

    PlatformPtr platform;

    //=========================================================================
    static evgHandle create();
    static void destroy (evgHandle device);

    //=========================================================================
    inline static void _viewport (evgHandle dh, int x, int y, int w, int h)
    {
        (static_cast<Device*> (dh))->viewport (x, y, w, h);
    }

    inline static void _clear (evgHandle device, 
                               uint32_t  clear_flags, 
                               uint32_t  color, 
                               double    depth, 
                               int       stencil) 
    {
        struct PixelARGB {
            uint8_t a, r, g, b;
        };

        struct PixelRGBA {
            uint8_t r, g, b, a;
        };

        union ConvertPixel {
            PixelARGB pixel;
            uint32_t packed;
        };

        GLbitfield bits = 0;
        if ((clear_flags & EVG_CLEAR_COLOR) != 0) {
            ConvertPixel C; C.packed = color;
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

    inline static void _ortho (evgHandle dh,
                               float left, float right,
                               float top, float bottom,
                               float near, float far)
    {
        (static_cast<Device*> (dh))->ortho (left, right, top, bottom, near, far);
    }

    inline static void _draw (evgHandle dh, evgDrawMode mode, uint32_t start, uint32_t nverts)
    {
        (static_cast<Device*> (dh))->draw (mode, start, nverts);
    }

    static void _present (evgHandle dh);
    static void _flush (evgHandle dh);

    //=========================================================================
    inline static void _clear_context (evgHandle dh)
    {
        (static_cast<Device*> (dh))->platform->clear_context();
    }

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
        (static_cast<Device*> (dh))->platform->load_swap (static_cast<const SwapChain*> (sh));
    }

    static void _load_texture (evgHandle dh, evgHandle th, int unit);

    inline static void _load_vertex_buffer (evgHandle dh, evgHandle vbh)
    {
        (static_cast<Device*> (dh))->active_vertex_buffer =
            static_cast<Buffer*> (vbh);
    }

private:
    CopyMethod m_copy_method;
    evgMatrix4 active_projection;
    Buffer* active_index_buffer { nullptr };
    Buffer* active_vertex_buffer { nullptr };
    Program* active_program { nullptr };
    Swap* active_swap { nullptr };
    Texture* active_texture[8];

    bool setup_extensions();
    EL_DISABLE_COPY (Device);
};

class SwapChain {
public:
    virtual ~SwapChain() {}

    Device* device { nullptr };
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
        delete static_cast<SwapChain*> (sh);
    }
};

class Texture {
public:
    Texture() = delete;
    virtual ~Texture() = default;

    inline void upload (const uint8_t** data)
    {
        if (uploaded)
            return;
        assert (data != nullptr);
        uploaded = upload_data (data);
    }
    
    inline bool bind() const noexcept {
        glBindTexture (gl_target, texture);
        return glGetError() == GL_NO_ERROR;
    }

    inline evgTextureType type()   const noexcept { return _setup.type; }
    inline evgColorFormat format() const noexcept { return _setup.format; }

    // inline uint32_t levels() const noexcept { return _setup.levels; }
    inline uint32_t width()  const noexcept { return _setup.width; }
    inline uint32_t height() const noexcept { return _setup.height; }
    inline uint32_t depth()  const noexcept { return _setup.depth; }
    inline uint32_t size()   const noexcept { return _setup.size; }

    inline bool has_uploaded() const noexcept { return uploaded; }
    inline bool is_a (TextureType type) const noexcept { return _setup.type == type; }
    inline bool is_dummy() const noexcept { return dummy; }
    inline bool is_dynamic() const noexcept { return dynamic; }
    inline bool is_render_target() const noexcept { return render_target; }
    inline bool use_mipmaps() const noexcept { return mipmaps; }

    //=========================================================================
    inline static const evgTextureInterface* interface() {
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

    bool dynamic { false },
        render_target { false },
        dummy { false },
        mipmaps { false };

    void* active_sampler { nullptr };
    void* fbo { nullptr };

    bool uploaded = false;

    static evgHandle _create (evgHandle dh, const evgTextureInfo* setup);
    static void _destroy (evgHandle t);
    static void _fill_info (evgHandle tex, evgTextureInfo* setup);
    static void _update (evgHandle tex, const uint8_t* data);

    EL_DISABLE_COPY (Texture);
};

struct Texture2D : public Texture {
    explicit Texture2D (Device& device, const TextureSetup& setup)
        : Texture (device, setup) {

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
    inline GLuint object()   const noexcept { return buffer; }
    inline GLuint VAO() const noexcept { return vao; }

    //=========================================================================
    inline static const evgBufferInterface* interface()
    {
        static const evgBufferInterface I = {
            .create  = _create,
            .destroy = _destroy,
            .fill_info = _fill_info,
            .update  = _update
        };
        return &I;
    }

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

    inline static void _fill_info (evgHandle bh, evgBufferInfo* info) {
        auto buffer = static_cast<Buffer*> (bh);
        memcpy (info, &buffer->info, sizeof (evgBufferInfo));
    }

    inline static void _update (evgHandle bh, uint32_t size, const void* data)
    {
        auto buffer = static_cast<Buffer*> (bh);
        buffer->update (size, data);
    }

private:
    explicit Buffer (evgBufferType type, uint32_t size, uint32_t flags);
    evgBufferInfo info;
    bool dynamic = false;
    GLuint vao { 0 },
        buffer { 0 },
        target { 0 };
};

class Shader final {
public:
    Shader (Device& d, evgShaderType t)
        : device (d), type (t)
    {
        atts.reserve (4);
    }

    ~Shader() {}

    struct Attribute {
        Attribute (const std::string& n, evgAttributeType t, uint32_t i)
            : index (i), type (t), name (n) { }

        std::string name;
        evgAttributeType type;
        uint32_t index;
    };

    struct Uniform {
        Uniform() {}
        std::string name;
    };

    evgShaderType get_type() const noexcept { return type; }
    GLuint object() const noexcept { return gl_shader; }
    bool parse (const char* text);
    void release();

    inline const std::vector<Attribute>& attributes() const noexcept { return atts; }

    //=========================================================================
    inline static const evgShaderInterface* interface()
    {
        static const evgShaderInterface I = {
            .create = _create,
            .destroy = _destroy,
            .parse = _parse,
            .add_attribute = _add_attribute,
            .add_uniform = _add_uniform
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

    inline static void _add_attribute (evgHandle sh, const char* name, evgAttributeType type, uint32_t index)
    {
        (static_cast<Shader*> (sh))->atts.push_back ({ name, type, index });
    }

    inline static void _add_uniform (evgHandle sh, evgUniformType vtype)
    {
        // (static_cast<Shader*>(sh))->unis.push_back ({index, type, name});
    }

private:
    Device& device;
    evgShaderType type;
    GLuint gl_shader = 0;
    std::vector<Attribute> atts;
};

class Program final {
public:
    Program (Device& d);
    ~Program();

    bool link (Shader* vs, Shader* fs);
    inline GLuint object() const noexcept { return gl_program; }
    inline bool have_program() const noexcept { return gl_program > 0; }
    inline bool can_run() const noexcept { return have_program() && gl_vert != 0 && gl_frag != 0; }
    void load_buffers (Buffer* vb, Buffer* ib);

    bool create_program();
    bool delete_program();

    inline static const evgProgramInterface* interface()
    {
        static const evgProgramInterface I = {
            .create = _create,
            .destroy = _destroy,
            .link = _link
        };
        return &I;
    }

private:
    Device& device;
    GLuint gl_vert = 0;
    GLuint gl_frag = 0;
    GLuint gl_program = 0;
    std::vector<uint32_t> vs_atts;
    std::vector<uint32_t> vs_types;

    static evgHandle _create (evgHandle dh);
    static void _destroy (evgHandle ph);
    static void _link (evgHandle ph, evgHandle vs, evgHandle fs);
};

} // namespace gl
