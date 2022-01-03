
#include "engine/nodes/ImageNode.h"
#include "engine/nodes/NodeTypes.h"

#include "PortCount.h"

namespace Element {

ImageNode::ImageNode()
{
    auto ports = PortCount()
        .with (PortType::Video, 0, 1)
        .toPortList();
    setPorts (ports);
}

ImageNode::~ImageNode()
{
}

void ImageNode::getPluginDescription (PluginDescription& desc) const
{
    desc.name             = "Image";
    desc.descriptiveName  = "Single image display.";
    desc.manufacturerName = "Kushview";
    desc.fileOrIdentifier = EL_INTERNAL_ID_IMAGE;
    desc.uniqueId         = EL_INTERNAL_UID_IMAGE;
    desc.pluginFormatName = EL_INTERNAL_FORMAT_NAME;
}

void ImageNode::prepareVideo (VideoContext& video)
{
}

void ImageNode::renderVideo (VideoContext& video)
{
}

void ImageNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    ignoreUnused (sampleRate, maxBufferSize);
}

void ImageNode::releaseResources() {}

void ImageNode::getState (MemoryBlock&) {}

void ImageNode::setState (const void* data, int size)
{
    ignoreUnused (data, size);
}

//=============================================================================
VideoOutputNode::VideoOutputNode() {
    auto ports = PortCount()
        .with (PortType::Video, 1, 0)
        .toPortList();
    setPorts (ports);
}

VideoOutputNode::~VideoOutputNode() { }

void VideoOutputNode::getPluginDescription (PluginDescription& desc) const {
    desc.name = "Video Output";
    desc.descriptiveName = "Video Output";
    desc.manufacturerName = "Kushview";
    desc.fileOrIdentifier = "el.videoOutput"; //EL_INTERNAL_ID_IMAGE;
    desc.uniqueId         = 1122; // EL_INTERNAL_UID_IMAGE;
    desc.pluginFormatName = EL_INTERNAL_FORMAT_NAME;
}

void VideoOutputNode::prepareVideo (VideoContext& video) {}

void VideoOutputNode::renderVideo (VideoContext& video) {}

void VideoOutputNode::prepareToRender (double sampleRate, int maxBufferSize) {}

void VideoOutputNode::releaseResources() {}

void VideoOutputNode::getState (MemoryBlock&) {}

void VideoOutputNode::setState (const void*, int sizeInBytes) {}

} // namespace Element
