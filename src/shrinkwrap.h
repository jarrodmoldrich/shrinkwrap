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
tpxl * generateTypePixelMap(const uch * imageAtlasRGBA, pxl_pos x, pxl_pos y, pxl_size width, pxl_size height,
                            pxl_size rowWidth);

// Remove n-sized pixel gaps along the x-axis by consolidating them into partial-alpha blocks
// Note: will create a new type pixel map, be sure to destroy your old one if not used
tpxl * reduceStateDither(const tpxl * typePixels, pxl_size width, pxl_size height, pxl_size bleed);

// Dilate alpha left and right on the x-axis, according to continuities on the y-axis to include partial-alpha
// induced by bilinear filtering
// Note: will create a new type pixel map, be sure to destroy your old one if not used
tpxl * dilateAlpha(const tpxl * typePixels, pxl_size width, pxl_size height, pxl_size bleed);

// Bleeds alpha one pixel down the Y axis to mitigate problems with curves not accounting for the last pixel
tpxl * yShiftAlpha(const tpxl * typePixels, pxl_size width, pxl_size height);
        
// Take the image lines and return segments of vertex boundaries between different alpha types
// Note: imageLines can be destroyed safely after this operation
curvesp buildCurves(const tpxl * typePixels, pxl_size width, pxl_size height);

// Optimise a downward segment of vertices - bleed left if changing to partial alpha, right changing from partial-alpha
// Note: Will mutate curve geometries in-place
void smoothCurves(curvesp curves, float maxBleed, pxl_size width, pxl_size height);

// Iterates through each downward vertex list of each section and creates 2 indexed triangle lists for full and partial
// alpha with one final vertex list
// Note: Safe to destroy geometrySections after this process
shrinkwrap_geometryp triangulate(const curvesp curves);

// Set UV's to frame in texture space and translate geometry by the frames offset if desired
// Note: Will mutate curve geometries in-place
void fixGeometriesInTextureSpace(shrinkwrap_geometryp geometry, float frameX, float frameY, float textureWidth,
                                 float textureHeight, float frameOffsetX, float frameOffsetY);

// Destructors
///////////////////////////////
void destroyCurves(curvesp toDestroy);
void destroyShrinkWrapGeometry(shrinkwrap_geometryp toDestroy);
#endif
