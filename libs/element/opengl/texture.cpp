
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

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
        memcpy (setup, &tex->_setup, sizeof (evgTextureInfo));
}

void Texture::_update (evgHandle tex, const uint8_t* data) {
    auto texture = static_cast<Texture*> (tex);
    const uint8_t* d2[] = { data };
    texture->upload (&data);
}

} // namespace gl
