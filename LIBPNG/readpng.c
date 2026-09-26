/******************************************************************************
 * This file implements basic PNG reading.  This is based on the
 * W3 spec at:
 *
 *      http://www.w3.org/TR/2003/REC-PNG-20031110/
 *
 * This implementation is geared toward using PNG images as OpenGL
 * or DirectX textures; therefore, all images are converted to a 1-,
 * 2-, 3-, or 4-byte-per-pixel representation (in OpenGL terms,
 * a format of GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, or GL_RGBA,
 * with a type of GL_UNSIGNED_BYTE)
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readpng.h"

/******************************************************************************
 * Error utility macro - store the line number where we barfed, and go
 * to the error exit label
 *****************************************************************************/
static int PngErrLine;
#define ERROR() do { PngErrLine = __LINE__; goto error; } while(0)

#if defined(USE_LIBPNG) // [
#include <png.h>

static void pngReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PngInStruct *io;

    io = png_get_io_ptr(png_ptr);
    if (!io) {
        png_error(png_ptr, "pngReadData - NULL io_ptr");
    }
    if (io->read(data, length, 1, io->f) != 1) {
        png_error(png_ptr, "pngReadData - EOD");
    }
}

int ReadPNG(PngInStruct *io, int flags,
            int *w, int *h, int *comps, unsigned char **img,
            unsigned char **pPall)
{
    png_structp png_ptr;
    png_infop info_ptr;
    int i, j, width, height, byteWidth;
    int nc;
    int bits, tRNS;
    png_bytep *rows;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) ERROR();

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) ERROR();

    if (setjmp(png_jmpbuf(png_ptr))) ERROR();

    png_set_read_fn(png_ptr, io, pngReadData);
    png_read_info(png_ptr, info_ptr);

    // Read initial info
    *w = width = png_get_image_width(png_ptr, info_ptr);
    *h = height = png_get_image_height(png_ptr, info_ptr);
    bits = png_get_bit_depth(png_ptr, info_ptr);
    tRNS = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);

    png_set_gamma_fixed(png_ptr, PNG_GAMMA_LINEAR, PNG_GAMMA_LINEAR);
    if (bits < 16) flags &= ~PNG_READ_16;
    if ((bits == 16) && !(flags & PNG_READ_16)) png_set_expand_16(png_ptr);
    if (bits < 8) png_set_packing(png_ptr);
    if (!pPall) png_set_palette_to_rgb(png_ptr);
    if (bits < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (tRNS) png_set_tRNS_to_alpha(png_ptr);
#ifdef PNG_LITTLE_ENDIAN
    if (bits == 16) png_set_swap(png_ptr);
#endif

    switch (png_get_color_type(png_ptr, info_ptr)) {
    case PNG_COLOR_TYPE_RGB :
        nc = *comps = tRNS ? 4 : 3;
        if (pPall) *pPall = NULL;
        pPall = NULL;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA :
        nc = *comps = 4;
        if (pPall) *pPall = NULL;
        pPall = NULL;
        break;
    case PNG_COLOR_TYPE_GRAY :
        nc = *comps = 1;
        if (pPall) *pPall = NULL;
        pPall = NULL;
        break;
    case PNG_COLOR_TYPE_PALETTE :
        nc = *comps = pPall ? 1 : 3;
        break;
    }

    byteWidth = (flags & PNG_READ_16) ? (2*width*nc) : width*nc;
    *img = malloc(byteWidth * height);
    if (!*img) ERROR();

    rows = malloc(sizeof(png_bytep) * height);
    if (!rows) ERROR();

    for (i = 0; i < height; i++) {
        rows[i] = (*img) + i*byteWidth;
    }

    png_read_image(png_ptr, rows);
    png_read_end(png_ptr, info_ptr);

    if (pPall) {
        png_colorp palette;
        int numPalette = 0;

        png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);
        if (!numPalette) ERROR();

        *pPall = malloc(4*256);
        if (!*pPall) ERROR();

        for (i = 0; i < numPalette; i++) {
            (*pPall)[4*i  ] = palette[i].red;
            (*pPall)[4*i+1] = palette[i].green;
            (*pPall)[4*i+2] = palette[i].blue;
            (*pPall)[4*i+3] = 0xFF;
        }

        if (tRNS) {
            int numTrans;
            png_bytep transAlpha;
            png_color_16p transColor;

            png_get_tRNS(png_ptr, info_ptr, &transAlpha, &numTrans,
                                            &transColor);
            if (!numTrans) ERROR();

            for (i = 0; i < numTrans; i++) {
                (*pPall)[4*i+3] = transAlpha[i];
            }
        }

        for (i = numPalette; i < 256; i++) {
            (*pPall)[4*i  ] =
            (*pPall)[4*i+1] =
            (*pPall)[4*i+2] =
            (*pPall)[4*i+3] = 0xFF;
        }
    }

    free(rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return flags;

error :
    if (*img) free(*img);
    *img = NULL;
    if (rows) free(rows);
    if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return -PngErrLine;
}

#else // ] [

#include "png_int.h"

#ifdef PNG_LITTLE_ENDIAN
static void SWAB4(PNG_U32 *ptr)
{
    PNG_U8 tmp, *ptr8 = (PNG_U8 *)ptr;
    tmp = ptr8[0]; ptr8[0] = ptr8[3]; ptr8[3] = tmp;
    tmp = ptr8[1]; ptr8[1] = ptr8[2]; ptr8[2] = tmp;
}
#else
#define SWAB4(a)
#endif

/* Interlacing fun.  Here is the interlacing pattern we are
 * implementing:
 *
 * +---+---+---+---+---+---+---+---+
 * | 0 | 5 | 3 | 5 | 1 | 5 | 3 | 5 |
 * +---+---+---+---+---+---+---+---+
 * | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
 * +---+---+---+---+---+---+---+---+
 * | 4 | 5 | 4 | 5 | 4 | 5 | 4 | 5 |
 * +---+---+---+---+---+---+---+---+
 * | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
 * +---+---+---+---+---+---+---+---+
 * | 2 | 5 | 3 | 5 | 2 | 5 | 3 | 5 |
 * +---+---+---+---+---+---+---+---+
 * | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
 * +---+---+---+---+---+---+---+---+
 * | 4 | 5 | 4 | 5 | 4 | 5 | 4 | 5 |
 * +---+---+---+---+---+---+---+---+
 * | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
 * +---+---+---+---+---+---+---+---+
 */

/* It's called "Adam 7" interlacing for a reason :-) */
#define ILACE_PASSES    7

struct {
    int w, h;           /* Width / height of each subimage in the tile */
    int r0, c0;         /* Offset to first pixel in each subimage */
    int pixStride;      /* Stride to next pixel in each subimage scanline */
    int rowStride;      /* Stride to next scanline in subimage */
} ilaceInfo[ILACE_PASSES] = {
   /* w  h  r0 c0  pix row */
    { 1, 1,  0, 0,  8,  8 },
    { 1, 1,  0, 4,  8,  8 },
    { 2, 1,  4, 0,  4,  8 },
    { 2, 2,  0, 2,  4,  4 },
    { 4, 2,  2, 0,  2,  4 },
    { 4, 4,  0, 1,  2,  2 },
    { 8, 4,  1, 0,  1,  2 },
};

/* Those offsets, sizes, and strides describe the following table:
 *  0,0, // First pixel
 *
 *  0,4, // Second pixel
 *
 *  4,0, 4,4, // Third set of pixels
 *
 *  0,2, 0,6, // Fourth set of pixels
 *  4,2, 4,6,
 *
 *  2,0, 2,2, 2,4, 2,6, // Fifth set of pixels
 *  6,0, 6,2, 6,4, 6,6,
 *
 *  0,1, 0,3, 0,5, 0,7, // Sixth set of pixels
 *  2,1, 2,3, 2,5, 2,7,
 *  4,1, 4,3, 4,5, 4,7,
 *  6,1, 6,3, 6,5, 6,7,
 *
 *  1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7, // Seventh and final set of pixels
 *  3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
 *  5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
 *  7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7,
 */

/******************************************************************************
 * Filter functions
 *****************************************************************************/
#include "filter.c"

/******************************************************************************
 * Unpacking functions
 *****************************************************************************/
#include "unpack.c"

/******************************************************************************
 * Data decompresser
 *****************************************************************************/
#define USE_PUFF 1
#define USE_SSHZLIB 1

#if USE_PUFF
#  include "puff.c"
#endif

#if USE_SSHZLIB
#  define ZLIB_STANDALONE
#  define ZLIB_DECOMPRESS
#  include "sshzlib.c"
#endif

/******************************************************************************
 * Routine to calculate the size of a subimage
 *****************************************************************************/
static int calcSubSize(int size, int mul, int offs, int stride)
{
    int even, odd, result;

    even = size / 8;
    odd = size % 8;
    result = even*mul;
    if (odd >= offs) {
        result += (odd - offs + stride - 1) / stride;
    }
    return result;
}

/******************************************************************************
 * The actual PNG reader
 *****************************************************************************/
int ReadPNG(PngInStruct *io, int flags,
            int *w, int *h, int *comps, unsigned char **img,
            unsigned char **pPall)
{
    PNG_U32 chunkHdr, chunkSize, totImgSize, crc;
    PNG_IHDR_Struct ihdr;
    PNG_U8 palette[4*256];
    PNG_U8 trans[256];
    int plteFound = 0;
    int numIdx = 0;
    int trFound = 0;
    int bitsPerPixel, bytesPerRow;
    int res, ncomp;
    int iw, ih;
    int i, n;
    int filtStride;
    long idatPos;
    unsigned long idatSize;
#if USE_PUFF
    unsigned long uncompBytes;
#endif

#if USE_SSHZLIB
    int iuncompBytes;
#endif
    unsigned char *idatData = NULL;
    unsigned char *uncompData = NULL;
    unsigned char *iptr = NULL;
    unsigned char *upArg = NULL;
    void (*unpack)(int w, int s, PNG_U8 *dst, PNG_U8 *plte, PNG_U8 *src);
#if USE_SSHZLIB
    void *pz = NULL;
#endif

    *img = NULL;

    /* Read first 4 magic bytes */
    if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();
    if (chunkHdr != PNG_MAGIC_0) ERROR();

    /* Read second 4 magic bytes */
    if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();
    if (chunkHdr != PNG_MAGIC_1) ERROR();

    /* Read first chunk.  Chunks are always [size,hdr,data,crc]
     * First chunk *MUST* be IHDR per the PNG specification
     */
    if (io->read(&chunkSize, sizeof(chunkSize), 1, io->f) != 1) ERROR();
    SWAB4(&chunkSize);
    if (chunkSize != PNG_IHDR_SIZE) ERROR();

    if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();
    if (chunkHdr != PNG_IHDR) ERROR();

    if (io->read(&ihdr, chunkSize, 1, io->f) != 1) ERROR();
    if (io->read(&crc, sizeof(crc), 1, io->f) != 1) ERROR();

    /* Do some byte-swap magic and store off the width/height of the image */
    SWAB4(&ihdr.width); SWAB4(&ihdr.height);
    *w = iw = ihdr.width; *h = ih = ihdr.height;

    /* Process remaining chunks in the data */
    while (chunkHdr != PNG_IDAT) {
        /* First, remember where we are in the file.  This is
         * so we can reprocess the IDAT chunks in the third loop,
         * below
         */
        idatPos = io->tell(io->f);

        /* Now, read the next chunk size/header */
        if (io->read(&chunkSize, sizeof(chunkSize), 1, io->f) != 1) ERROR();
        SWAB4(&chunkSize);
        if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();

        /* Process the chunk */
        switch (chunkHdr) {
        case PNG_IDAT :
            /* Found image data!  We will process this below. */
            break;

        case PNG_PLTE :
            /* Palette. Only one palette is allowed.  Read it if present.
             * Note that we ignore the error case where a palette is
             * present in non-indexed images
             */
            if (plteFound) ERROR();
            plteFound = chunkSize;
            if (chunkSize > 3*256) ERROR();
            if (chunkSize % 3) ERROR();
            if (io->read(palette, chunkSize, 1, io->f) != 1) ERROR();
            break;

        case PNG_tRNS :
            /* Transparency.  Only one tRNS chunk is allowed. */
            if (trFound) ERROR();
            /* Validate transparency size */
            switch (ihdr.colorType) {
            case PNG_TYPE_L :
                if (chunkSize != 2) ERROR();
                break;
            case PNG_TYPE_RGB :
                if (chunkSize != 6) ERROR();
                break;
            case PNG_TYPE_I :
                if (chunkSize > 256) ERROR();
                break;
            default :
                ERROR();
            }
            if (io->read(trans, chunkSize, 1, io->f) != 1) ERROR();
            trFound = chunkSize;
            break;

        case PNG_IEND :
            /* This is the end-of-image chunk.  If we see this,
             * we did not get any image data.
             */
            ERROR();

        default :
            /* Unknown chunk.  We ignore all of these.  TBD:
             * would be useful to error out if we see any unknown
             * *required* chunks (with upper-case names).  Skip
             * over the data with io->seek.
             */
            if (io->seek(io->f, chunkSize, SEEK_CUR) != 0) ERROR();
            break;
        }

        if (chunkHdr != PNG_IDAT) {
            if (io->read(&crc, sizeof(crc), 1, io->f) != 1) ERROR();
        }
    }

    /* Most image types don't have a palette */
    if( pPall ) *pPall = NULL;

    /* If not 16-bit, don't do the conversion */
    if (ihdr.depth != 16) {
        flags &= ~PNG_READ_16;
    }
    /* OK.  Do some interpretation of the IHDR.  Main duty of this
     * switch is to assign the unpack function, and calculate
     * the bits-per-pixel and the number of components in the
     * image.  We also calcluate the upArg - if an indexed image
     * upArg is the palette.  Otherwise, if a trans chunk is
     * found, upArg is the trans chunk data.  Otherwise, we leave
     * it NULL.
     */
    switch (ihdr.colorType) {
    case PNG_TYPE_L :
        switch (ihdr.depth) {
        case 1 :  unpack = trFound ? u_L1_LA : u_L1_L;   break;
        case 2 :  unpack = trFound ? u_L2_LA : u_L2_L;   break;
        case 4 :  unpack = trFound ? u_L4_LA : u_L4_L;   break;
        case 8 :  unpack = trFound ? u_L_LA : u_L_L;   break;
        case 16 :
            if (flags & PNG_READ_16) {
                unpack = trFound ? u_L16_LA_16 : u_L16_L_16;
            }
            else {
                unpack = trFound ? u_L16_LA : u_L16_L;
            }
            break;
        default : ERROR();
        }
        bitsPerPixel = ihdr.depth;
        *comps = ncomp = trFound ? 2 : 1;
        upArg = trFound ? trans : NULL;
        break;
    case PNG_TYPE_LA :
        switch (ihdr.depth) {
        case 8 :  unpack = u_LA_LA;  break;
        case 16 :
            if (flags & PNG_READ_16) {
                unpack = u_LA16_LA_16;
            }
            else {
                unpack = u_LA16_LA;
            }
            break;
        default : ERROR();
        }
        bitsPerPixel = 2*ihdr.depth;
        *comps = ncomp = 2;
        break;
    case PNG_TYPE_RGB :
        switch (ihdr.depth) {
        case 8 :  unpack = trFound ? u_RGB_RGBA : u_RGB_RGB;     break;
        case 16 :
            if (flags & PNG_READ_16) {
                unpack = trFound ? u_RGB16_RGBA_16 : u_RGB16_RGB_16;
            }
            else {
                unpack = trFound ? u_RGB16_RGBA : u_RGB16_RGB;
            }
            break;
        default : ERROR();
        }
        bitsPerPixel = 3*ihdr.depth;
        *comps = ncomp = trFound ? 4 : 3;
        upArg = trFound ? trans : NULL;
        break;
    case PNG_TYPE_I :
        if( pPall ) {
            switch (ihdr.depth) {
            case 1 :  unpack = u_I1_I8; break;
            case 2 :  unpack = u_I2_I8; break;
            case 4 :  unpack = u_I4_I8; break;
            case 8 :  unpack = u_I8_I8; break;
            default : ERROR();
            }
            *comps = ncomp = 1;
        }
        else {
            switch (ihdr.depth) {
            case 1 :  unpack = trFound ? u_I1_RGBA : u_I1_RGB; break;
            case 2 :  unpack = trFound ? u_I2_RGBA : u_I2_RGB; break;
            case 4 :  unpack = trFound ? u_I4_RGBA : u_I4_RGB; break;
            case 8 :  unpack = trFound ? u_I8_RGBA : u_I8_RGB; break;
            default : ERROR();
            }
            *comps = ncomp = trFound ? 4 : 3;
        }

        bitsPerPixel = ihdr.depth;
        numIdx = 1 << bitsPerPixel;

        if (!plteFound) ERROR();

        /* Pad out palette with white if needed.  The PNG spec
         * wants us to error out if any undefined pixels are found.
         * This is a royal pain, so I'm not going to do it.
         * I fill out all 256 entries just to be safe.
         */
        n = plteFound / 3;
        while (plteFound < (3*256)) {
            palette[plteFound++] = 0xFF;
            palette[plteFound++] = 0xFF;
            palette[plteFound++] = 0xFF;
        }

        /* Expand palette to RGBA */
        for (i = 255; i >= 0; i--) {
            palette[4*i+3] = ((i >= n) || !trFound) ? 0xFF : trans[i];
            palette[4*i+2] = palette[3*i+2];
            palette[4*i+1] = palette[3*i+1];
            palette[4*i  ] = palette[3*i  ];
        }

        /* Copy the palette over if requested */
        if( pPall ) {
            *pPall = malloc(4*256);
            if( !*pPall ) ERROR();
            memcpy(*pPall, palette, 4*256);
        }
        upArg = palette;
        break;
    case PNG_TYPE_RGBA :
        switch (ihdr.depth) {
        case 8 :  unpack = u_RGBA_RGBA;   break;
        case 16 :
            if (flags & PNG_READ_16) {
                unpack = u_RGBA16_RGBA_16;
            }
            else {
                unpack = u_RGBA16_RGBA;
            }
            break;
        default : ERROR();
        }
        bitsPerPixel = 4*ihdr.depth;
        *comps = ncomp = 4;
        break;
    default :
        ERROR();
    }

    /* Get total size of IDAT */
    idatSize = 0;
    do {
        if (io->seek(io->f, chunkSize, SEEK_CUR) != 0) ERROR();
        idatSize += chunkSize;
        if (io->read(&crc, sizeof(crc), 1, io->f) != 1) ERROR();
        if (io->read(&chunkSize, sizeof(chunkSize), 1, io->f) != 1) ERROR();
        SWAB4(&chunkSize);
        if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();
    } while (chunkHdr == PNG_IDAT);

    /* OK, rerun that loop, this time sucking the data in */
    idatData = malloc(idatSize);
    if (!idatData) ERROR();

    if (io->seek(io->f, idatPos, SEEK_SET) != 0) ERROR();

    idatSize = 0;
    do {
        if (io->read(&chunkSize, sizeof(chunkSize), 1, io->f) != 1) ERROR();
        SWAB4(&chunkSize);
        if (io->read(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();
        if (chunkHdr == PNG_IDAT) {
            if (io->read(idatData+idatSize, chunkSize, 1, io->f) != 1) ERROR();
            idatSize += chunkSize;
            if (io->read(&crc, sizeof(crc), 1, io->f) != 1) ERROR();
        }
    } while (chunkHdr == PNG_IDAT);

    bytesPerRow = 1 + (iw * bitsPerPixel + 7)/8;

#if 0
    /* Calculate size of uncompressed image */
    if (ihdr.interlace) {
        int sub, tw, th, bpr;

        uncompBytes = 0;
        for (sub = 0; sub < ILACE_PASSES; sub++) {
            /* Figure out size of subimage */
            tw = calcSubSize(iw, ilaceInfo[sub].w,
                                 ilaceInfo[sub].c0,
                                 ilaceInfo[sub].pixStride);
            th = calcSubSize(ih, ilaceInfo[sub].h,
                                 ilaceInfo[sub].r0,
                                 ilaceInfo[sub].rowStride);
            if ((tw == 0) || (th == 0)) continue;

            /* Calculate bytes-per-row for subimage and add
             * total subimage size to uncompressed size
             */
            bpr = 1 + (tw * bitsPerPixel + 7) / 8;
            uncompBytes += th * bpr;
        }
    }
    else {
        /* Non-interlaced is easy :-) */
        uncompBytes = ih * bytesPerRow;
    }
#endif

#if USE_PUFF
    idatSize -= 2;

    res = puff(NIL, &uncompBytes, idatData+2, &idatSize);
    if (res != 0) {
#if USE_SSHZLIB
        goto skipPuff;
#else
        ERROR();
#endif
    }
    uncompData = malloc(uncompBytes);
    if (!uncompData) ERROR();

    /* So, we have compressed data and a dest buffer.  Expand the data. */
    res = puff(uncompData, &uncompBytes, idatData+2, &idatSize) != 0;
    if (res != 0) {
#if !USE_SSHZLIB
        ERROR();
#endif
    }
#if USE_SSHZLIB
skipPuff :
#endif
#else
    res = -1;
#endif

#if USE_SSHZLIB
    if (res != 0) {
        pz = zlib_decompress_init();

        i = zlib_decompress_block(pz, idatData, idatSize,
                                      &uncompData, &iuncompBytes);
        if (!i) {
            ERROR();
        }
    }
#endif

    /* All done with compressed IDAT data  TBD - it would be really
     * nice not to have to suck in all that memory.  Oh, well.
     */
    free(idatData); idatData = NULL;

    /* Allocate destination image */
    if (flags & PNG_READ_16) {
        *img = iptr = malloc(iw * ih * ncomp * 2);
    }
    else {
        *img = iptr = malloc(iw * ih * ncomp);
    }
    if (!*img) ERROR();

    /* Calculate the stride to the previous pixel, in bytes */
    filtStride = (bitsPerPixel + 7) / 8;

    /* OK.  Now, gotta deinterlace and defilter. */
    if (ihdr.interlace) {
        int sub;
        unsigned char *curr;

        curr = uncompData + 1;

        for (sub = 0; sub < ILACE_PASSES; sub++) {
            unsigned char *prev, *optr;
            int i, tw, th, bpr, pixStride, rowStride;

            /* Calculate the size of the current subimage */
            tw = calcSubSize(iw, ilaceInfo[sub].w,
                                 ilaceInfo[sub].c0,
                                 ilaceInfo[sub].pixStride);
            th = calcSubSize(ih, ilaceInfo[sub].h,
                                 ilaceInfo[sub].r0,
                                 ilaceInfo[sub].rowStride);
            if ((tw == 0) || (th == 0)) continue;

            bpr = 1 + (tw * bitsPerPixel + 7) / 8;

            /* Now, process the subimage */
            prev = NULL;
            if (flags & PNG_READ_16) {
                optr = iptr + (ilaceInfo[sub].r0*iw+ilaceInfo[sub].c0)*ncomp*2;
                pixStride = ilaceInfo[sub].pixStride*ncomp*2;
                rowStride = ilaceInfo[sub].rowStride*iw*ncomp*2;
            }
            else {
                optr = iptr + (ilaceInfo[sub].r0*iw + ilaceInfo[sub].c0)*ncomp;
                pixStride = ilaceInfo[sub].pixStride*ncomp;
                rowStride = ilaceInfo[sub].rowStride*iw*ncomp;
            }


            for (i = 0; i < th; i++) {
                switch (curr[-1]) {
                case PNG_FILTER_NONE :
                    break;
                case PNG_FILTER_SUB :
                    recon_sub(bpr-1, filtStride, prev, curr);
                    break;
                case PNG_FILTER_UP :
                    recon_up(bpr-1, filtStride, prev, curr);
                    break;
                case PNG_FILTER_AVG :
                    recon_avg(bpr-1, filtStride, prev, curr);
                    break;
                case PNG_FILTER_PAETH :
                    recon_paeth(bpr-1, filtStride, prev, curr);
                    break;
                default :
                    ERROR();
                }
                (*unpack)(tw, pixStride, optr, upArg, curr);

                optr += rowStride;
                prev = curr;
                curr += bpr;
            }
        }
    }
    else {
        int i, j;
        unsigned char *curr, *prev, *optr;

        prev = NULL;
        curr =  uncompData + 1;
        optr = iptr;

        for (i = 0; i < ih; i++) {
            switch (curr[-1]) {
            case PNG_FILTER_NONE :
                break;
            case PNG_FILTER_SUB :
                recon_sub(bytesPerRow-1, filtStride, prev, curr);
                break;
            case PNG_FILTER_UP :
                recon_up(bytesPerRow-1, filtStride, prev, curr);
                break;
            case PNG_FILTER_AVG :
                recon_avg(bytesPerRow-1, filtStride, prev, curr);
                break;
            case PNG_FILTER_PAETH :
                recon_paeth(bytesPerRow-1, filtStride, prev, curr);
                break;
            default :
                ERROR();
            }
            if (flags & PNG_READ_16) {
                (*unpack)(iw, ncomp*2, optr, upArg, curr);
                optr += iw * ncomp*2;
            }
            else {
                (*unpack)(iw, ncomp, optr, upArg, curr);
                optr += iw * ncomp;
            }

            prev = curr;
            curr += bytesPerRow;
        }
    }

#if USE_PUFF
    free(uncompData); uncompData = NULL;
#else
    zlib_decompress_cleanup(pz); pz = NULL;
#endif

    if (flags & PNG_FLIP_Y) {
        int i, j, bw, t;
        unsigned char *top, *bot;

        /* HACK - the better place to do this is as we are reading the image */
        bw = iw*ncomp;
        top = iptr;
        bot = iptr + (ih - 1)*bw;

        for (i = 0; i < (ih/2); i++) {
            for (j = 0; j < iw*ncomp; j++) {
                t = top[j]; top[j] = bot[j]; bot[j] = t;
            }
            top += bw;
            bot -= bw;
        }
    }

    return flags;

error :
#if USE_PUFF
    if (uncompData) free(uncompData);
#else
    if (pz) zlib_decompress_cleanup(pz);
#endif
    if (idatData) free(idatData);
    if (*img) free(*img);
    return -PngErrLine;
}

#endif // ]

int PngInitStdIn(PngInStruct *io, char *filename)
{
#if defined(ANDROID_NDK) /* [ */
    io->read = (PngReadFunc)hwFread;
    io->f = hwFopen(filename, "rb");
    io->seek = (PngSeekFunc) hwFseek;
    io->tell = (PngTellFunc) hwFtell;
#else /* ] [ */
    io->read = (PngReadFunc) fread;
    io->seek = (PngSeekFunc) fseek;
    io->tell = (PngTellFunc) ftell;
    io->f = fopen(filename, "rb");
#endif /* ] */
    if (!io->f) return 0;
    return 1;
}

void PngTermStdIn(PngInStruct *io)
{
#if defined(ANDROID_NDK) /* [ */
    if (io->f) hwFclose(io->f);
#else /* ] [ */
    if (io->f) fclose((FILE *)io->f);
#endif /* ] */
}

static int memRead(void *data, size_t size, size_t nelems, void *f)
{
    PngInStruct *io;
    size_t total;

    io = (PngInStruct *) f;
    total = size * nelems;
    if (total > (io->size - io->offs)) return 0;

    memcpy(data, io->ptr + io->offs, total);
    io->offs += total;

    return nelems;
}

static int memSeek(void *f, long offs, int whence)
{
    int pos;
    PngInStruct *io;

    io = (PngInStruct *) f;
    switch (whence) {
    case SEEK_SET :
        pos = offs;
        break;
    case SEEK_CUR :
        pos = io->offs + offs;
        break;
    case SEEK_END :
        pos = io->size + offs;
        break;
    default :
        return -1;
    }
    if ((pos < 0) || (pos > io->size)) return -1;
    io->offs = pos;
    return 0;
}

static long memTell(void *f)
{
    PngInStruct *io;

    io = (PngInStruct *) f;
    return io->offs;
}

int PngInitMemIn(PngInStruct *io, unsigned char *data, int dataSize)
{
    io->read = (PngReadFunc) memRead;
    io->seek = (PngSeekFunc) memSeek;
    io->tell = (PngTellFunc) memTell;
    io->f = io;
    io->ptr = data;
    io->offs = 0;
    io->size = dataSize;
    return 1;
}
