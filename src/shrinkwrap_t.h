//
//  shrinkwrap_t.h
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
#ifndef shrinkwrap_shrinkwrap_t_h
#define shrinkwrap_shrinkwrap_t_h

#include <stddef.h>
#include <stdint.h>
#include "array.h"

struct vert_struct {
        float x;
        float y;
        float u;
        float v;
};
typedef struct vert_struct vert;
typedef vert * vertp;

typedef struct shrinkwrap_struct {
        array * vertices;
        array * indicesPartialAlpha;
        array * indicesFullAlpha;
        float origX;
        float origY;
} shrinkwrap;

// Forward declarations
///////////////////////////////
struct curves_list_struct;
typedef struct curves_list_struct curve_list;

static inline vertp get_vert(array * array, size_t i) {return (vertp)array_get(array, i);}
static inline uint32_t get_index(array * array, size_t i) {return *(uint32_t *)array_get(array, i);}
static inline vertp add_vert(array * array) {return (vertp)array_push(array);}
static inline uint32_t * add_index(array * array) {return (uint32_t *)array_push(array);}

static const size_t shrinkwrap_size = sizeof(shrinkwrap);
#endif
