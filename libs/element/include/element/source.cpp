
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <element/graphics.h>
#include <stb/stb_image.h>

#include "../../src/graphics_context.hpp"
#include "source.hpp"

namespace element {

static const char* vert_source = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
})";

static const char* frag_source = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4 (1.0f, 0.5f, 0.2f, 1.0f);
})";

inline static egColorFormat convert_stbi_color (int ncomponents)
{
    switch (ncomponents) {
        case STBI_rgb:
        case STBI_rgb_alpha:
            return EVG_COLOR_FORMAT_RGBA;
            break;
        default:
            break;
    }

    return EVG_COLOR_FORMAT_UNKNOWN;
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
        _data = load_image_data (file, _format, _width, _height);
    }

    const uint8_t* data() const noexcept { return _data; }
    const int width() const noexcept { return static_cast<uint32_t> (_width); }
    const int height() const noexcept { return static_cast<uint32_t> (_height); }
    auto format() const noexcept { return _format; }
    operator const uint8_t*() const noexcept { return _data; }

private:
    egColorFormat _format { EVG_COLOR_FORMAT_UNKNOWN };
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
    ~Impl()
    {
        // texture.reset();
        // data.reset();
    }

    void load_buffers (GraphicsContext& g)
    {
        if (verts != nullptr)
            return;

        verts.reset (g.get_device().create_vertex_buffer (
            evg_vertex_data_new (3, false, false, false, 0, 0),
            EVG_OPT_DYNAMIC));
    }

    void load_program (GraphicsContext& g)
    {
        if (program_linked)
            return;

        if (program == nullptr) {
            vshader.reset (g.reserve_vertex_shader());
            vshader->add_attribute ("aPos", EVG_ATTRIB_POSITION);
            fshader.reset (g.reserve_fragment_shader());
            program.reset (g.reserve_program());
        }

        if (vshader->parse (vert_source))
            if (fshader->parse (frag_source))
                program_linked = program->link (vshader.get(), fshader.get());
    }

    void expose_triangle (GraphicsContext& g)
    {
        auto& device = g.get_device();

        auto pts = verts->points();
        evg_vec3_set (pts,    -1.0f, -1.0f, 0.0);
        evg_vec3_set (pts + 1, 1.0f, -1.0f, 0.0);
        evg_vec3_set (pts + 2, 0.0f,  1.0f, 0.0);
        verts->update();

        device.load_index_buffer (nullptr);
        device.load_vertex_buffer (verts.get());
        device.load_program (program.get());
        device.draw (EVG_DRAW_MODE_TRIANGLES_STRIP, 0, 3);
    }

    // void expose_rectangle ()
private:
    friend class TestVideoSource;
    TestVideoSource& source;
    std::unique_ptr<ImageData> data;
    std::unique_ptr<evg::Texture> texture;
    std::unique_ptr<evg::Program> program;
    std::unique_ptr<evg::Shader> vshader, fshader;
    std::unique_ptr<evg::VertexBuffer> verts;
    std::unique_ptr<evg::IndexBuffer> index;
    bool program_linked = false;
    int ticker = 0;
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
    impl->load_buffers (gc);
    impl->load_program (gc);
    impl->expose_triangle (gc);
#if 0
    if (data_changed)
    {
        auto& d = *impl->data;
        if (nullptr != d.data()) {
            if (impl->texture == nullptr) {
                impl->texture.reset (gc.load_image_data (d, d.format(), d.width(), d.height()));
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

    // gc.draw_sprite (impl->texture, width, height);
#endif
}

} // namespace element
