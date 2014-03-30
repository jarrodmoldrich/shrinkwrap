//
//  shrinkwrap_curve.c
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
#include <math.h>
#include "shrinkwrap_internal_t.h"

// Internal functions (forward declaration)
///////////////////////////////////////////////////////////////////////////////
curvep * new_point(float x, float y, curven * scanline);
curvep * init_curve(curve * c, float x, float y, curve_list * cl, alpha a);
curvep * get_last_point(curve * c);
int is_first(const curvep * p, const curve * c);
int is_last(const curvep * p);
curvep * append_point_to_curve(curvep * lastPoint, curve_list * cl, float x, float y);
curvep * prepend_point_to_curve(curven * n, float x, float y, curve_list * cl);
curve * destroy_curve(curve * c);
curve_list * create_curve_list(size_t scanlines);
curve_list * destroy_curve_list(curve_list * cl);
curven * create_node(curve * c, curvep * p);
curven * add_curve_to_scanline(curve_list * cl, size_t index, curve * c, curvep * p);
void remove_curve_from_scanline(curven * scanlinelist, curve * c, curvep * p);
void add_curve(curve_list * cl, curve * c, curvep * p);
curven * find_curve_at(curve_list * cl, pxl_pos x, pxl_pos y);
curven * find_next_curve_on_scanline(curven * n, curve * c, int skip);
curven * find_next_curve_on_line(curve_list * cl, pxl_pos y, curve * c);
curven * find_next_curve(curvep * p, curve * c, int skip);
curven * find_prev_curve(curvep * p, curve * c);
conserve conserve_direction(const curven * scanline, curve * c);
pxl_pos find_next_end(const tpxl * tpixels, pxl_pos y, pxl_size w, int * outTerminate);
void find_next_pixel(const tpxl * tpixels, pxl_pos startx, pxl_pos currenty, pxl_size w, pxl_pos * outx,
                                 pixel_find * found);
void try_add_curve(curve_list * cl, alpha type, alpha lastType, const tpxl * tpixels, float x,
                               float y, pxl_size w, pxl_size h);
void protect_right_point(curvep * p);
void protect_subdivision_points(curve_list * cl, pxl_size w);
float getnewx(curvep * p);
int validate_scanlines(const curven * scanLines, size_t count);
int validate_scanline(curven * scanLine);
int validate_curves(const curve_list * cl);
float optimise(curvep * p1, curvep * p2, curvep * p3, conserve conserve);
float calculate_average_difference(vertp p1, vertp p2, float startx, float starty, float newx, float endY);
float calculate_max_difference_on_curve(curvep * p1, curvep * p3, float new23);
float findx(curven * curveNode, float y);
float limit_point(float x, curvep * p, float w);
curvep * find_next_removeable(curvep * p, curvep ** outPrev);
curvep * find_next_nonremoved(curvep * p);
curven * find_yrelative_point(curve_list * cl, float x, float y, int32_t step, size_t h);
void remove_points(curve * c);
curven * find_master_node(curve_list * cl, curve * c);
curvep * add_point(curven * left, curven * right, float newx, float newY, curve_list * cl, pxl_diff step,
                      curvep * lastPoint);
int is_end_point(const curven * n, pxl_diff step);
int comes_before(const curven * scanLine, const curve * a, const curve * b);
void fix_curve_ending(curvep * ending, curven * n, curve_list * cl, pxl_diff step, pxl_size w,
                    pxl_size h, pxl_size bleed);
void fix_curve_endings(curve_list * cl, size_t scanlines, pxl_size w, pxl_size h, pxl_size bleed);
void collapse_curve_ending(curvep * ending, curve * c, curve_list * cl, pxl_diff step, pxl_size w,
                         pxl_size h, pxl_size bleed);
void collapse_curve_endings(curve_list * cl, size_t scanlines, pxl_size w, pxl_size h, pxl_size bleed);
void smooth_fix_up(curve_list * cl, size_t scanlines);

// Exposed functions
///////////////////////////////////////////////////////////////////////////////
// Take the image lines and return segments of vertex boundaries between different alpha types.
// Note: imageLines can be destroyed safely after this operation.
curve_list * build_curves(const tpxl * tpixels, pxl_size w, pxl_size h) {
        // Alocate curve linked lists for each scanline.
        curve_list * cl = create_curve_list(h);
        // For each scanline except the last.
        // No new curves can start on the last line to reduce complications.
        const tpxl * pixel = tpixels;
        for (pxl_pos y = 0; y < h-1; y++) {
                alpha lastType = ALPHA_ZERO;
                for (pxl_pos x = 0; x < w; x++, pixel++) {
                        alpha type = (alpha)*pixel;
                        try_add_curve(cl, type, lastType, tpixels, x, y, w, h);
                        // Add a terminating alpha-zero edge if required.
                        if (x == w-1 && type != ALPHA_ZERO) {
                                try_add_curve(cl, ALPHA_ZERO, type, tpixels, w, y, w, h);
                        }
                        lastType = type;
                }
        }
        return cl;
}

// Try to remove middle points of 3 point sets sequentially in a curve
// while avoiding points marked for preservation.
size_t smoothCurve(curve * c, float w, float maxBleed) {
        size_t removeCount = 0;
        curvep * p = c->pointList;
        p->newx = p->vertex.x;
        curvep * p2 = find_next_removeable(p, &p);
        if (p2 == NULL) return 0;
        curvep * p3 = find_next_nonremoved(p2);
        while (p3) {
                if (p2->preserve == TRUE) {
                        p2->newx = p2->vertex.x;
                } else {
                        conserve dir = conserve_direction(p2->scanlineList, c);
                        float newx = optimise(p, p2, p3, dir);
                        newx = limit_point(newx, p3, w);
                        float avg = calculate_max_difference_on_curve(p, p3, newx);
                        if (avg < maxBleed) {
                                assert(p2->preserve != PRESERVE_DONOTREMOVE);
                                p2->preserve = PRESERVE_WILLREMOVE;
                                p3->newx = newx;
                                p3->moved = TRUE;
                                removeCount++;
                                p = p3;
                                p2 = find_next_removeable(p, &p);
                                if (p2 == NULL) break;
                                p3 = find_next_nonremoved(p2);
                                continue;
                        }
                }
                p = p2;
                p2 = find_next_removeable(p, &p);
                if (p2 == NULL) break;
                p3 = find_next_nonremoved(p2);
        }
        return removeCount;
}

// Iteratively reduce vertices for all curves.
void smooth_curves(curve_list * cl, float bleed, pxl_size w, pxl_size scanlines) {
        assert(validate_scanlines(cl->scanlines, scanlines));
        assert(validate_curves(cl));
        fix_curve_endings(cl, scanlines, w, scanlines, bleed);
        assert(validate_scanlines(cl->scanlines, scanlines));
        assert(validate_curves(cl));
        collapse_curve_endings(cl, scanlines, w, scanlines, bleed);
        assert(validate_scanlines(cl->scanlines, scanlines));
        assert(validate_curves(cl));
        protect_subdivision_points(cl, w);
        assert(validate_scanlines(cl->scanlines, scanlines));
        assert(validate_curves(cl));
        curven * c = cl->head->next;
        while(c) {
                size_t remove = 0;
                do {
                        remove = smoothCurve(c->curve, (float)w, bleed);
                } while (remove > 0);
                remove_points(c->curve);
                assert(validate_scanlines(cl->scanlines, scanlines));
                c = c->next;
        }
        smooth_fix_up(cl, scanlines);
}


// Internal functions
///////////////////////////////////////////////////////////////////////////////
curvep * new_point(float x, float y, curven * scanline) {
        curvep * p = (curvep *)malloc(curvep_size);
        p->vertex.x = x;
        p->vertex.y = y;
        p->index = 0;
        p->next = NULL;
        p->newx = 0;
        p->moved = FALSE;
        p->preserve = PRESERVE_CANREMOVE;
        p->scanlineList = scanline;
        return p;
}

curvep * init_curve(curve * c, float x, float y, curve_list * cl, alpha a) {
        curven * scanline = cl->scanlines + (size_t)y;
        c->alphaType = a;
        curvep * p = new_point(x, y, scanline);
        c->pointList = p;
        return p;
}

curvep * get_last_point(curve * c) {
        curvep * p = c->pointList;
        while (p->next) {
                p = p->next;
        }
        return p;
}

int is_first(const curvep * p, const curve * c) {
        return p == c->pointList;
}

int is_last(const curvep * p) {
        return p->next == NULL;
}

curvep * append_point_to_curve(curvep * lastPoint, curve_list * cl, float x, float y) {
        assert(lastPoint->next == NULL);
        curven * scanline = cl->scanlines + (size_t)y;
        curvep * p = new_point(x, y, scanline);
        lastPoint->next = p;
        return p;
}

curvep * prepend_point_to_curve(curven * n, float x, float y, curve_list * cl) {
        curve * c = n->curve;
        curven * scanline = cl->scanlines + (size_t)y;
        curvep * p = new_point(x, y, scanline);
        p->next = c->pointList;
        c->pointList = p;
        n->point = p;
        return p;
}

curve * destroy_curve(curve * c) {
        curvep * p = c->pointList;
        while (p) {
                curvep * nextPoint = p->next;
                free(p);
                p = nextPoint;
        }
        free(c);
        return NULL;
}

curve_list * create_curve_list(size_t scanlines) {
        curve_list * cl = (curve_list *)malloc(curves_size);
        cl->scanlines = (curven *)malloc(curven_size * scanlines);
        cl->head = (curven *)malloc(curven_size);
        cl->head->next = NULL;
        cl->lastCurve = cl->head;
        for (size_t i = 0; i < scanlines; i++) {
                // First node has dummy information and will be ignored
                curven * scanline = cl->scanlines + i;
                scanline->curve = NULL;
                scanline->next = NULL;
                scanline->point = NULL;
        }
        return cl;
}

curve_list * destroy_curve_list(curve_list * cl) {
        free(cl->scanlines);
        cl->scanlines = NULL;
        curven * n = cl->head;
        while (n) {
                curven * next = n->next;
                free(n);
                n = next;
        }
        return NULL;
}

curven * create_node(curve * c, curvep * p) {
        curven * n = (curven *)malloc(curven_size);
        n->curve = c;
        n->point = p;
        n->next = NULL;
        return n;
}

curven * add_curve_to_scanline(curve_list * cl, size_t index, curve * c, curvep * p) {
        curven * scanlines = cl->scanlines;
        curven * n = (curven *)malloc(curven_size);
        n->curve = c;
        n->point = p;
        curven * prev = scanlines + index;
        curven * next = prev->next;
        while (next) {
                int before = p->vertex.x < next->point->vertex.x;
                if (before || (p->vertex.x == next->point->vertex.x))
                        break;
                prev = next;
                next = next->next;
        }
        prev->next = n;
        n->next = next;
        return n;
}

void remove_curve_from_scanline(curven * scanlinelist, curve * c, curvep * p) {
        curven * node = scanlinelist->next;
        while (node) {
                if (node->curve == c) {
                        assert(node->point == p && "Point does not exist in scanline");
                        node->point = NULL;
                        return;
                }
                node = node->next;
        }
        // Should go without saying, but, NEVER remove this assert.  Fix the problem instead.
        assert(FALSE && "Trying to remove curve reference from scanline more than once");
}

void add_curve(curve_list * cl, curve * c, curvep * p) {
        curven * n = (curven *)malloc(curven_size);
        n->curve = c;
        n->point = p;
        n->next = NULL;
        cl->lastCurve->next = n;
        cl->lastCurve = n;
}

curven * find_curve_at(curve_list * cl, pxl_pos x, pxl_pos y) {
        curven * n = cl->scanlines[y].next;
        while (n) {
                if (n->point->vertex.x == x) return n;
                n = n->next;
        }
        return NULL;
}

curven * find_next_curve_on_scanline(curven * n, curve * c, int skip) {
        n = n->next;
        int leftHit = FALSE;
        while (n) {
                if (leftHit) {
                        if (skip == FALSE || (n->point && n->point->next) || n->next == NULL) {
                                return n;
                        }
                } else if (n->curve == c) {
                        leftHit = TRUE;
                }
                n = n->next;
        }
        return NULL;
}

curven * find_next_curve_on_line(curve_list * cl, pxl_pos y, curve * c) {
        curven * n = cl->scanlines + y;
        return find_next_curve_on_scanline(n, c, FALSE);
}

curven * find_next_curve(curvep * p, curve * c, int skip) {
        curven * n = p->scanlineList;
        return find_next_curve_on_scanline(n, c, skip);
}

curven * find_prev_curve(curvep * p, curve * c) {
        curven * prev = p->scanlineList;
        curven * n;
        prev = prev->next;
        if (prev->curve == c) return NULL;
        n = prev->next;
        while (n) {
                if (n->curve == c) {
                        return prev;
                }
                prev = n;
                n = prev->next;
        }
        return NULL;
}

// Returns true if point scanline contains curve
int point_line_has_curve(const curvep * p, const curve * c) {
        const curven * n = p->scanlineList;
        n = n->next;
        while (n) {
                if (n->curve == c) {
                        return TRUE;
                }
                n = n->next;
        }
        return FALSE;
}

// Mark to protect curve from clipping through side with partial-alpha
conserve conserve_direction(const curven * scanline, curve * c) {
        alpha a = c->alphaType;
        if (a == ALPHA_PARTIAL) return CONSERVE_RIGHT;
        if (a == ALPHA_ZERO) return CONSERVE_LEFT;
        const curven * prev = scanline;
        curven * n = prev->next;
        if (n != NULL && n->curve == c) return CONSERVE_RIGHT;
        while (n != NULL) {
                if (n->curve == c) {
                        alpha prevType = prev->curve->alphaType;
                        if (prevType == ALPHA_ZERO) return CONSERVE_RIGHT;
                        return CONSERVE_LEFT;
                }
                prev = n;
                n = n->next;
        }
        return CONSERVE_LEFT;
}

// Finds the last empty pixel on the following scanline.
// If last pixel isn't empty the width is returned.
pxl_pos find_next_end(const tpxl * tpixels, pxl_pos y, pxl_size w, int * outTerminate) {
        const tpxl * pixel = tpixels + ((y+2) * w - 1);
        const tpxl * above = (y > 0) ? (pixel - w) : 0;
        const tpxl reference = above ? *above : ALPHA_ZERO;
        pxl_pos x = w;
        *outTerminate = FALSE;
        while (x > 0 && *pixel == ALPHA_ZERO) {
                int continued = reference == ALPHA_ZERO || *above == reference;
                if (continued == FALSE) {
                        *outTerminate = TRUE;
                        break;
                }
                pixel--;
                above--;
                x--;
        }
        return x;
}

// Will look for the continuation of the left hand side of the edge.
// If edge is alpha-zero and continues past the width, width will be returned in outx.
void find_next_pixel(const tpxl * tpixels, pxl_pos startx, pxl_pos currenty, pxl_size w, pxl_pos * outx,
                   pixel_find * found) {
        assert(outx != NULL);
        assert(found != NULL);
        if (startx == w) {
                int terminate;
                *outx = find_next_end(tpixels, currenty, w, &terminate);
                *found = terminate ? FIND_TERMINATE : FIND_YES;
                return;
        }
        const tpxl* current = tpixels + currenty * w + startx;
        tpxl value = *current;
        const tpxl* below = current + w;
        pxl_pos x = startx;
        if (*below == value) {
                tpxl reference = (x > 0) ? current[-1] : ALPHA_INVALID;
                while (x > 0) {
                        x--;
                        current--;
                        below--;
                        if (*current != reference && (value == ALPHA_ZERO || *current == value)) {
                                *outx = x;
                                *found = FIND_TERMINATE;
                                return;
                        }
                        if(*below != value) {
                                x++;
                                break;
                        }
                }
                *found = FIND_YES;
                *outx = x;
                return;
        } else {
                pxl_pos x = startx;
                while (x < w-1) {
                        x++;
                        below++;
                        current++;
                        if (*current != value) {
                                *found = FIND_NO;
                                return;
                        }
                        if (*below == value) {
                                *found = FIND_YES;
                                *outx = x;
                                return;
                        }
                }
                if (value == ALPHA_ZERO) {
                        *found = FIND_YES;
                        *outx = w;
                        return;
                }
                *found = FIND_NO;
                return;
        }
}

// Will look at curves currently on the scanline, and if none found at x location
// trace curve downwards and add all left border pixels, adding curve to scanline
// records and also to the main list.
void try_add_curve(curve_list * cl, alpha a, alpha prev, const tpxl * tpixels, float x,
                        float y, pxl_size w, pxl_size h) {
        assert(y < h-1);
        if (a == prev) {
                return;
        }
        curven * existing = find_curve_at(cl, x, y);
        if (existing) return;
        curven * n = (curven *)malloc(curven_size);
        curve * c = (curve *)malloc(curve_size);
        curvep * lastPoint = init_curve(c, x, y, cl, a);
        n->curve = c;
        add_curve(cl, c, lastPoint);
        add_curve_to_scanline(cl, y, c, lastPoint);
        pixel_find found;
        pxl_pos newx;
        find_next_pixel(tpixels, x, y, w, &newx, &found);
        while (found != FIND_NO && y < h-1) {
                y++;
                x = newx;
                lastPoint = append_point_to_curve(lastPoint, cl, x, y);
                add_curve_to_scanline(cl, y, c, lastPoint);
                if (found == FIND_TERMINATE) break;
                find_next_pixel(tpixels, x, y, w, &newx, &found);
        }
}

// Ensure the subsequent point on the scanline is protected
// from being removed.
void protect_right_point(curvep * p) {
        float x = p->vertex.x;
        const curven * scanline = p->scanlineList;
        curven * n = scanline->next;
        int pointHit = FALSE;
        while (n) {
                if (pointHit == TRUE && n->point->vertex.x >= x) {
                        n->point->preserve = PRESERVE_DONOTREMOVE;
                        return;
                }
                assert(n->point->vertex.x <= x);
                if (n->point == p) {
                        pointHit = TRUE;
                }
                n = n->next;
        }
}

// Beginnings of curves should make sure the curve to the right preserves
// it's point on the same scanline to fullfil the assumptions of the
// triangulate function.
void protect_subdivision_points(curve_list * cl, pxl_size w) {
        curven * n = cl->head->next;
        while (n) {
                protect_right_point(n->point);
                curvep * p = n->curve->pointList;
                curvep * last = NULL;
                int wasBorder = FALSE;
                while (p) {
                        int border = p->vertex.x == 0 || p->vertex.x == w;
                        if (border != wasBorder) {
                                if (border) {
                                        p->preserve = PRESERVE_DONOTREMOVE;
                                } else {
                                        last->preserve = PRESERVE_DONOTREMOVE;
                                }
                        }
                        last = p;
                        wasBorder = border;
                        p = p->next;
                }
                assert(last);
                protect_right_point(last);
                n = n->next;
        }
}

// There are probably more elegant ways I could have done this
// Like making a new curve each optimisation run... maybe later.
float getnewx(curvep * p) {
        if (p->moved == TRUE) {
                return p->newx;
        }
        return p->vertex.x;
}

// Validates that all scan line points are ordered by
// x coordinates.
int validate_scanlines(const curven * scanLines, size_t count) {
        const curven * prevLine = scanLines;
        for (size_t i = 1; i < count; i++) {
                const curven * line  = prevLine+1;
                const curven * prev = prevLine->next;
                if (prev != NULL) {
                        const curven * n = prev->next;
                        while (n) {
                                if (comes_before(line, n->curve, prev->curve) == TRUE) {
                                        return FALSE;
                                }
                                prev = n;
                                n = n->next;
                        }
                }
                prevLine++;
        }
        return TRUE;
}

// Will check that all points on the scan line are in order
// of x coordinates.
int validateScanline(curven * scanLine) {
        curven * n = scanLine->next;
        if (n == NULL) return TRUE;
        curven * next = n->next;
        while (next) {
                if (getnewx(next->point) < getnewx(n->point)) {
                        return FALSE;
                }
//                if (check->curve->alphaType == checkNext->curve->alphaType) {
//                        return FALSE;
//                }
                n = next;
                next = n->next;
        }
        return n->curve->alphaType == ALPHA_ZERO;
}

// Will make sure that all points in all curves are ordered
// by y coordinates.
int validate_curves(const curve_list * cl) {
        const curven * n = cl->head->next;
        while (n) {
                const curvep * p = n->curve->pointList;
                if (p == NULL) {
                        return FALSE;
                }
                float minY = p->vertex.y;
                p = p->next;
                while (p) {
                        if (minY >= p->vertex.y) {
                                return FALSE;
                        }
                        p = p->next;
                }
                n = n->next;
        }
        return TRUE;
}

// Find a new x position for the 3rd point supposing we
// had to skip the 2nd point but avoid clipping to one
// side.
float optimise(curvep * p1, curvep * p2, curvep * p3, conserve conserve) {
        assert(p2->preserve != PRESERVE_DONOTREMOVE);
        if (conserve == CONSERVE_NONE) {return getnewx(p3);}
        float slope2 = (getnewx(p2) - getnewx(p1))/(float)(p2->vertex.y - p1->vertex.y);
        float slope3 = (getnewx(p3) - getnewx(p2))/(float)(p3->vertex.y - p2->vertex.y);
        if ((conserve == CONSERVE_RIGHT && slope3 > slope2) || (conserve == CONSERVE_LEFT && slope3 <= slope2)) {
                return slope2 * (p3->vertex.y - p1->vertex.y) + getnewx(p1);
        }
        return getnewx(p3);
}

// Find the average pixel error for each scanline between
// the simplified line to the original line.
float calculate_average_difference(vertp v1, vertp v2, float startx, float starty, float newx, float endY) {
        float xDistance = (newx - startx);
        float yDistance = (endY - starty);
        float subStartRatio = (v1->y - starty) / yDistance;
        float subEndRatio = (v2->y - starty) / yDistance;
        float subStartNew = subStartRatio * xDistance + startx;
        float subEndNew = subEndRatio * xDistance + startx;
        return fabsf(((subStartNew - v1->x) + (subEndNew - v2->x)) * 0.5);
}

// Calculate the average absolute pixel error of the new shortcut line (o1->n23) compared
// to the original (o1...o3).
float calculate_max_difference_on_curve(curvep * p1, curvep * p3, float new23) {
        curvep * point1 = p1;
        curvep * point2 = p1->next;
        float startx = getnewx(p1);
        float starty = p1->vertex.y;
        float endY = p3->vertex.y;
        float max = 0.0;
        while (point1 != p3) {
                assert(point2 != NULL && "End point for average difference does not exist on same curve");
                float avg = calculate_average_difference(&point1->vertex, &point2->vertex, startx, starty, new23, endY);
                if (avg > max) {
                        max = avg;
                }
                point1 = point2;
                point2 = point2->next;
        }
        return max;
}

// Find the corresponding x value in curve at y location
float findx(curven * n, float y) {
        curve * c = n->curve;
        curvep * p = c->pointList;
        curvep * prev = NULL;
        while (p) {
                float pX = getnewx(p);
                float pY = p->vertex.y;
                if (pY == y) {
                        return pX;
                } else if (pY > y) {
                        assert(prev != NULL);
                        vertp prevV = &prev->vertex;
                        float prevX = getnewx(prev);
                        float prevY = prevV->y;
                        float ratio = (y - prevY) / (pY - prevY);
                        return prevX + (pX - prevX) * ratio;
                }
                prev = p;
                p = p->next;
        }
        assert(FALSE);
        return 0;
}

// Ensure that the x location does not overlap the borders
// or prior left and right curves
float limit_point(float x, curvep * p, float w) {
        float prevX = 0;
        float nextX = w;
        float y = p->vertex.y;
        curven * prev = NULL;
        curven * n = p->scanlineList->next;
        while (n) {
                if (n->point == p) {
                        break;
                }
                prev = n;
                n = n->next;
                assert(n != NULL);
        }
        if (prev) {
                prevX = findx(prev, y);
        }
        curven * next = n->next;
        if (next) {
                nextX = findx(next, y);
        }
        x = (x < prevX) ? prevX : x;
        x = (x > nextX) ? nextX : x;
        return x;
}

// Find following point on the curve that is permitted
// to be removed, optionally returning the previous point.
curvep * find_next_removeable(curvep * p, curvep ** outPrev) {
        assert(p->preserve != PRESERVE_WILLREMOVE);
        curvep * prev = p;
        p = prev->next;
        while (p) {
                if (p->preserve == PRESERVE_CANREMOVE) {
                        if (outPrev) {
                                *outPrev = prev;
                        }
                        return p;
                }
                if (p->preserve != PRESERVE_WILLREMOVE) {
                        prev = p;
                }
                p = p->next;
        }
        return NULL;
}

// Find following point that has not been removed.
curvep * find_next_nonremoved(curvep * p) {
        p = p->next;
        while (p) {
                if (p->preserve != PRESERVE_WILLREMOVE) {
                        return p;
                }
                p = p->next;
        }
        return NULL;
}

// Find curve point above or below given coordinates.
curven * find_yrelative_point(curve_list * cl, float x, float y, int32_t step, size_t h) {
        assert(step == -1 || step == 1);
        size_t yAbove = (size_t)y;
        assert(yAbove != 0);
        do {
                yAbove += step;
                curven * scanline = cl->scanlines + yAbove;
                curven * c = scanline->next;
                while (c) {
                        curvep * p = c->point;
                        if (p && p->preserve != PRESERVE_WILLREMOVE) {
                                float thisX = getnewx(p);
                                if (thisX == x) {
                                        return c;
                                }
                        }
                        c = c->next;
                }
        } while (yAbove > 0 && yAbove < h);
        return NULL;
}

// Iterate through curves and alter linked lists to skip remove points.
void remove_points(curve * c) {
        curvep * p = c->pointList;
        curvep * p2 = find_next_nonremoved(p);
        assert(p->preserve != PRESERVE_WILLREMOVE);
        if (p2 == NULL) {
                assert(p->next == NULL);
                return;
        }
        assert(p2->preserve != PRESERVE_WILLREMOVE);
        while (TRUE) {
                p->next = p2;
                p = p2;
                p2 = find_next_nonremoved(p);
                if (p2 == NULL) {
                        assert(p->next == NULL);
                        return;
                }
                assert(p2->preserve != PRESERVE_WILLREMOVE);
        }
}

// Find the curve node-point from the curves list
curven * find_master_node(curve_list * cl, curve * c) {
        curven * n = cl->head->next;
        while (n) {
                if (n->curve == c) {
                        return n;
                }
                n = n->next;
        }
        return NULL;
}

// Prepend/append the left curve above or below according to the step
// variable.
curvep * add_point(curven * left, curven * right, float newx, float newY, curve_list * cl, pxl_diff step,
                      curvep * last) {
        curvep * p;
        left = find_master_node(cl, left->curve);
        right = find_master_node(cl, right->curve);
        assert(left && right);
        if (step < 0) {
                p = prepend_point_to_curve(left, newx, newY, cl);
        } else {
                p = append_point_to_curve(last, cl, newx, newY);
        }
        curven * n = add_curve_to_scanline(cl, newY, left->curve, p);
        if (n->next == NULL) {
                curvep * next;
                if (step < 0) {
                        assert(validate_curves(cl));
                        next = prepend_point_to_curve(right, newx, newY, cl);
                        assert(validate_curves(cl));
                } else {
                        curvep * lastPoint = get_last_point(right->curve);
                        next = append_point_to_curve(lastPoint, cl, newx, newY);
                }
                curven * nextNode = create_node(right->curve, next);
                assert(nextNode);
                n->next = nextNode;
        }
        return p;
}

// Detects whether curve point is at beginning or ending according
// to step.
int is_end_point(const curven * n, pxl_diff step) {
        int firstPoint = step == -1 && is_first(n->point, n->curve);
        int lastPoint = step == 1 && is_last(n->point);
        return firstPoint == TRUE || lastPoint == TRUE;
}

// Determines if a curve 'a' is linked previous to curve 'b' on the
// scanline.
int comes_before(const curven * scanLine, const curve * a, const curve * b) {
        int hitA = FALSE;
        const curven * n = scanLine->next;
        while (n) {
                if (scanLine->curve == b) {
                        return hitA;
                } else if (scanLine->curve == a) {
                        hitA = TRUE;
                }
                n = n->next;
        }
        return FALSE;
}

// Curve endings need to be extended to account for curve disconnections
// due to the pixel length of the last pixel and also for gaps on the
// left and right border.
void fix_curve_ending(curvep * ending, curven * n, curve_list * cl, pxl_diff step, pxl_size w,
                    pxl_size h, pxl_size bleed) {
        curve * c = n->curve;
        alpha a = c->alphaType;
        if (a == ALPHA_ZERO) return;
        float x = ending->vertex.x;
        float y = ending->vertex.y;
        curven * prev = find_prev_curve(ending, c);
        curven * next = find_next_curve(ending, c, FALSE);
        if (next == NULL) {
                find_next_curve(ending, c, FALSE);
        }
        assert(next);
        float nextX = next->point->vertex.x;
        pxl_pos newY = (pxl_pos)y + step;
        if (newY < 0 || newY >= h) return;
        alpha prevType = prev ? prev->curve->alphaType : ALPHA_ZERO;
        alpha nextType = next->curve->alphaType;
        if (x == 0) {
                if (nextX - x > bleed) return;
                if (is_end_point(next, step)) return;
                add_point(n, next, 0, newY, cl, step, ending);
        } else if (nextX == w) {
                if (nextX - x > bleed) return;
                if (is_end_point(prev, step)) return;
                assert(validateScanline(cl->scanlines + newY));
                add_point(n, next, w, newY, cl, step, ending);
                assert(validateScanline(cl->scanlines + newY));
        } else if (isAlpha(prevType) != isAlpha(nextType)) {
                if (isAlpha(prevType)) {
                        if (is_end_point(prev, step)) return;
                        curven * nextPoint = find_next_curve_on_line(cl, newY, prev->curve);
                        assert(nextPoint);
                        float newx = nextPoint->point->vertex.x;
                        assert(validateScanline(cl->scanlines + newY));
                        add_point(n, next, newx, newY, cl, step, ending);
                        assert(validateScanline(cl->scanlines + newY));
                } else {
                        if (is_end_point(next, step)) return;
                        float newx = findx(next, newY);
                        assert(validateScanline(cl->scanlines + newY));
                        add_point(n, next, newx, newY, cl, step, ending);
                        assert(validateScanline(cl->scanlines + newY));
                }
        }// else if (step == 1 && isAlpha(prevType) == FALSE && isAlpha(nextType) == FALSE) {
//                addPoint(node, next, x, newY, curves, step, ending);
//        }
}

// Fixes top and bottom end of curves to account for disconnection between curves
void fix_curve_endings(curve_list * cl, size_t scanlines, pxl_size w, pxl_size h, pxl_size bleed) {
        curven * n = cl->head->next;
        while (n) {
                curve * c = n->curve;
                curvep * first = c->pointList;
                fix_curve_ending(first, n, cl, -1, w, h, bleed);
                curvep * last = get_last_point(c);
                fix_curve_ending(last, n, cl, 1, w, h, bleed);
                n = n->next;
        }
}

// Make collapse nearby curve endings on the same scanline to the same point.
void collapse_curve_ending(curvep * ending, curve * c, curve_list * cl, pxl_diff step, pxl_size w,
                    pxl_size h, pxl_size bleed) {
        alpha type = c->alphaType;
        if (type == ALPHA_ZERO) return;
        float x = ending->vertex.x;
        const curven * prev = find_prev_curve(ending, c);
        const curven * next = find_next_curve(ending, c, FALSE);
        assert(next);
        float nextX = next->point->vertex.x;
        if (nextX - x > bleed) return;
        float newx;
        if (x == 0) {
                newx = 0;
        } else if (nextX == w) {
                newx = w;
        } else {
                alpha prevType = prev ? prev->curve->alphaType : ALPHA_ZERO;
                alpha nextType = next->curve->alphaType;
                if (prevType == ALPHA_ZERO) {
                        if (nextType == ALPHA_ZERO) {
                                newx = 0.5 * (nextX + x);
                        } else {
                                newx = nextX;
                        }
                } else if (nextType == ALPHA_ZERO) {
                        newx = x;
                } else {
                        return;
                }
        }
        assert(validateScanline(ending->scanlineList));
        ending->vertex.x = newx;
        next->point->vertex.x = newx;
        assert(validateScanline(ending->scanlineList));
}

// Collapse curve endings for all curves.
void collapse_curve_endings(curve_list * cl, size_t scanlines, pxl_size w, pxl_size h, pxl_size bleed) {
        curven * n = cl->head->next;
        while (n) {
                curve * c = n->curve;
                curvep * first = c->pointList;
                collapse_curve_ending(first, c, cl, -1, w, h, bleed);
                curvep * last = get_last_point(c);
                collapse_curve_ending(last, c, cl, 1, w, h, bleed);
                n = n->next;
        }
}

// Remove superfluous curves
void smooth_fix_up(curve_list * cl, size_t scanlines) {
//        curve_node * scanline = curves->scanlines;
//        for (size_t i = 0; i < scanlines; i++) {
//                curve_node * prev = scanline->next;
//                if (prev != NULL) {
//                        curve_node * linePoint = prev->next;
//                        while (linePoint) {
//                                if (linePoint->point == NULL || prev->point == NULL) {
//                                        goto next;
//                                }
//                                float x = linePoint->point->vertex.x;
//                                float prevX = prev->point->vertex.x;
//                                if (prevX > x) {
//                                        float middle = (x + prevX) * 0.5;
//                                        linePoint->point->vertex.x = middle;
//                                        prev->point->vertex.x = middle;
//                                }
//                                next:
//                                        prev = linePoint;
//                                        linePoint = linePoint->next;
//                        }
//                }
//                scanline++;
//        }
        curven * prev = cl->head;
        curven * c = prev->next;
        while (c) {
                if (c->point == NULL || c->point->next == NULL) {
                        prev->next = c->next;
                        c = c->next;
                        if (c == NULL) break;
                }
                prev = c;
                c = prev->next;
        }
        
}
