
#pragma once

#include <element/evg/image_source.hpp>

#include "engine/VideoNode.h"

namespace Element {

class ImageNode : public VideoNode {
public:
    ImageNode();
    ~ImageNode();
    
    void getPluginDescription (PluginDescription& desc) const override;

    void prepareVideo (VideoContext& video) override;
    void renderVideo (VideoContext& video) override;
   
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void getState (MemoryBlock&) override;
    void setState (const void*, int sizeInBytes) override;

private:
    evg::ImageSource source;
};

class VideoOutputNode : public VideoNode {
public:
    VideoOutputNode();
    ~VideoOutputNode();
    
    void getPluginDescription (PluginDescription& desc) const override;

    void prepareVideo (VideoContext& video) override;
    void renderVideo (VideoContext& video) override;
   
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void getState (MemoryBlock&) override;
    void setState (const void*, int sizeInBytes) override;
};

}
