//
//  shrinkwrap_triangle_internal.h
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
#ifndef shrinkwrap_triangle_internal_h
#define shrinkwrap_triangle_internal_h
#include "shrinkwrap_internal_t.h"

curven * find_next_curve(curvep * p, curve * c, int skip);
int point_line_has_curve(const curvep * p, const curve * c);

int self_intersection_curve_left(const vert * a1, const vert * b1, float stopy, const curvep * first);
int self_intersection_curve_right(const vert * a1, const vert * b1, float stopy, curve * left,
                               const curven * right, const vert * before2, const vert * before,
                               curvep * first, const curvep * l);
int self_intersection(const curvep * l, curvep * r, const curvep * rprev2,
                     const curvep * rprev, curve * left, const curven * right);
shrinkwrap * create_shrink_wrap(uint32_t numVertices);
uint32_t assign_indices(curve_list * cl);
void add_vertices(shrinkwrap * sw, curve_list * cl);
int point_is_same(const curvep * a, const curvep * b);
int triangle_is_degenerate(const curvep * p1, const curvep * p2, const curvep * p3);
void add_triangle(shrinkwrap * shrinkwrap, alpha a, const curvep * p1, const curvep * p2,
                 const curvep * p3);
int intersect(const vert * a1, const vert * b1, const vert * a2, const vert * b2);
curvep * get_next_right(curvep * pr, const curvep * pl, curve * left, const curven ** inOutRight);

float lineInwards(const vert * a1, const vert * b1, const vert * a2, const vert * b2, int right);
int self_intersection_curve_left(const vert * a1, const vert * b1, float stopy, const curvep * first);
int self_intersection_curve_right(const vert * a1, const vert * b1, float stopy, curve * left,
                               const curven * right, const vert * before2, const vert * before,
                               curvep * first, const curvep * l);
int self_intersection(const curvep * l, curvep * r, const curvep * rprev2,
                     const curvep * rprev, curve * left, const curven * right);

uint32_t assign_indices(curve_list * cl);
shrinkwrap * create_shrink_wrap(uint32_t numverts);
void destroy_shrinkwrap(shrinkwrap * sw);
void add_vertices(shrinkwrap * sw, curve_list * cl);
int point_is_same(const curvep * a, const curvep * b);
int triangle_is_degenerate(const curvep * p1, const curvep * p2, const curvep * p3);
void add_triangle(shrinkwrap * sw, alpha a, const curvep * p1, const curvep * p2,
                 const curvep * p3);
int intersect(const vert * a1, const vert * b1, const vert * a2, const vert * b2);
curvep * get_next_right(curvep * pr, const curvep * pl, curve * left,
                                 const curven ** inOutRight);
#endif
