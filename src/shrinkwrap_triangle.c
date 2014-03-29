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
#include "shrinkwrap_internal_t.h"
#include "shrinkwrap_internal.h"

// Internal functions (forward declaration)
///////////////////////////////////////////////////////////////////////////////
int selfIntersectionCurveLeft(const vert * a1, const vert * b1, float stopY, const curve_point * first);
int selfIntersectionCurveRight(const vert * a1, const vert * b1, float stopY, curve * left,
                               const curve_node * right, const vert * before2, const vert * before,
                               curve_point * first, const curve_point * l);
int selfIntersection(const curve_point * l, curve_point * r, const curve_point * rPrev2,
                     const curve_point * rPrev, curve * left, const curve_node * right);
shrinkwrap_geometryp createShrinkWrap(uint32_t numVertices);
uint32_t assignIndices(curvesp curves);
void addVertices(shrinkwrap_geometryp shrinkWrap, curvesp curves);
int pointIsSame(const curve_point * a, const curve_point * b);
int triangleIsDegenerate(const curve_point * p1, const curve_point * p2, const curve_point * p3);
void addTriangle(shrinkwrap_geometryp shrinkwrap, alpha_type type, const curve_point * p1, const curve_point * p2,
                 const curve_point * p3);
int intersect(const vert * a1, const vert * b1, const vert * a2, const vert * b2);
curve_point * getNextRight(curve_point * pr, const curve_point * pl, curve * left, const curve_node ** inOutRight);

// Exposed functions
///////////////////////////////////////////////////////////////////////////////
// Iterates through each downward vertex list of each section and creates 2 indexed triangle lists for full and partial
// alpha with one final vertex list
// Note: Safe to destroy geometrySections after this process
shrinkwrap_geometryp triangulate(const curvesp curves) {
        uint32_t numVerts = assignIndices(curves);
        shrinkwrap_geometryp shrinkWrap = createShrinkWrap(numVerts);
        addVertices(shrinkWrap, curves);
        // Iterate through each curve
        curve_node * left = curves->head->next;
        while(left) {
                if (left->curve->alphaType == ALPHA_ZERO) {
                        goto nextCurve;
                }
                int rightSide = TRUE;
                alpha_type type = left->curve->alphaType;
                // Get next point on RIGHT curve
                const curve_node * right = findNextCurve(left->point, left->curve, TRUE);
                const curve_point * pl = left->point;
                const curve_point * pl2 = NULL;
                curve_point * pr = right->point;
                curve_point * pr2 = pr->next;
                const curve_point * prPrev = NULL;
                // While curve has more than one point remaining
                while (TRUE) {
                        if (rightSide) {
                                while (pr2) {
                                        addTriangle(shrinkWrap, type, pl, pr, pr2);
                                        const curve_point * prPrev2 = prPrev;
                                        prPrev = pr;
                                        pr = pr2;
                                        pl2 = pl->next;
                                        if (pl2 != NULL) {
                                                int intersect = selfIntersection(pl2, pr, prPrev2, prPrev, left->curve,
                                                                                 right);
                                                if (intersect == FALSE) break;
                                                if (pr->vertex.y > pl2->vertex.y) {
                                                        const curve_point * pl3 = pl2->next;
                                                        assert(pl3 != NULL);
                                                        addTriangle(shrinkWrap, type, pl, pl2, pl3);
                                                        pl2 = pl3;
                                                        break;
                                                }
                                        }
                                        pr2 = getNextRight(pr, pl, left->curve, &right);
                                        if (pr2 == NULL) {
                                                assert(pl2 == NULL);
                                                goto nextCurve;
                                        }
                                }
                                rightSide = FALSE;
                        } else {
                                while (pl2) {
                                        addTriangle(shrinkWrap, type, pl, pr, pl2);
                                        pl = pl2;
                                        const curve_node * prevRight = right;
                                        pr2 = getNextRight(pr, pl, left->curve, &right);
                                        if (pr2 != NULL) {
                                                int intersect = selfIntersection(pl, pr2, prPrev, pr, left->curve,
                                                                                 right);
                                                if (intersect == FALSE) break;
                                                if (pl->vertex.y > pr2->vertex.y) {
                                                        curve_point * pr3 = getNextRight(pr2, pr, left->curve, &right);
                                                        assert(pr3 != NULL);
                                                        addTriangle(shrinkWrap, type, pr, pr2, pr3);
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
        return shrinkWrap;
}

// Set UV's to frame in texture space and translate geometry by the frame offset if desired.
// Note: Will mutate geometry.
void fixGeometriesInTextureSpace(shrinkwrap_geometryp geometry, float frameX, float frameY, float textureWidth,
                                 float textureHeight, float frameOffsetX, float frameOffsetY) {
        array_descp verts = geometry->vertices;
        // Small optimisation - find reciprocal to avoid divide in loop.
        // Could cause inaccuracy - if so divide in loop.
        float invWidth = 1.0 / textureWidth;
        float invHeight = 1.0 / textureHeight;
        frameX -= frameOffsetX;
        frameY -= frameOffsetY;
        size_t count = array_size(verts);
        for (size_t v = 0; v < count; v++) {
                vertp vert = getVert(verts, v);
                vert->u = ((float)frameX + vert->x) * invWidth;
                vert->v = ((float)frameY + vert->y) * invHeight;
                vert->x -= frameOffsetX;
                vert->y -= frameOffsetY;
        }
}


// Internal functions
///////////////////////////////////////////////////////////////////////////////
// Determine if line 2 faces in to line 1's normal - usually 'down-facing' in terms of the curve.
float lineInwards(const vert * a1, const vert * b1, const vert * a2, const vert * b2, int right) {
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
int selfIntersectionCurveLeft(const vert * a1, const vert * b1, float stopY, const curve_point * first) {
        const curve_point * prev = first;
        const curve_point * next = prev->next;
        int start = TRUE;
        while (next) {
                const vert * vert1 = &prev->vertex;
                const vert * vert2 = &next->vertex;
                if (start == TRUE) {
                        int concave = lineInwards(a1, b1, vert1, vert2, FALSE);
                        if (concave) return TRUE;
                        start = FALSE;
                }
                if (vert1->y <= stopY && vert2->y <= stopY) return FALSE;
                if (intersect(a1, b1, vert1, vert2)) return TRUE;
                prev = next;
                next = prev->next;
        }
        return FALSE;
}

// Determine if line intersects with any line segments continuing from the right curve provided.
// TODO: Merge left and right implementation using function pointer and context data.
int selfIntersectionCurveRight(const vert * a1, const vert * b1, float stopY, curve * left,
                               const curve_node * right, const vert * before2, const vert * before,
                               curve_point * first, const curve_point * l) {
        if (before2 != NULL && intersect(a1, b1, before2, before)) return TRUE;
        curve_point * prev = first;
        curve_point * next = getNextRight(prev, l, left, &right);
        int start = TRUE;
        while (next) {
                const vert * vert1 = &prev->vertex;
                const vert * vert2 = &next->vertex;
                if (start == TRUE) {
                        int concave = lineInwards(a1, b1, vert1, vert2, TRUE);
                        if (concave) return TRUE;
                        start = FALSE;
                }
                if (vert1->y >= stopY && vert2->y >= stopY) return FALSE;
                if (intersect(a1, b1, vert1, vert2)) return TRUE;
                prev = next;
                next = getNextRight(prev, l, left, &right);
        }
        return FALSE;
}

// Determine if the left and right curves intersect the line provided.
int selfIntersection(const curve_point * l, curve_point * r, const curve_point * rPrev2,
                     const curve_point * rPrev, curve * left, const curve_node * right) {
        const vert * a1 = &l->vertex;
        const vert * b1 = &r->vertex;
        float maxY = (a1->y > b1->y) ? a1->y : b1->y;
        int intersection = 0;
        intersection = selfIntersectionCurveLeft(a1, b1, maxY, l);
        if (intersection) return TRUE;
        const vert * prevVert = &rPrev->vertex;
        const vert * prev2Vert = (rPrev2) ? &rPrev2->vertex : NULL;
        return selfIntersectionCurveRight(a1, b1, maxY, left, right, prev2Vert, prevVert, r, l);
}

// Sequentially visit curves and their points and add incrementing indices.
uint32_t assignIndices(curvesp curves) {
        curve_node * node = curves->head->next;
        uint32_t index = 0;
        while(node) {
                curve_point * point = node->point;
                while(point) {
                        point->index = index;
                        index++;
                        point = point->next;
                }
                node = node->next;
        }
        return index;
}

shrinkwrap_geometryp createShrinkWrap(uint32_t numVertices) {
        shrinkwrap_geometryp shrinkWrap = (shrinkwrap_geometryp)malloc(shrinkwrap_geometry_size);
        uint32_t estimate = numVertices + numVertices/3 * 2;
        shrinkWrap->vertices = array_create(numVertices, sizeof(vert));
        shrinkWrap->indicesFullAlpha = array_create(estimate, sizeof(uint32_t));
        shrinkWrap->indicesPartialAlpha = array_create(estimate, sizeof(uint32_t));
        return shrinkWrap;
}

// Visit each curve and their points and add vertices to shrinkwrap.
void addVertices(shrinkwrap_geometryp shrinkWrap, curvesp curves) {
        curve_node * node = curves->head->next;
        while(node) {
                curve_point * point = node->point;
                while(point) {
                        vertp v = addVert(shrinkWrap->vertices);
                        v->x = point->vertex.x;
                        v->y = point->vertex.y;
                        point = point->next;
                }
                node = node->next;
        }
}

// Determine if two points share the exact same floating-point coordinates.
int pointIsSame(const curve_point * a, const curve_point * b) {
        const vert * va = &a->vertex;
        const vert * vb = &b->vertex;
        return va->x == vb->x && va->y == vb->y;
}

// Determine if any of the points on the triangle are the same.
int triangleIsDegenerate(const curve_point * p1, const curve_point * p2, const curve_point * p3) {
        return pointIsSame(p1, p2) || pointIsSame(p2, p3) || pointIsSame(p3, p1);
}

// Add triangle indices to shrinkwrap.
void addTriangle(shrinkwrap_geometryp shrinkwrap, alpha_type type, const curve_point * p1, const curve_point * p2,
                 const curve_point * p3) {
        assert(type != ALPHA_ZERO && type != ALPHA_INVALID);
        array_descp array = (type == ALPHA_FULL) ? shrinkwrap->indicesFullAlpha : shrinkwrap->indicesPartialAlpha;
//        printf("%6.3f %6.3f -> %6.3f %6.3f -> %6.3f %6.3f\n", p1->vertex.x, p1->vertex.y, p2->vertex.x, p2->vertex.y,
//               p3->vertex.x, p3->vertex.y);
        if (triangleIsDegenerate(p1, p2, p3)) return;
        *addIndex(array) = p1->index;
        *addIndex(array) = p2->index;
        *addIndex(array) = p3->index;
}

// Returns TRUE if ray 1 and 2 are intersecting.
int intersect(const vert * a1, const vert * b1, const vert * a2, const vert * b2) {
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
curve_point * getNextRight(curve_point * pr, const curve_point * pl, curve * left,
                                 const curve_node ** inOutRight) {
        const curve_node * newRight = findNextCurve(pr, left, TRUE);
        const curve_node * rightCurve = *inOutRight;
        curve_point * pr2;
        if (newRight == NULL) {
                return NULL;
        }
        int rightChanged = rightCurve->curve != newRight->curve;
        if (rightChanged) {
                pr2 = newRight->point;
                rightCurve = newRight;
        } else {
                pr2 = pr->next;
                if (pr2 && pointLineHasCurve(pr2, left) == FALSE) {
                        return NULL;
                }
        }
        if (pr2 == NULL) {
                return NULL;
        }
        *inOutRight = rightCurve;
        return pr2;
}