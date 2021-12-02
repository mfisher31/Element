
#pragma once

#include <cstring>
#include <memory>
#include <element/graphics.h>

namespace element {

class GraphicsDevice final {
public:
    ~GraphicsDevice()
    {
        destroy();
    }

    bool is_loaded() const noexcept { return device != nullptr; }

    static std::unique_ptr<GraphicsDevice> load (const egDeviceDescriptor* dptr)
    {
        if (auto device = valid (dptr) ? dptr->create() : nullptr)
            return std::unique_ptr<GraphicsDevice> (new GraphicsDevice (dptr, device));
        return nullptr;
    }

    void unload()
    {
        destroy();
        setup_dummy();
    }

    //=========================================================================
    void enter_context() { desc.enter_context (device); }
    void leave_context() { desc.leave_context (device); }

    //=========================================================================
    egSwapChain* create_swap (const egSwapSetup* setup) { return desc.swap_create (device, setup); }
    void load_swap (egSwapChain* swap) { desc.swap_load (device, swap); }
    void unload_swap() { load_swap (nullptr); }
    void destroy_swap (egSwapChain* swap) { desc.swap_destroy (swap); }

    //=========================================================================
    egTexture* create_2d_texture (egColorFormat format,
                        uint32_t width, uint32_t height,
                                  const uint8_t** data)
    {
        const egTextureSetup setup = {
            .type = EL_TEXTURE_2D,
            .format = format,
            .levels = 1,
            .flags = 0,
            .width = width,
            .height = height,
            // .levels = 1
        };

        return desc.texture_create (device, &setup, data);
    }

    //=========================================================================
    void begin_frame() {}
    void end_frame() {}

private:
    explicit GraphicsDevice (const egDeviceDescriptor* ds, egDevice* d)
        : device (d)
    {
        memcpy (&desc, ds, sizeof (egDeviceDescriptor));
    }

    egDevice* device { nullptr };
    egDeviceDescriptor desc;
    static egDevice* _create() { return nullptr; }
    static void _noop_void (egDevice*) {}

    void destroy()
    {
        if (device != nullptr && desc.destroy != nullptr)
            desc.destroy (device);
        device = nullptr;
    }

    void setup_dummy()
    {
        device = nullptr;
        desc.create = _create;
        desc.destroy = _noop_void;
        desc.enter_context = _noop_void;
        desc.leave_context = _noop_void;
    }

    static constexpr bool valid (const egDeviceDescriptor* desc) noexcept
    {
        /* clang-format off */
        if (desc == nullptr)
            return false;

        return desc->create != nullptr &&
               desc->destroy != nullptr && 
               desc->enter_context != nullptr && 
               desc->leave_context != nullptr &&
               desc->swap_create != nullptr &&
               desc->swap_destroy != nullptr &&
               desc->swap_destroy != nullptr &&
               desc->texture_create != nullptr &&
               desc->texture_destroy != nullptr &&
               desc->texture_fill_setup != nullptr;
        /* clang-format on */
    }
};
}
