
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
using TextureSetup = evgTextureSetup;
using SwapSetup = evgSwapSetup;

class Program;
class Swap;
class SwapChain;
class IndexBuffer;
class VertexBuffer;
class Texture;
class Shader;

struct WindowSetup {
    WindowSetup() = default;
    virtual ~WindowSetup() = default;
};

class Platform {
public:
    virtual ~Platform() = default;
    virtual bool initialize() = 0;

    //=========================================================================
    virtual SwapChain* create_swap (const evgSwapSetup* setup) =0;
    virtual void load_swap (const SwapChain* swap) = 0;

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
    void draw (egDrawMode mode, uint32_t start, uint32_t nverts);

    CopyMethod copy_method() const noexcept { return m_copy_method; }

    PlatformPtr platform;

    //=========================================================================
    static evgHandle create();
    static void destroy (evgHandle device);

    //=========================================================================
    inline static void _viewport (evgHandle dh, int x, int y, int w, int h) {
        (static_cast<Device*> (dh))->viewport (x, y, w, h);
    }

    inline static void _ortho (evgHandle dh,
                               float left, float right,
                               float top, float bottom,
                               float near, float far)
    {
        (static_cast<Device*> (dh))->ortho (left, right, top, bottom, near, far);
    }

    inline static void _draw (evgHandle dh, egDrawMode mode, uint32_t start, uint32_t nverts)
    {
        (static_cast<Device*> (dh))->draw (mode, start, nverts);
    }

    static void _present (evgHandle dh);
    static void _flush (evgHandle dh);

    //=========================================================================
    inline static void _clear_context (evgHandle dh) {
        (static_cast<Device*> (dh))->platform->clear_context();
    }
    
    inline static void _load_index_buffer (evgHandle dh, evgHandle ibh)
    {
        (static_cast<Device*> (dh))->active_index_buffer =
            static_cast<IndexBuffer*> (ibh);
    }

    inline static void _load_program (evgHandle dh, evgHandle ph)
    {
        (static_cast<Device*> (dh))->active_program =
            static_cast<Program*> (ph);
    }

    inline static void _load_shader (evgHandle dh, evgHandle sh)
    {
        // (static_cast<Device*> (dh))->load_shader (
        //     static_cast<Shader*> (sh));
    }

    inline static void _load_swap (evgHandle dh, evgHandle sh)
    {
        (static_cast<Device*> (dh))->platform->load_swap (
            static_cast<const SwapChain*> (sh));
    }

    inline static void _load_texture (evgHandle dh, evgHandle sh, int index)
    {
        // gl::unused (device, tex, unit);
    }

    inline static void _load_vertex_buffer (evgHandle dh, evgHandle vbh)
    {
        (static_cast<Device*> (dh))->active_vertex_buffer =
            static_cast<VertexBuffer*> (vbh);
    }

private:
    CopyMethod m_copy_method;
    evgMatrix4 active_projection;
    IndexBuffer* active_index_buffer { nullptr };
    VertexBuffer* active_vertex_buffer { nullptr };
    Program* active_program { nullptr };
    Swap* active_swap { nullptr };

    bool setup_extensions();
    EL_DISABLE_COPY (Device);
};

class SwapChain {
public:
    virtual ~SwapChain() {}

    Device* device { nullptr };
    SwapSetup setup { 0 };

    static const evgSwapInterface* interface() {
        static const evgSwapInterface I = {
            .create = _create,
            .destroy = _destroy
        };
        return &I;
    }

private:
    static evgHandle _create (evgHandle dh, const evgSwapSetup* setup) {
        return (static_cast<Device*> (dh))->platform->create_swap (setup);
    }

    static void _destroy (evgHandle sh) {
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

    inline void fill_setup (TextureSetup* setup) const noexcept
    {
        if (setup != nullptr)
            memcpy (setup, &_setup, sizeof (TextureSetup));
    }

    inline TextureType type() const noexcept { return _setup.type; }
    inline egColorFormat format() const noexcept { return _setup.format; }

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
    EL_DISABLE_COPY (Texture);
};

struct Texture2D : public Texture {
    explicit Texture2D (Device& device, const TextureSetup& setup)
        : Texture (device, setup) {}

protected:
    bool upload_data (const uint8_t** data);

private:
    bool bind_data (const uint8_t** data);
    Texture2D() = delete;
    EL_DISABLE_COPY (Texture2D);
};

struct IndexBuffer {
    IndexBuffer() = delete;
    explicit IndexBuffer (uint32_t size, uint32_t flags);

    bool is_dynamic() const noexcept { return dynamic; }
    uint32_t object() const noexcept { return eao; }

    inline uint32_t element_type() const noexcept
    {
        return GL_UNSIGNED_INT;
    }

    inline const void* draw_offset (uint32_t start) const noexcept
    {
        return (const void*) (start * sizeof (uint32_t));
    }

    void update (void* data);

    //=========================================================================
    static const evgIndexBufferInterface* interface()
    {
        static const evgIndexBufferInterface I = {
            .create = _create,
            .destroy = _destroy,
            .fill_setup = _fill_setup,
            .update = _update
        };
        return &I;
    }

private:
    GLuint eao { 0 };
    bool dynamic = false;
    evgIndexArraySetup setup;

    static evgHandle _create (evgHandle /*dev*/, uint32_t size, uint32_t flags)
    {
        return new IndexBuffer (size, flags);
    }

    static void _destroy (evgHandle handle)
    {
        delete static_cast<IndexBuffer*> (handle);
    }

    static void _fill_setup (evgHandle array, evgIndexArraySetup* setup)
    {
        auto self = static_cast<IndexBuffer*> (array);
        memcpy (setup, &self->setup, sizeof (evgIndexArraySetup));
    }

    static void _update (evgHandle array, void* data)
    {
        (static_cast<IndexBuffer*> (array))->update (data);
    }
};

struct VertexBuffer {
    explicit VertexBuffer (evgVertexData* data, uint32_t flags);

    ~VertexBuffer()
    {
        // if (data != nullptr) {
        //     evg_vertex_data_free (data);
        //     data = nullptr;
        // }
    }

    bool create_buffers();
    void flush();
    bool destroy_buffers();

    inline size_t size() const noexcept { return num_vectors; }
    inline bool is_dynamic() const noexcept { return dynamic; }
    inline evgVertexData* get_data() const noexcept { return data; }

    inline GLuint get_points() const noexcept { return points; }

private:
    size_t num_vectors = 0;
    bool dynamic = false;
    GLuint vao { 0 },
        points { 0 },
        normals { 0 },
        tangents { 0 },
        colors { 0 };
    struct UVBuffer {
        UVBuffer (GLuint b, GLuint s)
            : buffer (b),
              size (s) {}
        GLuint buffer;
        GLuint size;
    };

    std::vector<UVBuffer> uv_buffers;
    evgVertexData* data { nullptr };
};

class Shader final {
public:
    Shader (Device& d, evgShaderType t)
        : device (d), type (t)
    {
        if (type == EVG_SHADER_VERTEX)
            atts.push_back ({ 0, EVG_ATTRIB_POSITION, "aPos" });
    }

    ~Shader() {}

    struct Attribute {
        Attribute (uint32_t i, evgAttributeType t, const std::string& n)
            : index (i), type (t), name (n) {}

        uint32_t index;
        evgAttributeType type;
        std::string name;
    };

    evgShaderType get_type() const noexcept { return type; }
    GLuint object() const noexcept { return gl_shader; }
    bool parse (const char* text);
    void release();

    inline const std::vector<Attribute>& attributes() const noexcept { return atts; }

    static void add_attribute (evgHandle sh, evgAttributeType type, evgValueType vtype);
    static void add_uniform (evgHandle sh, evgValueType vtype);

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
    void load_buffers (VertexBuffer* vb, IndexBuffer* ib);

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
