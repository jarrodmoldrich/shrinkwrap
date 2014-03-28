//
//  pngload.h
//  makeshrinkwrap
//
//  Created by Jarrod Moldrich on 16/02/2014.
//  Copyright (c) 2014 Effective Games. All rights reserved.
//

#ifndef makeshrinkwrap_pngload_h
#define makeshrinkwrap_pngload_h

#include <stdio.h>
#include "pixel_t.h"

struct readpng_context_struct;
typedef struct readpng_context_struct * readpng_contextp;

int readpng_init(readpng_contextp context, FILE *infile, ulg *pWidth, ulg *pHeight);
int readpng_get_bgcolor(readpng_contextp context, uch *red, uch *green, uch *blue);
uch *readpng_get_image(readpng_contextp context, double display_exponent, int *pChannels, ulg *pRowbytes);
void readpng_cleanup(readpng_contextp context, int free_image_data);

readpng_contextp readpng_createcontext();
void readpng_destroycontext(readpng_contextp context);
#endif
