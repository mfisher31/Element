/** 
    This file is part of Element.
    Copyright (C) 2016-2021  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include <element/element.h>
#include <element/graphics.h>
#include <element/evg/device.hpp>

struct elVideo{};

namespace element {
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
    
    VideoDisplay* create_display (const evgSwapSetup* setup);
    bool load_device_descriptor (const evgDescriptor* desc);
    evg::Device& graphics_device() { return *graphics; }

    bool is_running()   const noexcept { return running.load() == 1; }
    bool should_stop()  const noexcept { return stopflag.load() == 1; }
    void start_thread();
    void stop_thread();

private:
    std::unique_ptr<evg::Device> graphics;
    std::mutex compositor_lock;
    std::vector<VideoDisplay*> displays;
    std::thread video_thread;
    std::atomic<int> stopflag { 0 }, running { 0 };
    static void thread_entry (Video& video);
};

}
