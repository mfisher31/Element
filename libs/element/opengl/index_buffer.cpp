
#include "opengl.hpp"
#include "helpers.hpp"

namespace gl {

IndexBuffer::IndexBuffer (uint32_t _size, uint32_t flags) {
    dynamic = (flags & EVG_OPT_DYNAMIC) != 0;
    eao = 0;
    gl::buffer_bind_data (GL_ELEMENT_ARRAY_BUFFER, &eao, setup.size, nullptr, 
        dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

void IndexBuffer::update (void* data) {
    gl::update_buffer (GL_ELEMENT_ARRAY_BUFFER, eao, data, setup.size);
}

}
