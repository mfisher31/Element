#pragma once

#include "opengl.hpp"

namespace gl {

static inline GLenum color_format (evgColorFormat input)
{
    switch (input) {
        case EVG_COLOR_FORMAT_BGRA:
        case EVG_COLOR_FORMAT_BGRX:
            return GL_BGRA;

        case EVG_COLOR_FORMAT_RGBA:
            return GL_RGBA;

        case EVG_COLOR_FORMAT_UNKNOWN:
            break;
    }
    return 0;
};

static inline GLenum color_format_internal (evgColorFormat input)
{
    switch (input) {
        case EVG_COLOR_FORMAT_BGRA:
            return GL_SRGB8_ALPHA8;
        case EVG_COLOR_FORMAT_BGRX:
            return GL_SRGB8;
        case EVG_COLOR_FORMAT_RGBA:
            return GL_SRGB8_ALPHA8;
        case EVG_COLOR_FORMAT_UNKNOWN:
            break;
    }
    return 0;
};

static inline GLenum color_format_type (evgColorFormat input)
{
    switch (input) {
        case EVG_COLOR_FORMAT_BGRA:
        case EVG_COLOR_FORMAT_BGRX:
        case EVG_COLOR_FORMAT_RGBA:
            return GL_UNSIGNED_BYTE;

        case EVG_COLOR_FORMAT_UNKNOWN:
            break;
    }
    return 0;
};

static inline GLenum texture_target (evgTextureType input) {
    switch (input) {
        case EVG_TEXTURE_2D:    return GL_TEXTURE_2D; break;
        case EVG_TEXTURE_3D:    return GL_TEXTURE_3D; break;
        case EVG_TEXTURE_CUBE:  return GL_TEXTURE_CUBE_MAP; break;
    };
    return GL_INVALID_ENUM;
}

static inline GLenum topology (evgDrawMode input) {
    switch (input) {
        case EVG_DRAW_MODE_POINTS: return GL_POINTS;
        case EVG_DRAW_MODE_LINES: return GL_LINES;
        case EVG_DRAW_MODE_LINES_STRIP: return GL_LINE_STRIP;
        case EVG_DRAW_MODE_TRIANGLES: return GL_TRIANGLES;
        case EVG_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    }
    return GL_INVALID_ENUM;
}

static inline GLenum shader_type (evgShaderType input)
{
    switch (input) {
        case EVG_SHADER_VERTEX:
            return GL_VERTEX_SHADER;
        case EVG_SHADER_FRAGMENT:
            return GL_FRAGMENT_SHADER;
        default:
            break;
    }
    return GL_INVALID_ENUM;
}

static inline GLenum attribute_data_type (evgAttributeType input) {
    switch (input) {
        case EVG_ATTRIB_POSITION:
            return GL_FLOAT;
        default: break;
    }

    return GL_INVALID_ENUM;
}

template <typename... Tps>
static inline void unused (Tps&&...) noexcept
{
}

static inline const char* error_string (GLenum code)
{
    static const struct {
        GLenum code;
        const char* message;
    } errmap[] = {
        {
            GL_INVALID_ENUM,
            "GL_INVALID_ENUM",
        },
        {
            GL_INVALID_VALUE,
            "GL_INVALID_VALUE",
        },
        {
            GL_INVALID_OPERATION,
            "GL_INVALID_OPERATION",
        },
        {
            GL_INVALID_FRAMEBUFFER_OPERATION,
            "GL_INVALID_FRAMEBUFFER_OPERATION",
        },
        {
            GL_OUT_OF_MEMORY,
            "GL_OUT_OF_MEMORY",
        },
        {
            GL_STACK_UNDERFLOW,
            "GL_STACK_UNDERFLOW",
        },
        {
            GL_STACK_OVERFLOW,
            "GL_STACK_OVERFLOW",
        },
    };
    for (size_t i = 0; i < sizeof (errmap) / sizeof (*errmap); i++) {
        if (errmap[i].code == code)
            return errmap[i].message;
    }
    return "Unknown";
}

static inline bool check_ok (const char* fn)
{
    auto err = glGetError();
    if (err == GL_NO_ERROR)
        return true;

    int retries = 8;
    do {
        fprintf (stderr, "%s failed, glGetError returned %s(0x%X)\n", fn, error_string (err), err);
        err = glGetError();

        if (--retries == 0) {
            fprintf (stderr, "Too many GL errors, moving on\n");
            break;
        }
    } while (err != GL_NO_ERROR);
    return false;
}

static inline bool gen_textures (GLsizei count, GLuint* textures)
{
    glGenTextures (count, textures);
    return check_ok ("glGenTextures");
}

static inline bool bind_texture (GLenum target, GLuint texture)
{
    glBindTexture (target, texture);
    return check_ok ("glBindTexture");
}

static inline bool delete_textures (GLsizei nbufs, GLuint* buffers)
{
    glDeleteTextures (nbufs, buffers);
    return check_ok ("glDeleteTextures");
}

static inline bool gen_buffers (GLsizei num_buffers, GLuint* buffers)
{
    glGenBuffers (num_buffers, buffers);
    return check_ok ("glGenBuffers");
}

static inline bool bind_buffer (GLenum target, GLuint buffer)
{
    glBindBuffer (target, buffer);
    return check_ok ("glBindBuffer");
}

static inline void delete_buffers (GLsizei num_buffers, GLuint* buffers)
{
    glDeleteBuffers (num_buffers, buffers);
    check_ok ("glDeleteBuffers");
}

static inline bool gen_vertex_arrays (GLsizei num_arrays, GLuint* arrays)
{
    glGenVertexArrays (num_arrays, arrays);
    return check_ok ("glGenVertexArrays");
}

static inline bool bind_vertex_array (GLuint array)
{
    glBindVertexArray (array);
    return check_ok ("glBindVertexArray");
}

static inline void delete_vertex_arrays (GLsizei num_arrays, GLuint* arrays)
{
    glDeleteVertexArrays (num_arrays, arrays);
    check_ok ("glDeleteVertexArrays");
}

static inline bool bind_renderbuffer (GLenum target, GLuint buffer)
{
    glBindRenderbuffer (target, buffer);
    return check_ok ("glBindRendebuffer");
}

static inline bool gen_framebuffers (GLsizei num_arrays, GLuint* arrays)
{
    glGenFramebuffers (num_arrays, arrays);
    return check_ok ("glGenFramebuffers");
}

static inline bool bind_framebuffer (GLenum target, GLuint buffer)
{
    glBindFramebuffer (target, buffer);
    return check_ok ("glBindFramebuffer");
}

static inline void delete_framebuffers (GLsizei num_arrays, GLuint* arrays)
{
    glDeleteFramebuffers (num_arrays, arrays);
    check_ok ("glDeleteFramebuffers");
}

static inline bool tex_param_f (GLenum target, GLenum param, GLfloat val)
{
    glTexParameterf (target, param, val);
    return check_ok ("glTexParameterf");
}

static inline bool tex_param_i (GLenum target, GLenum param, GLint val)
{
    glTexParameteri (target, param, val);
    return check_ok ("glTexParameteri");
}

static inline bool active_texture (GLenum texture_id)
{
    glActiveTexture (texture_id);
    return check_ok ("glActiveTexture");
}

static inline bool enable (GLenum capability)
{
    glEnable (capability);
    return check_ok ("glEnable");
}

static inline bool disable (GLenum capability)
{
    glDisable (capability);
    return check_ok ("glDisable");
}

static inline bool cull_face (GLenum faces)
{
    glCullFace (faces);
    return check_ok ("glCullFace");
}

static inline bool get_integer_v (GLenum pname, GLint* params)
{
    glGetIntegerv (pname, params);
    return check_ok ("glGetIntegerv");
}

static inline bool init_face (GLenum target, GLenum type, uint32_t num_levels,
                       GLenum format, GLint internal_format, bool compressed,
                       uint32_t width, uint32_t height, uint32_t size,
                       const uint8_t*** p_data)
{
    bool success = true;
    const uint8_t** data = p_data ? *p_data : nullptr;
    uint32_t i;

    for (i = 0; i < num_levels; i++) {
        if (compressed) {
            glCompressedTexImage2D (target, i, internal_format, width, height, 0, size, data ? *data : NULL);
            if (! gl::check_ok ("glCompressedTexImage2D"))
                success = false;

        } else {
            glTexImage2D (target, i, internal_format, width, height, 0, format, type, data ? *data : NULL);
            if (! gl::check_ok ("glTexImage2D"))
                success = false;
        }

        if (data)
            data++;

        size /= 4;
        if (width > 1)
            width /= 2;
        if (height > 1)
            height /= 2;
    }

    if (data)
        *p_data = data;
    return success;
}

static inline bool init_face2 (GLenum target, GLenum type, uint32_t num_levels,
                       GLenum format, GLint internal_format, bool compressed,
                       uint32_t width, uint32_t height, uint32_t size,
                       const uint8_t*** p_data)
{
    bool success = true;
    const uint8_t** data = p_data ? *p_data : nullptr;
    uint32_t i;

    for (i = 0; i < num_levels; i++) {
        if (compressed) {
            glCompressedTexImage2D (target, i, internal_format, width, height, 0, size, data ? *data : NULL);
            if (! gl::check_ok ("glCompressedTexImage2D"))
                success = false;

        } else {
            glTexImage2D (target, i, internal_format, width, height, 0, format, type, data ? *data : NULL);
            if (! gl::check_ok ("glTexImage2D"))
                success = false;
        }

        if (data)
            data++;

        size /= 4;
        if (width > 1)
            width /= 2;
        if (height > 1)
            height /= 2;
    }

    if (data)
        *p_data = data;
    return success;
}

static bool buffer_data (GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    glBufferData (target, size, data, usage);
    return check_ok ("glBufferData");
}

// generates, binds, then sets buffer data
static bool buffer_bind_data (GLenum target, GLuint* buffer, GLsizeiptr size,
                              const GLvoid* data, GLenum usage)
{
    bool success;
    if (! gen_buffers (1, buffer))
        return false;

    if (! bind_buffer (target, *buffer))
        return false;

    glBufferData (target, size, data, usage);
    success = check_ok ("glBufferData");

    bind_buffer (target, 0);
    return success;
}

static inline bool update_buffer (GLenum target_type, GLuint buffer, const void* data, size_t size)
{
    if (! bind_buffer (target_type, buffer)) {
        std::clog << "gl::update_buffer(" << (int)buffer << ")\n";
        return false;
    }

    bool success = false;
    if (auto ptr = glMapBufferRange (target_type, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)) {
        memcpy (ptr, data, size);
        glUnmapBuffer (target_type);
        success = check_ok ("glUnmapBuffer");
    }

    return success;
}

} // namespace gl
