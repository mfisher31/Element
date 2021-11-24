#ifndef LKV_BYTES_H
#define LKV_BYTES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct elBytes {
    size_t      size;
    uint8_t*    data;
} elBytes;

#ifdef __cplusplus
}
#endif

#endif
