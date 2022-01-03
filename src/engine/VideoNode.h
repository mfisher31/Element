#pragma once

#include "engine/NodeObject.h"
#include "element/evg/context.hpp"

namespace Element {

class VideoContext final {
public:
    VideoContext() = default;
    ~VideoContext() = default;
    kv::Rational getFrameRate() const noexcept { return rate; }

private:
    kv::Rational rate;
};

class VideoNode : public NodeObject {
public:
    VideoNode();
    ~VideoNode();

    virtual void prepareVideo (VideoContext& video) = 0;
    virtual void renderVideo (VideoContext& video) = 0;
    
    bool wantsMidiPipe() const override { return true; }

   #if 0
    virtual void prepareToRender (double sampleRate, int maxBufferSize) = 0;
    virtual void releaseResources() = 0;
    virtual void getState (MemoryBlock&) = 0;
    virtual void setState (const void*, int sizeInBytes) = 0;
   #endif
};

}
