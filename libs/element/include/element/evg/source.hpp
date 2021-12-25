#pragma once

#include "element/evg/context.hpp"

namespace evg {

class Source {
public:
    Source();
    virtual ~Source();
    virtual void expose (Context& ctx);
};

}
