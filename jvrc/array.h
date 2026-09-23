// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_ARRAY_H
#define JVR_ARRAY_H
#include <stddef.h>
#include <stdint.h>
#ifndef VK_NULL_HANDLE
#define VK_NULL_HANDLE (void *)0
#endif // !VK_NULL_HANDLE

typedef struct jvr_array {
    void *data;
    uint32_t capacity;
    uint32_t len;
    size_t elem_sz;
} jvr_array;

#define FOREACH(arr, type, elem) \
    for (type *elem = arr.data; elem != ((type *)arr.data) + arr.len; elem++)

#define JVR_ELEM(arr, type, index) ((type *)arr.data)[index]
#define JVR_ELEM_PTR(arr, type, index) ((type *)arr.data + index)

extern void jvr_array_new(jvr_array *arr, size_t elem_size, size_t count);
extern void jvr_array_push(jvr_array *arr, void *elem);
extern void jvr_array_from(jvr_array *arr, void *elems, size_t count);
extern void jvr_array_delete(jvr_array *arr);

#endif
