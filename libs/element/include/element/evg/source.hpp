#pragma once

#include "element/evg/context.hpp"

namespace evg {

class EL_API Source {
public:
    Source();
    virtual ~Source();
    virtual void expose (Context& ctx);
};

}
