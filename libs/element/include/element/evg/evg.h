#ifndef EVG_H_INCLUDED
#define EVG_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVG_COLOR_FORMAT_UNKNOWN = 0,
    EVG_COLOR_FORMAT_RGBA,
    EVG_COLOR_FORMAT_BGRX,
    EVG_COLOR_FORMAT_BGRA
} evgColorFormat;

typedef enum {
    EL_COLOR_SPACE_DEFAULT,
    EL_COLOR_SPACE_601,
    EL_COLOR_SPACE_709,
    EL_COLOR_SPACE_SRGB
} evgColorSpace;

typedef enum {
    EL_VIDEO_RANGE_DEFAULT,
    EL_VIDEO_RANGE_PARTIAL,
    EL_VIDEO_RANGE_FULL
} evgVideoRange;

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

#define EVG_MIPMAPS          (1 << 0x0000)
#define EVG_DYNAMIC          (1 << 0x0001)
#define EVG_RENDER_TARGET    (1 << 0x0002)
#define EVG_DUMMY            (1 << 0x0003)
#define EVG_FLIP_U           (1 << 0x0004)
#define EVG_FLIP_V           (1 << 0x0005)
#define EVG_CLEAR_COLOR      (1 << 0x0006)
#define EVG_CLEAR_DEPTH      (1 << 0x0007)
#define EVG_CLEAR_STENCIL    (1 << 0x0008)
#define EVG_FRAMEBUFFER_SRGB (1 << 0x0009)
#define EVG_DEPTH_TEST       (1 << 0x000A)
#define EVG_STENCIL_TEST     (1 << 0x000B)

typedef struct {
#if defined(__linux__)
    uint32_t xwindow;
#elif defined(__APPLE__)
    void* view;
#elif defined(_WIN32)
#endif
} evgWindow;

typedef struct {
    evgWindow window;
    uint32_t width;
    uint32_t height;
    uint32_t nbuffers;
    evgColorFormat format;
    evgStencilFormat stencil;
    uint32_t adapter;
} evgSwapInfo;

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
    EVG_VALUE_UNKNOWN,
    EVG_VALUE_BOOL,
    EVG_VALUE_FLOAT,
    EVG_VALUE_INT,
    EVG_VALUE_STRING,
    EVG_VALUE_VEC2,
    EVG_VALUE_VEC3,
    EVG_VALUE_VEC4,
    EVG_VALUE_INT2,
    EVG_VALUE_INT3,
    EVG_VALUE_INT4,
    EVG_VALUE_MAT4X4,
    EVG_VALUE_TEXTURE
} evgValueType;

//=============================================================================
typedef void* evgHandle;

typedef struct {
    evgHandle (*create) (evgHandle device, const evgSwapInfo* setup);
    void (*destroy) (evgHandle swap);
} evgSwapInterface;

//=============================================================================
typedef enum {
    EVG_SHADER_VERTEX,
    EVG_SHADER_FRAGMENT
} evgShaderType;

typedef enum {
    EVG_ATTRIBUTE,
    EVG_UNIFORM
} evgResourceType;

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
    void (*update_resource) (evgHandle handle, int key, uint32_t size, const void* data);
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

    void (*save_state) (evgHandle device);
    void (*restore_state) (evgHandle device);
    
    void (*enable) (evgHandle device, uint32_t enablement, bool enabled);

    void (*viewport) (evgHandle device, int x, int y, int width, int height);
    void (*clear) (evgHandle device, uint32_t clear_flags, uint32_t color, double depth, int stencil);
    void (*draw) (evgHandle device, evgDrawMode mode, uint32_t start, uint32_t nverts);
    void (*present) (evgHandle device);
    void (*flush) (evgHandle device);

    void (*load_vertex_buffer) (evgHandle device, evgHandle vbuf, int slot);
    void (*load_index_buffer) (evgHandle device, evgHandle ibuf);
    void (*load_program) (evgHandle device, evgHandle program);
    void (*load_texture) (evgHandle device, evgHandle texture, int slot);
    void (*load_target) (evgHandle device, evgHandle texture);
    void (*load_stencil) (evgHandle device, evgHandle stencil);
    void (*load_swap) (evgHandle device, evgHandle swap);

    const evgBufferInterface* buffer;
    const evgShaderInterface* shader;
    const evgProgramInterface* program;
    const evgTextureInterface* texture;
    const evgStencilInterface* stencil;
    const evgSwapInterface* swap;
} evgDescriptor;

//=============================================================================
inline static const char* evg_color_format_string (evgColorFormat format)
{
    switch (format) {
        case EVG_COLOR_FORMAT_UNKNOWN:
            return "Unknown";
            break;
        case EVG_COLOR_FORMAT_RGBA:
            return "RGBA";
            break;
        case EVG_COLOR_FORMAT_BGRX:
            return "BGRX";
            break;
        case EVG_COLOR_FORMAT_BGRA:
            return "BGRA";
            break;
    }
}

static inline uint32_t evg_color_format_is_compressed (evgColorFormat format)
{
    switch (format) {
        default:
            break;
    }
    return false;
}
static inline uint32_t evg_color_format_bpp (evgColorFormat format)
{
    switch (format) {
        case EVG_COLOR_FORMAT_UNKNOWN:
            return 0;
        case EVG_COLOR_FORMAT_RGBA:
        case EVG_COLOR_FORMAT_BGRA:
        case EVG_COLOR_FORMAT_BGRX:
            return 32;
        default:
            break;
    }

    return 0;
}

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
           desc->draw != NULL &&
           desc->present != NULL &&
           desc->flush != NULL &&

           desc->load_vertex_buffer != NULL &&
           desc->load_index_buffer != NULL &&
           desc->load_program != NULL &&
           desc->load_texture != NULL &&
           desc->load_target != NULL &&
           desc->load_stencil!= NULL &&
           desc->load_swap != NULL &&
           
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
