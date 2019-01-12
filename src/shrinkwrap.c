//
//  shrinkwrap.c
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
#include <math.h>
#include "shrinkwrap.h"
#include "shrinkwrap_t.h"
#include "internal/shrinkwrap_internal_t.h"

// Shrink wrap methodology
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A shrink wrap entails processing a bitmap and creating 2 non-overlapping geometries that represent the full and
// partial alpha sections respectively.  This is to reduce the GPU load on older iDevices where 2D sprites often have
// large semi-alpha sprites by increasing geometric complexity. By making a geometry that has full-alpha areas
// separated, alpha blending can be avoided for those pixels.  By reducing the partial-alpha section to disclude
// no-alpha sections, alpha blend can be further reduced.  The final mesh must have very few triangles to avoid
// unnecessary CPU burden.
//
// Shrink wrapping currently involves 6 distinct steps:
//
// 1) Convert bitmap into pixels with 3 alpha states - none/partial/full.
// 2) Bleed partial-alpha pixels into their neighbours to account for bilinear filtering and reduce alpha complexitity.
//    (TODO: Make this run on the Y axis as well)
// 3) Run a scan-line dithering filter that replaces high-frequency alpha changes with partial-alpha blocks.
// 4) Use simple X-axis scan-line edge detection to generate curves.
// 5) Reduce complexity of curves by removing superfluous points.
// 6) Iterate through all curves - find right-hand side pairs - to generate triangles for final geometry complete
//    with appropriate UVs.
//
// Currently: Outline generation, optimisation and triangulation are very simplistic and work by scanning down the
//            x-axis.  This was to keep triangle topology simple to process and to simplify the problem of mesh
//            optimisation.  More sophisticated geometry creation and optimisation could be used.
//
// Future: Start with a partial-alpha supporting quad and use recursively divided convex hulls to determine the
//         shapes of no-alpha and full-alpha regions.  Stitch the shapes together and use conventional
//         polygon optimisation and triangulation techniques from other open source libraries.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
