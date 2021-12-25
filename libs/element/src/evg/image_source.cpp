
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <stb/stb_image.h>

#include "element/evg/context.hpp"
#include "element/evg/image_source.hpp"

namespace evg {

inline static ColorFormat convert_stbi_color (int num_components)
{
    switch (num_components) {
        case STBI_rgb:
            return EVG_COLOR_FORMAT_RGBA;
            break;
        case STBI_rgb_alpha:
            return EVG_COLOR_FORMAT_RGBA;
            break;
        default:
            break;
    }

    return EVG_COLOR_FORMAT_UNKNOWN;
}

static stbi_uc* load_image_data (const std::string& filename,
                                 ColorFormat& format,
                                 int& width,
                                 int& height)
{
    int w, h;
    int ncomponents;
    stbi_set_flip_vertically_on_load (true);
    auto image = stbi_load (filename.c_str(), &w, &h, &ncomponents, STBI_rgb_alpha);

    if (image == nullptr) {
        width = height = 0;
        format = EVG_COLOR_FORMAT_UNKNOWN;
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
        p_data = load_image_data (file, m_format, m_width, m_height);
    }

    const uint8_t* data() const noexcept { return p_data; }
    operator const uint8_t*() const noexcept { return p_data; }
    const int width() const noexcept { return m_width; }
    const int height() const noexcept { return m_height; }
    auto format() const noexcept { return m_format; }

private:
    ColorFormat m_format { EVG_COLOR_FORMAT_UNKNOWN };
    uint8_t* p_data { nullptr };
    int m_width { 0 },
        m_height { 0 };

    void free_data()
    {
        if (p_data)
            STBI_FREE (p_data);
        p_data = nullptr;
    }
};

//=============================================================================
class ImageSource::Impl {
public:
    Impl (ImageSource& s)
        : source (s) {}
    ~Impl() {}

    void expose_image (evg::Context& ctx)
    {
        
    }

    void load_texture (evg::Context& ctx)
    {
        if (! data)
            return;
        
        texture.reset (ctx.load_image_data (*data, data->format(), data->width(), data->height()));

        if (texture) {
            std::clog << "loaded " << evg_color_format_string (data->format()) << " image\n";
        } else {
            std::clog << "image load failed\n";
        }

        data.reset();
    }

private:
    friend class ImageSource;
    ImageSource& source;
    std::unique_ptr<ImageData> data;
    std::unique_ptr<evg::Texture> texture;
};

ImageSource::ImageSource()
{
    impl.reset (new Impl (*this));
}

ImageSource::~ImageSource()
{
    impl.reset();
}

bool ImageSource::load_file (const std::string& file)
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

void ImageSource::process_frame()
{
    // noop
}

void ImageSource::expose (evg::Context& g)
{
    g.get_device().enable (EVG_FRAMEBUFFER_SRGB, true);

    if (data_changed) {
        impl->load_texture (g);
        data_changed = false;
    }

    if (true) {
        auto& prog = g.default_program();
        prog.set_value ("scale", 1.0f);
        prog.set_value ("tx", 0.0f);
        prog.set_value ("ty", 0.0f);
    }

    g.draw_sprite (*impl->texture);
    g.get_device().enable (EVG_FRAMEBUFFER_SRGB, false);
}

} // namespace evg