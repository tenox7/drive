/******************************************************************************
 * This file implements basic PNG writing.  This is based on the
 * W3 spec at:
 *
 *      http://www.w3.org/TR/2003/REC-PNG-20031110/
 *
 * This implementation is optimized for minimum code size.  It does
 * not compress the images at all (disk size will be slightly greater
 * than memory size) and it only handles a subset of possible input types.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "writepng.h"

/******************************************************************************
 * Error utility macro - store the line number where we barfed, and go
 * to the error exit label
 *****************************************************************************/
static int PngErrLine;
#define ERROR() do { PngErrLine = __LINE__; goto error; } while(0)

#if defined(USE_LIBPNG) // [
#include <png.h>

static void pngWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PngOutStruct *io;

    io = png_get_io_ptr(png_ptr);
    if (!io) {
        png_error(png_ptr, "pngWriteData - NULL io_ptr");
    }
    if (io->write(data, length, 1, io->f) != 1) {
        png_error(png_ptr, "pngReadData - EOD");
    }
}

static void pngFlushData(png_structp png_ptr)
{
    // Nothing to do
}

int WritePNG(PngOutStruct *io, int flags,
             int w, int h, int comps, int bpp, void *img,
             unsigned char *pall)
{
    int i, icomps, byteWidth;
    int needTran = 0;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *rows;
    int colorType;
    png_color palette[256];
    unsigned char trans[256];

    /* Validate parameters */
    icomps = comps;
    switch (comps) {
    case 2 : case 4 :
        if (flags & PNG_FLAG_SKIP_ALPHA) {
            comps--;
        }
        break;
    }

    byteWidth = (bpp == 8) ? 1 : 2;

    switch( comps ) {
    case 1 :
        if( pall ) {
            if( bpp != 8 ) ERROR(); /* Only support 8 bpp indexes */
            colorType = PNG_COLOR_TYPE_PALETTE;
        }
        else {
            colorType = PNG_COLOR_TYPE_GRAY;
        }
        break;
    case 2 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
    case 3 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        colorType = PNG_COLOR_TYPE_RGB;
        break;
    case 4 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        colorType = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
    default :
        ERROR();
    }

    if (pall) {
        for (i = 0; i < 256; i++) {
            palette[i].red = pall[4*i+0];
            palette[i].green = pall[4*i+1];
            palette[i].blue = pall[4*i+2];
            trans[i] = pall[4*i+3];
            if (trans[i] != 0xFF) needTran = 1;
        }
    }

    rows = malloc(h * sizeof(png_bytep));
    if (!rows) ERROR();

    for (i = 0; i < h; i++) {
        rows[i] = i*w*icomps*byteWidth + (png_bytep)img;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) ERROR();

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) ERROR();

    if (setjmp(png_jmpbuf(png_ptr))) ERROR();

    png_set_write_fn(png_ptr, io, pngWriteData, pngFlushData);

    png_set_IHDR(png_ptr, info_ptr, w, h, bpp, colorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    if (!(flags & PNG_FLAG_GAMMA)) {
        png_set_gamma_fixed(png_ptr, PNG_GAMMA_LINEAR, PNG_GAMMA_LINEAR);
    }
    if (pall) {
        png_set_PLTE(png_ptr, info_ptr, palette, 256);
        if (needTran) {
            png_set_tRNS(png_ptr, info_ptr, trans, 256, NULL);
        }
    }

    png_write_info(png_ptr, info_ptr);

#ifdef PNG_LITTLE_ENDIAN
    if (bpp == 16) png_set_swap(png_ptr);
#endif

    if (icomps != comps) png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    png_set_packing(png_ptr);

    png_write_image(png_ptr, rows);

    png_write_end(png_ptr, NULL);

    free(rows);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;

error :
    if (rows) free(rows);
    if (png_ptr) png_destroy_write_struct(&png_ptr, &info_ptr);
    return -PngErrLine;
}

#else // ] [

#include "png_int.h"

#define ZLIB_STANDALONE
#define ZLIB_COMPRESS
#include "sshzlib.c"

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

static int pngWrite(PngOutStruct *io, PNG_U32 hdr, void *data, PNG_U32 size);

typedef struct {
    PNG_U8 *zPtr, *zHdr;
    int numWritten;
    PNG_U32 adlerA, adlerB;
} zStruct;

void zInit( zStruct *zs, PNG_U8 *zBuff );
void zAddByte( zStruct *zs, int byte );
void zTerm( zStruct *zs );

int WritePNG(PngOutStruct *io, int flags,
             int w, int h, int comps, int bpp, void *img,
             unsigned char *pall)
{
    PNG_U32 chunkHdr;
    PNG_IHDR_Struct ihdr;
    PNG_pHYs_Struct phys;
    PNG_U8 palette[3*256];
    PNG_U8 trans[256];
    PNG_U8 *ptr8;
    PNG_U16 *ptr16;
    int i, j, k, needTran, scanLen, byteWidth, inpWidth;
    PNG_U8 *scanLines, *pScan;
    int icomps;
    void *pz;
    unsigned char *outBlock;
    int outLen;

    /* Validate parameters */
    switch( comps ) {
    case 1 :
        if( pall ) {
            if( bpp != 8 ) ERROR(); /* Only support 8 bpp indexes */
        }
        break;
    case 2 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        break;
    case 3 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        break;
    case 4 :
        if( pall ) ERROR();     /* Only support 1-component indexes */
        break;
    default :
        ERROR();
    }

    icomps = comps;
    switch (comps) {
    case 2 : case 4 :
        if (flags & PNG_FLAG_SKIP_ALPHA) {
            comps--;
        }
        break;
    }

    switch (bpp ) {
    case 8 :
        byteWidth = comps*w;
        inpWidth = icomps*w;
        break;
    case 16 :
        byteWidth = 2*comps*w;
        inpWidth = 2*icomps*w;
        break;
    default :
        ERROR();
    }

    /*** Allocate temporary buffer ***/
    scanLen = byteWidth + 1;    /* Include filter method */

    /*** Write magic numbers ***/
    chunkHdr = PNG_MAGIC_0;
    if (io->write(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();

    chunkHdr = PNG_MAGIC_1;
    if (io->write(&chunkHdr, sizeof(chunkHdr), 1, io->f) != 1) ERROR();

    /*** Write header ***/
    ihdr.width = w;     SWAB4(&ihdr.width);
    ihdr.height = h;    SWAB4(&ihdr.height);
    ihdr.depth = bpp;
    switch( comps ) {
    case 1 : ihdr.colorType = pall ? PNG_TYPE_I : PNG_TYPE_L; break;
    case 2 : ihdr.colorType = PNG_TYPE_LA; break;
    case 3 : ihdr.colorType= PNG_TYPE_RGB; break;
    case 4 : ihdr.colorType = PNG_TYPE_RGBA; break;
    }
    ihdr.compression = PNG_COMPRESS_DEFLATE;
    ihdr.filter = PNG_FILTER_ADAPTIVE;
    ihdr.interlace = PNG_ILACE_NONE;
    if (!pngWrite(io, PNG_IHDR, &ihdr, PNG_IHDR_SIZE)) ERROR();

    /*** Write pHYs (some programs will barf without it) ***/
    phys.ppuX = 2835; SWAB4(&phys.ppuX);        /* i.e. 72 dpi */
    phys.ppuY = 2835; SWAB4(&phys.ppuY);        /* i.e. 72 dpi */
    phys.unit = PNG_pHYs_METRE;
    if (!pngWrite(io, PNG_pHYs, &phys, PNG_pHYs_SIZE)) ERROR();

    /*** Write palette, if present ***/
    if( pall ) {
        needTran = 0;
        for (i = 0; i < 256; i++) {
            palette[3*i+0] = pall[4*i+0];
            palette[3*i+1] = pall[4*i+1];
            palette[3*i+2] = pall[4*i+2];
            trans[i] = pall[4*i+3];
            if (trans[i] != 0xFF) needTran = 1;
        }

        if (!pngWrite(io, PNG_PLTE, palette, 3*256)) ERROR();

        if( needTran ) {
            if (!pngWrite(io, PNG_tRNS, trans, 256)) ERROR();
        }
    }

    /*** OK, write scanlines ***/
    ptr8 = (unsigned char *)img;

    scanLines = malloc(h*scanLen);
    if (!scanLines) ERROR();

    pScan = scanLines;

    for (i = 0; i < h; i++) {
        *pScan++ = PNG_FILTER_NONE;

        if( bpp == 8 ) {
            for (j = 0; j < w; j++) {
                for (k = 0; k < comps; k++) {
                    *pScan++ = ptr8[j*icomps+k];
                }
            }
        }
        else {
            ptr16 = (unsigned short *)ptr8;
            for (j = 0; j < w; j++) {
                for (k = 0; k < comps; k++) {
                    *pScan++ = (ptr16[j*icomps+k] >> 8);
                    *pScan++ = (ptr16[j*icomps+k]     );
                }
            }
        }

        ptr8 += inpWidth;
    }

    pz = zlib_compress_init();
    i = zlib_compress_block(pz, scanLines, h*scanLen, &outBlock, &outLen);
    if (!(i && outBlock)) ERROR();

    if (!pngWrite(io, PNG_IDAT, outBlock, outLen)) ERROR();

    free(scanLines); scanLines = NULL;
    zlib_compress_cleanup(pz); pz = NULL;

    if (!pngWrite(io, PNG_IEND, NULL, 0)) ERROR();

    return 0;

error :
    if (pz) zlib_compress_cleanup(pz);
    if (scanLines) free(scanLines);

    return -PngErrLine;
}

/* Table of CRCs of all 8-bit messages. */
static unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++ ) {
            if (c & 1) {
                c = 0xEDB88320L ^ (c >> 1);
            }
            else {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1] -- the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's completement of the final running CRC (see the
   crc() routine below)). */

static unsigned long update_crc(unsigned long crc, unsigned char *buf,
                                int len)
{
    unsigned long c = crc;
    int n;

    if (!crc_table_computed) {
        make_crc_table();
    }
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the butes buf[0..len-1]. */
static unsigned long calc_crc(unsigned char *buf, int len)
{
    return update_crc(0xFFFFFFFFL, buf, len) ^ 0xFFFFFFFFL;
}

static PNG_U32 png_crc(PNG_U32 hdr, void *data, int dataLen)
{
    unsigned long c;

    c = update_crc(0xFFFFFFFFL, (unsigned char *)&hdr, sizeof(hdr));
    c = update_crc(c, (unsigned char *)data, dataLen);

    return (PNG_U32)(c ^ 0xFFFFFFFFL);
}

static int pngWrite(PngOutStruct *io, PNG_U32 hdr, void *data, PNG_U32 size)
{
    PNG_U32 crc, swabSize;

    swabSize = size; SWAB4(&swabSize);
    crc = png_crc(hdr, data, size); SWAB4(&crc);

    if (io->write(&swabSize, sizeof(PNG_U32), 1, io->f) != 1) return 0;
    if (io->write(&hdr, sizeof(PNG_U32), 1, io->f) != 1) return 0;
    if (size) {
        if (io->write(data, size, 1, io->f) != 1) return 0;
    }
    if (io->write(&crc, sizeof(PNG_U32), 1, io->f) != 1) return 0;

    return 1;
}

#endif // ]

int PngInitStdOut(PngOutStruct *io, char *filename)
{
    io->write = (PngWriteFunc) fwrite;
    io->seek = (PngSeekFunc) fseek;
    io->tell = (PngTellFunc) ftell;
    io->f = fopen(filename, "wb+");
    if (!io->f) return 0;
    return 1;
}

void PngTermStdOut(PngOutStruct *io)
{
    if (io->f) fclose((FILE *)io->f);
}
