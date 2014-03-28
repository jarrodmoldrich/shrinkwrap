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
curve_pointp newPoint(float x, float y, curve_nodep scanline);
curve_pointp initCurve(curvep curve, float x, float y, curvesp curves, alpha_type alphaType);
curve_pointp getLastPoint(curvep curve);
int isFirstPoint(const curve_point * point, const curve * curve);
int isLastPoint(const curve_point * point);
curve_pointp appendCurvePoint(curve_pointp lastPoint, curvesp curves, float x, float y);
curve_pointp appendCurvePoint2(curvep curve, curvesp curves, float x, float y);
curve_pointp prependCurvePoint(curve_nodep node, float x, float y, curvesp curves);
curvep destroyCurveDesc(curvep desc);
curvesp createCurveGeometry(size_t scanlines);
curvesp destroyCurveGeometry(curvesp geometry);
curve_nodep createCurveNode(curvep curve, curve_pointp point);
curve_nodep addCurveToScanline2(curvesp geometry, size_t index, curvep curve, curve_pointp point, int biasLeft);
curve_nodep addCurveToScanline(curvesp geometry, size_t index, curvep curve, curve_pointp point);
void removeCurveFromScanline(curve_nodep scanlinelist, curvep curve, curve_pointp point);
void addCurveToGeometry(curvesp geometry, curvep curve, curve_pointp point);
curve_nodep findCurveAt(curvesp curves, pxl_pos x, pxl_pos y);
curve_node * findNextCurveOnScanline(curve_node * node, curve * curve, int skip);
curve_node * findNextCurveOnLine(curves * curves, pxl_pos y, curve * curve);
curve_node * findPrevCurve(curve_point * point, curve * curve);
conserve_direction conserveDirection(const curve_node * scanline, curvep curve);
pxl_pos findNextEnd(const tpxl * typePixels, pxl_pos y, pxl_size width, int * outTerminate);
void findNextPixel(const tpxl * typePixels, pxl_pos startX, pxl_pos currentY, pxl_size width, pxl_pos * outX,
                                 pixel_find * found);
void tryAddCurve(curvesp geometry, alpha_type type, alpha_type lastType, const tpxl * typePixels, float x,
                               float y, pxl_size width, pxl_size height);
void protectRightPoint(curve_pointp point);
void protectSubdividingPoints(curvesp curves, pxl_size width);
float getNewX(curve_pointp point);
int validateScanlines(const curve_node * scanLines, size_t count);
int validateScanline(curve_node * scanLine);
int validateCurves(const curves * curves);
float optimise(curve_pointp orig1, curve_pointp orig2, curve_pointp orig3, conserve_direction conserve);
float calculateAverageDifference(vertp orig1, vertp orig2, float startX, float startY, float newX, float endY);
float calculateMaxDifferenceOnCurve(curve_pointp orig1, curve_pointp orig3, float new23);
float findXAt(curve_nodep curveNode, float y);
float limitPoint(float x, curve_pointp point, float width);
curve_pointp findNextRemoveablePoint(curve_pointp point, curve_pointp * previous);
curve_pointp findNextNonRemovedPoint(curve_pointp point);
curve_nodep findYRelativePoint(curvesp curves, float x, float y, int32_t step, size_t height);
void removePoints(curvep curve);
curve_nodep findMasterNode(curvesp curves, curvep curve);
curve_pointp addPoint(curve_node * left, curve_node * right, float newX, float newY, curves * curves, pxl_diff step,
                      curve_point * lastPoint);
int isAnEndPoint(const curve_node * node, pxl_diff step);
int comesBefore(const curve_node * scanLine, const curve * a, const curve * b);
void fixCurveEnding(curve_point * ending, curve_node * node, curves * curves, pxl_diff step, pxl_size width,
                    pxl_size height, pxl_size bleed);
void fixCurveEndings(curvesp curves, size_t scanlines, pxl_size width, pxl_size height, pxl_size bleed);
void collapseCurveEnding(curve_point * ending, curve * curve, curves * curves, pxl_diff step, pxl_size width,
                         pxl_size height, pxl_size bleed);
void collapseCurveEndings(curvesp curves, size_t scanlines, pxl_size width, pxl_size height, pxl_size bleed);
void smoothFixUp(curvesp curves, size_t scanlines);

// Exposed functions
///////////////////////////////////////////////////////////////////////////////
// Take the image lines and return segments of vertex boundaries between different alpha types.
// Note: imageLines can be destroyed safely after this operation.
curvesp buildCurves(const tpxl * typePixels, pxl_size width, pxl_size height) {
        // Alocate curve linked lists for each scanline.
        curvesp curves = createCurveGeometry(height);
        // For each scanline except the last.
        // No new curves can start on the last line to reduce complications.
        const tpxl * pixel = typePixels;
        for (pxl_pos y = 0; y < height-1; y++) {
                alpha_type lastType = ALPHA_ZERO;
                for (pxl_pos x = 0; x < width; x++, pixel++) {
                        alpha_type type = (alpha_type)*pixel;
                        tryAddCurve(curves, type, lastType, typePixels, x, y, width, height);
                        // Add a terminating alpha-zero edge if required.
                        if (x == width-1 && type != ALPHA_ZERO) {
                                tryAddCurve(curves, ALPHA_ZERO, type, typePixels, width, y, width, height);
                        }
                        lastType = type;
                }
        }
        return curves;
}

// Try to remove middle points of 3 point sets sequentially in a curve
// while avoiding points marked for preservation.
size_t smoothCurve(curvep curve, float width, float maxBleed) {
        size_t removeCount = 0;
        curve_pointp point = curve->pointList;
        point->newX = point->vertex.x;
        curve_pointp point2 = findNextRemoveablePoint(point, &point);
        if (point2 == NULL) return 0;
        curve_pointp point3 = findNextNonRemovedPoint(point2);
        while (point3) {
                if (point2->preserve == TRUE) {
                        point2->newX = point2->vertex.x;
                } else {
                        conserve_direction conserve = conserveDirection(point2->scanlineList, curve);
                        float newX = optimise(point, point2, point3, conserve);
                        newX = limitPoint(newX, point3, width);
                        float avg = calculateMaxDifferenceOnCurve(point, point3, newX);
                        if (avg < maxBleed) {
                                assert(point2->preserve != PRESERVE_DONOTREMOVE);
                                printf("%f %f\n", point2->vertex.x, point3->vertex.x);
                                point2->preserve = PRESERVE_WILLREMOVE;
                                point3->newX = newX;
                                point3->moved = TRUE;
                                removeCount++;
                                point = point3;
                                point2 = findNextRemoveablePoint(point, &point);
                                if (point2 == NULL) break;
                                point3 = findNextNonRemovedPoint(point2);
                                continue;
                        }
                }
                point = point2;
                point2 = findNextRemoveablePoint(point, &point);
                if (point2 == NULL) break;
                point3 = findNextNonRemovedPoint(point2);
        }
        return removeCount;
}

// Iteratively reduce vertices for all curves.
void smoothCurves(curvesp curves, float maxBleed, pxl_size width, pxl_size scanlines) {
        assert(validateScanlines(curves->scanlines, scanlines));
        assert(validateCurves(curves));
        fixCurveEndings(curves, scanlines, width, scanlines, maxBleed);
        assert(validateScanlines(curves->scanlines, scanlines));
        assert(validateCurves(curves));
        collapseCurveEndings(curves, scanlines, width, scanlines, maxBleed);
        assert(validateScanlines(curves->scanlines, scanlines));
        assert(validateCurves(curves));
        protectSubdividingPoints(curves, width);
        assert(validateScanlines(curves->scanlines, scanlines));
        assert(validateCurves(curves));
        curve_nodep curve = curves->head->next;
        while(curve) {
                size_t removeCount = 0;
                do {
                        removeCount = smoothCurve(curve->curve, (float)width, maxBleed);
                } while (removeCount > 0);
                removePoints(curve->curve);
                assert(validateScanlines(curves->scanlines, scanlines));
                curve = curve->next;
        }
        smoothFixUp(curves, scanlines);
}


// Internal functions
///////////////////////////////////////////////////////////////////////////////
curve_pointp newPoint(float x, float y, curve_nodep scanline) {
        curve_pointp point = (curve_pointp)malloc(curve_point_size);
        point->vertex.x = x;
        point->vertex.y = y;
        point->index = 0;
        point->next = NULL;
        point->newX = 0;
        point->moved = FALSE;
        point->preserve = PRESERVE_CANREMOVE;
        point->scanlineList = scanline;
        return point;
}

curve_pointp initCurve(curvep curve, float x, float y, curvesp curves, alpha_type alphaType) {
        curve_nodep scanline = curves->scanlines + (size_t)y;
        curve->alphaType = alphaType;
        curve_pointp point = newPoint(x, y, scanline);
        curve->pointList = point;
        return point;
}

curve_pointp getLastPoint(curvep curve) {
        curve_pointp point = curve->pointList;
        while (point->next) {
                point = point->next;
        }
        return point;
}

int isFirstPoint(const curve_point * point, const curve * curve) {
        return point == curve->pointList;
}

int isLastPoint(const curve_point * point) {
        return point->next == NULL;
}

curve_pointp appendCurvePoint(curve_pointp lastPoint, curvesp curves, float x, float y) {
        assert(lastPoint->next == NULL);
        curve_nodep scanline = curves->scanlines + (size_t)y;
        curve_pointp point = newPoint(x, y, scanline);
        lastPoint->next = point;
        return point;
}

curve_pointp appendCurvePoint2(curvep curve, curvesp curves, float x, float y) {
        curve_pointp lastPoint = getLastPoint(curve);
        return appendCurvePoint(lastPoint, curves, x, y);
}

curve_pointp prependCurvePoint(curve_nodep node, float x, float y, curvesp curves) {
        curvep curve = node->curve;
        curve_nodep scanline = curves->scanlines + (size_t)y;
        curve_pointp point = newPoint(x, y, scanline);
        point->next = curve->pointList;
        curve->pointList = point;
        node->point = point;
        return point;
}

curvep destroyCurveDesc(curvep desc) {
        curve_pointp point = desc->pointList;
        while (point) {
                curve_pointp nextPoint = point->next;
                free(point);
                point = nextPoint;
        }
        free(desc);
        return NULL;
}

curvesp createCurveGeometry(size_t scanlines) {
        curvesp geometry = (curvesp)malloc(curves_size);
        geometry->scanlines = (curve_node_list)malloc(curve_node_size * scanlines);
        geometry->head = (curve_nodep)malloc(curve_node_size);
        geometry->head->next = NULL;
        geometry->lastCurve = geometry->head;
        for (size_t i = 0; i < scanlines; i++) {
                // First node has dummy information and will be ignored
                curve_nodep scanline = geometry->scanlines + i;
                scanline->curve = NULL;
                scanline->next = NULL;
                scanline->point = NULL;
        }
        return geometry;
}

curvesp destroyCurveGeometry(curvesp geometry) {
        free(geometry->scanlines);
        geometry->scanlines = NULL;
        curve_nodep node = geometry->head;
        while (node) {
                curve_nodep next = node->next;
                free(node);
                node = next;
        }
        return NULL;
}

curve_nodep createCurveNode(curvep curve, curve_pointp point) {
        curve_nodep node = (curve_nodep)malloc(curve_node_size);
        node->curve = curve;
        node->point = point;
        node->next = NULL;
        return node;
}

curve_nodep addCurveToScanline(curvesp geometry, size_t index, curvep curve, curve_pointp point) {
        curve_node_list scanlines = geometry->scanlines;
        curve_nodep node = (curve_nodep)malloc(curve_node_size);
        node->curve = curve;
        node->point = point;
        curve_nodep prev = scanlines + index;
        curve_nodep next = prev->next;
        while (next) {
                int before = point->vertex.x < next->point->vertex.x;
                if (before || (point->vertex.x == next->point->vertex.x))
                        break;
                prev = next;
                next = next->next;
        }
        prev->next = node;
        node->next = next;
        return node;
}

void removeCurveFromScanline(curve_nodep scanlinelist, curvep curve, curve_pointp point) {
        curve_nodep node = scanlinelist->next;
        while (node) {
                if (node->curve == curve) {
                        assert(node->point == point && "Point does not exist in scanline");
                        node->point = NULL;
                        return;
                }
                node = node->next;
        }
        // Should go without saying, but, NEVER remove this assert.  Fix the problem instead.
        assert(FALSE && "Trying to remove curve reference from scanline more than once");
}

void addCurveToGeometry(curvesp geometry, curvep curve, curve_pointp point) {
        curve_nodep node = (curve_nodep)malloc(curve_node_size);
        node->curve = curve;
        node->point = point;
        node->next = NULL;
        geometry->lastCurve->next = node;
        geometry->lastCurve = node;
}

curve_nodep findCurveAt(curvesp curves, pxl_pos x, pxl_pos y) {
        curve_nodep node = curves->scanlines[y].next;
        while (node) {
                if (node->point->vertex.x == x) return node;
                node = node->next;
        }
        return NULL;
}

curve_node * findNextCurveOnScanline(curve_node * node, curve * curve, int skip) {
        node = node->next;
        int leftHit = FALSE;
        while (node) {
                if (leftHit) {
                        if (skip == FALSE || (node->point && node->point->next) || node->next == NULL) {
                                return node;
                        }
                } else if (node->curve == curve) {
                        leftHit = TRUE;
                }
                node = node->next;
        }
        return NULL;
}

curve_node * findNextCurveOnLine(curves * curves, pxl_pos y, curve * curve) {
        curve_node * node = curves->scanlines + y;
        return findNextCurveOnScanline(node, curve, FALSE);
}

curve_node * findNextCurve(curve_point * point, curve * curve, int skip) {
        curve_node * node = point->scanlineList;
        return findNextCurveOnScanline(node, curve, skip);
}

curve_node * findPrevCurve(curve_point * point, curve * curve) {
        curve_node * prev = point->scanlineList;
        curve_node * node;
        prev = prev->next;
        if (prev->curve == curve) return NULL;
        node = prev->next;
        while (node) {
                if (node->curve == curve) {
                        return prev;
                }
                prev = node;
                node = prev->next;
        }
        return NULL;
}

// Returns true if point scanline contains curve
int pointLineHasCurve(const curve_point * point, const curve * curve) {
        const curve_node * node = point->scanlineList;
        node = node->next;
        while (node) {
                if (node->curve == curve) {
                        return TRUE;
                }
                node = node->next;
        }
        return FALSE;
}

// Mark to protect curve from clipping through side with partial-alpha
conserve_direction conserveDirection(const curve_node * scanline, curvep curve) {
        alpha_type thisType = curve->alphaType;
        if (thisType == ALPHA_PARTIAL) return CONSERVE_RIGHT;
        if (thisType == ALPHA_ZERO) return CONSERVE_LEFT;
        const curve_node * prev = scanline;
        curve_nodep this = prev->next;
        if (this != NULL && this->curve == curve) return CONSERVE_RIGHT;
        while (this != NULL) {
                if (this->curve == curve) {
                        alpha_type prevType = prev->curve->alphaType;
                        if (prevType == ALPHA_ZERO) return CONSERVE_RIGHT;
                        return CONSERVE_LEFT;
                }
                prev = this;
                this = this->next;
        }
        return CONSERVE_LEFT;
}

// Finds the last empty pixel on the following scanline.
// If last pixel isn't empty the width is returned.
pxl_pos findNextEnd(const tpxl * typePixels, pxl_pos y, pxl_size width, int * outTerminate) {
        const tpxl * pixel = typePixels + ((y+2) * width - 1);
        const tpxl * above = (y > 0) ? (pixel - width) : 0;
        const tpxl reference = above ? *above : ALPHA_ZERO;
        pxl_pos x = width;
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
// If edge is alpha-zero and continues past the width, width will be returned in outX.
void findNextPixel(const tpxl * typePixels, pxl_pos startX, pxl_pos currentY, pxl_size width, pxl_pos * outX,
                   pixel_find * found) {
        assert(outX != NULL);
        assert(found != NULL);
        if (startX == width) {
                int terminate;
                *outX = findNextEnd(typePixels, currentY, width, &terminate);
                *found = terminate ? FIND_TERMINATE : FIND_YES;
                return;
        }
        const tpxl* current = typePixels + currentY * width + startX;
        tpxl value = *current;
        const tpxl* below = current + width;
        pxl_pos x = startX;
        if (*below == value) {
                tpxl reference = (x > 0) ? current[-1] : ALPHA_INVALID;
                while (x > 0) {
                        x--;
                        current--;
                        below--;
                        if (*current != reference && (value == ALPHA_ZERO || *current == value)) {
                                *outX = x;
                                *found = FIND_TERMINATE;
                                return;
                        }
                        if(*below != value) {
                                x++;
                                break;
                        }
                }
                *found = FIND_YES;
                *outX = x;
                return;
        } else {
                pxl_pos x = startX;
                while (x < width-1) {
                        x++;
                        below++;
                        current++;
                        if (*current != value) {
                                *found = FIND_NO;
                                return;
                        }
                        if (*below == value) {
                                *found = FIND_YES;
                                *outX = x;
                                return;
                        }
                }
                if (value == ALPHA_ZERO) {
                        *found = FIND_YES;
                        *outX = width;
                        return;
                }
                *found = FIND_NO;
                return;
        }
}

// Will look at curves currently on the scanline, and if none found at x location
// trace curve downwards and add all left border pixels, adding curve to scanline
// records and also to the main list.
void tryAddCurve(curvesp geometry, alpha_type type, alpha_type lastType, const tpxl * typePixels, float x,
                        float y, pxl_size width, pxl_size height) {
        assert(y < height-1);
        if (type == lastType) {
                return;
        }
        curve_nodep existing = findCurveAt(geometry, x, y);
        if (existing) return;
        curve_nodep node = (curve_nodep)malloc(curve_node_size);
        curvep desc = (curvep)malloc(curve_desc_size);
        curve_pointp lastPoint = initCurve(desc, x, y, geometry, type);
        node->curve = desc;
        addCurveToGeometry(geometry, desc, lastPoint);
        addCurveToScanline(geometry, y, desc, lastPoint);
        pixel_find found;
        pxl_pos newX;
        findNextPixel(typePixels, x, y, width, &newX, &found);
        while (found != FIND_NO && y < height-1) {
                y++;
                x = newX;
                lastPoint = appendCurvePoint(lastPoint, geometry, x, y);
                addCurveToScanline(geometry, y, desc, lastPoint);
                if (found == FIND_TERMINATE) break;
                findNextPixel(typePixels, x, y, width, &newX, &found);
        }
}

// Ensure the subsequent point on the scanline is protected
// from being removed.
void protectRightPoint(curve_pointp point) {
        float x = point->vertex.x;
        const curve_node * scanline = point->scanlineList;
        curve_nodep lineNode = scanline->next;
        int pointHit = FALSE;
        while (lineNode) {
                if (pointHit == TRUE && lineNode->point->vertex.x >= x) {
                        lineNode->point->preserve = PRESERVE_DONOTREMOVE;
                        return;
                }
                assert(lineNode->point->vertex.x <= x);
                if (lineNode->point == point) {
                        pointHit = TRUE;
                }
                lineNode = lineNode->next;
        }
}

// Beginnings of curves should make sure the curve to the right preserves
// it's point on the same scanline to fullfil the assumptions of the
// triangulate function.
void protectSubdividingPoints(curvesp curves, pxl_size width) {
        curve_nodep node = curves->head->next;
        while (node) {
                protectRightPoint(node->point);
                curve_pointp point = node->curve->pointList;
                curve_pointp last = NULL;
                int wasBorder = FALSE;
                while (point) {
                        int border = point->vertex.x == 0 || point->vertex.x == width;
                        if (border != wasBorder) {
                                if (border) {
                                        point->preserve = PRESERVE_DONOTREMOVE;
                                } else {
                                        last->preserve = PRESERVE_DONOTREMOVE;
                                }
                        }
                        last = point;
                        wasBorder = border;
                        point = point->next;
                }
                assert(last);
                protectRightPoint(last);
                node = node->next;
        }
}

// There are probably more elegant ways I could have done this
// Like making a new curve each optimisation run... maybe later.
float getNewX(curve_pointp point) {
        if (point->moved == TRUE) {
                return point->newX;
        }
        return point->vertex.x;
}

// Validates that all scan line points are ordered by
// x coordinates.
int validateScanlines(const curve_node * scanLines, size_t count) {
        const curve_node * prevLine = scanLines;
        for (size_t i = 1; i < count; i++) {
                const curve_node * line  = prevLine+1;
                const curve_node * prevNode = prevLine->next;
                if (prevNode != NULL) {
                        const curve_node * node = prevNode->next;
                        while (node) {
                                if (comesBefore(line, node->curve, prevNode->curve) == TRUE) {
                                        return FALSE;
                                }
                                prevNode = node;
                                node = node->next;
                        }
                }
                prevLine++;
        }
        return TRUE;
}

// Will check that all points on the scan line are in order
// of x coordinates.
int validateScanline(curve_node * scanLine) {
        curve_node * check = scanLine->next;
        if (check == NULL) return TRUE;
        curve_node * checkNext = check->next;
        while (checkNext) {
                if (getNewX(checkNext->point) < getNewX(check->point)) {
                        return FALSE;
                }
//                if (check->curve->alphaType == checkNext->curve->alphaType) {
//                        return FALSE;
//                }
                check = checkNext;
                checkNext = check->next;
        }
        return check->curve->alphaType == ALPHA_ZERO;
}

// Will make sure that all points in all curves are ordered
// by y coordinates.
int validateCurves(const curves * curves) {
        const curve_node * node = curves->head->next;
        while (node) {
                const curve_point * point = node->curve->pointList;
                if (point == NULL) {
                        return FALSE;
                }
                float minY = point->vertex.y;
                point = point->next;
                while (point) {
                        if (minY >= point->vertex.y) {
                                return FALSE;
                        }
                        point = point->next;
                }
                node = node->next;
        }
        return TRUE;
}

// Find a new x position for the 3rd point supposing we
// had to skip the 2nd point but avoid clipping to one
// side.
float optimise(curve_pointp orig1, curve_pointp orig2, curve_pointp orig3, conserve_direction conserve) {
        assert(orig2->preserve != PRESERVE_DONOTREMOVE);
        if (conserve == CONSERVE_NONE) {return getNewX(orig3);}
        float slope2 = (getNewX(orig2) - getNewX(orig1))/(float)(orig2->vertex.y - orig1->vertex.y);
        float slope3 = (getNewX(orig3) - getNewX(orig2))/(float)(orig3->vertex.y - orig2->vertex.y);
        if ((conserve == CONSERVE_RIGHT && slope3 > slope2) || (conserve == CONSERVE_LEFT && slope3 <= slope2)) {
                return slope2 * (orig3->vertex.y - orig1->vertex.y) + getNewX(orig1);
        }
        return getNewX(orig3);
}

// Find the average pixel error for each scanline between
// the simplified line to the original line.
float calculateAverageDifference(vertp orig1, vertp orig2, float startX, float startY, float newX, float endY) {
        float xDistance = (newX - startX);
        float yDistance = (endY - startY);
        float subStartRatio = (orig1->y - startY) / yDistance;
        float subEndRatio = (orig2->y - startY) / yDistance;
        float subStartNew = subStartRatio * xDistance + startX;
        float subEndNew = subEndRatio * xDistance + startX;
        return fabsf(((subStartNew - orig1->x) + (subEndNew - orig2->x)) * 0.5);
}

// Calculate the average absolute pixel error of the new shortcut line (o1->n23) compared
// to the original (o1->o2->o3).
float calculateMaxDifferenceOnCurve(curve_pointp orig1, curve_pointp orig3, float new23) {
        curve_pointp point1 = orig1;
        curve_pointp point2 = orig1->next;
        float startX = getNewX(orig1);
        float startY = orig1->vertex.y;
        float endY = orig3->vertex.y;
        float max = 0.0;
        while (point1 != orig3) {
                assert(point2 != NULL && "End point for average difference does not exist on same curve");
                float avg = calculateAverageDifference(&point1->vertex, &point2->vertex, startX, startY, new23, endY);
                if (avg > max) {
                        max = avg;
                }
                point1 = point2;
                point2 = point2->next;
        }
        printf("+%f\n", max);
        return max;
}

// Find the corresponding x value in curve at y location
float findXAt(curve_nodep curveNode, float y) {
        curvep curve = curveNode->curve;
        curve_pointp point = curve->pointList;
        curve_pointp prevPoint = NULL;
        while (point) {
                float pointX = getNewX(point);
                float pointY = point->vertex.y;
                if (pointY == y) {
                        return pointX;
                } else if (pointY > y) {
                        assert(prevPoint != NULL);
                        vertp prevVert = &prevPoint->vertex;
                        float prevX = getNewX(prevPoint);
                        float prevY = prevVert->y;
                        float ratio = (y - prevY) / (pointY - prevY);
                        return prevX + (pointX - prevX) * ratio;
                }
                prevPoint = point;
                point = point->next;
        }
        assert(FALSE);
        return 0;
}

// Ensure that the x location does not overlap the borders
// or prior left and right curves
float limitPoint(float x, curve_pointp point, float width) {
        float prevX = 0;
        float nextX = width;
        float y = point->vertex.y;
        curve_nodep prevCurveNode = NULL;
        curve_nodep curveNode = point->scanlineList->next;
        while (curveNode) {
                if (curveNode->point == point) {
                        break;
                }
                prevCurveNode = curveNode;
                curveNode = curveNode->next;
                assert(curveNode != NULL);
        }
        if (prevCurveNode) {
                prevX = findXAt(prevCurveNode, y);
        }
        curve_nodep nextCurveNode = curveNode->next;
        if (nextCurveNode) {
                nextX = findXAt(nextCurveNode, y);
        }
        x = (x < prevX) ? prevX : x;
        x = (x > nextX) ? nextX : x;
        return x;
}

// Find following point on the curve that is permitted
// to be removed, optionally returning the previous point.
curve_pointp findNextRemoveablePoint(curve_pointp point, curve_pointp * previous) {
        assert(point->preserve != PRESERVE_WILLREMOVE);
        curve_pointp prev = point;
        point = prev->next;
        while (point) {
                if (point->preserve == PRESERVE_CANREMOVE) {
                        if (previous) {
                                *previous = prev;
                        }
                        return point;
                }
                if (point->preserve != PRESERVE_WILLREMOVE) {
                        prev = point;
                }
                point = point->next;
        }
        return NULL;
}

// Find following point that has not been removed.
curve_pointp findNextNonRemovedPoint(curve_pointp point) {
        point = point->next;
        while (point) {
                if (point->preserve != PRESERVE_WILLREMOVE) {
                        return point;
                }
                point = point->next;
        }
        return NULL;
}

// Find curve point above or below given coordinates.
curve_nodep findYRelativePoint(curvesp curves, float x, float y, int32_t step, size_t height) {
        assert(step == -1 || step == 1);
        size_t yAbove = (size_t)y;
        assert(yAbove != 0);
        do {
                yAbove += step;
                curve_nodep scanline = curves->scanlines + yAbove;
                curve_nodep curve = scanline->next;
                while (curve) {
                        curve_pointp point = curve->point;
                        if (point && point->preserve != PRESERVE_WILLREMOVE) {
                                float thisX = getNewX(point);
                                if (thisX == x) {
                                        return curve;
                                }
                        }
                        curve = curve->next;
                }
        } while (yAbove > 0 && yAbove < height);
        return NULL;
}

// Iterate through curves and alter linked lists to skip remove points.
void removePoints(curvep curve) {
        curve_pointp point = curve->pointList;
        curve_pointp point2 = findNextNonRemovedPoint(point);
        assert(point->preserve != PRESERVE_WILLREMOVE);
        if (point2 == NULL) {
                assert(point->next == NULL);
                return;
        }
        assert(point2->preserve != PRESERVE_WILLREMOVE);
        while (TRUE) {
                point->next = point2;
                point = point2;
                point2 = findNextNonRemovedPoint(point);
                if (point2 == NULL) {
                        assert(point->next == NULL);
                        return;
                }
                assert(point2->preserve != PRESERVE_WILLREMOVE);
        }
}

// Find the curve node-point from the curves list
curve_nodep findMasterNode(curvesp curves, curvep curve) {
        curve_nodep node = curves->head->next;
        while (node) {
                if (node->curve == curve) {
                        return node;
                }
                node = node->next;
        }
        return NULL;
}

// Prepend/append the left curve above or below according to the step
// variable.
curve_pointp addPoint(curve_node * left, curve_node * right, float newX, float newY, curves * curves, pxl_diff step,
                      curve_point * lastPoint) {
        curve_pointp point;
        left = findMasterNode(curves, left->curve);
        right = findMasterNode(curves, right->curve);
        assert(left && right);
        if (step < 0) {
                point = prependCurvePoint(left, newX, newY, curves);
        } else {
                point = appendCurvePoint(lastPoint, curves, newX, newY);
        }
        curve_nodep node = addCurveToScanline(curves, newY, left->curve, point);
        if (node->next == NULL) {
                curve_pointp next;
                if (step < 0) {
                        assert(validateCurves(curves));
                        next = prependCurvePoint(right, newX, newY, curves);
                        assert(validateCurves(curves));
                } else {
                        next = appendCurvePoint2(right->curve, curves, newX, newY);
                }
                curve_nodep nextNode = createCurveNode(right->curve, next);
                assert(nextNode);
                node->next = nextNode;
        }
        return point;
}

// Detects whether curve point is at beginning or ending according
// to step.
int isAnEndPoint(const curve_node * node, pxl_diff step) {
        int firstPoint = step == -1 && isFirstPoint(node->point, node->curve);
        int lastPoint = step == 1 && isLastPoint(node->point);
        return firstPoint == TRUE || lastPoint == TRUE;
}

// Determines if a curve 'a' is linked previous to curve 'b' on the
// scanline.
int comesBefore(const curve_node * scanLine, const curve * a, const curve * b) {
        int hitA = FALSE;
        const curve_node * node = scanLine->next;
        while (node) {
                if (scanLine->curve == b) {
                        return hitA;
                } else if (scanLine->curve == a) {
                        hitA = TRUE;
                }
                node = node->next;
        }
        return FALSE;
}

// Curve endings need to be extended to account for curve disconnections
// due to the pixel length of the last pixel and also for gaps on the
// left and right border.
void fixCurveEnding(curve_point * ending, curve_node * node, curves * curves, pxl_diff step, pxl_size width,
                    pxl_size height, pxl_size bleed) {
        curve * curve = node->curve;
        alpha_type type = curve->alphaType;
        if (type == ALPHA_ZERO) return;
        float x = ending->vertex.x;
        float y = ending->vertex.y;
        curve_node * prev = findPrevCurve(ending, curve);
        curve_node * next = findNextCurve(ending, curve, FALSE);
        if (next == NULL) {
                findNextCurve(ending, curve, FALSE);
        }
        assert(next);
        float nextX = next->point->vertex.x;
        pxl_pos newY = (pxl_pos)y + step;
        if (newY < 0 || newY >= height) return;
        alpha_type prevType = prev ? prev->curve->alphaType : ALPHA_ZERO;
        alpha_type nextType = next->curve->alphaType;
        if (x == 0) {
                if (nextX - x > bleed) return;
                if (isAnEndPoint(next, step)) return;
                addPoint(node, next, 0, newY, curves, step, ending);
        } else if (nextX == width) {
                if (nextX - x > bleed) return;
                if (isAnEndPoint(prev, step)) return;
                assert(validateScanline(curves->scanlines + newY));
                addPoint(node, next, width, newY, curves, step, ending);
                assert(validateScanline(curves->scanlines + newY));
        } else if (isAlpha(prevType) != isAlpha(nextType)) {
                if (isAlpha(prevType)) {
                        if (isAnEndPoint(prev, step)) return;
                        curve_node * nextPoint = findNextCurveOnLine(curves, newY, prev->curve);
                        assert(nextPoint);
                        float newX = nextPoint->point->vertex.x;
                        assert(validateScanline(curves->scanlines + newY));
                        addPoint(node, next, newX, newY, curves, step, ending);
                        assert(validateScanline(curves->scanlines + newY));
                } else {
                        if (isAnEndPoint(next, step)) return;
                        float newX = findXAt(next, newY);
                        assert(validateScanline(curves->scanlines + newY));
                        addPoint(node, next, newX, newY, curves, step, ending);
                        assert(validateScanline(curves->scanlines + newY));
                }
        }// else if (step == 1 && isAlpha(prevType) == FALSE && isAlpha(nextType) == FALSE) {
//                addPoint(node, next, x, newY, curves, step, ending);
//        }
}

// Fixes top and bottom end of curves to account for disconnection between curves
void fixCurveEndings(curvesp curves, size_t scanlines, pxl_size width, pxl_size height, pxl_size bleed) {
        curve_nodep node = curves->head->next;
        while (node) {
                curvep curve = node->curve;
                curve_pointp first = curve->pointList;
                fixCurveEnding(first, node, curves, -1, width, height, bleed);
                curve_pointp last = getLastPoint(curve);
                fixCurveEnding(last, node, curves, 1, width, height, bleed);
                node = node->next;
        }
}

// Make collapse nearby curve endings on the same scanline to the same point.
void collapseCurveEnding(curve_point * ending, curve * curve, curves * curves, pxl_diff step, pxl_size width,
                    pxl_size height, pxl_size bleed) {
        alpha_type type = curve->alphaType;
        if (type == ALPHA_ZERO) return;
        float x = ending->vertex.x;
        const curve_node * prev = findPrevCurve(ending, curve);
        const curve_node * next = findNextCurve(ending, curve, FALSE);
        assert(next);
        float nextX = next->point->vertex.x;
        if (nextX - x > bleed) return;
        float newX;
        if (x == 0) {
                newX = 0;
        } else if (nextX == width) {
                newX = width;
        } else {
                alpha_type prevType = prev ? prev->curve->alphaType : ALPHA_ZERO;
                alpha_type nextType = next->curve->alphaType;
                if (prevType == ALPHA_ZERO) {
                        if (nextType == ALPHA_ZERO) {
                                newX = 0.5 * (nextX + x);
                        } else {
                                newX = nextX;
                        }
                } else if (nextType == ALPHA_ZERO) {
                        newX = x;
                } else {
                        return;
                }
        }
        assert(validateScanline(ending->scanlineList));
        ending->vertex.x = newX;
        next->point->vertex.x = newX;
        assert(validateScanline(ending->scanlineList));
}

// Collapse curve endings for all curves.
void collapseCurveEndings(curvesp curves, size_t scanlines, pxl_size width, pxl_size height, pxl_size bleed) {
        curve_nodep node = curves->head->next;
        while (node) {
                curvep curve = node->curve;
                curve_pointp first = curve->pointList;
                collapseCurveEnding(first, curve, curves, -1, width, height, bleed);
                curve_pointp last = getLastPoint(curve);
                collapseCurveEnding(last, curve, curves, 1, width, height, bleed);
                node = node->next;
        }
}

// Remove superfluous curves
void smoothFixUp(curvesp curves, size_t scanlines) {
//        curve_nodep scanline = curves->scanlines;
//        for (size_t i = 0; i < scanlines; i++) {
//                curve_nodep prev = scanline->next;
//                if (prev != NULL) {
//                        curve_nodep linePoint = prev->next;
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
        curve_nodep prev = curves->head;
        curve_nodep curve = prev->next;
        while (curve) {
                if (curve->point == NULL || curve->point->next == NULL) {
                        prev->next = curve->next;
                        curve = curve->next;
                        if (curve == NULL) break;
                }
                prev = curve;
                curve = prev->next;
        }
        
}
