
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

//==
Buffer::Buffer (evgBufferType btype, uint32_t capacity, uint32_t flags)
{
    info.type = btype;
    info.capacity = capacity;
    info.size = capacity;
    dynamic = (flags & EVG_OPT_DYNAMIC) != 0;
    
    switch (info.type) {
        case EVG_BUFFER_ARRAY:
            target = GL_ARRAY_BUFFER;
            break;
        case EVG_BUFFER_INDEX:
            target = GL_ELEMENT_ARRAY_BUFFER;
            break;
    }
}

Buffer::~Buffer()
{
    destroy_buffers();
}

bool Buffer::create_buffers()
{
    if (buffer != 0) {
        return true;
    }

    if (target == 0) {
        return false;
    }

    if (! gl::gen_buffers (1, &buffer))
        return false;

    if (! gl::bind_buffer (target, buffer))
        return false;

    bool result = true;
    glBufferData (target, info.capacity, nullptr, 
        is_dynamic() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    if (! check_ok ("gl::Buffer::create_buffers()"))
        result = false;
    if (! gl::bind_buffer (target, 0))
        result = false;
    
    return result;
}

void Buffer::update (uint32_t size, const void* data)
{
    gl::update_buffer (target, buffer, data, size);
}

bool Buffer::destroy_buffers()
{
    if (buffer != 0) {
        glDeleteBuffers (1, &buffer);
        buffer = 0;
    }

    return true;
}

} // namespace gl
