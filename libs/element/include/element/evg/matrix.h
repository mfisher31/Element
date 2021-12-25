
#ifndef EVG_MATRIX_H
#define EVG_MATRIX_H

#include "element/evg/vector.h"

struct evgMatrix4 {
	struct evgVec4 x, y, z, t;
};

static inline void evg_mat4_reset (evgMatrix4* mat) {
	evg_vec4_reset (&mat->x);
	evg_vec4_reset (&mat->y);
	evg_vec4_reset (&mat->z);
	evg_vec4_reset (&mat->t);
}

static inline void evg_mat4_identity (evgMatrix4 *mat)
{
	evg_mat4_reset (mat);
	mat->x.x = 1.0f;
	mat->y.y = 1.0f;
	mat->z.z = 1.0f;
	mat->t.w = 1.0f;
}

static inline void evg_mat4_copy (evgMatrix4 *dst, const evgMatrix4 *src) {
	memcpy (dst, src, sizeof (evgMatrix4));
}

static inline void evg_mat4_ortho (evgMatrix4* dst, float left, float right, float bottom, float top, float near, float far)
{
	evg_mat4_reset (dst);

    float rml = right - left;
    float bmt = bottom - top;
    float fmn = far - near;

    dst->x.x = 2.0f / rml;
    dst->t.x = (left + right) / -rml;

    dst->y.y = 2.0f / -bmt;
    dst->t.y = (bottom + top) / bmt;

    dst->z.z = -2.0f / fmn;
    dst->t.z = (far + near) / -fmn;

    dst->t.w = 1.0f;
}

static inline void evg_mat4_multiply (evgMatrix4 *dst, const evgMatrix4 *m1, const evgMatrix4 *m2)
{
	const evgVec4 *m1v = (const evgVec4 *)m1;
	const float *m2f = (const float *)m2;
	evgVec4 out[4];
	int i, j;

	evgVec4 temp;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			evg_vec4_set (&temp, m2f[j], m2f[j + 4], m2f[j + 8], m2f[j + 12]);
			out[i].ptr[j] = evg_vec4_dot (&m1v[i], &temp);
		}
	}

	evg_mat4_copy (dst, (evgMatrix4*)out);
}

#endif
