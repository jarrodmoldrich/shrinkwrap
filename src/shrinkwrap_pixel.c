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
#include "internal/shrinkwrap_pixel_internal.h"

// Inline functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline alpha alpha_type(const uch alpha, const uch threshold)
{
        return (alpha == 0) ? ALPHA_ZERO : ((alpha >= threshold) ? ALPHA_FULL : ALPHA_PARTIAL);
}

static inline uch get_alpha(const uch * pixel)
{
        return pixel[3];
}

static inline const uch * increment_position(const uch * pixels, pxl_size count)
{
        return pixels + count * c_pixelSize;
}

static inline const uch * next_pixel(const uch * pixel)
{
        return pixel + c_pixelSize;
}

// Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tpxl * generate_typemap(const uch * rgba, pxl_pos x, pxl_pos y, pxl_size w, pxl_size h,
                            pxl_size row)
{
        assert(w > 0);
        assert(h > 0);
        
        tpxl * tpixels = (tpxl *)malloc(sizeof(tpxl) * w * h);
        tpxl * tpixel = tpixels;
        const uch * lineStart = increment_position(rgba, y * row + x);
        // for each row
        for (idx i = 0; i < h; i++) {
                const uch * pixel = lineStart;
                for (pxl_pos p = 0; p < w; p++) {
                        uch a = get_alpha(pixel);
                        *tpixel = alpha_type(a, c_default_threshold);
                        pixel = next_pixel(pixel);
                        tpixel++;
                }
                lineStart = increment_position(lineStart, row);
        }
        return tpixels;
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
tpxl * dilate_alpha(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed)
{
        assert(bleed > 0);
        assert(w > 0);
        assert(h > 0);
        // create new image_line_array
        tpxl * newtpixels = (tpxl *)malloc(sizeof(tpxl) * w * h);
        const tpxl * src = tpixels;
        tpxl * dest = newtpixels;
        const tpxl * prev = NULL;
        const tpxl * next = src + w;
        // for each line
        for (size_t line = 0; line < h; line++) {
                tpxl * pix = dest;
                const tpxl * srcpix = src;
                alpha last = *srcpix;
                *pix = *srcpix;
                pix++; srcpix++;
                // for each pixel
                for (pxl_pos p = 1; p < w; p++) {
                        alpha type = *srcpix;
                        if (type != last) {
                                if (type == ALPHA_PARTIAL) {
                                        pxl_pos lbleed = (p > bleed) ? (p-bleed) : 0;
                                        if (prev) lbleed = find_partial_before(lbleed, prev);
                                        if (next) lbleed = find_partial_before(lbleed, next);
                                        tpxl * left = dest + lbleed;
                                        while (left != pix) {
                                                *left = ALPHA_PARTIAL;
                                                left++;
                                        }
                                } else if (last == ALPHA_PARTIAL) {
                                        pxl_pos rbleed = (p < w-bleed-1) ? (p+bleed) : (w-1);
                                        if (prev) rbleed = find_partial_after(rbleed, prev, w);
                                        if (next) rbleed = find_partial_after(rbleed, next, w);
                                        tpxl * begin = pix;
                                        tpxl * right = dest + rbleed;
                                        // Skip ahead
                                        pix = right + 1;
                                        srcpix = src + rbleed + 1;
                                        p = rbleed; // Will +1 on continue
                                        do {
                                                *right = ALPHA_PARTIAL;
                                                right--;
                                        } while (right >= begin);
                                        last = type;
                                        continue;
                                }
                                last = type;
                        }
                        *pix = *srcpix;
                        pix++; srcpix++;
                }
                prev = src;
                dest += w;
                src += w;
                if (line < h - 1) {
                        next += w;
                } else {
                        next = NULL;
                }
        }
        return newtpixels;
}

// Step by step, traverse 2D pixel area and turn quick alternations between state on each line
// to a contiguous partial alpha region.
void reduce_state_dither_internal(const tpxl * tpixels, tpxl * dest_tpixels, pxl_size w, pxl_size h,
                               pxl_size bleed, pxl_size move, pxl_size lineMove, alpha mask)
{
        const tpxl * src = tpixels;
        tpxl * dest = dest_tpixels;
        // for each line
        for (pxl_size y = 0; y < h; y++) {
                tpxl * pix = dest;
                const tpxl * srcpix = src;
                pxl_pos last = 0;
                pxl_pos border = 0;
                alpha laststate = (alpha)*srcpix;
                for (pxl_pos x = 0; x < w; x++) {
                        // if pixel state has changed
                        alpha state = (alpha)*srcpix;
                        if (state != laststate) {
                                // if distance since previous state change is less than bleed
                                if (border != 0 && x - border < bleed && border - last < bleed) {
                                        tpxl * write = dest + last;
                                        const tpxl * read = src + last;
                                        pxl_pos p = last;
                                        for (; p < x || (p < w && *read == state); p++) {
                                                *write |= mask;
                                                write += move;
                                                read += move;
                                        }
                                        // Skip ahead
                                        last = x;
                                        border = p;
                                        x = p;
                                        pix = write;
                                        srcpix = read;
                                        continue;
                                }
                                last = border;
                                border = x;
                                laststate = *srcpix;
                        }
                        pix += move; srcpix += move;
                }
                dest += lineMove;
                src += lineMove;
        }
}

// Combine high frequency changes on the x and y axis scanlines to contiguous partial alpha sections
tpxl * reduce_dither(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed)
{
        assert(bleed > 0);
        assert(w > 0);
        assert(h > 0);
        size_t size = sizeof(tpxl) * w * h;
        tpxl * dest_tpixels = (tpxl *)malloc(size);
        memset(dest_tpixels, 0, size);
        reduce_state_dither_internal(tpixels, dest_tpixels, w, h, bleed, 1, w, ALPHA_FLAG_MARKDITHER_X);
        reduce_state_dither_internal(tpixels, dest_tpixels, h, w, bleed, w, 1, ALPHA_FLAG_MARKDITHER_Y);
        const tpxl * src = tpixels;
        tpxl * dest = dest_tpixels;
        // for each line
        for (pxl_size y = 0; y < h; y++) {
                const tpxl * srcpix = src;
                tpxl * pix = dest;
                for (pxl_pos x = 0; x < w; x++) {
                        *pix = (*pix == ALPHA_FLAG_MARKDITHER_FULL) ? ALPHA_PARTIAL : *srcpix;
                        pix++;
                        srcpix++;
                }
                src += w;
                dest += w;
        }
        return dest_tpixels;
}


static inline pxl_pos find_partial_before(pxl_pos x, const tpxl * otherline)
{
        x--;
        const tpxl * pixel = otherline + x;
        while (x >= 0 && *pixel == ALPHA_PARTIAL) {
                x--;
                pixel--;
        }
        return x+1;
}

static inline pxl_pos find_partial_after(pxl_pos x, const tpxl * otherline, pxl_size w)
{
        x++;
        const tpxl * pix = otherline + x;
        while (x < w && *pix == ALPHA_PARTIAL) {
                x++;
                pix++;
        }
        return x-1;
}

tpxl * yshift_alpha(const tpxl * tpixels, pxl_size w, pxl_size h)
{
        assert(w > 0);
        assert(h > 0);
        // create new image_line_array
        tpxl * dest_tpix = (tpxl *)malloc(sizeof(tpxl) * w * h);
        const tpxl * above = NULL;
        const tpxl * src = tpixels;
        tpxl * dest = dest_tpix;
        // for each line
        for (size_t line = 0; line < h; line++) {
                tpxl * pixel = dest;
                const tpxl * abovepix = above;
                const tpxl * srcpix = src;
                // for each pixel
                for (pxl_pos p = 0; p < w; p++) {
                        if (above == NULL) {
                                *pixel = *src;
                        } else {
                                int bleed = (isAlpha(*abovepix) && isAlpha(*srcpix) == FALSE);
                                *pixel = (bleed) ? *abovepix : *srcpix;
                                above++;
                        }
                        pixel++; src++;
                }
                above = src;
                dest += w;
                src += w;
        }
        return dest_tpix;
}
