#ifndef EVG_VIDEO_H
#define EVG_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EL_COLOR_SPACE_DEFAULT,
    EL_COLOR_SPACE_601,
    EL_COLOR_SPACE_709,
    EL_COLOR_SPACE_SRGB
} egVideoColorSpace;

typedef enum {
    EL_VIDEO_RANGE_DEFAULT,
    EL_VIDEO_RANGE_PARTIAL,
    EL_VIDEO_RANGE_FULL
} egVideoRange;

#ifdef __cplusplus
}
#endif

#endif
