//
//  pngload.c
//  makeshrinkwrap
//
//  Copied and modified from libpng.
//

#include "pngload.h"
#include <stdio.h>
#include "lpng169/png.h"        // libpng header; includes zlib.h
#include "expat-2.1.0/lib/expat.h"

#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif
#ifdef DEBUG
#  define Trace(x)  {fprintf x ; fflush(stderr); fflush(stdout);}
#else
#  define Trace(x)  ;
#endif

// future versions of libpng will provide this macro:
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr)   ((png_ptr)->jmpbuf)
#endif

typedef struct readpng_context_struct {
        png_structp png_ptr;
        png_infop info_ptr;
        uch * image_data;
        int bit_depth, color_type;
        png_uint_32  width, height;
} readpng_context;
static const size_t readpng_context_size = sizeof(readpng_context);

readpng_contextp readpng_createcontext() {
        return (readpng_contextp)malloc(readpng_context_size);
}

void readpng_destroycontext(readpng_contextp context) {
        if (context != NULL) free(context);
}

// return value = 0 for success, 1 for bad sig, 2 for bad IHDR, 4 for no mem
int readpng_init(readpng_contextp context, FILE *infile, ulg *pWidth, ulg *pHeight) {
        uch sig[8];
        
        // first do a quick check that the file really is a PNG image; could
        // have used slightly more general png_sig_cmp() function instead
        
        fread(sig, 1, 8, infile);
        if (!png_check_sig(sig, 8))
                return 1;   // bad signature
        
        // could pass pointers to user-defined error handlers instead of NULLs:
        context->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!context->png_ptr)
                return 4;   // out of memory
        
        context->info_ptr = png_create_info_struct(context->png_ptr);
        if (!context->info_ptr) {
                png_destroy_read_struct(&context->png_ptr, NULL, NULL);
                return 4;   // out of memory
        }
        
        // we could create a second info struct here (end_info), but it's only
        // useful if we want to keep pre- and post-IDAT chunk info separated
        // (mainly for PNG-aware image editors and converters)
        
        
        // setjmp() must be called in every function that calls a PNG-reading
        // libpng function
        if (setjmp(png_jmpbuf(context->png_ptr))) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                return 2;
        }
        
        
        png_init_io(context->png_ptr, infile);
        png_set_sig_bytes(context->png_ptr, 8);  // we already read the 8 signature bytes
        
        png_read_info(context->png_ptr, context->info_ptr);  // read all PNG info up to image data
        
        // alternatively, could make separate calls to png_get_image_width(),
        // etc., but want bit_depth and color_type for later [don't care about
        // compression_type and filter_type => NULLs]
        png_get_IHDR(context->png_ptr, context->info_ptr, &context->width, &context->height, &context->bit_depth,
                     &context->color_type, NULL, NULL, NULL);
        *pWidth = context->width;
        *pHeight = context->height;
        
        
        // OK, that's all we need for now; return happy
        return 0;
}

// returns 0 if succeeds, 1 if fails due to no bKGD chunk, 2 if libpng error;
// scales values to 8-bit if necessary
int readpng_get_bgcolor(readpng_contextp context, uch *red, uch *green, uch *blue) {
        png_color_16p pBackground;
        
        
        // setjmp() must be called in every function that calls a PNG-reading
        // libpng function
        
        if (setjmp(png_jmpbuf(context->png_ptr))) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                return 2;
        }
        
        
        if (!png_get_valid(context->png_ptr, context->info_ptr, PNG_INFO_bKGD))
                return 1;
        
        // it is not obvious from the libpng documentation, but this function
        // takes a pointer to a pointer, and it always returns valid red, green
        // and blue values, regardless of color_type:
        png_get_bKGD(context->png_ptr, context->info_ptr, &pBackground);
        
        
        // however, it always returns the raw bKGD data, regardless of any
        // bit-depth transformations, so check depth and adjust if necessary
        if (context->bit_depth == 16) {
                *red   = pBackground->red   >> 8;
                *green = pBackground->green >> 8;
                *blue  = pBackground->blue  >> 8;
        } else if (context->color_type == PNG_COLOR_TYPE_GRAY && context->bit_depth < 8) {
                if (context->bit_depth == 1)
                        *red = *green = *blue = pBackground->gray? 255 : 0;
                else if (context->bit_depth == 2)
                        *red = *green = *blue = (255/3) * pBackground->gray;
                else // bit_depth == 4
                        *red = *green = *blue = (255/15) * pBackground->gray;
        } else {
                *red   = (uch)pBackground->red;
                *green = (uch)pBackground->green;
                *blue  = (uch)pBackground->blue;
        }
        
        return 0;
}

// display_exponent == LUT_exponent * CRT_exponent
uch *readpng_get_image(readpng_contextp context, double display_exponent, int *pChannels, ulg *pRowbytes)
{
        double  gamma;
        png_uint_32  i, rowbytes;
        png_bytepp  row_pointers = NULL;
        
        // setjmp() must be called in every function that calls a PNG-reading
        // libpng function
        if (setjmp(png_jmpbuf(context->png_ptr))) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                return NULL;
        }
        
        
        // expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
        // transparency chunks to full alpha channel; strip 16-bit-per-sample
        // images to 8 bits per sample; and convert grayscale to RGB[A]
       
        if (context->color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_expand(context->png_ptr);
        if (context->color_type == PNG_COLOR_TYPE_GRAY && context->bit_depth < 8)
                png_set_expand(context->png_ptr);
        if (png_get_valid(context->png_ptr, context->info_ptr, PNG_INFO_tRNS))
                png_set_expand(context->png_ptr);
        if (context->bit_depth == 16)
                png_set_strip_16(context->png_ptr);
        if (context->color_type == PNG_COLOR_TYPE_GRAY ||
            context->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(context->png_ptr);
        
        
        // unlike the example in the libpng documentation, we have *no* idea where
        // this file may have come from--so if it doesn't have a file gamma, don't
        // do any correction ("do no harm")
        if (png_get_gAMA(context->png_ptr, context->info_ptr, &gamma))
                png_set_gamma(context->png_ptr, display_exponent, gamma);
        
        
        // all transformations have been registered; now update info_ptr data,
        // get rowbytes and channels, and allocate image memory
        png_read_update_info(context->png_ptr, context->info_ptr);
        
        *pRowbytes = rowbytes = (png_uint_32)png_get_rowbytes(context->png_ptr, context->info_ptr);
        *pChannels = (int)png_get_channels(context->png_ptr, context->info_ptr);
        
        if ((context->image_data = (uch *)malloc(rowbytes*context->height)) == NULL) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                return NULL;
        }
        if ((row_pointers = (png_bytepp)malloc(context->height*sizeof(png_bytep))) == NULL) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                free(context->image_data);
                context->image_data = NULL;
                return NULL;
        }
        
        Trace((stderr, "readpng_get_image:  channels = %d, rowbytes = %u, height = %u\n", *pChannels, rowbytes, context->height));
        
        
        // set the individual row_pointers to point at the correct offsets
        for (i = 0;  i < context->height;  ++i)
                row_pointers[i] = context->image_data + i*rowbytes;
        
        
        // now we can go ahead and just read the whole image
        png_read_image(context->png_ptr, row_pointers);
        
        
        // and we're done!  (png_read_end() can be omitted if no processing of
        // post-IDAT text/time/etc. is desired)
        free(row_pointers);
        row_pointers = NULL;
        
        png_read_end(context->png_ptr, NULL);
        
        return context->image_data;
}

void readpng_cleanup(readpng_contextp context, int free_image_data) {
        if (free_image_data && context->image_data) {
                free(context->image_data);
                context->image_data = NULL;
        }
        
        if (context->png_ptr && context->info_ptr) {
                png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
                context->png_ptr = NULL;
                context->info_ptr = NULL;
        }
}