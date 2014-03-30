//
//  shrinkwrap.h
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
#ifndef shrinkwrap_shrinkwrap_h
#define shrinkwrap_shrinkwrap_h

#include "xmlload.h"
#include "pixel_t.h"
#include "shrinkwrap_t.h"

// Function declarations
///////////////////////////////
// Generate a type pixel map that categorises each pixel based on alpha type
tpxl * generate_typemap(const uch * imageatlasrgba, pxl_pos x, pxl_pos y, pxl_size w, pxl_size h,
                            pxl_size rowwidth);

// Remove n-sized pixel gaps along the x-axis by consolidating them into partial-alpha blocks
// Note: will create a new type pixel map, be sure to destroy your old one if not used
tpxl * reduce_dither(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed);

// Dilate alpha left and right on the x-axis, according to continuities on the y-axis to include partial-alpha
// induced by bilinear filtering
// Note: will create a new type pixel map, be sure to destroy your old one if not used
tpxl * dilate_alpha(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed);

// Bleeds alpha one pixel down the Y axis to mitigate problems with curve_list not accounting for the last pixel
tpxl * yshift_alpha(const tpxl * tpixels, pxl_size w, pxl_size h);

// Take the image lines and return segments of vertex boundaries between different alpha types
// Note: imageLines can be destroyed safely after this operation
curve_list * build_curves(const tpxl * tpixels, pxl_size w, pxl_size h);

// Optimise a downward segment of vertices - bleed left if changing to partial alpha, right changing from partial-alpha
// Note: Will mutate curve geometries in-place
void smooth_curves(curve_list * cl, float bleed, pxl_size w, pxl_size h);

// Iterates through each downward vertex list of each section and creates 2 indexed triangle lists for full and partial
// alpha with one final vertex list
// Note: Safe to destroy geometrySections after this process
shrinkwrap * triangulate(const curve_list * curve_list);

// Set UV's to frame in texture space and translate geometry by the frames offset if desired
// Note: Will mutate curve geometries in-place
void set_texture_coordinates(shrinkwrap * geometry, float framex, float framey, float texturewidth,
                                 float textureheight, float frameoffsetx, float frameoffsety);

// Destructors
///////////////////////////////
void destroy_curves(curve_list * todestroy);
void destroy_shrinkwrap(shrinkwrap * todestroy);
#endif
