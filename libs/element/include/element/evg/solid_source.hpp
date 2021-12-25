
#pragma once

#include <memory>
#include <mutex>

#include "element/evg/source.hpp"

namespace evg {

class SolidSource : public Source {
public:
    SolidSource();
    ~SolidSource();
    void process_frame();
    std::mutex& render_mutex() { return _render_mutex; }

    void expose (Context& gc) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    std::mutex _render_mutex;
};

} // namespace evg
