
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

void Texture::enable_fbo() noexcept
{
    if (fbo == 0)
        glGenFramebuffers (1, &fbo);
}

void Texture::prepare_render (int side /* Stensil* st */) noexcept
{
    enable_fbo();
    glBindFramebuffer (GL_DRAW_FRAMEBUFFER, fbo);
    check_ok ("glBindFramebuffer");

    if (! bind()) {
        std::clog << "Texture::bind() failed\n";
        return;
    }

    if (m_setup.type == EVG_TEXTURE_2D) {
        glFramebufferTexture2D (GL_DRAW_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D,
                                texture,
                                0);

    } else if (m_setup.type == EVG_TEXTURE_CUBE) {
        glFramebufferTexture2D (GL_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
                                texture,
                                0);
    }

    check_ok ("glFramebufferTexture2D");
}

evgHandle Texture::_create (evgHandle dh, const evgTextureInfo* setup)
{
    if (nullptr == dh)
        return nullptr;

    auto device = static_cast<Device*> (dh);
    std::unique_ptr<Texture> tex;

    switch (setup->type) {
        case EVG_TEXTURE_2D:
            tex = std::make_unique<Texture2D> (*device, *setup);
            break;
        case EVG_TEXTURE_3D:
            // tex = std::make_unique<evgTexture3D> (*device, *setup);
            break;
        case EVG_TEXTURE_CUBE:
            // tex = std::make_unique<evgTextureCube> (*device, *setup);
            break;
    }

    if (tex == nullptr)
        return nullptr;

    return tex.release();
}

void Texture::_destroy (evgHandle t)
{
    std::unique_ptr<Texture> tex ((Texture*) t);
    if (tex == nullptr)
        return;

    switch (tex->type()) {
        case EVG_TEXTURE_2D:
            break;
        case EVG_TEXTURE_3D:
            break;
        case EVG_TEXTURE_CUBE:
            break;
    }

    tex.reset();
}

void Texture::_fill_info (evgHandle th, evgTextureInfo* setup)
{
    auto tex = static_cast<Texture*> (th);
    if (setup != nullptr)
        memcpy (setup, &tex->m_setup, sizeof (evgTextureInfo));
}

void Texture::_update (evgHandle tex, const uint8_t* data)
{
    auto texture = static_cast<Texture*> (tex);
    const uint8_t* d2[] = { data };
    texture->upload (&data);
}

} // namespace gl
