
#include "element/evg/device.hpp"

namespace evg {

std::unique_ptr<Device> Device::open (const evgDescriptor* dptr)
{
    if (auto device = evg_descriptor_valid (dptr) ? dptr->create() : nullptr)
        return std::unique_ptr<Device> (new Device (dptr, device));
    return nullptr;
}

Device::Device (const evgDescriptor* ds, evgHandle d)
    : device (d)
{
    memcpy (&desc, ds, sizeof (evgDescriptor));
}

Device::~Device()
{
    destroy();
}
void Device::destroy()
{
    if (device != nullptr && desc.destroy != nullptr)
        desc.destroy (device);
    device = nullptr;
}

//=========================================================================
void Device::enter_context() { desc.enter_context (device); }
void Device::leave_context() { desc.leave_context (device); }
void Device::clear_context() { desc.clear_context (device); }

//=========================================================================
void Device::load_program (Program* program) noexcept
{
    desc.load_program (device, program != nullptr ? program->handle : nullptr);
}

void Device::load_index_buffer (Buffer* const ib) noexcept
{
    desc.load_index_buffer (device, ib != nullptr ? ib->handle : nullptr);
}

void Device::load_vertex_buffer (Buffer* vbuf, int location) noexcept
{
    desc.load_vertex_buffer (device, vbuf != nullptr ? vbuf->handle : nullptr, location);
}

void Device::load_swap (const Swap* const swap) noexcept
{
    desc.load_swap (device, swap != nullptr ? swap->handle : nullptr);
}

//=========================================================================
void Device::viewport (int x, int y, int width, int height)
{
    desc.viewport (device, x, y, width, height);
}

void Device::clear (uint32_t flags, uint32_t color, double depth, int stencil) {
    desc.clear (device, flags, color, depth, stencil);
}

void Device::ortho (float left, float right, float top, float bottom, float near, float far)
{
    desc.ortho (device, left, right, top, bottom, near, far);
}

void Device::draw (evgDrawMode mode, uint32_t start, uint32_t count)
{
    desc.draw (device, mode, start, count);
}

void Device::present() { desc.present (device); }
void Device::flush() { desc.flush (device); }

//=========================================================================
Swap* Device::create_swap (const evgSwapSetup* setup)
{
    auto iface = desc.swap;
    if (auto handle = iface->create (device, setup)) {
        auto swap = new Swap (iface, handle);
        return swap;
    }

    return nullptr;
}

//=========================================================================
Texture* Device::create_2d_texture (evgColorFormat format, uint32_t width, uint32_t height)
{
    evgTextureInfo setup = {
        .type = EVG_TEXTURE_2D,
        .format = format,
        .levels = 1,
        .flags = EVG_OPT_DYNAMIC,
        .width = width,
        .height = height
    };

    auto iface = desc.texture;
    if (auto handle = iface->create (device, &setup)) {
        auto obj = new Texture (iface, handle);
        return obj;
    }

    return nullptr;
}

//=========================================================================
Buffer* Device::create_index_buffer (uint32_t capacity, uint32_t flags)
{
    auto const iface = desc.buffer;
    if (auto handle = iface->create (device, EVG_BUFFER_INDEX, capacity, flags)) {
        auto ib = new Buffer (iface, handle);
        return ib;
    }
    return nullptr;
}

//=========================================================================
Buffer* Device::create_vertex_buffer (uint32_t capacity, uint32_t flags)
{
    auto iface = desc.buffer;
    if (auto handle = iface->create (device, EVG_BUFFER_ARRAY, capacity, flags)) {
        auto buf = new Buffer (iface, handle);
        return buf;
    }

    return nullptr;
}

//=========================================================================
Shader* Device::create_shader (evgShaderType type)
{
    auto iface = desc.shader;
    if (auto handle = iface->create (device, type)) {
        auto shader = new Shader (iface, handle);
        return shader;
    }

    return nullptr;
}

Program* Device::create_program()
{
    auto iface = desc.program;
    if (auto handle = iface->create (device)) {
        auto program = new Program (iface, handle);
        return program;
    }

    return nullptr;
}
} // namespace evg
