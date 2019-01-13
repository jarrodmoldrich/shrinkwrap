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
CP * new_point(float x, float y, CN * scanline);
int is_end_point(const CN * n, pxl_diff step);
int is_first(const CP * p, const C * c);
int is_last(const CP * p);
CP * append_point_to_curve(CP * lastPoint, curve_list * cl, float x, float y);
CP * prepend_point_to_curve(CN * n, float x, float y, curve_list * cl);
CP * add_point(CN * left, CN * right, float newx, float newY, curve_list * cl, pxl_diff step,
                      CP * lastPoint);
void remove_point(C * c, CP * p);

// Curve functions
CP * init_curve(C * c, float x, float y, const curve_list * cl, alpha a);
CP * get_last_point(C * c);
C * destroy_curve(C * c);

// Curve lists
CN * create_node(C * c, CP * p);
curve_list * create_curve_list(size_t scanlines);
void try_add_curve(curve_list * cl, alpha type, alpha lastType, const tpxl * tpixels, float x,
                               float y, pxl_size w, pxl_size h);
void add_curve(curve_list * cl, C * c, CP * p);
CN * find_master_node(curve_list * cl, C * c);

// Scanline functions
CN * add_curve_to_scanline(curve_list * cl, size_t index, C * c, CP * p);
CN * find_next_curve_on_scanline(CN * n, C * c, int skip);
void remove_curve_from_scanline(CN * scanlinelist, C * c, CP * p);

// Curve adjacency
CN * find_curve_at(curve_list * cl, pxl_pos x, pxl_pos y);
CN * find_next_curve_on_line(curve_list * cl, pxl_pos y, C * c);
CN * find_next_curve(CP * p, C * c, int skip);
CN * find_prev_curve(CP * p, C * c);
int comes_before(const CN * scanLine, const C * a, const C * b);

// Typemap conversion
pxl_pos find_next_end(const tpxl * tpixels, pxl_pos y, pxl_size w, int * outTerminate);
void find_next_pixel(const tpxl * tpixels, pxl_pos startx, pxl_pos currenty, pxl_size w, pxl_pos * outx,
                                 pixel_find * found);

// Clean-up
void fix_curve_ending(CP * ending, CN * n, curve_list * cl, pxl_diff step, pxl_size w,
                    pxl_size h, pxl_size bleed);
void fix_curve_endings(curve_list * cl, pxl_size w, pxl_size h, pxl_size bleed);
void collapse_curve_ending(CP * ending, C * c, curve_list * cl, pxl_diff step, pxl_size w,
                         pxl_size h, pxl_size bleed);
void collapse_curve_endings(curve_list * cl, pxl_size w, pxl_size h, pxl_size bleed);
void smooth_fix_up(curve_list * cl);

// Optimisation
conserve conserve_direction(const CN * scanline, const C * c);
void protect_right_point(CP * p);
void protect_subdivision_points(curve_list * cl, pxl_size w);
float getnewx(CP * p);
float optimise(CP * p1, CP * p2, CP * p3, conserve conserve, float maxBleed);
float calculate_average_difference(vertp p1, vertp p2, float startx, float starty, float newx, float endY);
float calculate_max_difference_on_curve(CP * p1, CP * p3, float new23);
float findx(CN * curveNode, float y);
float limit_point(float x, CP * p, float w);
CP * find_next_removeable(CP * p, CP ** outPrev);
CP * find_next_nonremoved(CP * p);
CN * find_yrelative_point(curve_list * cl, float x, float y, int32_t step, size_t h);
void remove_points(C * c);

// Validation
int validate_scanlines(const curve_list * cl);
int validate_scanline(CN * scanLine);
int validate_curves(const curve_list * cl);
#endif // shrinkwrap_shrinkwrap_curve_internal_t_h
