/*
 * Copyright (c) 2012 Ross Cunniff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifdef PNG_LITTLE_ENDIAN
#  define PNG_MAKE_ID(a,b,c,d)    (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#else
#  define PNG_MAKE_ID(a,b,c,d)    (((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

#define PNG_MAGIC_0     PNG_MAKE_ID(0x89,0x50,0x4E,0x47)
#define PNG_MAGIC_1     PNG_MAKE_ID(0x0D,0x0A,0x1A,0x0A)

#define PNG_IHDR        PNG_MAKE_ID('I','H','D','R')
#define PNG_PLTE        PNG_MAKE_ID('P','L','T','E')
#define PNG_IDAT        PNG_MAKE_ID('I','D','A','T')
#define PNG_IEND        PNG_MAKE_ID('I','E','N','D')
#define PNG_tRNS        PNG_MAKE_ID('t','R','N','S')
#define PNG_pHYs        PNG_MAKE_ID('p','H','Y','s')

extern void *hwFopen(const char *, const char *);
extern void hwFclose(void *);
extern int hwFread(void *, int, int, void *);
extern int hwFwrite(void *, int, int, void *);
extern int hwFgetc(void *);
extern char *hwFgets(char *, int, void *);
extern int hwFseek(void *, long, int);
extern long hwFtell(void *);

typedef unsigned int    PNG_U32;
typedef unsigned short  PNG_U16;
typedef unsigned char   PNG_U8;

typedef struct {
    PNG_U32 width;
    PNG_U32 height;
    PNG_U8 depth;
    PNG_U8 colorType;
    PNG_U8 compression;
    PNG_U8 filter;
    PNG_U8 interlace;
} PNG_IHDR_Struct;

#define PNG_IHDR_SIZE   13

/* Values for "colorType" */
#define PNG_TYPE_L      0
#define PNG_TYPE_RGB    2
#define PNG_TYPE_I      3
#define PNG_TYPE_LA     4
#define PNG_TYPE_RGBA   6

/* Values for "compression" */
#define PNG_COMPRESS_DEFLATE    0

/* Values for "filter" */
#define PNG_FILTER_ADAPTIVE 0

/* Values for "interlace */
#define PNG_ILACE_NONE  0
#define PNG_ILACE_ADAM7 1

/* Filter methods, first byte in every scanline */
#define PNG_FILTER_NONE  0
#define PNG_FILTER_SUB   1
#define PNG_FILTER_UP    2
#define PNG_FILTER_AVG   3
#define PNG_FILTER_PAETH 4

typedef struct {
    PNG_U32 ppuX;
    PNG_U32 ppuY;
    PNG_U8 unit;
} PNG_pHYs_Struct;

#define PNG_pHYs_SIZE   9

/* Unit types for pHYs */
#define PNG_pHYs_UNKNOWN        0
#define PNG_pHYs_METRE          1

#include "puff.h"
