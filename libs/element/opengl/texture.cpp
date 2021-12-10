
#include "opengl.hpp"
#include "helpers.hpp"

namespace gl {

evgHandle Texture::_create (evgHandle dh, const evgTextureSetup* setup, const uint8_t** data)
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

    tex->upload (data);
    return tex->has_uploaded() ? tex.release() : nullptr;
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

void Texture::_fill_setup (evgHandle th, evgTextureSetup* setup)
{
    if (th != nullptr && setup != nullptr)
        (static_cast<Texture*>(th))->fill_setup (setup);
}

} // namespace gl