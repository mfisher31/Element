#ifndef EVG_H_INCLUDED
#define EVG_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "element/evg/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct evgVec2 evgVec2;
typedef struct evgVec3 evgVec3;

typedef enum {
    EVG_COLOR_FORMAT_UNKNOWN = 0,
    EVG_COLOR_FORMAT_RGBA,
    EVG_COLOR_FORMAT_BGRX,
    EVG_COLOR_FORMAT_BGRA
} evgColorFormat;

typedef enum {
    EVG_DRAW_MODE_POINTS,
    EVG_DRAW_MODE_LINES,
    EVG_DRAW_MODE_LINES_STRIP,
    EVG_DRAW_MODE_TRIANGLES,
    EVG_TRIANGLE_STRIP
} evgDrawMode;

typedef enum {
    EVG_CULL_BACK,
    EVG_CULL_FRONT,
    EVG_CULL_OFF
} evgCullMode;

typedef enum {
    EVG_STENCIL_NONE = 0,
    EVG_STENCIL_16,
    EVG_STENCIL_24_S8,
    EVG_STENCIL_32F,
    EVG_STENCIL_32F_S8X24,
} evgStencilFormat;

#define EVG_OPT_USE_MIPMAPS   (1 << 0)
#define EVG_OPT_DYNAMIC       (1 << 1)
#define EVG_OPT_RENDER_TARGET (1 << 2)
#define EVG_OPT_DUMMY         (1 << 3)

#define EVG_CLEAR_COLOR   (1 << 0)
#define EVG_CLEAR_DEPTH   (1 << 1)
#define EVG_CLEAR_STENCIL (1 << 2)

#define EVG_FLIP_U (1 << 0)
#define EVG_FLIP_V (1 << 1)

typedef struct {
#if defined(__linux__)
    uint32_t xwindow;
#elif defined(__APPLE__)
#elif defined(_WIN32)
#endif
} evgWindow;

typedef struct {
    evgWindow window;
    uint32_t width;
    uint32_t height;
    uint32_t nbuffers;
    evgColorFormat color_format;
    evgStencilFormat zstencil_format;
    uint32_t adapter;
} evgSwapSetup;

typedef enum {
    EVG_ATTRIB_POSITION,
    EVG_ATTRIB_NORMAL,
    EVG_ATTRIB_TANGENT,
    EVG_ATTRIB_COLOR,
    EVG_ATTRIB_TEXCOORD,
    EVG_ATTRIB_TARGET,
} evgAttributeType;

typedef enum {
    EVG_BOOL,
    EVG_CHAR,
    EVG_BYTE,
    EVG_INT,
    EVG_UNSIGNED_INT,
    EVG_FLOAT,
    EVG_DOUBLE
} evgDataType;

typedef enum {
    EVG_ATTRIBUTE,
    EVG_UNIFORM
} evgResourceType;

typedef enum {
    EVG_UNIFORM_UNKNOWN,
    EVG_UNIFORM_BOOL,
    EVG_UNIFORM_FLOAT,
    EVG_UNIFORM_INT,
    EVG_UNIFORM_STRING,
    EVG_UNIFORM_VEC2,
    EVG_UNIFORM_VEC3,
    EVG_UNIFORM_VEC4,
    EVG_UNIFORM_INT2,
    EVG_UNIFORM_INT3,
    EVG_UNIFORM_INT4,
    EVG_UNIFORM_MAT4X4,
    EVG_UNIFORM_TEXTURE
} evgValueType;

typedef enum {
    EVG_SHADER_VERTEX,
    EVG_SHADER_FRAGMENT
} evgShaderType;


//===

typedef struct {
    uint8_t width;
    void* array;
} evgTexCoords;

typedef struct {
    uint32_t size;
    evgVec3* points;
    evgVec3* normals;
    evgVec3* tangents;
    uint32_t* colors;

    uint32_t num_textures;
    evgTexCoords* texcoords;
} evgVertexBuffer;

/**
 * Create and initialize Vertex data.
 * 
 * Use this when registering vertex buffers with a graphics
 * device.
 * 
 * @param size Number of elements for each initialized type.
 * @param use_normals Initialze data for normals
 * @param use_tangents Initialze data for use_tangents
 * @param use_colors Initialze data for use_colors
 * @param num_textures Allocate for num textures.
 * @param texture_width Width of texture data.
 * @return evgVertexBuffer or NULL
 */
inline static evgVertexBuffer* evg_vertex_buffer_new (uint32_t size,
                                                      bool use_normals,
                                                      bool use_tangents,
                                                      bool use_colors,
                                                      uint32_t num_textures,
                                                      uint32_t texcoord_size)
{
    evgVertexBuffer* data = (evgVertexBuffer*) malloc (sizeof (evgVertexBuffer));
    memset (data, 0, sizeof (evgVertexBuffer));
    data->size = size;

    if (! size) {
        if (data)
            free (data);
        return NULL;
    }

    const bool use_points = true;
#define __initmember(field, T)                               \
    if (use_##field) {                                       \
        data->field = (T*) malloc (sizeof (T) * data->size); \
        memset (data->field, 0, sizeof (T) * data->size);    \
    }
    __initmember (points, evgVec3);
    __initmember (normals, evgVec3);
    __initmember (tangents, evgVec3);
    __initmember (colors, uint32_t);
#undef __initmember

    data->num_textures = num_textures;
    if (data->num_textures == 0)
        return data;

    if (texcoord_size < 2)
        texcoord_size = 2;
    if (texcoord_size > 4)
        texcoord_size = 4;

    uint32_t uv_size = data->num_textures * sizeof (evgTexCoords);
    data->texcoords = (evgTexCoords*) malloc (uv_size);
    memset (data->texcoords, 0, uv_size);

    size_t vec_size = data->size;
    switch (texcoord_size) {
        case 2:
            vec_size += sizeof (evgVec2);
            break;
        case 3:
            vec_size += sizeof (evgVec3);
            break;
        case 4:
            vec_size += sizeof (evgVec4);
            break;
    }

    for (size_t i = 0; i < data->num_textures; i++) {
        data->texcoords[i].width = texcoord_size;
        data->texcoords[i].array = malloc (vec_size);
        memset (data->texcoords[0].array, 0, vec_size);
    }

    return data;
}

inline static evgVec3* evg_vertex_buffer_points (const evgVertexBuffer* buffer)
{
    return buffer->points;
}

inline static evgVec2* evg_vertex_buffer_texcoords_vec2 (const evgVertexBuffer* buffer, int texture_index)
{
    return (evgVec2*) buffer->texcoords[texture_index].array;
}

/** 
 * Free Vertex data.
 * 
 * @param data Data to free. Does nothing if NULL.
 */
inline static void evg_vertex_buffer_free (evgVertexBuffer* buffer)
{
    if (buffer == NULL)
        return;

    if (buffer->points != NULL)
        free (buffer->points);
    if (buffer->normals != NULL)
        free (buffer->normals);
    if (buffer->tangents != NULL)
        free (buffer->tangents);
    if (buffer->colors != NULL)
        free (buffer->colors);
    if (buffer->texcoords != NULL) {
        for (uint32_t i = 0; i < buffer->num_textures; ++i)
            if (buffer->texcoords[i].array != NULL)
                free (buffer->texcoords[i].array);
        free (buffer->texcoords);
    }
    free (buffer);
}

//=============================================================================
typedef void* evgHandle;

typedef struct {
    evgHandle (*create) (evgHandle device, const evgSwapSetup* setup);
    void (*destroy) (evgHandle swap);
} evgSwapInterface;

//=============================================================================
typedef struct {
    const char* symbol;
    evgResourceType type;
    evgValueType value_type;
    uint32_t key;
} evgResource;

typedef struct {
    evgHandle (*create) (evgHandle device, evgShaderType type);
    void (*destroy) (evgHandle handle);
    bool (*parse) (evgHandle shader, const char* program);
    void (*add_resource) (evgHandle shader, const char* symbol,
                          evgResourceType resource,
                          evgValueType value_type);
} evgShaderInterface;

typedef struct {
    evgHandle (*create) (evgHandle device);
    void (*destroy) (evgHandle program);
    void (*link) (evgHandle program, evgHandle vertex_shader, evgHandle fragment_shader);
    const evgResource* (*resource) (evgHandle program, uint32_t index);
} evgProgramInterface;

//=============================================================================
typedef enum {
    EVG_TEXTURE_2D,
    EVG_TEXTURE_3D,
    EVG_TEXTURE_CUBE
} evgTextureType;

/** 
 * Parameters to use when creating textures.
 * 
 * Not all fields are used for each type, but defined
 * in one struct for convenience writing new Graphics 
 * drivers.
 */
typedef struct {
    evgTextureType type;    // type of texture (required all)
    evgColorFormat format;  // color format to use (required all)
    uint32_t levels;        // number of levels to use
    uint32_t flags;         // flags see EVG_TEXTURE_** flags enum
    uint32_t width, height; // width and height (not used for Cube textures)
    uint32_t depth;         // texture depth (required 3d only)
    uint32_t size;          // texture size (required cube only)
} evgTextureInfo;

typedef struct {
    evgHandle (*create) (evgHandle device, const evgTextureInfo* setup);
    void (*destroy) (evgHandle tex);
    void (*fill_info) (evgHandle tex, evgTextureInfo* setup);
    void (*update) (evgHandle tex, const uint8_t* data);
} evgTextureInterface;

//=============================================================================
typedef enum {
    EVG_BUFFER_ARRAY,
    EVG_BUFFER_INDEX
} evgBufferType;

typedef struct {
    evgBufferType type; // the type of buffer.
    uint32_t size;      // actual used size in bytes
    uint32_t capacity;  // available capacity in bytes
} evgBufferInfo;

typedef struct {
    evgHandle (*create) (evgHandle device, evgBufferType type, uint32_t capacity, uint32_t flags);
    void (*destroy) (evgHandle buffer);
    void (*fill_info) (evgHandle buffer, evgBufferInfo* info);
    void (*update) (evgHandle buffer, uint32_t size, const void* data);
} evgBufferInterface;

//=============================================================================
typedef struct {
    evgHandle (*create) (evgHandle device, uint32_t width, uint32_t height, evgStencilFormat format);
    void (*destroy) (evgHandle stencil);
} evgStencilInterface;

typedef struct {
    const char* name;

    evgHandle (*create)();
    void (*destroy) (evgHandle device);

    void (*enter_context) (evgHandle device);
    void (*leave_context) (evgHandle device);
    void (*clear_context) (evgHandle device);

    void (*viewport) (evgHandle device, int x, int y, int width, int height);
    void (*clear) (evgHandle device, uint32_t clear_flags, uint32_t color, double depth, int stencil);

    void (*ortho) (evgHandle, float left, float right, float top, float bottom, float near, float far);
    void (*draw) (evgHandle device, evgDrawMode mode, uint32_t start, uint32_t nverts);

    void (*present) (evgHandle device);
    void (*flush) (evgHandle device);

    void (*load_index_buffer) (evgHandle device, evgHandle ibuf);
    void (*load_program) (evgHandle device, evgHandle program);
    void (*load_shader) (evgHandle device, evgHandle shader);
    void (*load_swap) (evgHandle device, evgHandle swap);
    void (*load_texture) (evgHandle device, evgHandle texture, int slot);
    void (*load_vertex_buffer) (evgHandle device, evgHandle vbuf, int slot);
    void (*load_stencil) (evgHandle device, evgHandle stencil);
    void (*load_target) (evgHandle device, evgHandle texture);

    const evgSwapInterface* swap;
    const evgBufferInterface* buffer;
    const evgTextureInterface* texture;
    const evgShaderInterface* shader;
    const evgProgramInterface* program;
    const evgStencilInterface* stencil;
} evgDescriptor;

inline static bool evg_descriptor_valid (const evgDescriptor* desc)
{
    /* clang-format off */
    return desc != NULL &&
           desc->create != NULL &&
           desc->destroy != NULL && 

           desc->enter_context != NULL && 
           desc->leave_context != NULL &&
           desc->clear_context != NULL &&

           desc->viewport != NULL &&
           desc->clear != NULL &&
           
           desc->ortho != NULL &&
           desc->draw != NULL &&
           desc->present != NULL &&
           desc->flush != NULL &&

           desc->load_program != NULL &&
           desc->load_index_buffer != NULL &&
           desc->load_shader != NULL &&
           desc->load_swap != NULL &&
           desc->load_texture != NULL &&
           desc->load_vertex_buffer != NULL &&
           
           desc->texture != NULL &&
           desc->texture->create != NULL &&
           desc->texture->destroy != NULL &&
           desc->texture->fill_info != NULL &&
           desc->texture->update != NULL &&

           desc->buffer != NULL &&
           desc->buffer->create != NULL &&
           desc->buffer->destroy != NULL &&
           desc->buffer->fill_info != NULL &&
           desc->buffer->update != NULL &&

           desc->shader != NULL &&
           desc->shader->create != NULL &&
           desc->shader->destroy != NULL &&
           desc->shader->parse != NULL &&

           desc->program != NULL &&
           desc->program->create != NULL &&
           desc->program->destroy != NULL &&
           desc->program->link != NULL &&
           
           desc->swap != NULL &&
           desc->swap->create != NULL &&
           desc->swap->destroy != NULL;
    /* clang-format on */
}

#ifdef __cplusplus
}
#endif

#endif
