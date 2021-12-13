
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <element/graphics.h>
#include <stb/stb_image.h>

#include "../../src/graphics_context.hpp"
#include "source.hpp"

namespace element {

static const char* vert_source = R"(
#version 330 core
layout (location = 0) in vec3 pos;
void main() {
    gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
})";

static const char* frag_source = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4 (1.0f, 0.5f, 0.2f, 1.0f);
}
)";

//============================

static const char* image_vert_source = R"(
#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4 (pos, 1.0);
    TexCoord = vec2 (aTexCoord.x, aTexCoord.y);
}
)";

static const char* image_frag_source = R"(
#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, TexCoord);
}
)";

inline static evgColorFormat convert_stbi_color (int ncomponents)
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
                                 evgColorFormat& format,
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
    evgColorFormat _format { EVG_COLOR_FORMAT_UNKNOWN };
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
            sizeof (evgVec3) * 3, EVG_OPT_DYNAMIC));

        if (! verts) {
            std::cerr << "array buffer failed to allocate\n";
        }
    }

    void load_program (GraphicsContext& g)
    {
        if (program_linked)
            return;

        if (program == nullptr) {
            vshader.reset (g.reserve_vertex_shader());
            vshader->add_attribute ("pos", EVG_ATTRIB_POSITION);
            fshader.reset (g.reserve_fragment_shader());
            program.reset (g.reserve_program());
        }

        if (vshader->parse (vert_source))
            if (fshader->parse (frag_source))
                program_linked = program->link (vshader.get(), fshader.get());
    }

    void expose_triangle (GraphicsContext& g)
    {
        if (! verts)
            return;

        auto& device = g.get_device();
        auto pts = (evgVec3*) verts->data();
        evg_vec3_set (pts, -1.0f, -1.0f, 0.0);
        evg_vec3_set (pts + 1, 1.0f, -1.0f, 0.0);
        evg_vec3_set (pts + 2, 0.0f, 1.0f, 0.0);
        verts->flush();

        device.load_index_buffer (nullptr);
        device.load_vertex_buffer (verts.get());
        device.load_program (program.get());
        device.draw (EVG_TRIANGLE_STRIP, 0, 3);
    }

    //======
    void load_square_buffers (GraphicsContext& g)
    {
        if (verts != nullptr)
            return;

        verts.reset (g.get_device().create_vertex_buffer (
            sizeof (evgVec3) * 4, EVG_OPT_DYNAMIC));

        index.reset (g.get_device().create_index_buffer (
            sizeof (uint32_t) * 6, EVG_OPT_DYNAMIC));

        if (! verts) {
            std::cerr << "array buffer failed to allocate\n";
        }
    }

    void load_square_program (GraphicsContext& g)
    {
        if (program_linked)
            return;

        if (program == nullptr) {
            vshader.reset (g.reserve_vertex_shader());
            vshader->add_attribute ("pos", EVG_ATTRIB_POSITION);
            fshader.reset (g.reserve_fragment_shader());
            program.reset (g.reserve_program());
        }

        if (vshader->parse (vert_source))
            if (fshader->parse (frag_source))
                program_linked = program->link (vshader.get(), fshader.get());
    }

    void expose_square (GraphicsContext& g)
    {
        if (! verts || ! index)
            return;

        auto& device = g.get_device();
        auto pts = (evgVec3*) verts->data();
        evg_vec3_set (pts, scale * 0.5f, scale * 0.5f, 0.0f);
        evg_vec3_set (pts + 1, scale * 0.5f, scale * -0.5f, 0.0f);
        evg_vec3_set (pts + 2, scale * -0.5f, scale * -0.5f, 0.0f);
        evg_vec3_set (pts + 3, scale * -0.5f, scale * 0.5f, 0.0f);
        verts->flush();

        auto idx = (uint32_t*) index->data();
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 3;
        idx[3] = 1;
        idx[4] = 2;
        idx[5] = 3;
        index->flush();

        device.load_texture (nullptr, 0);
        device.load_index_buffer (index.get());
        device.load_vertex_buffer (verts.get());
        device.load_program (program.get());
        device.draw (EVG_DRAW_MODE_TRIANGLES, 0, 6);
    }

    //=====
    void load_image_buffers (GraphicsContext& g)
    {
        if (verts != nullptr)
            return;

        uint32_t vsize = sizeof (evgVec3);
        vsize += sizeof (evgVec2);

        verts.reset (g.get_device().create_vertex_buffer (
            vsize * 4, EVG_OPT_DYNAMIC));

        index.reset (g.get_device().create_index_buffer (
            sizeof (uint32_t) * 6, EVG_OPT_DYNAMIC));
    }

    void load_image_program (GraphicsContext& g)
    {
        if (program_linked)
            return;

        if (program == nullptr) {
            vshader.reset (g.reserve_vertex_shader());
            vshader->add_attribute ("pos", EVG_ATTRIB_POSITION);
            vshader->add_attribute ("aTexCoord", EVG_ATTRIB_TEXCOORD);
            fshader.reset (g.reserve_fragment_shader());
            program.reset (g.reserve_program());
        }

        if (vshader->parse (image_vert_source))
            if (fshader->parse (image_frag_source))
                program_linked = program->link (vshader.get(), fshader.get());
    }

    void expose_image (GraphicsContext& g)
    {
        if (! verts || ! index || ! texture)
            return;

        auto& device = g.get_device();

        auto data = (uint8_t*) verts->data();

        if (verts_changed) {
            verts_changed = false;
            evg_vec3_set ((evgVec3*) data, scale * 0.5f, scale * 0.5f, 0.0f);
            data += sizeof (evgVec3);
            evg_vec2_set ((evgVec2*) data, 1.0, 1.0);
            data += sizeof (evgVec2);

            evg_vec3_set ((evgVec3*) data, scale * 0.5f, scale * -0.5f, 0.0f);
            data += sizeof (evgVec3);
            evg_vec2_set ((evgVec2*) data, 1.0, 0.0);
            data += sizeof (evgVec2);

            evg_vec3_set ((evgVec3*) data, scale * -0.5f, scale * -0.5f, 0.0f);
            data += sizeof (evgVec3);
            evg_vec2_set ((evgVec2*) data, 0.0, 0.0);
            data += sizeof (evgVec2);

            evg_vec3_set ((evgVec3*) data, scale * -0.5f, scale * 0.5f, 0.0f);
            data += sizeof (evgVec3);
            evg_vec2_set ((evgVec2*) data, 0.0, 1.0);
            data += sizeof (evgVec2);
            verts->flush();

            auto idx = (uint32_t*) index->data();
            idx[0] = 0;
            idx[1] = 1;
            idx[2] = 3;
            idx[3] = 1;
            idx[4] = 2;
            idx[5] = 3;
            index->flush();
        }

        device.load_index_buffer (index.get());
        device.load_vertex_buffer (verts.get());
        device.load_texture (nullptr, 0);
        device.load_texture (texture.get(), 0);
        device.load_program (program.get());
        device.draw (EVG_DRAW_MODE_TRIANGLES, 0, 6);
    }

    void load_texture (GraphicsContext& gc)
    {
        texture.reset (gc.load_image_data (data->data(), data->format(), data->width(), data->height()));
        if (texture) {
            std::clog << "loaded image\n";
        } else {
            std::clog << "image load failed\n";
        }
    }

private:
    friend class TestVideoSource;
    TestVideoSource& source;
    std::unique_ptr<ImageData> data;
    std::unique_ptr<evg::Texture> texture;
    std::unique_ptr<evg::Program> program;
    std::unique_ptr<evg::Shader> vshader, fshader;
    std::unique_ptr<evg::Buffer> verts;
    std::unique_ptr<evg::Buffer> index;
    bool verts_changed = true;
    bool program_linked = false;

    int ticker = 0;
    float scale = 1.f;
    float interval = 0.05f;
    float abs_interval = 0.05f;
};

TestVideoSource::TestVideoSource (ObjectMode m)
    : mode (m)
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
    if (++impl->ticker == 1) {
        if (impl->scale >= 2.f) {
            impl->interval = impl->abs_interval * -1.0f;
        } else if (impl->scale <= 0.5) {
            impl->interval = impl->abs_interval * 1.0f;
        }

        impl->verts_changed = true;
        impl->scale += impl->interval;
        impl->ticker = 0;
    }
}

void TestVideoSource::expose_frame (GraphicsContext& gc)
{
    switch (mode) {
        case Image: {
            if (data_changed) {
                impl->load_texture (gc);
                data_changed = false;
            }
            impl->load_image_buffers (gc);
            impl->load_image_program (gc);
            impl->expose_image (gc);
        } break;

        case Square: {
            impl->load_square_buffers (gc);
            impl->load_square_program (gc);
            impl->expose_square (gc);
        } break;

        case Triangle: {
            impl->load_buffers (gc);
            impl->load_program (gc);
            impl->expose_triangle (gc);
        } break;
    }
}

} // namespace element
