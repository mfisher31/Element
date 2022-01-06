
#ifndef EL_VECTOR_H
#define EL_VECTOR_H

#include <stdlib.h>
#include <string.h>

#if defined(__arm__) || defined(__aarch64__)
    #include "element/evg/sse2neon.h"
#else
    #include <xmmintrin.h>
#endif

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
        __m128 m;
    };
} evgVec3;

static inline void evg_vec3_reset (evgVec3* vec)
{
    vec->m = _mm_setzero_ps();
}

static inline void evg_vec3_set (evgVec3* vec, float x, float y, float z)
{
    vec->m = _mm_set_ps (0.0f, z, y, x);
}

static inline void evg_vec3_copy (evgVec3* dst, evgVec3* src)
{
    dst->m = src->m;
}

static inline void evg_vec3_add (evgVec3* dst, const evgVec3* a, const evgVec3* b)
{
    dst->m = _mm_add_ps (a->m, b->m);
}

//=============================================================================
typedef struct evgVec4 {
    union {
        struct {
            float x, y, z, w;
        };
        float ptr[4];
        __m128 m;
    };
} evgVec4;

static inline void evg_vec4_reset (evgVec4* vec)
{
    vec->x = vec->y = vec->z = vec->w = 0.f;
}

static inline void evg_vec4_set (evgVec4* vec, float x, float y, float z, float w)
{
    vec->m = _mm_set_ps (z, y, x, w);
}

static inline float evg_vec4_dot (const evgVec4* v1, const evgVec4* v2)
{
    evgVec4 add;
    __m128 mul = _mm_mul_ps (v1->m, v2->m);
    add.m = _mm_add_ps (_mm_movehl_ps (mul, mul), mul);
    add.m = _mm_add_ps (_mm_shuffle_ps (add.m, add.m, 0x55), add.m);
    return add.x;
}

#ifdef __cplusplus
}
#endif

#endif
