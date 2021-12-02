
#include <cassert>
#include <cstring>
#include <iostream>

#include "video.hpp"
#include "element/source.hpp"
#include "graphics_context.hpp"
#include "graphics_device.hpp"

namespace element {

template <uint32_t R>
struct fps_to_nanoseconds {
    static constexpr const auto value = std::chrono::nanoseconds (1000000000 / R);
};

using FPS30 = fps_to_nanoseconds<30>;
using FPS60 = fps_to_nanoseconds<60>;
using FPS24 = fps_to_nanoseconds<24>;
using FPS5  = fps_to_nanoseconds<5>;

struct ScopedGraphics final {
    explicit ScopedGraphics (GraphicsDevice& g)
        : graphics (g)
    {
        graphics.enter_context();
    }

    ~ScopedGraphics()
    {
        graphics.leave_context();
    }

private:
    GraphicsDevice& graphics;
    EL_DISABLE_COPY (ScopedGraphics);
};

static std::unique_ptr<TestVideoSource> source;
static std::unique_ptr<GraphicsContext> context;

static bool process_video (Video& video)
{
    if (! context)
    {
        context.reset (new GraphicsContext (video.graphics_device()));
    }

    if (! source)
    {
        source.reset (new TestVideoSource());
        source->load_file ("/home/mfisher/Desktop/500_x_500_SMPTE_Color_Bars.png");
    }

    if (context == nullptr || source == nullptr)
        return false;

    const auto stop_requested = video.should_stop();
    const auto interval = fps_to_nanoseconds<2>::value;
    auto& device = video.graphics_device();
    device.enter_context();

    {
        std::scoped_lock sl (source->render_mutex());
        source->process_frame();
        source->expose_frame (*context);
    }

    device.leave_context();
    std::this_thread::sleep_for (interval);
    return ! stop_requested;
}

void Video::thread_entry (Video& video)
{
    while (process_video (video)) {
    }
    std::clog << "[info] video thread stopped\n";
    video.stopflag.store (0);
}

Video::Video() {}

Video::~Video()
{
    if (is_running())
        stop_thread();

    if (graphics) {
        graphics->unload();
        graphics.reset();
    }
}

bool Video::load_device_descriptor (const egDeviceDescriptor* desc)
{
    if (graphics && graphics->is_loaded())
        return true;

    if (auto g = GraphicsDevice::load (desc)) {
        graphics = std::move (g);
        // FIXME: don't do this here.
        if (! is_running())
            start_thread();
    }

    return graphics && graphics->is_loaded();
}

void Video::start_thread()
{
    if (! graphics) {
        std::cerr << "[video] cannot start video engine without a graphics device.\n";
        return;
    }

    if (is_running())
        return;
    std::clog << "[video] engine starting\n";
    stopflag.store (0);
    running.store (1);
    video_thread = std::thread (Video::thread_entry, std::ref (*this));
}

void Video::stop_thread()
{
    if (! is_running())
        return;
    stopflag.store (1);
    video_thread.join();
    stopflag.store (0);
    running.store (0);
}

} // namespace element
