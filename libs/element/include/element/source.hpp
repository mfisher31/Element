
#pragma once

#include <string>
#include <memory>
#include <mutex>

namespace element {
class GraphicsContext;
class TestVideoSource {
public:
    enum ObjectMode {
        Triangle,
        Square,
        Image
    };

    explicit TestVideoSource (ObjectMode m = Image);
    ~TestVideoSource();
    
    bool load_file (const std::string& file);
    void process_frame();

    void expose_frame (GraphicsContext& gc);
    std::mutex& render_mutex() { return _render_mutex; }

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    std::mutex _render_mutex;
    bool data_changed = false;
    int width = 0, height = 0;
    ObjectMode mode;
};

}
