
#include <cassert>
#include <cstring>
#include <iostream>

#include "element/context.hpp"
#include "element/evg/matrix.h"
#include "element/source.hpp"
#include "graphics_context.hpp"
#include "graphics_device.hpp"
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
    TestDisplay (TestVideoSource& s, evg::Swap* sw)
        : source (s), swap (sw) { }

    void render (GraphicsContext& gc)
    {
        auto& device = gc.get_device();
        device.load_swap (swap.get());
        device.enter_context();

        // ortho (&projection, 0.0f, 640, 0.0f, 360, -100.0, 100.0);
        device.viewport (0, 0, 640, 360);
        device.clear (EVG_CLEAR_COLOR, 0xff050505, 0.0, 0);

        source.expose_frame (gc);
        device.flush();
        device.present();
        
        device.load_swap (nullptr);
        device.leave_context();
    }

private:
    TestVideoSource& source;
    std::unique_ptr<evg::Swap> swap;
    evgMatrix4 projection;

    void ortho (evgMatrix4* dst, float left, float right,
                float bottom, float top,
                float near, float far)
    {
        float rml = right - left;
        float bmt = bottom - top;
        float fmn = far - near;

        evg_vec4_reset (&dst->x);
        evg_vec4_reset (&dst->y);
        evg_vec4_reset (&dst->z);
        evg_vec4_reset (&dst->t);

        dst->x.x = 2.0f / rml;
        dst->t.x = (left + right) / -rml;

        dst->y.y = 2.0f / -bmt;
        dst->t.y = (bottom + top) / bmt;

        dst->z.z = -2.0f / fmn;
        dst->t.z = (far + near) / -fmn;

        dst->t.w = 1.0f;
    }
};

static std::unique_ptr<TestVideoSource> source;
static std::unique_ptr<GraphicsContext> context;
static std::unique_ptr<TestDisplay> display;
static std::mutex display_mutex;

static bool process_video (Video& video)
{
    if (! context) {
        context.reset (new GraphicsContext (video.graphics_device()));
    }

    if (! source) {
        source.reset (new TestVideoSource());
        source->load_file ("/home/mfisher/Desktop/500_x_500_SMPTE_Color_Bars.png");
    }

    if (context == nullptr || source == nullptr)
        return false;

    const auto stop_requested = video.should_stop();
    const auto interval = fps_to_nanoseconds<24>::value;
    auto& device = video.graphics_device();

    {
        device.enter_context();
        std::scoped_lock sl (source->render_mutex());
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
    source.reset();
    context.reset();
    display.reset();
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

VideoDisplay* Video::create_display (const evgSwapSetup* setup)
{
    if (source == nullptr || graphics == nullptr)
        return nullptr;

    if (setup == nullptr) {
        std::unique_ptr<TestDisplay> deleter;
        {
            std::scoped_lock sl (display_mutex);
            display.swap (deleter);
        }
        deleter.reset();
        return nullptr;
    }

    auto swap = graphics->create_swap (setup);
    if (swap == nullptr)
        return nullptr;

    auto display2 = std::make_unique<TestDisplay> (*source, swap);
    {
        std::scoped_lock sl (display_mutex);
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
