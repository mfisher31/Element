
#ifndef EL_VECTOR_H
#define EL_VECTOR_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct evgVec2 {
    union {
        struct {
            float x, y;
        };
        float ptr[2];
    };
} evgVec2;

static inline void evg_vec2_reset (evgVec2* vec)
{
    vec->x = vec->y = 0.0;
}

static inline void evg_vec2_set (evgVec2* vec, float x, float y)
{
    vec->x = x;
    vec->y = y;
}

//=============================================================================
typedef struct evgVec3 {
    union {
        struct {
            float x, y, z, w;
        };
        float ptr[4];
    };
} evgVec3;

static inline void evg_vec3_reset (evgVec3* vec)
{
    vec->x = vec->y = vec->z = vec->w = 0.f;
}

static inline void evg_vec3_set (evgVec3* vec, float x, float y, float z)
{
    vec->x = x;
    vec->y = y;
    vec->z = z;
    vec->w = 0.0;
}

static inline void evg_vec3_copy (evgVec3* dst, evgVec3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->w = src->w;
}

static inline void evg_vec3_neg (evgVec3* vec)
{
    vec->x = -vec->x;
    vec->y = -vec->y;
    vec->z = -vec->z;
    vec->w = 0.0;
}


//=============================================================================
typedef struct evgVec4 {
    union {
        struct {
            float x, y, z, w;
        };
        float ptr[4];
    };
} evgVec4;

static inline void evg_vec4_reset (evgVec4* vec)
{
    vec->x = vec->y = vec->z = vec->w = 0.f;
}

#ifdef __cplusplus
}
#endif

#endif
