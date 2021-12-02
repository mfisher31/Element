
#ifndef EL_GRAPHICS_H
#define EL_GRAPHICS_H

#include <element/element.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EL_COLOR_FORMAT_UNKNOWN = 0,
    EL_COLOR_FORMAT_RGBA,
    EL_COLOR_FORMAT_BGRX,
    EL_COLOR_FORMAT_BGRA
} egColorFormat;

typedef enum {
    EL_DRAW_MODE_POINTS,
    EL_DRAW_MODE_LINES,
    EL_DRAW_MODE_LINES_STRIP,
    EL_DRAW_MODE_TRIANGLES,
    EL_DRAW_MODE_TRIANGLES_STRIP
} egDrawMode;

typedef enum {
    EL_CULL_BACK,
    EL_CULL_FRONT,
    EL_CULL_OFF
} egCullMode;

typedef enum {
    EL_TEXTURE_2D,
    EL_TEXTURE_3D,
    EL_TEXTURE_CUBE
} egTextureType;

typedef enum {
    EL_ZSTENCIL_NONE = 0,
    EL_ZSTENCIL_16,
    EL_ZSTENCIL_24_S8,
    EL_ZSTENCIL_32F,
    EL_ZSTENCIL_32F_S8X24,
} egZstencilFormat;

enum {
    EVG_TEXTURE_USE_MIPMAPS = (1 << 0),
    EVG_TEXTURE_DYNAMIC = (1 << 1),
    EVG_TEXTURE_RENDER_TARGET = (1 << 2),
    EVG_TEXTURE_DUMMY = (1 << 3),
    EVG_TEXTURE_REF_BUFFER = (1 << 4),
    EVG_TEXTURE_SHARED = (1 << 5),
    EVG_TEXTURE_SHARED_KM = (1 << 6)
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
} egSwapSetup;

/** 
 * Parameters to use when creating textures.
 * 
 * Not all fields are used for each type, but defined
 * in one struct for convenience writing new Graphics 
 * drivers.
 */
typedef struct {
    egTextureType type;     // type of texture (required all)
    egColorFormat format;   // color format to use (required all)
    uint32_t levels;        // levels to use
    uint32_t flags;         // flags see EL_TEXTURE_** flags enum
    uint32_t width, height; // width and height (not used for Cube textures)
    uint32_t depth;         // texture depth (required 3d only)
    uint32_t size;          // texture size (required cube only)
} egTextureSetup;

//=============================================================================
struct egDevice;
struct egSwapChain;
struct egTexture;

typedef struct egDevice egDevice;
typedef struct egSwapChain egSwapChain;
typedef struct egTexture egTexture;

//=============================================================================
typedef struct {
    egDevice* (*create)();
    void (*destroy) (egDevice* device);
    void (*enter_context) (egDevice* device);
    void (*leave_context) (egDevice* device);

    egSwapChain* (*swap_create) (egDevice* device, const egSwapSetup* setup);
    void (*swap_destroy) (egSwapChain* swap);
    void (*swap_load) (egDevice* device, const egSwapChain* swap);

    egTexture* (*texture_create) (egDevice* device, const egTextureSetup* setup, const uint8_t** data);
    void (*texture_destroy) (egTexture* texture);
    void (*texture_fill_setup) (egTexture* texture, egTextureSetup* setup);
} egDeviceDescriptor;

#ifdef __cplusplus
} // extern C
#endif

#endif
