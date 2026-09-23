// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "array.h"
#include <stdlib.h>
#include <string.h>

uint32_t round_up(uint32_t elem)
{
    if (elem <= 4)
        return 4;
    uint32_t size = 4;
    while (size < elem) {
        size *= 2;
    }
    return size;
}

void jvr_array_new(jvr_array *arr, size_t elem_size, size_t count)
{
    arr->elem_sz = elem_size;
    arr->len = 0;
    if (count == 0) {
        arr->capacity = 0;
        arr->data = VK_NULL_HANDLE;
    } else {
        uint32_t new_count = round_up(count);
        arr->capacity = new_count;
        arr->data = malloc(elem_size * new_count);
        arr->len = count;
    }
}

void jvr_array_from(jvr_array *arr, void *elems, size_t count)
{
    arr->capacity = round_up(count);
    arr->len = count;
    arr->data = malloc(arr->elem_sz * arr->capacity);
    uint32_t i = 0;
    memcpy(arr->data, elems, count * arr->elem_sz);
}

void jvr_array_push(jvr_array *arr, void *elem)
{
    if (arr->capacity == 0) {
        arr->capacity = 4;
        arr->data = malloc(arr->elem_sz * arr->capacity);
    } else if (arr->len == arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->elem_sz * arr->capacity);
    }
    memcpy(arr->data + arr->elem_sz * arr->len, elem, arr->elem_sz);
    arr->len++;
}
void jvr_array_delete(jvr_array *arr)
{
    free(arr->data);
}
