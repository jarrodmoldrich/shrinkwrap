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

void html_prologue(FILE * output, pxl_size width, pxl_size height)
{
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

void html_epilogue(FILE * output)
{
        fprintf(output, "\t\t</script>\n");
        fprintf(output, "\t</body>\n");
        fprintf(output, "</html>\n");
}

void move(FILE * output, float x, float y)
{
        fprintf(output, "\t\t\tcontext.moveTo(%f, %f);\n", x * 4, y * 4);
}

void line(FILE * output, float x, float y)
{
        fprintf(output, "\t\t\tcontext.lineTo(%f, %f);\n", x * 4, y * 4);
}

void html_draw_triangles(FILE * output, array * vertArray, array * indexArray, const char * colour, float x,
                       float y)
{
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
                indices[0] = get_index(indexArray, index);
                indices[1] = get_index(indexArray, index+1);
                indices[2] = get_index(indexArray, index+2);
                verts[0] = get_vert(vertArray, indices[0]);
                verts[1] = get_vert(vertArray, indices[1]);
                verts[2] = get_vert(vertArray, indices[2]);
                move(output, verts[0]->x + x, verts[0]->y + y);
                line(output, verts[1]->x + x, verts[1]->y + y);
                line(output, verts[2]->x + x, verts[2]->y + y);
                line(output, verts[0]->x + x, verts[0]->y + y);
                index += 3;
                fprintf(output, "\t\t\tcontext.closePath();\n");
                fprintf(output, "\t\t\tcontext.fill();\n");
                fprintf(output, "\t\t\tcontext.stroke();\n");
        }
}

void htmlDrawCurve(FILE * out, curve * c, float x, float y)
{
        static const char * const c_colours[] = {"0, 255, 255", "0, 255, 0", "0, 0, 255",
                "255, 0, 0"};
        curvep * p = c->pointList;
        const char * const colour = c_colours[c->alphaType];
        fprintf(out, "\t\t\tcontext.fillStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(out, "\t\t\tcontext.beginPath();\n");
        float sx = p->vertex.x + x;
        float sy = p->vertex.y + y;
        move(out, sx - 2, sy - 2);
        line(out, sx + 2, sy - 2);
        line(out, sx - 2, sy + 2);
        fprintf(out, "\t\t\tcontext.closePath();\n");
        fprintf(out, "\t\t\tcontext.fill();\n");
        fprintf(out, "\t\t\tcontext.strokeStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(out, "\t\t\tcontext.beginPath();\n");
        int first = TRUE;
        vertp vert = NULL;
        assert(p);
        while (p) {
                vert = &p->vertex;
                float px = vert->x + x;
                float py = vert->y + y;
                if (first) {
                        first = FALSE;
                        move(out, px, py);
                } else {
                        line(out, px, py);
                }
                p = p->next;
        }
        fprintf(out, "\t\t\tcontext.stroke();\n");
        fprintf(out, "\t\t\tcontext.fillStyle=\"rgba(%s, 1)\"\n", colour);
        fprintf(out, "\t\t\tcontext.beginPath();\n");
        sx = vert->x + x;
        sy = vert->y + y;
        line(out, sx + 2, sy - 2);
        line(out, sx + 2, sy + 2);
        line(out, sx - 2, sy + 2);
        fprintf(out, "\t\t\tcontext.closePath();\n");
        fprintf(out, "\t\t\tcontext.fill();\n");
}

void html_draw_curves(FILE * output, curve_list * curves, float x, float y)
{
        curven * n = curves->head->next;
        while (n) {
                curve * c = n->curve;
                htmlDrawCurve(output, c, x, y);
                n = n->next;
        }
}

void save_diagnostic_html(FILE * out, shrinkwrap ** shrinkwraps, size_t count, pxl_size w, pxl_size h)
{
        html_prologue(out, w, h);
        while (count) {
                shrinkwrap * sw = *shrinkwraps;
                float x = sw->origX;
                float y = sw->origY;
                html_draw_triangles(out, sw->vertices, sw->indicesPartialAlpha, "0, 255, 255", x, y);
                html_draw_triangles(out, sw->vertices, sw->indicesFullAlpha, "255, 255, 0", x, y);
                shrinkwraps++;
                count--;
        }
        html_epilogue(out);
}
