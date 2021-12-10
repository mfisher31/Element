
#ifndef EVG_MATRIX_H
#define EVG_MATRIX_H

#include "element/evg/vector.h"

struct evgMatrix4 {
	struct evgVec4 x, y, z, t;
};

inline static void evg_mat4_reset (evgMatrix4* mat) {
	evg_vec4_reset (&mat->x);
	evg_vec4_reset (&mat->y);
	evg_vec4_reset (&mat->z);
	evg_vec4_reset (&mat->t);
}

#endif
