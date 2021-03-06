//
//  shrinkwrap_triangle.c
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
#include <assert.h>
#include "internal/shrinkwrap_triangle_internal.h"

// Exposed functions
///////////////////////////////////////////////////////////////////////////////
// Iterates through each downward vertex list of each section and creates 2 indexed triangle lists for full and partial
// alpha with one final vertex list
// Note: Safe to destroy geometrySections after this process
shrinkwrap * triangulate(curve_list * cl)
{
        uint32_t numVerts = assign_indices(cl);
        shrinkwrap * sw = create_shrink_wrap(numVerts);
        add_vertices(sw, cl);
        // Iterate through each curve
        CN * left = cl->head->next;
        while(left) {
                if (left->curve->alphaType == ALPHA_ZERO) {
                        goto nextCurve;
                }
                int rightSide = TRUE;
                alpha a = left->curve->alphaType;
                // Get next point on RIGHT curve
                const CN * right = find_next_curve(left->point, left->curve, TRUE);
                const CP * pl = left->point;
                const CP * pl2 = NULL;
                CP * pr = right->point;
                CP * pr2 = pr->next;
                const CP * prprev = NULL;
                // While curve has more than one point remaining
                while (TRUE) {
                        if (rightSide) {
                                while (pr2) {
                                        add_triangle(sw, a, pl, pr, pr2);
                                        const CP * prprev2 = prprev;
                                        prprev = pr;
                                        pr = pr2;
                                        pl2 = pl->next;
                                        if (pl2 != NULL) {
                                                int intersect = self_intersection(pl2, pr, prprev2, prprev, left->curve,
                                                                                 right);
                                                if (intersect == FALSE) break;
                                                if (pr->vertex.y > pl2->vertex.y) {
                                                        const CP * pl3 = pl2->next;
                                                        assert(pl3 != NULL);
                                                        add_triangle(sw, a, pl, pl2, pl3);
                                                        pl2 = pl3;
                                                        break;
                                                }
                                        }
                                        pr2 = get_next_right(pr, pl, left->curve, &right);
                                        if (pr2 == NULL) {
                                                assert(pl2 == NULL);
                                                goto nextCurve;
                                        }
                                }
                                rightSide = FALSE;
                        } else {
                                while (pl2) {
                                        add_triangle(sw, a, pl, pr, pl2);
                                        pl = pl2;
                                        const CN * prevRight = right;
                                        pr2 = get_next_right(pr, pl, left->curve, &right);
                                        if (pr2 != NULL) {
                                                int intersect = self_intersection(pl, pr2, prprev, pr, left->curve,
                                                                                 right);
                                                if (intersect == FALSE) break;
                                                if (pl->vertex.y > pr2->vertex.y) {
                                                        CP * pr3 = get_next_right(pr2, pr, left->curve, &right);
                                                        assert(pr3 != NULL);
                                                        add_triangle(sw, a, pr, pr2, pr3);
                                                        pr2 = pr3;
                                                        break;
                                                }
                                        }
                                        pl2 = pl->next;
                                        if (pl2 == NULL) {
                                                assert(pr2 == NULL);
                                                goto nextCurve;
                                        }
                                        right = prevRight;
                                }
                                rightSide = TRUE;
                        }
                }
        nextCurve:
                left = left->next;
        }
        return sw;
}

// Set UV's to frame in texture space and translate geometry by the frame offset if desired.
// Note: Will mutate geometry.
void set_texture_coordinates(shrinkwrap * geometry, float framex, float framey, float texturewidth,
                                 float textureheight, float frameoffsetx, float frameoffsety)
{
        array * verts = geometry->vertices;
        // Small optimisation - find reciprocal to avoid divide in loop.
        // Could cause inaccuracy - if so divide in loop.
        float invwidth = 1.0 / texturewidth;
        float invheight = 1.0 / textureheight;
        framex -= frameoffsetx;
        framey -= frameoffsety;
        size_t count = array_size(verts);
        for (size_t v = 0; v < count; v++) {
                vertp vert = get_vert(verts, v);
                vert->u = ((float)framex + vert->x) * invwidth;
                vert->v = ((float)framey + vert->y) * invheight;
                vert->x -= frameoffsetx;
                vert->y -= frameoffsety;
        }
}

// Internal functions
///////////////////////////////////////////////////////////////////////////////
// Determine if line 2 faces in to line 1's normal - usually 'down-facing' in terms of the curve.
float lineInwards(const vert * a1, const vert * b1, const vert * a2, const vert * b2, int right)
{
        float diff1X = a1->y - b1->y;
        float diff1Y = b1->x - a1->x;
        float diff2X = b2->x - a2->x;
        float diff2Y = b2->y - a2->y;
        int back = (diff1X * diff2X + diff1Y * diff2Y) < 0.0;
        float diff3X = b1->x - a1->x;
        float diff3Y = b1->y - a1->y;
        float dir = (diff3X * diff2X + diff3Y * diff2X);
        if (right) dir *= -1.0;
        return back && (dir > 0.0);
}

// Determine if line intersects with any line segments continuing from the left curve point provided.
// TODO: Merge left and right implementation using function pointer and context data.
int self_intersection_curve_left(const vert * a1, const vert * b1, float stopy, const CP * first)
{
        const CP * prev = first;
        const CP * next = prev->next;
        int start = TRUE;
        while (next) {
                const vert * vert1 = &prev->vertex;
                const vert * vert2 = &next->vertex;
                if (start == TRUE) {
                        int concave = lineInwards(a1, b1, vert1, vert2, FALSE);
                        if (concave) return TRUE;
                        start = FALSE;
                }
                if (vert1->y <= stopy && vert2->y <= stopy) return FALSE;
                if (intersect(a1, b1, vert1, vert2)) return TRUE;
                prev = next;
                next = prev->next;
        }
        return FALSE;
}

// Determine if line intersects with any line segments continuing from the right curve provided.
// TODO: Merge left and right implementation using function pointer and context data.
int self_intersection_curve_right(const vert * a1, const vert * b1, float stopy, C * left,
                               const CN * right, const vert * before2, const vert * before,
                               CP * first, const CP * l)
{
        if (before2 != NULL && intersect(a1, b1, before2, before)) return TRUE;
        CP * prev = first;
        CP * next = get_next_right(prev, l, left, &right);
        int start = TRUE;
        while (next) {
                const vert * vert1 = &prev->vertex;
                const vert * vert2 = &next->vertex;
                if (start == TRUE) {
                        int concave = lineInwards(a1, b1, vert1, vert2, TRUE);
                        if (concave) return TRUE;
                        start = FALSE;
                }
                if (vert1->y >= stopy && vert2->y >= stopy) return FALSE;
                if (intersect(a1, b1, vert1, vert2)) return TRUE;
                prev = next;
                next = get_next_right(prev, l, left, &right);
        }
        return FALSE;
}

// Determine if the left and right curve_list intersect the line provided.
int self_intersection(const CP * l, CP * r, const CP * rprev2,
                     const CP * rprev, C * left, const CN * right)
{
        const vert * a1 = &l->vertex;
        const vert * b1 = &r->vertex;
        float maxy = (a1->y > b1->y) ? a1->y : b1->y;
        int intersection = 0;
        intersection = self_intersection_curve_left(a1, b1, maxy, l);
        if (intersection) return TRUE;
        const vert * prevvert = &rprev->vertex;
        const vert * prev2vert = (rprev2) ? &rprev2->vertex : NULL;
        return self_intersection_curve_right(a1, b1, maxy, left, right, prev2vert, prevvert, r, l);
}

// Sequentially visit curve_list and their points and add incrementing indices.
uint32_t assign_indices(curve_list * cl)
{
        CN * n = cl->head->next;
        uint32_t i = 0;
        while(n) {
                CP * p = n->point;
                while(p) {
                        p->index = i;
                        i++;
                        p = p->next;
                }
                n = n->next;
        }
        return i;
}

shrinkwrap * create_shrink_wrap(uint32_t numverts)
{
        shrinkwrap * sw = (shrinkwrap *)malloc(shrinkwrap_size);
        uint32_t estimate = numverts + numverts/3 * 2;
        sw->vertices = array_create(numverts, sizeof(vert));
        sw->indicesFullAlpha = array_create(estimate, sizeof(uint32_t));
        sw->indicesPartialAlpha = array_create(estimate, sizeof(uint32_t));
        return sw;
}

void destroy_shrinkwrap(shrinkwrap * sw) 
{
        array_destroy(sw->indicesFullAlpha);
        array_destroy(sw->indicesPartialAlpha);
        array_destroy(sw->vertices);
        free(sw);
}

// Visit each curve and their points and add vertices to shrinkwrap.
void add_vertices(shrinkwrap * sw, curve_list * cl)
{
        CN * node = cl->head->next;
        while(node) {
                CP * point = node->point;
                while(point) {
                        vertp v = add_vert(sw->vertices);
                        v->x = point->vertex.x;
                        v->y = point->vertex.y;
                        point = point->next;
                }
                node = node->next;
        }
}

// Determine if two points share the exact same floating-point coordinates.
int point_is_same(const CP * a, const CP * b)
{
        const vert * va = &a->vertex;
        const vert * vb = &b->vertex;
        return va->x == vb->x && va->y == vb->y;
}

// Determine if any of the points on the triangle are the same.
int triangle_is_degenerate(const CP * p1, const CP * p2, const CP * p3)
{
        return point_is_same(p1, p2) || point_is_same(p2, p3) || point_is_same(p3, p1);
}

// Add triangle indices to shrinkwrap.
void add_triangle(shrinkwrap * sw, alpha a, const CP * p1, const CP * p2,
                 const CP * p3)
{
        assert(a != ALPHA_ZERO && a != ALPHA_INVALID);
        array * array = (a == ALPHA_FULL) ? sw->indicesFullAlpha : sw->indicesPartialAlpha;
//        printf("%6.3f %6.3f -> %6.3f %6.3f -> %6.3f %6.3f\n", p1->vertex.x, p1->vertex.y, p2->vertex.x, p2->vertex.y,
//               p3->vertex.x, p3->vertex.y);
        if (triangle_is_degenerate(p1, p2, p3)) return;
        *add_index(array) = p1->index;
        *add_index(array) = p2->index;
        *add_index(array) = p3->index;
}

// Returns TRUE if ray 1 and 2 are intersecting.
int intersect(const vert * a1, const vert * b1, const vert * a2, const vert * b2)
{
        // Lifted from: http://wiki.processing.org/w/Line-Line_intersection
        // @author Ryan Alexander
        float bx = b1->x - a1->x;
        float by = b1->y - a1->y;
        float dx = b2->x - a2->x;
        float dy = b2->y - a2->y;
        float b_dot_d_perp = bx * dy - by * dx;
        if(b_dot_d_perp == 0) return FALSE;
        float cx = a2->x - a1->x;
        float cy = a2->y - a1->y;
        float t = (cx * dy - cy * dx) / b_dot_d_perp;
        if(t <= 0 || t >= 1) return FALSE;
        float u = (cx * by - cy * bx) / b_dot_d_perp;
        int result = (u > 0 && u < 1);
//        if (result) {
//                printf("Intersection (%5.3f, %5.3f) (%5.3f, %5.3f) (%5.3f, %5.3f) (%5.3f, %5.3f)\n",
//                       a1->x, a1->y, b1->x, b1->y, a2->x, a2->y, b2->x, b2->y);
//        }
        return result;
}

// Find next point on the right with y equal/greater and directly subsequent
// to the left hand curve (unless curve is ending).
CP * get_next_right(CP * pr, const CP * pl, C * left,
                                 const CN ** inOutRight)
{
        const CN * newRight = find_next_curve(pr, left, TRUE);
        const CN * rightCurve = *inOutRight;
        CP * pr2;
        if (newRight == NULL) {
                return NULL;
        }
        int rightChanged = rightCurve->curve != newRight->curve;
        if (rightChanged) {
                pr2 = newRight->point;
                rightCurve = newRight;
        } else {
                pr2 = pr->next;
                if (pr2 && point_line_has_curve(pr2, left) == FALSE) {
                        return NULL;
                }
        }
        if (pr2 == NULL) {
                return NULL;
        }
        *inOutRight = rightCurve;
        return pr2;
}
