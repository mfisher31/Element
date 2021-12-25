
#include <cassert>
#include <cstring>
#include <iostream>

#include "element/context.hpp"
#include "element/graphics.hpp"
#include "element/evg/image_source.hpp"
#include "element/evg/solid_source.hpp"

#include "video.hpp"

namespace element {
template <uint32_t R>
struct fps_to_nanoseconds {
    static constexpr const auto value = std::chrono::nanoseconds (1000000000 / R);
};

using FPS30 = fps_to_nanoseconds<30>;
using FPS60 = fps_to_nanoseconds<60>;
using FPS24 = fps_to_nanoseconds<24>;
using FPS5 = fps_to_nanoseconds<5>;

class TestDisplay : public VideoDisplay {
public:
    TestDisplay (evg::ImageSource& s, evg::SolidSource& sd, evg::Swap* sw)
        : source (s), solid (sd), swap (sw) {}

    void render (evg::Context& g)
    {
        auto& device = g.get_device();
        device.load_swap (swap.get());
        device.enter_context();

        device.viewport (0, 0, 640, 360);
        device.clear (EVG_CLEAR_COLOR, 0xff050505, 0.0, 0);
        g.ortho (0.0f, 640.0, 0.0f, 360.0, -100.0, 100.0);

        g.save_state();
        source.expose (g);
        g.restore_state();

        // g.save_state();
        // solid.expose_frame (g);
        // g.restore_state();

        device.flush();
        device.present();

        device.load_swap (nullptr);
        device.leave_context();
    }

private:
    evg::Source& source;
    evg::Source& solid;
    std::unique_ptr<evg::Swap> swap;
    evgMatrix4 projection;
};

static std::unique_ptr<evg::ImageSource> source;
static std::unique_ptr<evg::SolidSource> solid;
static std::unique_ptr<evg::Context> context;
static std::unique_ptr<TestDisplay> display;
static std::mutex display_mutex;

static bool process_video (Video& video)
{
    if (! context) {
        context.reset (new evg::Context (video.graphics_device()));
    }

    if (! source) {
        solid.reset (new evg::SolidSource());
        source.reset (new evg::ImageSource());
        source->load_file ("/home/mfisher/Desktop/500_x_500_SMPTE_Color_Bars.png");
    }

    if (context == nullptr || source == nullptr)
        return false;

    const auto stop_requested = video.should_stop();
    const auto interval = fps_to_nanoseconds<24>::value;
    auto& device = video.graphics_device();

    {
        device.enter_context();
        std::scoped_lock sl1 (solid->render_mutex());
        solid->process_frame();
        std::scoped_lock sl2 (source->render_mutex());
        source->process_frame();
        device.leave_context();

        {
            std::scoped_lock sl2 (display_mutex);
            if (display)
                display->render (*context);
        }
    }

    std::this_thread::sleep_for (interval);
    return ! stop_requested;
}

void Video::thread_entry (Video& video)
{
    while (process_video (video))
        ;
    display.reset();
    source.reset();
    solid.reset();
    context.reset();
    std::clog << "[info] video thread stopped\n";
    video.stopflag.store (0);
    video.running.store (0);
}

Video::Video() {}

Video::~Video()
{
    if (is_running())
        stop_thread();

    if (graphics) {
        // graphics->unload();
        graphics.reset();
    }
}

VideoDisplay* Video::create_display (const evgSwapInfo* setup)
{
    if (source == nullptr || graphics == nullptr)
        return nullptr;

    if (setup == nullptr) {
        std::unique_ptr<TestDisplay> deleter;
        {
            std::lock_guard<std::mutex> sl (display_mutex);
            display.swap (deleter);
        }
        deleter.reset();
        return nullptr;
    }

    auto swap = graphics->create_swap (setup);
    if (swap == nullptr)
        return nullptr;

    auto display2 = std::make_unique<TestDisplay> (*source, *solid, swap);
    {
        std::lock_guard<std::mutex> sl (display_mutex);
        display.swap (display2);
    }

    display2.reset();
    return display.get();
}

bool Video::load_device_descriptor (const evgDescriptor* desc)
{
    if (graphics)
        return true;

    if (auto g = evg::Device::open (desc)) {
        graphics = std::move (g);
        // FIXME: don't do this here.
        if (! is_running())
            start_thread();
    }

    return graphics != nullptr;
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
    if (is_running()) {
        stopflag.store (1);

        while (is_running()) {
            std::this_thread::sleep_for (std::chrono::milliseconds (14));
        }

        stopflag.store (0);
        running.store (0);
    }

    if (video_thread.joinable())
        video_thread.join();
}

} // namespace element
