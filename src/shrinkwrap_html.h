//
//  shrinkwrap_html.h
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
#ifndef shrinkwrap_shrinkwrap_html_h
#define shrinkwrap_shrinkwrap_html_h

#include <stdio.h>
#include "shrinkwrap_internal_t.h"
#include "pixel_t.h"

void htmlPrologue(FILE * output, pxl_size width, pxl_size height);

void htmlMoveTo(FILE * output, float x, float y);

void htmlLineTo(FILE * output, float x, float y);

void htmlEpilogue(FILE * output);

void htmlDrawTriangles(FILE * output, array_descp vertArray, array_descp indexArray, const char * colour, float x,
                       float y);

void htmlDrawCurves(FILE * output, curvesp curves, float x, float y);

void saveDiagnosticHtml(FILE * output, shrinkwrap_geometryp * geometry_list, size_t count, pxl_size width,
                        pxl_size height);


#endif
