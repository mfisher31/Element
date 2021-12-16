
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

bool Stencil::create_buffer()
{
    glGenRenderbuffers (1, &gl_object);
    if (! check_ok ("glGenRenderbuffers"))
        return false;

    if (! bind_renderbuffer (GL_RENDERBUFFER, gl_object))
        return false;

    glRenderbufferStorage (GL_RENDERBUFFER, gl_format, width, height);
    if (! check_ok ("glRenderbufferStorage"))
        return false;

    bind_renderbuffer (GL_RENDERBUFFER, 0);
    return true;
}

void Stencil::prepare_render() const noexcept
{
    glFramebufferRenderbuffer (GL_DRAW_FRAMEBUFFER, gl_attachment, GL_RENDERBUFFER, gl_object);
    check_ok ("Stencil::prepare_render()");
}

evgHandle Stencil::_create (evgHandle device, uint32_t width, uint32_t height, evgStencilFormat format)
{
    std::unique_ptr<Stencil> st (new Stencil());
    st->format = format;
    st->width = width;
    st->height = height;
    st->gl_object = 0;
    st->gl_format = gl::stencil_format (format);
    st->gl_attachment = gl::stencil_attachment (format);
    return st->create_buffer() ? st.release() : nullptr;
}

void Stencil::_destroy (evgHandle sh)
{
    auto st = static_cast<Stencil*> (sh);
    if (st->gl_object != 0) {
        glDeleteRenderbuffers (1, &st->gl_object);
        st->gl_object = 0;
    }
    delete static_cast<Stencil*> (sh);
}

} // namespace gl