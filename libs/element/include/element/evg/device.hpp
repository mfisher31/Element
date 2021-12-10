
#pragma once

#include <functional>
#include <memory>

#include "element/element.h"
#include "element/evg/evg.h"

namespace evg {

#if 0
class Shader;
class Swap;
class Program;
class Texture;

class Device {
public:
    virtual ~Device() = default;
    virtual Swap* create_swap() = 0;
    virtual Texture* create_texture() = 0;
    virtual Shader* create_shader (evgShaderType type) = 0;
    virtual Program* create_program() = 0;

    virtual void load_texture (Texture*) = 0;
    virtual void load_swap (Swap* swap) = 0;
    virtual void load_program (Program* program) = 0;

    virtual void enter_context() = 0;
    virtual void leave_context() = 0;

    virtual void viewport (int x, int y, int width, int height) = 0;
    virtual void ortho (float left, float right, float top, float bottom, float near, float far) = 0;

protected:
    Device() = default;

private:
    EL_DISABLE_COPY(Device);
};

#else

using TextureType = evgTextureType;
using TextureSetup = evgTextureSetup;

class Swap final {
public:
    ~Swap()
    {
        iface.destroy (handle);
    }

private:
    friend class Device;
    explicit Swap (const evgSwapInterface* i, evgHandle h) {
        handle = h;
        memcpy (&iface, i, sizeof (evgSwapInterface));
    }
    evgHandle handle = nullptr;
    evgSwapInterface iface;
    EL_DISABLE_COPY (Swap);
};

class Texture final {
public:
    ~Texture() {}

    TextureType type() const noexcept { return info.type; }
    bool is_2d() const noexcept { return info.type == EVG_TEXTURE_2D; }
    bool is_3d() const noexcept { return info.type == EVG_TEXTURE_3D; }
    bool is_cube() const noexcept { return info.type == EVG_TEXTURE_CUBE; }

    uint32_t width() const noexcept { return info.width; }
    uint32_t height() const noexcept { return info.height; }
    uint32_t depth() const noexcept { return info.depth; }

private:
    friend class Device;
    explicit Texture (const evgDescriptor* d, evgHandle h)
    {
        handle = h;
        memcpy (&desc, d, sizeof (evgDescriptor));
        memset (&info, 0, sizeof (TextureSetup));
        desc.texture_fill_setup (handle, &info);
    }

    evgDescriptor desc;
    TextureSetup info;
    evgHandle handle { nullptr };
};

class IndexBuffer final {
public:
    ~IndexBuffer() { destroy(); }

    uint32_t size() const noexcept { return setup.size; }
    void update (const void* data) noexcept
    {
        iface.update (handle, (void*) data);
    }

private:
    friend class Device;
    explicit IndexBuffer (const evgIndexBufferInterface* i, evgHandle h)
        : handle (h)
    {
        memcpy (&iface, i, sizeof (evgIndexBufferInterface));
        iface.fill_setup (handle, &setup);
    }

    void destroy()
    {
        if (handle == nullptr)
            return;
        iface.destroy (handle);
        handle = nullptr;
    }

    evgHandle handle = nullptr;
    evgIndexBufferInterface iface;
    evgIndexArraySetup setup;
};

class VertexBuffer final {
public:
    ~VertexBuffer()
    {
        if (data != nullptr) {
            evg_vertex_data_free (data);
            data = nullptr;
        }

        if (handle && f_destroy)
            f_destroy (handle);
    }

    inline void update() { f_update (handle); }
    inline evgVec3* points() const noexcept { return data->points; }
    inline uint32_t size() const noexcept { return m_size; }
    inline uint32_t num_textures() const noexcept { return m_ntex; };

private:
    VertexBuffer() {}
    friend class Device;
    evgHandle handle { nullptr };
    std::function<void (evgHandle)> f_destroy;
    std::function<void (evgHandle)> f_update;
    evgVertexData* data { nullptr };
    uint32_t m_size { 0 };
    uint32_t m_ntex { 0 };
};

class Shader final {
public:
    ~Shader() {}

    bool parse (const char* program) { return f_parse (handle, program); }
    void add_uniform (int type, const char* name) { }
    void uniforms() const noexcept {}

private:
    friend class Device;
    friend class Program;
    Shader() {}
    evgHandle handle = nullptr;
    std::function<bool (evgHandle, const char*)> f_parse;
};

class Uniform final {
public:
    Uniform() {}
};

class Program final {
public:
    ~Program() {}

    bool link (Shader* verts, Shader* frags)
    {
        iface.link (handle, verts->handle, frags->handle);
        return true;
    }

private:
    friend class Device;
    evgHandle handle = nullptr;
    evgProgramInterface iface;
    explicit Program (const evgProgramInterface* i, evgHandle h) {
        handle = h;
        memcpy (&iface, i, sizeof (evgProgramInterface));
    }
};

class Device final {
public:
    ~Device()
    {
        destroy();
    }

    inline static std::unique_ptr<Device> open (const evgDescriptor* dptr)
    {
        if (auto device = evg_descriptor_valid (dptr) ? dptr->create() : nullptr)
            return std::unique_ptr<Device> (new Device (dptr, device));
        return nullptr;
    }

    //=========================================================================
    inline void enter_context() { desc.enter_context (device); }
    inline void leave_context() { desc.leave_context (device); }
    inline void clear_context() { desc.clear_context (device); }

    //=========================================================================
    void load_program (Program* program) noexcept {
        desc.load_program (device, program != nullptr ? program->handle : nullptr);
    }

    void load_index_buffer (IndexBuffer* const ib) noexcept
    {
        desc.load_index_buffer (device, ib != nullptr ? ib->handle : nullptr);
    }

    void load_vertex_buffer (VertexBuffer* vbuf) noexcept
    {
        desc.load_vertex_buffer (device, vbuf != nullptr ? vbuf->handle : nullptr);
    }

    void load_swap (const Swap* const swap) noexcept
    {
        desc.load_swap (device, swap != nullptr ? swap->handle : nullptr);
    }

    //=========================================================================
    void viewport (int x, int y, int width, int height)
    {
        desc.viewport (device, x, y, width, height);
    }

    void ortho (float left, float right, float top, float bottom, float near, float far)
    {
        desc.ortho (device, left, right, top, bottom, near, far);
    }

    void draw (egDrawMode mode, uint32_t start, uint32_t count) {
        desc.draw (device, mode, start, count);
    }

    void present() {
        desc.present (device);
    }
    
    //=========================================================================
    Swap* create_swap (const evgSwapSetup* setup)
    {
        auto iface = desc.swap;
        if (auto handle = iface->create (device, setup)) {
            auto swap = new Swap (iface, handle);
            return swap;
        }

        return nullptr;
    }

    //=========================================================================
    Texture* create_2d_texture (egColorFormat format,
                                uint32_t width,
                                uint32_t height,
                                const uint8_t** data)
    {
        evgTextureSetup setup = {
            .type = EVG_TEXTURE_2D,
            .format = format,
            .levels = 1,
            .flags = EVG_OPT_DYNAMIC,
            .width = width,
            .height = height
        };

        if (auto handle = desc.texture_create (device, &setup, data)) {
            auto obj = new Texture (&desc, handle);
            return obj;
        }

        return nullptr;
    }

    //=========================================================================
    IndexBuffer* create_index_buffer (uint32_t size, uint32_t flags)
    {
        auto const iface = desc.index_buffer;
        if (auto handle = iface->create (device, size, flags)) {
            auto ib = new IndexBuffer (iface, handle);
            return ib;
        }
        return nullptr;
    }

    //=========================================================================
    VertexBuffer* create_vertex_buffer (evgVertexData* data, uint32_t flags)
    {
        if (auto handle = desc.vertex_buffer_create (device, data, flags)) {
            auto buf = new VertexBuffer();
            buf->handle = handle;
            buf->data = data;
            buf->f_destroy = desc.vertex_buffer_destroy;
            buf->f_update = desc.vertex_buffer_flush;
            return buf;
        }

        return nullptr;
    }

    //=========================================================================
    Shader* create_shader (evgShaderType type)
    {
        if (auto handle = desc.shader_create (device, type)) {
            auto shader = new Shader();
            shader->handle = handle;
            shader->f_parse = desc.shader_parse;
            return shader;
        }
        return nullptr;
    }

    Program* create_program()
    {
        auto iface = desc.program;
        if (auto handle = iface->create (device)) {
            auto program = new Program (iface, handle);
            return program;
        }

        return nullptr;
    }

private:
    explicit Device (const evgDescriptor* ds, evgHandle d)
        : device (d)
    {
        memcpy (&desc, ds, sizeof (evgDescriptor));
    }

    evgHandle device { nullptr };
    evgDescriptor desc;

    void destroy()
    {
        if (device != nullptr && desc.destroy != nullptr)
            desc.destroy (device);
        device = nullptr;
    }
};
#endif

template <class Dev>
struct Descriptor : evgDescriptor {
    using device_type = Dev;
    using descriptor_type = Descriptor<Dev>;

    explicit inline Descriptor (evgHandle (*allocator)() = nullptr,
                                void (*deleter) (evgHandle) = nullptr)
    {
        memset ((evgDescriptor*) this, 0, sizeof (evgDescriptor));
        create = allocator;
        destroy = deleter;
        enter_context = _enter_context;
        leave_context = _leave_context;
    }

private:
    inline static void _enter_context (evgHandle dh)
    {
        (static_cast<device_type*> (dh))->enter_context();
    }

    inline static void _leave_context (evgHandle dh)
    {
        (static_cast<device_type*> (dh))->leave_context();
    }
};

} // namespace evg
