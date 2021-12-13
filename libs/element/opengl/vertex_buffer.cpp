
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

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
        std::cerr << "already have buffer\n";
        return true;
    }

    if (target == 0) {
        std::cerr << "target is zero\n";
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
    
    if (info.type == EVG_BUFFER_ARRAY)
    {
        if (! gl::gen_vertex_arrays (1, &vao))
            result = false;
    }

// glBindBuffer (target, buffer);
//     auto error = glGetError();
//     if (error != GL_NO_ERROR) {
//         std::clog << "glBIND BUFFER ERROR\n";
//         std::clog << error_string (error) << " target=" << (int)target << " buffer=" << (int)buffer << std::endl;
//         return false;
//     }

    return result;
}

void Buffer::update (uint32_t size, const void* data)
{
    // if (info.type == EVG_BUFFER_INDEX) {
    //     std::clog << "size=" << (int)size << std::endl;
    //     for (int i = 0; i < 6; ++i) {
    //         std::clog << "  val " << i << " = " << (int) ((uint32_t*)data)[i] << std::endl;
    //     }
    // }
    gl::update_buffer (target, buffer, data, size);
}

bool Buffer::destroy_buffers()
{
    if (buffer != 0)
        glDeleteBuffers (1, &buffer);
    if (vao != 0)
        glDeleteVertexArrays (1, &vao);

    buffer = vao = 0;
    return true;
}

} // namespace gl
