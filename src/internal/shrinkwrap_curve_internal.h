//
//  shrinkwrap_curve_internal.c
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
#ifndef shrinkwrap_shrinkwrap_curve_internal_t_h
#define shrinkwrap_shrinkwrap_curve_internal_t_h

#include "shrinkwrap_internal_t.h"

// Point functions
curvep * new_point(float x, float y, curven * scanline);
int is_end_point(const curven * n, pxl_diff step);
int is_first(const curvep * p, const curve * c);
int is_last(const curvep * p);
curvep * append_point_to_curve(curvep * lastPoint, curve_list * cl, float x, float y);
curvep * prepend_point_to_curve(curven * n, float x, float y, curve_list * cl);
curvep * add_point(curven * left, curven * right, float newx, float newY, curve_list * cl, pxl_diff step,
                      curvep * lastPoint);
void remove_point(curve * c, curvep * p);

// Curve functions
curvep * init_curve(curve * c, float x, float y, curve_list * cl, alpha a);
curvep * get_last_point(curve * c);
curve * destroy_curve(curve * c);

// Curve lists
curven * create_node(curve * c, curvep * p);
curve_list * create_curve_list(size_t scanlines);
void try_add_curve(curve_list * cl, alpha type, alpha lastType, const tpxl * tpixels, float x,
                               float y, pxl_size w, pxl_size h);
void add_curve(curve_list * cl, curve * c, curvep * p);
curven * find_master_node(curve_list * cl, curve * c);

// Scanline functions
curven * add_curve_to_scanline(curve_list * cl, size_t index, curve * c, curvep * p);
curven * find_next_curve_on_scanline(curven * n, curve * c, int skip);
void remove_curve_from_scanline(curven * scanlinelist, curve * c, curvep * p);

// Curve adjacency
curven * find_curve_at(curve_list * cl, pxl_pos x, pxl_pos y);
curven * find_next_curve_on_line(curve_list * cl, pxl_pos y, curve * c);
curven * find_next_curve(curvep * p, curve * c, int skip);
curven * find_prev_curve(curvep * p, curve * c);
int comes_before(const curven * scanLine, const curve * a, const curve * b);

// Typemap conversion
pxl_pos find_next_end(const tpxl * tpixels, pxl_pos y, pxl_size w, int * outTerminate);
void find_next_pixel(const tpxl * tpixels, pxl_pos startx, pxl_pos currenty, pxl_size w, pxl_pos * outx,
                                 pixel_find * found);

// Clean-up
void fix_curve_ending(curvep * ending, curven * n, curve_list * cl, pxl_diff step, pxl_size w,
                    pxl_size h, pxl_size bleed);
void fix_curve_endings(curve_list * cl, pxl_size w, pxl_size h, pxl_size bleed);
void collapse_curve_ending(curvep * ending, curve * c, curve_list * cl, pxl_diff step, pxl_size w,
                         pxl_size h, pxl_size bleed);
void collapse_curve_endings(curve_list * cl, pxl_size w, pxl_size h, pxl_size bleed);
void smooth_fix_up(curve_list * cl);

// Optimisation
conserve conserve_direction(const curven * scanline, curve * c);
void protect_right_point(curvep * p);
void protect_subdivision_points(curve_list * cl, pxl_size w);
float getnewx(curvep * p);
float optimise(curvep * p1, curvep * p2, curvep * p3, conserve conserve);
float calculate_average_difference(vertp p1, vertp p2, float startx, float starty, float newx, float endY);
float calculate_max_difference_on_curve(curvep * p1, curvep * p3, float new23);
float findx(curven * curveNode, float y);
float limit_point(float x, curvep * p, float w);
curvep * find_next_removeable(curvep * p, curvep ** outPrev);
curvep * find_next_nonremoved(curvep * p);
curven * find_yrelative_point(curve_list * cl, float x, float y, int32_t step, size_t h);
void remove_points(curve * c);

// Validation
int validate_scanlines(const curve_list * cl);
int validate_scanline(curven * scanLine);
int validate_curves(const curve_list * cl);
#endif // shrinkwrap_shrinkwrap_curve_internal_t_h
