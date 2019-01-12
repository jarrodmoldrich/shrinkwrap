#include "shrinkwrap_internal_t.h"

static inline alpha alpha_type(const uch alpha, const uch threshold);
static inline uch get_alpha(const uch * pixel);
static inline const uch * increment_position(const uch * pixels, pxl_size count);
static inline const uch * next_pixel(const uch * pixel);
static inline pxl_pos find_partial_before(pxl_pos x, const tpxl * otherLine);
static inline pxl_pos find_partial_after(pxl_pos x, const tpxl * otherLine, pxl_size w);
tpxl * yshift_alpha(const tpxl * typePixels, pxl_size w, pxl_size h);
tpxl * generate_typemap(const uch * rgba, pxl_pos x, pxl_pos y, pxl_size w, pxl_size h,
                            pxl_size row);
tpxl * dilate_alpha(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed);
void reduce_state_dither_internal(const tpxl * tpixels, tpxl * dest_tpixels, pxl_size w, pxl_size h,
                               pxl_size bleed, pxl_size move, pxl_size lineMove, alpha mask);
tpxl * reduce_dither(const tpxl * tpixels, pxl_size w, pxl_size h, pxl_size bleed);
static inline pxl_pos find_partial_before(pxl_pos x, const tpxl * otherline);
static inline pxl_pos find_partial_after(pxl_pos x, const tpxl * otherline, pxl_size w);
tpxl * yshift_alpha(const tpxl * tpixels, pxl_size w, pxl_size h);
