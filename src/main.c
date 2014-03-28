//
//  main.c
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
#include <string.h>
#include "xmlload.h"
#include "pngload.h"
#include "shrinkwrap.h"
#include "shrinkwrap_html.h"
#include "array.h"
#define PROGNAME "shrinkwrap"
#define VERSION "0.0.0"
#define LONGNAME "Shrink wrap geometry creator for the Starling shrinkWrap extension"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

static const size_t XML_BUFFER_SIZE = 8192;
xml_imagep loadXML(FILE ** file, const char * filename) {
        if (!(*file = fopen(filename, "r"))) {
                fprintf(stderr, PROGNAME ":  can't open xml file [%s]\n", filename);
                return NULL;
        }
        return processXML(*file, XML_BUFFER_SIZE);
}

void loadPNG(readpng_contextp context, FILE ** file, const char * filename, uch ** outPixels, size_t * outWidth,
             size_t * outHeight) {
        *outPixels = NULL;
        *outWidth = 0;
        *outHeight = 0;
        double display_exponent = 1.0;
        int image_channels;
        
        uch bg_red=0, bg_green=0, bg_blue=0;
        ulg image_width = 0, image_height = 0, image_rowbytes = 0;
        int error = 0;
        if (!(*file = fopen(filename, "rb"))) {
                fprintf(stderr, PROGNAME ":  can't open PNG file [%s]\n", filename);
                ++error;
        } else {
                int rc = 0;
                if ((rc = readpng_init(context, *file, &image_width, &image_height)) != 0) {
                        switch (rc) {
                                case 1:
                                        fprintf(stderr, PROGNAME ":  [%s] is not a PNG file: incorrect signature\n", filename);
                                        break;
                                case 2:
                                        fprintf(stderr, PROGNAME ":  [%s] has bad IHDR (libpng longjmp)\n", filename);
                                        break;
                                case 4:
                                        fprintf(stderr, PROGNAME ":  insufficient memory\n");
                                        break;
                                default:
                                        fprintf(stderr, PROGNAME
                                                ":  unknown readpng_init() error\n");
                                        break;
                        }
                        ++error;
                }
                if (error) {
                        fclose(*file);
                        *file = NULL;
                        return;
                }
        }
        
        if (error) {
                fprintf(stderr, PROGNAME ":  aborting.\n");
                exit(2);
        }
        
        /* if the user didn't specify a background color on the command line,
         * check for one in the PNG file--if not, the initialized values of 0
         * (black) will be used */
        
        if (readpng_get_bgcolor(context, &bg_red, &bg_green, &bg_blue) > 1) {
                readpng_cleanup(context, TRUE);
                fclose(*file);
                *file = NULL;
                fprintf(stderr, PROGNAME ":  libpng error while checking for background color\n");
                exit(2);
        }
        
        *outPixels = readpng_get_image(context, display_exponent, &image_channels, &image_rowbytes);
        *outWidth = image_width;
        *outHeight = image_height;
}

void processImageList(FILE * output, xml_imagep firstImage, uch * imageAtlasRGBA, pxl_size atlasWidth,
                      pxl_size atlasHeight) {
        char * outFilename = "data/curves.html";
        FILE * outFile = fopen(outFilename, "w");
        htmlPrologue(outFile, (float)atlasWidth, (float)atlasHeight);
        char * outFilename2 = "data/curves-smooth.html";
        FILE * outFile2 = fopen(outFilename2, "w");
        htmlPrologue(outFile2, (float)atlasWidth, (float)atlasHeight);
        array_descp geometries = array_create(64, sizeof(shrinkwrap_geometryp));
        xml_imagep image = firstImage;
        const pxl_size bleed = 3;
        const float smoothBleed = 4.0;
        int i = 0;
        while (image) {
                i++;
                printf("%d\n", i);
                const pxl_pos x = image->x;
                const pxl_pos y = image->y;
                const pxl_pos width = image->width;
                const pxl_pos height = image->height;
                const pxl_diff frameOffsetX = image->xOffset;
                const pxl_diff frameOffsetY = image->yOffset;
                const pxl_pos frameX = ((pxl_diff)x + frameOffsetX < 0) ? 0 : (x + frameOffsetX);
                const pxl_pos frameY = ((pxl_diff)y + frameOffsetY < 0) ? 0 : (y + frameOffsetY);
                tpxl * typePixels = generateTypePixelMap(imageAtlasRGBA, x, y, width, height, atlasWidth);
                tpxl * antiDither = reduceStateDither(typePixels, width, height, bleed);
                tpxl * dilated = dilateAlpha(antiDither, width, height, bleed);
                tpxl * finalPixels = dilated;
                curvesp curves = buildCurves(finalPixels, width, height);
                htmlDrawCurves(outFile, curves, x, y);
                if (i == 30 || i == 31) {
                        image = getNextImage(image);
                        continue;
                }
                smoothCurves(curves, smoothBleed, width, height);
                htmlDrawCurves(outFile2, curves, x, y);
                shrinkwrap_geometryp geometry = triangulate(curves);
                fixGeometriesInTextureSpace(geometry, frameX, frameY, atlasWidth, atlasHeight, frameOffsetX,
                                            frameOffsetY - 0.5);
                shrinkwrap_geometryp * entry = (shrinkwrap_geometryp *)array_push(geometries);
                geometry->origX = frameX;
                geometry->origY = frameY;
                *entry = geometry;
                image = getNextImage(image);
        }
        htmlEpilogue(outFile);
        htmlEpilogue(outFile2);
        shrinkwrap_geometryp * first = array_get(geometries, 0);
        size_t count = array_size(geometries);
        saveDiagnosticHtml(output, first, count, (float)atlasWidth, (float)atlasHeight);
}

size_t stringLen(const char * str, size_t max) {
        size_t size = 0;
        while (*str != '\0' && size < max) {
                size++;
        }
        return size;
}

int stringEqual(const char * a, const char * b) {
        size_t lenA = stringLen(a, 256);
        size_t lenB = stringLen(b, 256);
        if (lenA == lenB) {
                return strncmp(a, b, lenB) == 0;
        }
        return FALSE;
}
int helpRequested(int argc, const char ** argv) {
        if (argc != 2) return FALSE;
        const char * arg = argv[1];
        int requested;
        requested = stringEqual(arg, "--help");
        requested = requested || stringEqual(arg, "-help");
        return requested;
}

int main(int argc, const char ** argv) {
        if (argc != 4 || helpRequested(argc, argv)) {
                system("nroff -man shrinkwrap.1 | more");
                exit(2);
        }
        
        const char * pngFilename = argv[1];
        const char * xmlFilename = argv[2];
        const char * outFilename = argv[3];
        FILE * pngFile = NULL;
        FILE * xmlFile = NULL;
        FILE * outFile = fopen(outFilename, "w");
        
        if (outFile == NULL) {
                fprintf(stderr, PROGNAME ":  unable to open output file\n");
                exit(2);
        }
        
        readpng_contextp readPNGContextP = readpng_createcontext();
        uch * pixels;
        size_t width;
        size_t height;
        loadPNG(readPNGContextP, &pngFile, pngFilename, &pixels, &width, &height);
        
        readpng_cleanup(readPNGContextP, FALSE);
        if (!pixels) {
                fprintf(stderr, PROGNAME ":  unable to decode PNG image\n");
                if (pngFile) {
                        fclose(pngFile);
                }
                exit(3);
        }
        
        xml_imagep imageList = loadXML(&xmlFile, xmlFilename);
        processImageList(outFile, imageList, pixels, (pxl_size)width, (pxl_size)height);
        destroyImageStructList(imageList);
        imageList = NULL;
        
        free(pixels);
        readpng_destroycontext(readPNGContextP);
        
        fclose(outFile);
        
        return 0;
}
