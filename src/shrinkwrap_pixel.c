//
//  shrinkwrap_pixel.c
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
#include <string.h>
#include <assert.h>
#include "shrinkwrap_internal_t.h"

// Inline functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline alpha alphaType(const uch alpha, const uch threshold)
{
        return (alpha == 0) ? ALPHA_ZERO : ((alpha >= threshold) ? ALPHA_FULL : ALPHA_PARTIAL);
}

static inline uch getAlpha(const uch * pixel)
{
        return pixel[3];
}

static inline const uch * incrementPosition(const uch * pixels, pxl_size count)
{
        return pixels + count * c_pixelSize;
}

static inline const uch * nextPixel(const uch * pixel)
{
        return pixel + c_pixelSize;
}

static inline pxl_pos findPartialAlphaBefore(pxl_pos x, const tpxl * otherLine);
static inline pxl_pos findPartialAlphaAfter(pxl_pos x, const tpxl * otherLine, pxl_size width);
tpxl * yshift_alpha(const tpxl * typePixels, pxl_size width, pxl_size height);

// Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tpxl * generate_typemap(const uch * imageAtlasRGBA, pxl_pos x, pxl_pos y, pxl_size width, pxl_size height,
                            pxl_size rowWidth)
{
        assert(width > 0);
        assert(height > 0);
        
        tpxl * typePixels = (tpxl *)malloc(sizeof(tpxl) * width * height);
        tpxl * typePixel = typePixels;
        const uch * lineStart = incrementPosition(imageAtlasRGBA, y * rowWidth + x);
        // for each row
        for (idx i = 0; i < height; i++) {
                const uch * pixel = lineStart;
                for (pxl_pos p = 0; p < width; p++) {
                        uch alpha = getAlpha(pixel);
                        *typePixel = alphaType(alpha, c_default_threshold);
                        pixel = nextPixel(pixel);
                        typePixel++;
                }
                lineStart = incrementPosition(lineStart, rowWidth);
        }
        return typePixels;
}

// Using scan lines above and below to determine bleed distance.
// This will reduce geometry complexity for anti-aliased borders.
// example (for bleedOffset==1):
//               // Bleed left    // Bleed right
// -----------X* // -------XXXXX* // -------XXXXXX
// -------XXXXX* // ------XXXXXX* // ------XXXXXXX
// -------X***** // ------XX***** // ------XXXXXX*
// ------X****** // --XXXXX****** // --XXXXXX*****
// --XXXXX****** // -XXXXXX****** // -XXXXXXX*****
tpxl * dilate_alpha(const tpxl * typePixels, pxl_size width, pxl_size height, pxl_size bleed)
{
        assert(bleed > 0);
        assert(width > 0);
        assert(height > 0);
        // create new image_line_array
        tpxl * newTypePixels = (tpxl *)malloc(sizeof(tpxl) * width * height);
        const tpxl * currentSrc = typePixels;
        tpxl * current = newTypePixels;
        const tpxl * prev = NULL;
        const tpxl * next = currentSrc + width;
        // for each line
        for (size_t line = 0; line < height; line++) {
                tpxl * pixel = current;
                const tpxl * pixelSrc = currentSrc;
                alpha lastType = *pixelSrc;
                *pixel = *pixelSrc;
                pixel++; pixelSrc++;
                // for each pixel
                for (pxl_pos p = 1; p < width; p++) {
                        alpha thisType = *pixelSrc;
                        if (thisType != lastType) {
                                if (thisType == ALPHA_PARTIAL) {
                                        pxl_pos bleedLeft = (p > bleed) ? (p-bleed) : 0;
                                        if (prev) bleedLeft = findPartialAlphaBefore(bleedLeft, prev);
                                        if (next) bleedLeft = findPartialAlphaBefore(bleedLeft, next);
                                        tpxl * left = current + bleedLeft;
                                        while (left != pixel) {
                                                *left = ALPHA_PARTIAL;
                                                left++;
                                        }
                                } else if (lastType == ALPHA_PARTIAL) {
                                        pxl_pos bleedRight = (p < width-bleed-1) ? (p+bleed) : (width-1);
                                        if (prev) bleedRight = findPartialAlphaAfter(bleedRight, prev, width);
                                        if (next) bleedRight = findPartialAlphaAfter(bleedRight, next, width);
                                        tpxl * begin = pixel;
                                        tpxl * right = current + bleedRight;
                                        // Skip ahead
                                        pixel = right + 1;
                                        pixelSrc = currentSrc + bleedRight + 1;
                                        p = bleedRight; // Will +1 on continue
                                        do {
                                                *right = ALPHA_PARTIAL;
                                                right--;
                                        } while (right >= begin);
                                        lastType = thisType;
                                        continue;
                                }
                                lastType = thisType;
                        }
                        *pixel = *pixelSrc;
                        pixel++; pixelSrc++;
                }
                prev = currentSrc;
                current += width;
                currentSrc += width;
                if (line < height - 1) {
                        next += width;
                } else {
                        next = NULL;
                }
        }
        return newTypePixels;
}

// Step by step, traverse 2D pixel area and turn quick alternations between state on each line
// to a contiguous partial alpha region.
void reduceStateDitherInternal(const tpxl * typePixels, tpxl * newTypePixels, pxl_size width, pxl_size height,
                               pxl_size bleed, pxl_size move, pxl_size lineMove, alpha mask)
{
        const tpxl * currentSrc = typePixels;
        tpxl * current = newTypePixels;
        // for each line
        for (pxl_size y = 0; y < height; y++) {
                tpxl * pixel = current;
                const tpxl * pixelSrc = currentSrc;
                pxl_pos lastBorder = 0;
                pxl_pos thisBorder = 0;
                alpha lastState = (alpha)*pixelSrc;
                for (pxl_pos x = 0; x < width; x++) {
                        // if pixel state has changed
                        alpha thisState = (alpha)*pixelSrc;
                        if (thisState != lastState) {
                                // if distance since previous state change is less than bleed
                                if (thisBorder != 0 && x - thisBorder < bleed && thisBorder - lastBorder < bleed) {
                                        tpxl * writePixel = current + lastBorder;
                                        const tpxl * readPixel = currentSrc + lastBorder;
                                        pxl_pos p = lastBorder;
                                        for (; p < x || (p < width && *readPixel == thisState); p++) {
                                                *writePixel |= mask;
                                                writePixel += move;
                                                readPixel += move;
                                        }
                                        // Skip ahead
                                        lastBorder = x;
                                        thisBorder = p;
                                        x = p;
                                        pixel = writePixel;
                                        pixelSrc = readPixel;
                                        continue;
                                }
                                lastBorder = thisBorder;
                                thisBorder = x;
                                lastState = *pixelSrc;
                        }
                        pixel += move; pixelSrc += move;
                }
                current += lineMove;
                currentSrc += lineMove;
        }
}

// Combine high frequency changes on the x and y axis scanlines to contiguous partial alpha sections
tpxl * reduce_dither(const tpxl * typePixels, pxl_size width, pxl_size height, pxl_size bleed)
{
        assert(bleed > 0);
        assert(width > 0);
        assert(height > 0);
        size_t size = sizeof(tpxl) * width * height;
        tpxl * newTypePixels = (tpxl *)malloc(size);
        memset(newTypePixels, 0, size);
        reduceStateDitherInternal(typePixels, newTypePixels, width, height, bleed, 1, width, ALPHA_FLAG_MARKDITHER_X);
        reduceStateDitherInternal(typePixels, newTypePixels, height, width, bleed, width, 1, ALPHA_FLAG_MARKDITHER_Y);
        const tpxl * currentSrc = typePixels;
        tpxl * current = newTypePixels;
        // for each line
        for (pxl_size y = 0; y < height; y++) {
                const tpxl * pixelSrc = currentSrc;
                tpxl * pixel = current;
                for (pxl_pos x = 0; x < width; x++) {
                        *pixel = (*pixel == ALPHA_FLAG_MARKDITHER_FULL) ? ALPHA_PARTIAL : *pixelSrc;
                        pixel++;
                        pixelSrc++;
                }
                currentSrc += width;
                current += width;
        }
        return newTypePixels;
}


static inline pxl_pos findPartialAlphaBefore(pxl_pos x, const tpxl * otherLine)
{
        if (x == 0) return 0;
        const tpxl * pixel = otherLine + x;
        while (*pixel == ALPHA_PARTIAL) {
                if (x == 0) return 0;
                x--;
                pixel--;
        }
        return x+1;
}

static inline pxl_pos findPartialAlphaAfter(pxl_pos x, const tpxl * otherLine, pxl_size width)
{
        pxl_pos end = width-1;
        if (x == end) return width-1;
        const tpxl * pixel = otherLine + x;
        while (*pixel == ALPHA_PARTIAL) {
                if (x == end) return width-1;
                x++;
                pixel++;
        }
        return x-1;
}

tpxl * yshift_alpha(const tpxl * typePixels, pxl_size width, pxl_size height)
{
        assert(width > 0);
        assert(height > 0);
        // create new image_line_array
        tpxl * newTypePixels = (tpxl *)malloc(sizeof(tpxl) * width * height);
        const tpxl * aboveSrc = NULL;
        const tpxl * currentSrc = typePixels;
        tpxl * current = newTypePixels;
        // for each line
        for (size_t line = 0; line < height; line++) {
                tpxl * pixel = current;
                const tpxl * pixelAbove = aboveSrc;
                const tpxl * pixelSrc = currentSrc;
                // for each pixel
                for (pxl_pos p = 0; p < width; p++) {
                        if (aboveSrc == NULL) {
                                *pixel = *currentSrc;
                        } else {
                                int bleedAbove = (isAlpha(*pixelAbove) && isAlpha(*pixelSrc) == FALSE);
                                *pixel = (bleedAbove) ? *pixelAbove : *pixelSrc;
                                aboveSrc++;
                        }
                        pixel++; currentSrc++;
                }
                aboveSrc = currentSrc;
                current += width;
                currentSrc += width;
        }
        return newTypePixels;
}
