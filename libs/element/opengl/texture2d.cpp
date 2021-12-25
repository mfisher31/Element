
#include "helpers.hpp"

namespace gl {

Texture::Texture (Device& dev, const evgTextureInfo& s)
    : device (dev)
{
    memcpy (&m_setup, &s, sizeof (evgTextureInfo));
    gl_format = gl::color_format (m_setup.format);
    gl_format_internal = gl::color_format_internal (m_setup.format);
    gl_format_type = gl::color_format_type (m_setup.format);
    gl_target = gl::texture_target (m_setup.type);

    dynamic = (m_setup.flags & EVG_DYNAMIC) != 0;
    render_target = (m_setup.flags & EVG_RENDER_TARGET) != 0;
    dummy = (m_setup.flags & EVG_DUMMY) != 0;
    mipmaps = (m_setup.flags & EVG_MIPMAPS) != 0;

    if (gl::gen_textures (1, &texture)) {
        gl::bind_texture (gl_target, texture);
        gl::bind_texture (gl_target, 0);
    }
}

static inline uint32_t evg_nlevels (uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t size = std::max (width, height);
    size = std::max (size, depth);
    uint32_t nlevels = 1;

    while (size > 1) {
        size /= 2;
        ++nlevels;
    }

    return nlevels;
}

static inline uint32_t evg_nlevels_2d (uint32_t width, uint32_t height) {
    return evg_nlevels (width, height, 1);
}

bool Texture2D::bind_data (const uint8_t** data)
{
    uint32_t row_size = width() * evg_color_format_bpp (format());
    uint32_t tex_size = height() * row_size / 8;
    uint32_t num_levels = m_setup.levels;
    bool compressed = evg_color_format_is_compressed (format());
    bool success;

    if (! num_levels)
        num_levels = evg_nlevels_2d (width(), height());

    if (! gl::bind_texture (GL_TEXTURE_2D, texture))
        return false;
    success = gl::init_face (GL_TEXTURE_2D, gl_format_type, num_levels, gl_format, gl_format_internal, 
                                            compressed, width(), height(), tex_size, &data);

    // glTexImage2D (gl_target, num_levels - 1, GL_RGBA, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, *data);
    if (! gl::check_ok ("tex image 2d"))
        success = false;

    if (! gl::tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_levels - 1))
        success = false;
    
    gl::tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl::tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (! gl::bind_texture (GL_TEXTURE_2D, 0))
        success = false;

    return success;
}

bool Texture2D::upload_data (const uint8_t** data)
{
    if (texture == 0) {
        std::cerr << "texture not gnerated yet\n";
        if  (! gl::gen_textures (1, &texture))
            return false;
    }

    if (! is_dummy()) {
        // if (is_dynamic() && ! create_pixel_unpack_buffer (tex))
        //     goto fail;
        if (bind_data (data)) {
            return true;
        }
    } else {
        std::clog << "dummy not supported\n";
        // if (! gl_bind_texture (GL_TEXTURE_2D, tex->base.texture))
        //     goto fail;

        // uint32_t row_size =
        //     tex->width * gs_get_format_bpp (tex->base.format);
        // uint32_t tex_size = tex->height * row_size / 8;
        // bool compressed = gs_is_compressed_format (tex->base.format);
        // bool did_init = gl_init_face (GL_TEXTURE_2D, tex->base.gl_type, 1, tex->base.gl_format, tex->base.gl_internal_format, compressed, tex->width, tex->height, tex_size, NULL);
        // did_init =
        //     gl_tex_param_i (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        // bool did_unbind = gl_bind_texture (GL_TEXTURE_2D, 0);
        // if (! did_init || ! did_unbind)
        //     goto fail;
    }

    return false;
}

}
