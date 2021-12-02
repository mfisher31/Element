#pragma once

#include <element/graphics.h>

namespace element {
class GraphicsDevice;
class GraphicsContext {
public:
    explicit GraphicsContext (GraphicsDevice&);
    ~GraphicsContext();

    void draw_sprite (const egTexture* tex, int width, int height);
    egTexture* load_image_data (const uint8_t*, egColorFormat format, int width, int height);

private:
    GraphicsDevice& device;
};
}
