//
//  shrinkwrap_internal_t.h
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
#ifndef shrinkwrap_shrinkwrap_internal_t_h
#define shrinkwrap_shrinkwrap_internal_t_h

#include "../shrinkwrap_t.h"
#include "../pixel_t.h"

// Defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

// Enumerations
///////////////////////////////
typedef enum alpha_enum {
        ALPHA_ZERO = 0x00,
        ALPHA_PARTIAL = 0x01,
        ALPHA_FULL = 0x02,
        ALPHA_INVALID = 0x04,
        ALPHA_FLAG_MARKDITHER_X = 0x08,
        ALPHA_FLAG_MARKDITHER_Y = 0x10,
        ALPHA_FLAG_MARKDITHER_FULL = ALPHA_FLAG_MARKDITHER_X | ALPHA_FLAG_MARKDITHER_Y,
        ALPHA_ANYALPHA = ALPHA_PARTIAL | ALPHA_FULL
} alpha;
static inline int isAlpha(alpha value) {return (value & ALPHA_ANYALPHA) != 0;}

typedef enum conserve_enum {
        CONSERVE_NONE,
        CONSERVE_LEFT,
        CONSERVE_RIGHT
} conserve;

typedef enum preserve_enum {
        PRESERVE_CANREMOVE = 0,
        PRESERVE_WILLREMOVE = 1,
        PRESERVE_DONOTREMOVE = 2,
        PRESERVE_DEBUG = 4
} preserve;

typedef enum pixel_find_enum {
        FIND_NO,
        FIND_YES,
        FIND_TERMINATE
} pixel_find;

// Structs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef size_t idx;
static const pxl_size c_pixelSize = 4;
static const uch c_default_threshold = 255;

struct curve_node_struct;
typedef struct curve_node_struct CN;

struct curve_point_struct;
typedef struct curve_point_struct CP;

struct curve_struct;
typedef struct curve_struct C;
        
struct curve_point_struct {
        vert vertex;
        uint32_t index;
        CP * next;
        CN * scanlineList;
        uch preserve;
        uch moved;
        float newx;
};
static const size_t cp_size = sizeof(CP);

struct curve_struct {
        CP * pointList;
        CP * removed;
        alpha alphaType;
};
static const size_t c_size = sizeof(C);

struct curve_node_struct {
        C * curve;
        CP * point;
        struct curve_node_struct * next;
};
static const size_t cn_size = sizeof(CN);

// curve geometry contains:
// 1) a linked list of all intersecting curves of each scanline in order of x position
// 2) a linked list of all curves
struct curves_list_struct {
        CN * scanlines;
        size_t linecount;
        CN * head;
        CN * lastCurve;
};
static const size_t curves_size = sizeof(curve_list);
#endif
