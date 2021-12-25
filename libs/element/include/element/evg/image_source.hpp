#pragma once

#include <memory>
#include <mutex>

#include "element/evg/source.hpp"

namespace evg {

class Context;

class ImageSource : public Source {
public:
    ImageSource();
    ~ImageSource();
    
    bool load_file (const std::string& file);
    void process_frame();
    void expose (Context& ctx) override;
    std::mutex& render_mutex() { return _render_mutex; }

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    std::mutex _render_mutex;
    bool data_changed = false;
    int width = 0, height = 0;
};

}
