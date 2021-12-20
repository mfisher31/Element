
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <element/graphics.h>
#include <stb/stb_image.h>

#include "../../src/graphics_context.hpp"
#include "source.hpp"
#include <element/evg/matrix.h>
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
layout (location = 1) in vec2 uv;

out VertInOut {
    vec2 uv;
} vout;

uniform mat4 projection;
uniform float scale = 1.0;
uniform float tx = 0.0;
uniform float ty = 0.0;

void main()
{
    mat4 tr = mat4 (scale, 0.0, 0.0, tx,
                    0.0, scale, 0.0, ty,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0);
    gl_Position = projection * vec4(vec4(pos, 1.0) * tr);
    vout.uv = uv;
}
)";

static const char* image_frag_source = R"(
#version 330 core

out vec4 FragColor;
in VertInOut {
    vec2 uv;
} vin;

uniform sampler2D image;

void main()
{
	FragColor = texture(image, vin.uv);
}
)";

inline static evgColorFormat convert_stbi_color (int ncomponents)
{
    switch (ncomponents) {
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
                                 evgColorFormat& format,
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

    const uint8_t* data()   const noexcept { return p_data; }
    operator const uint8_t*() const noexcept { return p_data; }
    const int width()       const noexcept { return m_width; }
    const int height()      const noexcept { return m_height; }
    auto format()           const noexcept { return m_format; }

private:
    evg::ColorFormat m_format { EVG_COLOR_FORMAT_UNKNOWN };
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

class TestVideoSource::Impl {
public:
    Impl (TestVideoSource& s) : source (s) { }
    ~Impl() {}

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
            vshader->add_attribute ("pos", EVG_VALUE_VEC3);
            vshader->add_attribute ("uv", EVG_VALUE_VEC2);

            fshader.reset (g.reserve_fragment_shader());
            fshader->add_texture ("image");
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
        device.load_vertex_buffer (verts.get(), 0);
        device.load_vertex_buffer (uv.get(), 1);
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
            vshader->add_attribute ("pos", EVG_VALUE_VEC3);
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
        device.load_vertex_buffer (verts.get(), 0);
        device.load_vertex_buffer (uv.get(), 1);
        device.load_program (program.get());
        device.draw (EVG_DRAW_MODE_TRIANGLES, 0, 6);
    }

    //=====
    void load_image_buffers (GraphicsContext& g)
    {
        if (verts != nullptr)
            return;

        verts.reset (g.get_device().create_vertex_buffer (
            sizeof (evgVec3) * 4, EVG_OPT_DYNAMIC));
        uv.reset (g.get_device().create_vertex_buffer (
            sizeof (evgVec2) * 4, EVG_OPT_DYNAMIC));
        index.reset (g.get_device().create_index_buffer (
            sizeof (uint32_t) * 6, EVG_OPT_DYNAMIC));
    }

    void load_image_program (GraphicsContext& g)
    {
        if (program_linked)
            return;

        if (program == nullptr) {
            vshader.reset (g.reserve_vertex_shader());
            vshader->add_attribute ("pos", EVG_VALUE_VEC3);
            vshader->add_attribute ("uv", EVG_VALUE_VEC2);
            vshader->add_uniform ("projection", EVG_VALUE_MAT4X4);
            vshader->add_uniform ("scale", EVG_VALUE_FLOAT);
            vshader->add_uniform ("tx", EVG_VALUE_FLOAT);
            vshader->add_uniform ("ty", EVG_VALUE_FLOAT);

            fshader.reset (g.reserve_fragment_shader());
            fshader->add_texture ("image");

            program.reset (g.reserve_program());
        }

        if (vshader->parse (image_vert_source))
            if (fshader->parse (image_frag_source))
                program_linked = program->link (vshader.get(), fshader.get());

        int image_key = -1;
        if (program_linked) {
            uint32_t index = 0;
            auto res = program->resource (index);
            while (res != nullptr) {
                std::clog << "Resource: " << res->symbol << " " << (int64_t) res->key << std::endl;
                if (strcmp (res->symbol, "image") == 0) {
                    image_key = (int) res->key;
                }
                if (strcmp (res->symbol, "projection") == 0) {
                    proj_key = (int) res->key;
                }
                res = program->resource (++index);
            }

            if (image_key >= 0) {
                int slot = 1;
                program->set_value (image_key, sizeof (int), &slot);
            }
        }
    }

    void ortho (evgMatrix4* dst, float left, float right, 
                                 float bottom, float top, 
                                 float near, float far)
    {
        float rml = right - left;
        float bmt = bottom - top;
        float fmn = far - near;

        evg_vec4_reset (&dst->x);
        evg_vec4_reset (&dst->y);
        evg_vec4_reset (&dst->z);
        evg_vec4_reset (&dst->t);

        dst->x.x = 2.0f / rml;
        dst->t.x = (left + right) / -rml;

        dst->y.y = 2.0f / -bmt;
        dst->t.y = (bottom + top) / bmt;

        dst->z.z = -2.0f / fmn;
        dst->t.z = (far + near) / -fmn;

        dst->t.w = 1.0f;
    }

    void expose_image (GraphicsContext& g)
    {
        if (! program)
            return;

        if (verts_changed) {
            verts_changed = false;
        }

        ortho (&projection, 0.0f, 640.0, 0.0f, 360.0, -100.0, 100.0);
        
        if (true) {
            program->set_value ("scale", 1.0f);
            program->set_value ("tx", 0.f); //320.0f - 180.0f);
            program->set_value ("ty", 0.0f);
            program->set_value ("projection", projection);
        }

        // std::clog << "w=" << texture->width() << " h=" << texture->height() << std::endl;
        auto& device = g.get_device();
        device.load_texture (texture.get(), 1);
        device.load_program (program.get());
        g.draw_sprite (*texture);

        // render to temporary texture
        // device.load_target (target.get());
        // device.load_stencil (stencil.get());
        // device.viewport (0, 0, texture->width(), texture->height());
        // device.draw (EVG_TRIANGLE_STRIP, 0, 4);
        // device.flush();

        // // render temp to the display
        // device.load_stencil (nullptr);
        // device.load_target (nullptr);
        // device.load_texture (target.get(), 1);
        // device.draw (EVG_TRIANGLE_STRIP, 0, 4);
        // device.flush();

        clear_state (device);
        device.viewport (0, 0, 640, 360);
    }

    void clear_state (evg::Device& device) {
        device.load_index_buffer (nullptr); //index.get());
        device.load_vertex_buffer (nullptr, 0);
        device.load_vertex_buffer (nullptr, 1);
        device.load_texture (nullptr, 0);
        device.load_texture (nullptr, 1);
        device.load_program (nullptr);
        device.load_target (nullptr);
        device.load_stencil (nullptr);
    }

    void load_texture (GraphicsContext& gc)
    {
        if (texture)
            return;
        texture.reset (gc.load_image_data (*data, data->format(), data->width(), data->height()));
        target.reset (gc.get_device().create_2d_texture (data->format(), data->width(), data->height()));
        target->update (nullptr);
        stencil.reset (gc.get_device().create_stencil (
            data->width(), data->height(), EVG_STENCIL_24_S8));

        if (texture) {
            std::clog << "loaded " << evg_color_format_string(data->format()) << " image\n";
        } else {
            std::clog << "image load failed\n";
        }

        data.reset();
    }

private:
    friend class TestVideoSource;
    TestVideoSource& source;
    std::unique_ptr<ImageData> data;
    std::unique_ptr<evg::Texture> texture;
    std::unique_ptr<evg::Program> program;
    std::unique_ptr<evg::Shader> vshader, fshader;
    std::unique_ptr<evg::Buffer> verts;
    std::unique_ptr<evg::Buffer> uv;
    std::unique_ptr<evg::Buffer> index;
    std::unique_ptr<evg::Stencil> stencil;
    std::unique_ptr<evg::Texture> target;
    evgMatrix4 projection, sm;
    int proj_key = -1;

    bool verts_changed = true;
    bool program_linked = false;

    bool ticker_enabled = true;
    int ticker = 0;
    float scale = .72f;
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
    if (! impl->ticker_enabled)
        return;

    if (++impl->ticker == 2) {
        if (impl->scale >= 2.f) {
            impl->interval = impl->abs_interval * -1.0f;
        } else if (impl->scale <= 0.5) {
            impl->interval = impl->abs_interval * 1.0f;
        }
std::clog << "tick " << impl->scale << std::endl;
        impl->verts_changed = true;
        impl->scale += impl->interval;
        impl->ticker = 0;
        
    }
}

void TestVideoSource::expose_frame (GraphicsContext& gc)
{
    gc.get_device().enable (EVG_FRAMEBUFFER_SRGB, true);
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
    gc.get_device().enable (EVG_FRAMEBUFFER_SRGB, false);
}

} // namespace element
