
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include <element/element.h>
#include <element/graphics.h>

struct elVideo{};

namespace element {

// template<typename Prim>
// class CType {
// public:
//     using primitive_type = Prim;
//     virtual ~CType() = default;

//     primitive_type* c_obj() const noexcept { return &prim; }

// private:
//     primitive_type prim;
// };

class GraphicsDevice;

class Compositor final {
public:
    Compositor() = default;
    ~Compositor()= default;

private:
    EL_DISABLE_COPY (Compositor);
};

class Video final {
public:
    Video();
    ~Video();
    
    Compositor* create_compositor() { return nullptr; }
    bool load_device_descriptor (const egDeviceDescriptor* desc);
    GraphicsDevice& graphics_device() { return *graphics; }

    bool is_running()   const noexcept { return running.load() == 1; }
    bool should_stop()  const noexcept { return stopflag.load() == 1; }
    void start_thread();
    void stop_thread();

private:
    std::unique_ptr<GraphicsDevice> graphics;
    std::mutex compositor_lock;
    std::thread video_thread;
    std::atomic<int> stopflag { 0 }, running { 0 };
    static void thread_entry (Video& video);
};

}
