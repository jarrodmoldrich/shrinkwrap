//
//  shrinkwrap_html.c
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
#include <assert.h>
#include "shrinkwrap_html.h"

void htmlPrologue(FILE * output, pxl_size width, pxl_size height) {
        fprintf(output, "<!DOCTYPE HTML>\n");
        fprintf(output, "<html>\n");
        fprintf(output, "\t<head>\n");
        fprintf(output, "\t\t<style>\n");
        fprintf(output, "\t\t\tbody {\n");
        fprintf(output, "\t\t\t\tmargin: 0px;\n");
        fprintf(output, "\t\t\t\tpadding: 0px;\n");
        fprintf(output, "\t\t\t}\n");
        fprintf(output, "\t\t</style>\n");
        fprintf(output, "\t</head>\n");
        fprintf(output, "\t<body>\n");
        fprintf(output, "\t\t<img id=\"bg\" src=\"test.png\" width=\"4096\" height=\"2048\">\n");
        fprintf(output, "\t\t<div id='d1' style=\"position:absolute; top:0; left:0; z-index:1\">\n");
        fprintf(output, "\t\t\t<canvas id=\"myCanvas\" width=\"%ud\" height=\"%ud\"></canvas>\n", width * 4, height * 4);
        fprintf(output, "\t\t</div>\n");
        fprintf(output, "\t\t<script>\n");
        fprintf(output, "\t\t\tvar canvas = document.getElementById('myCanvas');\n");
        fprintf(output, "\t\t\tvar context = canvas.getContext('2d');\n");
//        fprintf(output, "\t\t\tvar img=document.getElementById(\"bg\");\n");
//        fprintf(output, "\t\t\tctx.drawImage(img,0,0);\n");
}

void htmlMoveTo(FILE * output, float x, float y) {
        fprintf(output, "\t\t\tcontext.moveTo(%f, %f);\n", x * 4, y * 4);
}

void htmlLineTo(FILE * output, float x, float y) {
        fprintf(output, "\t\t\tcontext.lineTo(%f, %f);\n", x * 4, y * 4);
}

void htmlEpilogue(FILE * output) {
        fprintf(output, "\t\t</script>\n");
        fprintf(output, "\t</body>\n");
        fprintf(output, "</html>\n");
}

void htmlDrawTriangles(FILE * output, array_descp vertArray, array_descp indexArray, const char * colour, float x,
                       float y) {
        size_t index = 0;
        size_t numIndices = array_size(indexArray);
        if (numIndices == 0) return;
        assert((numIndices % 3) == 0 && "Indices not divisible by 3!");
        
        uint32_t indices[3];
        vertp verts[3];
        while (index <= numIndices-3) {
                fprintf(output, "\t\t\tcontext.fillStyle=\"rgba(%s, .5)\"\n", colour);
                fprintf(output, "\t\t\tcontext.strokeStyle=\"rgba(%s, 1)\"\n", colour);
                fprintf(output, "\t\t\tcontext.beginPath();\n");
                indices[0] = getIndex(indexArray, index);
                indices[1] = getIndex(indexArray, index+1);
                indices[2] = getIndex(indexArray, index+2);
                verts[0] = getVert(vertArray, indices[0]);
                verts[1] = getVert(vertArray, indices[1]);
                verts[2] = getVert(vertArray, indices[2]);
                htmlMoveTo(output, verts[0]->x + x, verts[0]->y + y);
                htmlLineTo(output, verts[1]->x + x, verts[1]->y + y);
                htmlLineTo(output, verts[2]->x + x, verts[2]->y + y);
                htmlLineTo(output, verts[0]->x + x, verts[0]->y + y);
                index += 3;
                fprintf(output, "\t\t\tcontext.closePath();\n");
                fprintf(output, "\t\t\tcontext.fill();\n");
                fprintf(output, "\t\t\tcontext.stroke();\n");
        }
}

void htmlDrawCurve(FILE * output, curvep curve, float posX, float posY) {
        static const char * const c_colours[] = {"0, 255, 255", "0, 255, 0", "0, 0, 255",
                "255, 0, 0"};
        curve_pointp point = curve->pointList;
        const char * const colour = c_colours[curve->alphaType];
        fprintf(output, "\t\t\tcontext.fillStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(output, "\t\t\tcontext.beginPath();\n");
        float sx = point->vertex.x + posX;
        float sy = point->vertex.y + posY;
        htmlMoveTo(output, sx - 2, sy - 2);
        htmlLineTo(output, sx + 2, sy - 2);
        htmlLineTo(output, sx - 2, sy + 2);
        fprintf(output, "\t\t\tcontext.closePath();\n");
        fprintf(output, "\t\t\tcontext.fill();\n");
        fprintf(output, "\t\t\tcontext.strokeStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(output, "\t\t\tcontext.beginPath();\n");
        int first = TRUE;
        vertp vert = NULL;
        assert(point);
        while (point) {
                vert = &point->vertex;
                float x = vert->x + posX;
                float y = vert->y + posY;
                if (first) {
                        first = FALSE;
                        htmlMoveTo(output, x, y);
                } else {
                        htmlLineTo(output, x, y);
                }
                point = point->next;
        }
        fprintf(output, "\t\t\tcontext.stroke();\n");
        fprintf(output, "\t\t\tcontext.fillStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(output, "\t\t\tcontext.beginPath();\n");
        sx = vert->x + posX;
        sy = vert->y + posY;
        htmlLineTo(output, sx + 2, sy - 2);
        htmlLineTo(output, sx + 2, sy + 2);
        htmlLineTo(output, sx - 2, sy + 2);
        fprintf(output, "\t\t\tcontext.closePath();\n");
        fprintf(output, "\t\t\tcontext.fill();\n");
}

void htmlDrawCurves(FILE * output, curvesp curves, float x, float y) {
        curve_nodep curveNode = curves->head->next;
        while (curveNode) {
                curvep curve = curveNode->curve;
                htmlDrawCurve(output, curve, x, y);
                curveNode = curveNode->next;
        }
}

void saveDiagnosticHtml(FILE * output, shrinkwrap_geometryp * geometry_list, size_t count, pxl_size width,
                        pxl_size height) {
        htmlPrologue(output, width, height);
        while (count) {
                shrinkwrap_geometryp geometry = *geometry_list;
                float x = geometry->origX;
                float y = geometry->origY;
                htmlDrawTriangles(output, geometry->vertices, geometry->indicesPartialAlpha, "0, 255, 255", x, y);
                htmlDrawTriangles(output, geometry->vertices, geometry->indicesFullAlpha, "255, 255, 0", x, y);
                geometry_list++;
                count--;
        }
        htmlEpilogue(output);
}
