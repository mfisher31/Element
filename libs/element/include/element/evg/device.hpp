
#pragma once

#include <functional>
#include <memory>

#include "element/element.h"
#include "element/evg/evg.h"

namespace evg {

using BufferInfo = evgBufferInfo;
using ColorFormat = evgColorFormat;
using DrawMode = evgDrawMode;
using StencilFormat = evgStencilFormat;
using SwapSetup = evgSwapInfo;
using TextureInfo = evgTextureInfo;
using TextureType = evgTextureType;
using ValueType = evgValueType;

template <typename Ctype>
class Interface {
public:
    using type = Ctype;
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
using BufferInterface = Interface<evgBufferInterface>;
using StencilInterface = Interface<evgStencilInterface>;

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

    inline TextureType type() const noexcept { return info.type; }
    inline bool is_2d() const noexcept { return info.type == EVG_TEXTURE_2D; }
    inline bool is_3d() const noexcept { return info.type == EVG_TEXTURE_3D; }
    inline bool is_cube() const noexcept { return info.type == EVG_TEXTURE_CUBE; }

    inline ColorFormat color_format() const noexcept { return info.format; }
    inline uint32_t width() const noexcept { return info.width; }
    inline uint32_t height() const noexcept { return info.height; }
    inline uint32_t depth() const noexcept { return info.depth; }

    inline void update (const uint8_t* data)
    {
        iface.update (handle, data);
    }

private:
    friend class Device;
    explicit Texture (const evgTextureInterface* i, evgHandle h)
        : TextureInterface (i, h)
    {
        iface.fill_info (handle, &info);
    }

    TextureInfo info;
};

class Buffer final : private BufferInterface {
public:
    using size_type = uint32_t;

    ~Buffer()
    {
        if (handle != nullptr) {
            iface.destroy (handle);
            handle = nullptr;
        }

        if (buffer_data != nullptr) {
            std::free (buffer_data);
            buffer_data = nullptr;
        }
    }

    inline bool is_array() const noexcept { return info.type == EVG_BUFFER_ARRAY; }
    inline bool is_index() const noexcept { return info.type == EVG_BUFFER_INDEX; }
    inline uint32_t capacity() const noexcept { return info.capacity; }
    inline uint32_t size() const noexcept { return buffer_size; }
    inline void flush() { iface.update (handle, buffer_size, buffer_data); }
    inline void* data() const noexcept { return buffer_data; }

    void resize (uint32_t size)
    {
        if (size <= info.capacity)
            buffer_size = size;
        else {
            // reallocate?
        }
    }

private:
    friend class Device;
    explicit Buffer (const evgBufferInterface* i, evgHandle h)
        : BufferInterface (i, h)
    {
        iface.fill_info (handle, &info);
        if (info.capacity > 0) {
            buffer_data = std::malloc (info.capacity);
            memset (buffer_data, 0, (size_t) info.capacity);
            buffer_size = info.capacity;
        }
    }

    BufferInfo info;
    uint32_t buffer_size = 0;
    void* buffer_data = nullptr;
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

    void add_resource (const char* symbol, evgResourceType resource, evgValueType value_type) noexcept
    {
        iface.add_resource (handle, symbol, resource, value_type);
    }

    void add_attribute (const char* symbol, evgValueType value_type) noexcept
    {
        add_resource (symbol, EVG_ATTRIBUTE, value_type);
    }

    void add_uniform (const char* symbol, evgValueType value_type) noexcept
    {
        add_resource (symbol, EVG_UNIFORM, value_type);
    }

    void add_texture (const char* symbol) noexcept
    {
        add_uniform (symbol, EVG_VALUE_TEXTURE);
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

    const evgResource* resource (uint32_t index) const noexcept
    {
        return iface.resource (handle, index);
    }

    inline int map_symbol (const char* symbol) const noexcept
    {
        uint32_t i = 0;
        auto res = resource (i);
        while (res != nullptr) {
            if (strcmp (res->symbol, symbol) == 0)
                return static_cast<int> (res->key);
            res = resource (i++);
        }
        return 0xffffffff;
    }

    void set_value (int key, uint32_t size, const void* data) noexcept
    {
        iface.update_resource (handle, key, size, data);
    }

    void set_value (const char* key, uint32_t size, const void* data) noexcept
    {
        auto ikey = map_symbol (key);
        if (ikey != 0xffffffff)
            set_value (ikey, size, data);
    }

    template <typename Val>
    void set_value (const char* symbol, const Val& value) noexcept
    {
        return set_value (symbol, sizeof (Val), &value);
    }

    template <typename Val>
    void set_value (int key, const Val& value) noexcept
    {
        return set_value (key, sizeof (Val), &value);
    }

private:
    friend class Device;
    explicit Program (const evgProgramInterface* i, evgHandle h)
        : ProgramInterface (i, h) {}
};

class Stencil final : private StencilInterface {
public:
    ~Stencil() = default;

    StencilFormat format() const noexcept { return f; }
    uint32_t width() const noexcept { return w; }
    uint32_t height() const noexcept { return h; }

private:
    friend class Device;
    Stencil (const evgStencilInterface* i, evgHandle h)
        : StencilInterface (i, h) {}

    evgStencilFormat f;
    uint32_t w = 0;
    uint32_t h = 0;
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
    void save_state();
    void restore_state();
    
    //=========================================================================
    void enable (uint32_t what, bool enabled);

    //=========================================================================
    void viewport (int x, int y, int width, int height);
    void clear (uint32_t flags, uint32_t color, double depth, int stencil);

    void draw (DrawMode mode, uint32_t start, uint32_t count);
    void present();
    void flush();

    //=========================================================================
    void load_program (Program* program) noexcept;
    void load_index_buffer (Buffer* const ib) noexcept;
    void load_vertex_buffer (Buffer* vbuf, int slot) noexcept;
    void load_swap (const Swap* const swap) noexcept;
    
    inline void load_texture (Texture* texture, int unit) noexcept
    {
        desc.load_texture (device, texture != nullptr ? texture->handle : nullptr, unit);
    }
    
    inline void load_stencil (Stencil* stencil) noexcept
    {
        desc.load_stencil (device, stencil != nullptr ? stencil->handle : nullptr);
    }
    
    inline void load_target (Texture* texture) noexcept
    {
        desc.load_target (device, texture != nullptr ? texture->handle : nullptr);
    }

    //=========================================================================
    Swap* create_swap (const SwapSetup* setup);

    //=========================================================================
    Texture* create_2d_texture (evgColorFormat format, uint32_t width, uint32_t height);
    Texture* create_3d_texture() { return nullptr; }

    //=========================================================================
    Buffer* create_index_buffer (uint32_t capacity, uint32_t flags);

    //=========================================================================
    Buffer* create_vertex_buffer (uint32_t capacity, uint32_t flags);

    //=========================================================================
    Shader* create_shader (evgShaderType type);

    //=========================================================================
    Program* create_program();

    //=========================================================================
    Stencil* create_stencil (uint32_t width, uint32_t height, StencilFormat format);

private:
    explicit Device (const evgDescriptor* ds, evgHandle d);
    evgHandle device { nullptr };
    evgDescriptor desc;
    void destroy();
};

class Display {
public:
    virtual ~Display() = default;

protected:
    Display () = default;

private:
    EL_DISABLE_COPY(Display);
};

} // namespace evg
