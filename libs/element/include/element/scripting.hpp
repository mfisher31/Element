
#pragma once

#include <memory>
#include <element/scripting.h>

namespace element {

class EL_API Scripting final {
public:
    Scripting();
    ~Scripting();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

}
