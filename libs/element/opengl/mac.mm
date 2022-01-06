
#include "opengl.hpp"
#include <OpenGL/OpenGL.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

namespace gl {

class MacSwap : public Swap {
public:
};

class MacPlatform : public Platform
{
public:
    MacPlatform() = default;

    bool initialize() override {
        return true;
    }

    //=========================================================================
    Swap* create_swap (const evgSwapInfo* setup) override {
        return nullptr;
    }

    void load_swap (const Swap* swap) override {

    }

    void swap_buffers() override {

    }

    //=========================================================================
    void* context_handle() const noexcept override { return nullptr; }
    void enter_context() override {}
    void leave_context() override {}
    void clear_context() override {}
};

Platform* create_platform()
{
    auto mp = new MacPlatform();
    return mp;    
}

void destroy_platform (Platform* p)
{
    delete dynamic_cast<MacPlatform*> (p);
}

}
