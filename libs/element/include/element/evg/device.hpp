
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

template <typename Ctype>
class Interface {
public:
    using primitive_type = Ctype;
    ~Interface() = default;

protected:
    Interface() = delete;
    explicit Interface (const Ctype* i, evgHandle h)
        : handle (h)
    {
        memcpy (&iface, i, sizeof (Ctype));
    }

    evgHandle handle { nullptr };
    Ctype iface;

private:
    EL_DISABLE_COPY (Interface);
};

using ProgramInterface = Interface<evgProgramInterface>;
using ShaderInterface = Interface<evgShaderInterface>;
using SwapInterface = Interface<evgSwapInterface>;
using TextureInterface = Interface<evgTextureInterface>;
using VertexBufferInterface = Interface<evgVertexBufferInterface>;

//=============================================================================
class Swap final : private SwapInterface {
public:
    ~Swap()
    {
        iface.destroy (handle);
    }

private:
    friend class Device;
    explicit Swap (const evgSwapInterface* i, evgHandle h)
        : Interface<evgSwapInterface> (i, h) {}
    EL_DISABLE_COPY (Swap);
};

class Texture final : private TextureInterface {
public:
    ~Texture() {}

    TextureType type() const noexcept { return info.type; }
    bool is_2d() const noexcept { return info.type == EVG_TEXTURE_2D; }
    bool is_3d() const noexcept { return info.type == EVG_TEXTURE_3D; }
    bool is_cube() const noexcept { return info.type == EVG_TEXTURE_CUBE; }

    uint32_t width() const noexcept  { return info.width; }
    uint32_t height() const noexcept { return info.height; }
    uint32_t depth() const noexcept  { return info.depth; }

private:
    friend class Device;
    explicit Texture (const evgTextureInterface*  i, evgHandle h)
        : TextureInterface (i, h)
    {
       iface.fill_setup (handle, &info);
    }

    TextureSetup info;
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

class VertexBuffer final : private VertexBufferInterface {
public:
    ~VertexBuffer()
    {
        if (handle != nullptr) {
            iface.destroy (handle);
            handle = nullptr;
        }
        
        if (data != nullptr) {
            evg_vertex_data_free (data);
            data = nullptr;
        }
    }

    inline void update() { iface.flush (handle); }
    inline evgVec3* points() const noexcept { return data->points; }
    inline uint32_t size() const noexcept { return m_size; }
    inline uint32_t num_textures() const noexcept { return m_ntex; };

private:
    friend class Device;
    explicit VertexBuffer (const evgVertexBufferInterface* i, evgHandle h)
        : VertexBufferInterface (i, h) {}

    uint32_t m_size { 0 };
    uint32_t m_ntex { 0 };
    evgVertexData* data = nullptr;
};

class Shader final : private ShaderInterface {
public:
    ~Shader()
    {
        if (handle) {
            iface.destroy (handle);
            handle = nullptr;
        }
    }

    bool parse (const char* program) { return iface.parse (handle, program); }
    void add_attribute (const char* name, evgAttributeType type, uint32_t index)
    {
        iface.add_attribute (handle, name, type, index);
    }

    void add_attribute (const char* name, evgAttributeType type)
    {
        add_attribute (name, type, 0);
    }

private:
    friend class Device;
    friend class Program;
    explicit Shader (const evgShaderInterface* i, evgHandle h)
        : ShaderInterface (i, h) {}
};

class Program final : private ProgramInterface {
public:
    ~Program()
    {
        if (handle) {
            iface.destroy (handle);
            handle = nullptr;
        }
    }

    bool link (Shader* verts, Shader* frags)
    {
        iface.link (handle, verts->handle, frags->handle);
        return true;
    }

private:
    friend class Device;
    explicit Program (const evgProgramInterface* i, evgHandle h)
        : ProgramInterface (i, h) {}
};

class Device final {
public:
    static std::unique_ptr<Device> open (const evgDescriptor* dptr);
    ~Device();

    //=========================================================================
    void enter_context();
    void leave_context();
    void clear_context();

    //=========================================================================
    void load_program (Program* program) noexcept;
    void load_index_buffer (IndexBuffer* const ib) noexcept;
    void load_vertex_buffer (VertexBuffer* vbuf) noexcept;
    void load_swap (const Swap* const swap) noexcept;

    //=========================================================================
    void viewport (int x, int y, int width, int height);
    void ortho (float left, float right, float top, float bottom, float near, float far);
    void draw (egDrawMode mode, uint32_t start, uint32_t count);
    void present();

    //=========================================================================
    Swap* create_swap (const evgSwapSetup* setup);

    //=========================================================================
    Texture* create_2d_texture (egColorFormat format, uint32_t width, uint32_t height, const uint8_t** data);
    Texture* create_3d_texture() { return nullptr; }

    //=========================================================================
    IndexBuffer* create_index_buffer (uint32_t size, uint32_t flags);

    //=========================================================================
    VertexBuffer* create_vertex_buffer (evgVertexData* data, uint32_t flags);

    //=========================================================================
    Shader* create_shader (evgShaderType type);

    //=========================================================================
    Program* create_program();

private:
    explicit Device (const evgDescriptor* ds, evgHandle d);
    evgHandle device { nullptr };
    evgDescriptor desc;
    void destroy();
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
        (static_cast<Dev*> (dh))->enter_context();
    }

    inline static void _leave_context (evgHandle dh)
    {
        (static_cast<Dev*> (dh))->leave_context();
    }
};

} // namespace evg
