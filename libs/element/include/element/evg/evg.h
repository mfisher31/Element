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
} egColorFormat;

typedef enum {
    EVG_DRAW_MODE_POINTS,
    EVG_DRAW_MODE_LINES,
    EVG_DRAW_MODE_LINES_STRIP,
    EVG_DRAW_MODE_TRIANGLES,
    EVG_DRAW_MODE_TRIANGLES_STRIP
} egDrawMode;

typedef enum {
    EVG_CULL_BACK,
    EVG_CULL_FRONT,
    EVG_CULL_OFF
} egCullMode;

typedef enum {
    EVG_TEXTURE_2D,
    EVG_TEXTURE_3D,
    EVG_TEXTURE_CUBE
} evgTextureType;

typedef enum {
    EVG_ZSTENCIL_NONE = 0,
    EVG_ZSTENCIL_16,
    EVG_ZSTENCIL_24_S8,
    EVG_ZSTENCIL_32F,
    EVG_ZSTENCIL_32F_S8X24,
} egZstencilFormat;

enum {
    EVG_OPT_USE_MIPMAPS = (1 << 0),
    EVG_OPT_DYNAMIC = (1 << 1),
    EVG_OPT_RENDER_TARGET = (1 << 2),
    EVG_OPT_DUMMY = (1 << 3)
};

typedef struct {
#if defined(__linux__)
    uint32_t xwindow;
#elif defined(__APPLE__)
#elif defined(_WIN32)
#endif
} egWindow;

typedef struct {
    egWindow window;
    uint32_t width;
    uint32_t height;
    uint32_t nbuffers;
    egColorFormat color_format;
    egZstencilFormat zstencil_format;
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

/** 
 * Parameters to use when creating textures.
 * 
 * Not all fields are used for each type, but defined
 * in one struct for convenience writing new Graphics 
 * drivers.
 */
typedef struct {
    evgTextureType type;    // type of texture (required all)
    egColorFormat format;   // color format to use (required all)
    uint32_t levels;        // levels to use
    uint32_t flags;         // flags see EVG_TEXTURE_** flags enum
    uint32_t width, height; // width and height (not used for Cube textures)
    uint32_t depth;         // texture depth (required 3d only)
    uint32_t size;          // texture size (required cube only)
} evgTextureSetup;

typedef struct {
    size_t width;
    void* array;
} evgTextureVerts;

typedef struct {
    size_t size;
    evgVec3* points;
    evgVec3* normals;
    evgVec3* tangents;
    uint32_t* colors;
    size_t ntextures;
    evgTextureVerts* texdata;
} evgVertexData;

/**
 * Create and initialize Vertex data.
 * 
 * Use this when registering vertex buffers with a graphics
 * device.
 * 
 * @param size Number of vectors for each initialized type.
 * @param use_normals Initialze data for normals
 * @param use_tangents Initialze data for use_tangents
 * @param use_colors Initialze data for use_colors
 * @param num_textures Allocate for num textures.
 * @param texture_width Width of texture data.
 * @return evgVertexData or NULL
 */
inline static evgVertexData* evg_vertex_data_new (size_t size,
                                                  bool use_normals,
                                                  bool use_tangents,
                                                  bool use_colors,
                                                  size_t num_textures,
                                                  size_t texture_width)
{
    evgVertexData* data = (evgVertexData*) malloc (sizeof (evgVertexData));
    memset (data, 0, sizeof (evgVertexData));
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

    if (! num_textures)
        return data;

    if (! texture_width)
        texture_width = 2;

    data->texdata = (evgTextureVerts*) malloc (num_textures * sizeof (evgTextureVerts));
    for (size_t i = 0; i < num_textures; i++) {
        data->texdata[i].width = texture_width;
        data->texdata[i].array = malloc (sizeof (evgVec2) * data->size);
        memset (data->texdata[0].array, 0, sizeof (evgVec2) * data->size);
    }
    return data;
}

/** 
 * Free Vertex data.
 * 
 * @param data Data to free. Does nothing if NULL.
 */
inline static void evg_vertex_data_free (evgVertexData* data)
{
    if (data == NULL)
        return;

    if (data->points != NULL)
        free (data->points);
    if (data->normals != NULL)
        free (data->normals);
    if (data->tangents != NULL)
        free (data->tangents);
    if (data->colors != NULL)
        free (data->colors);
    if (data->texdata != NULL) {
        for (uint32_t i = 0; i < data->ntextures; ++i)
            if (data->texdata[i].array != NULL)
                free (data->texdata[i].array);
        free (data->texdata);
    }
    free (data);
}

typedef struct {
    uint32_t type;
    uint32_t length;
    uint32_t stride;
    void* indices;
} evgIndexData;

//=============================================================================
typedef void* evgHandle;

typedef struct {
    evgHandle (*create)(evgHandle device, const evgSwapSetup* setup);
    void (*destroy)(evgHandle swap);
} evgSwapInterface;

typedef struct {
    evgHandle (*create)(evgHandle device);
    void (*destroy)(evgHandle program);
    void (*link)(evgHandle program, evgHandle verts, evgHandle frags);
} evgProgramInterface;

typedef struct {
    uint32_t size;
    uint32_t flags;
} evgIndexArraySetup;

typedef struct {
    evgHandle (*create)(evgHandle device, uint32_t size, uint32_t flags);
    void (*destroy) (evgHandle handle);
    void (*fill_setup) (evgHandle array, evgIndexArraySetup* setup);
    void (*update)(evgHandle array, void* data);
} evgIndexBufferInterface;

//=============================================================================
typedef struct {
    const char* name;
    
    evgHandle (*create)();
    void (*destroy) (evgHandle device);

    void (*enter_context) (evgHandle device);
    void (*leave_context) (evgHandle device);
    void (*clear_context) (evgHandle device);

    void (*viewport)(evgHandle device, int x, int y, int width, int height);
    void (*ortho)(evgHandle, float left, float right, float top, float bottom, float near, float far);
    void (*draw)(evgHandle device, egDrawMode mode, uint32_t start, uint32_t nverts);
    void (*present)(evgHandle device);
    void (*flush)(evgHandle device);

    void (*load_index_buffer) (evgHandle device, evgHandle ibuf);
    void (*load_program) (evgHandle device, evgHandle program);
    void (*load_shader)(evgHandle device, evgHandle shader);
    void (*load_swap) (evgHandle device, evgHandle swap);
    void (*load_texture) (evgHandle device, evgHandle texture, int index);
    void (*load_vertex_buffer) (evgHandle device, evgHandle vbuf);

    evgHandle (*texture_create) (evgHandle device, const evgTextureSetup* setup, const uint8_t** data);
    void (*texture_destroy) (evgHandle tex);
    void (*texture_fill_setup) (evgHandle tex, evgTextureSetup* setup);
    void (*texture_map) (evgHandle tex);

    evgHandle (*vertex_buffer_create) (evgHandle device, evgVertexData* data, uint32_t flags);
    void (*vertex_buffer_destroy) (evgHandle vbuf);
    evgVertexData* (*vertex_buffer_data) (evgHandle vbuf);
    void (*vertex_buffer_flush) (evgHandle vbuf);

    evgHandle (*shader_create)(evgHandle device, evgShaderType type);
    void (*shader_destroy) (evgHandle shader);
    bool (*shader_parse) (evgHandle shader, const char* program);
    void (*shader_add_attribute) (evgHandle shader, evgAttributeType type, evgValueType value_type);
    void (*shader_add_uniform) (evgHandle shader, evgValueType value_type);

    const evgSwapInterface* swap;
    const evgProgramInterface* program;
    const evgIndexBufferInterface* index_buffer;
} evgDescriptor;

inline static bool evg_descriptor_valid (const evgDescriptor* desc)
{
    /* clang-format off */
    return desc != NULL &&
           desc->create != NULL &&
           desc->destroy != NULL && 
           desc->enter_context != NULL && 
           desc->leave_context != NULL &&
           desc->present != NULL &&
           desc->viewport != NULL &&
           desc->ortho != NULL &&

           desc->load_program != NULL &&
           desc->load_index_buffer != NULL &&
           desc->load_shader != NULL &&
           desc->load_swap != NULL &&
           desc->load_texture != NULL &&
           desc->load_vertex_buffer != NULL &&
           
           desc->texture_create != NULL &&
           desc->texture_destroy != NULL &&
           desc->texture_fill_setup != NULL &&
        //    desc->texture_map != NULL &&

           desc->vertex_buffer_create != NULL &&
           desc->vertex_buffer_data != NULL &&
           desc->vertex_buffer_destroy != NULL &&
           desc->vertex_buffer_flush != NULL &&

           desc->shader_create != NULL &&
           desc->shader_destroy != NULL &&
           desc->shader_parse != NULL &&

           desc->program != NULL &&
           desc->program->create != NULL &&
           desc->program->destroy != NULL &&
           desc->program->link != NULL &&
           
           desc->swap != NULL &&
           desc->swap->create != NULL &&
           desc->swap->destroy != NULL &&

           desc->index_buffer != NULL &&
           desc->index_buffer->create != NULL &&
           desc->index_buffer->destroy != NULL &&
           desc->index_buffer->fill_setup != NULL &&
           desc->index_buffer->update != NULL;
    /* clang-format on */
}

#ifdef __cplusplus
}
#endif

#endif
