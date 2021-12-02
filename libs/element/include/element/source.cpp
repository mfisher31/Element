
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <element/graphics.h>
#include <stb/stb_image.h>

#include "../../src/graphics_context.hpp"
#include "source.hpp"

namespace element {

inline static egColorFormat convert_stbi_color (int ncomponents)
{
    switch (ncomponents) {
        case STBI_rgb:
        case STBI_rgb_alpha:
            return EL_COLOR_FORMAT_RGBA;
            break;
        default:
            break;
    }

    return EL_COLOR_FORMAT_UNKNOWN;
}

static stbi_uc* load_image_data (const std::string& filename,
                                 egColorFormat& format,
                                 int& width,
                                 int& height)
{
    int w, h;
    int ncomponents;
    auto image = stbi_load (filename.c_str(), &w, &h, &ncomponents, STBI_rgb_alpha);

    if (image == nullptr) {
        width = height = 0;
        format = EL_COLOR_FORMAT_UNKNOWN;
        return nullptr;
    }

    width = w;
    height = h;
    format = convert_stbi_color (ncomponents);
    return image;
}

struct ImageData {
    explicit ImageData() {}
    ~ImageData() { free_data(); }

    void load_file (const char* file)
    {
        free_data();
        _data = load_image_data (file, _format, _width, _height);
    }

    const uint8_t* data() const noexcept { return _data; }
    const int width() const noexcept { return static_cast<uint32_t> (_width); }
    const int height() const noexcept { return static_cast<uint32_t> (_height); }
    auto format() const noexcept { return _format; }
    operator const uint8_t*() const noexcept { return _data; }

private:
    egColorFormat _format { EL_COLOR_FORMAT_UNKNOWN };
    stbi_uc* _data { nullptr };
    int _width { 0 },
        _height { 0 };

    void free_data()
    {
        if (_data)
            STBI_FREE (_data);
        _data = nullptr;
    }
};

class TestVideoSource::Impl {
public:
    Impl (TestVideoSource& s) : source (s) {}
    ~Impl() { data.reset(); }

private:
    friend class TestVideoSource;
    TestVideoSource& source;
    std::unique_ptr<ImageData> data;
    egTexture* texture { nullptr };
};

TestVideoSource::TestVideoSource()
{
    impl.reset (new Impl (*this));
}

TestVideoSource::~TestVideoSource()
{
    impl.reset();
}

bool TestVideoSource::load_file (const std::string& file)
{
    auto newdata = std::make_unique<ImageData>();
    newdata->load_file (file.c_str());
    bool result = false;
    if (newdata->data() != nullptr) {
        std::scoped_lock sl (render_mutex());
        std::swap (impl->data, newdata);
        data_changed = true;
        result = true;
    }
    newdata.reset();
    return result;
}

void TestVideoSource::process_frame()
{
}

void TestVideoSource::expose_frame (GraphicsContext& gc)
{
    if (data_changed)
    {
        auto& d = *impl->data;
        if (nullptr != d.data()) {
            if (impl->texture == nullptr) {
                impl->texture = gc.load_image_data (d, d.format(), d.width(), d.height());
                if (impl->texture) {
                    std::clog << "loaded image\n";
                } else {
                    std::clog << "image load failed\n";
                }
            }
        }
        data_changed = false;
    }

    if (! impl->texture)
        return;

    gc.draw_sprite (impl->texture, width, height);
}

} // namespace element
