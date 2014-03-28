//
//  array.c
//
//  shrinkwrap is a tool for triangulating bitmap alpha.
//  Copyright (c) 2014 Jarrod Moldrich. All rights reserved.
//
//  This file is part of shrinkwrap.
//
//  shrinkwrap is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//  shrinkwrap is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with shrinkwrap.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "array.h"

struct array_desc_struct {
        void * elements;
        size_t count;
        size_t capacity;
        size_t stride;
};
static const size_t array_desc_size = sizeof(array_desc);

array_descp array_create(size_t startCapacity, size_t stride) {
        assert(stride > 0);
        assert(startCapacity > 0);
        array_descp array = malloc(array_desc_size);
        array->stride = stride;
        array->count = 0;
        array->elements = malloc(stride * startCapacity);
        array->capacity = startCapacity;
        return array;
}

void array_resize(array_descp desc, size_t capacity) {
        assert(capacity > 0);
        void * elements = malloc(desc->stride * capacity);
        size_t count = (capacity < desc->count) ? capacity : desc->count;
        memcpy(elements, desc->elements, count * desc->stride);
        desc->elements = elements;
        desc->count = count;
        desc->capacity = capacity;
}

void * array_push(array_descp desc) {
        if (desc->count == desc->capacity) {
                array_resize(desc, desc->capacity * 2);
        }
        ptrdiff_t pos = (ptrdiff_t)desc->elements + desc->stride * desc->count;
        desc->count++;
        return (void *)pos;
}

void * array_get(array_descp desc, size_t i) {
        assert(i < desc->count);
        return (void*)((ptrdiff_t)desc->elements + i * desc->stride);
}

size_t array_size(array_descp desc) {
        return desc->count;
}