
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o gentexfont gentexfont.c -lX11 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library;
FT_Face face;

#include "hw.h"
#include "TexFont.h"

#define TEX_DEFAULT             256
#define GAP_DEFAULT             1
#define FILE_DEFAULT            "default.txf"
#define HEIGHT_DEFAULT          24
#define SDF_DEFAULT             0

typedef struct {
      short width;
      short height;
      short xoffset;
      short yoffset;
      short advance;
      unsigned char *bitmap;
} PerGlyphInfo, *PerGlyphInfoPtr;

typedef struct {
      int min_char;
      int max_char;
      int max_ascent;
      int max_descent;
      PerGlyphInfo glyph[1];
} FontInfo, *FontInfoPtr;

FontInfoPtr fontinfo;
int format = TXF_FORMAT_BITMAP;
int gap = GAP_DEFAULT;
char *fontName = NULL;
int fontHeight = HEIGHT_DEFAULT;
int sdfSize = 0;
int bold = 0;
int italic = 0;
int antialias = 0;

/*#define REPORT_GLYPHS*/
#ifdef REPORT_GLYPHS
#define DEBUG_GLYPH4(msg,a,b,c,d) printf(msg,a,b,c,d)
#define DEBUG_GLYPH(msg) printf(msg)
#else
#define DEBUG_GLYPH4(msg,a,b,c,d) { /* nothing */ }
#define DEBUG_GLYPH(msg) { /* nothing */ }
#endif

#define MAX_GLYPHS_PER_GRAB 512  /* this is big enough for 2^9 glyph
                                    character sets */

FontInfoPtr getGlyphs(void)
{
    int numchars;
    int width, height, pixwidth;
    int i, j;
    unsigned char *bitmapData;
    int x, y;
    int spanLength;
    int charWidth, charHeight, maxSpanLength;
    int grabList[MAX_GLYPHS_PER_GRAB];
    int glyphsPerGrab = MAX_GLYPHS_PER_GRAB;
    int numToGrab, thisglyph;
    FontInfoPtr myfontinfo;
    TEXTMETRIC metrics;
    GLYPHMETRICS glyph;
    char str[2];
    MAT2 mat;
    int grey;

    numchars = 256;

    myfontinfo = (FontInfoPtr) malloc(sizeof(FontInfo) +
                                      (numchars - 1) * sizeof(PerGlyphInfo));
    if (!myfontinfo) {
        return NULL;
    }

    if (!GetTextMetrics(hdc, &metrics)) {
        return NULL;
    }
    DEBUG_GLYPH4("Metrics: ascent=%d, descent=%d, width=%d, height=%d\n",
                 metrics.tmAscent, metrics.tmDescent,
                 metrics.tmMaxCharWidth, metrics.tmHeight);

    myfontinfo->min_char = 0;
    myfontinfo->max_char = 255;
    myfontinfo->max_ascent = metrics.tmAscent;
    myfontinfo->max_descent = metrics.tmDescent;

    width = metrics.tmMaxCharWidth;
    height = metrics.tmHeight;

    maxSpanLength = (width + 7) / 8;

    if ((glyphsPerGrab * 8 * maxSpanLength) >= (1 << 15)) {
        glyphsPerGrab = (1 << 15) / (8 * maxSpanLength);
    }
    pixwidth = glyphsPerGrab * 8 * maxSpanLength;

    hbitmap = CreateCompatibleBitmap(hdc, pixwidth, height);
    if (!hbitmap) {
        fprintf(stderr, "CreateCompatibleBitmap failed\n");
        return NULL;
    }

    if (!SelectObject(hdc, hbitmap)) {
        fprintf(stderr, "SelectObject failed\n");
        return NULL;
    }

    (void) SelectObject(hdc, CreateSolidBrush(RGB(0,0,0)));
    Rectangle(hdc, 0, 0, pixwidth, height);
    (void) SetTextColor(hdc, RGB(255,255,255));
    (void) SetBkColor(hdc, RGB(0,0,0));
    (void) SetTextAlign(hdc, TA_BASELINE | TA_LEFT);

    numToGrab = 0;

    mat.eM11.value = 1; mat.eM11.fract = 0;
    mat.eM12.value = 0; mat.eM12.fract = 0;
    mat.eM21.value = 0; mat.eM21.fract = 0;
    mat.eM22.value = 1; mat.eM22.fract = 0;

    for (i = 0; i < numchars; i++) {
        glyphIndex = FT_Get_Char_Index(face, 
        if (GetGlyphOutline(hdc, i, GGO_METRICS, &glyph, 0, NULL, &mat) ==
            GDI_ERROR) {
            fprintf(stderr, "GetGlyphOutline failed\n");
            return NULL;
        }

        charWidth = glyph.gmBlackBoxX;
        charHeight = glyph.gmBlackBoxY;

        myfontinfo->glyph[i].width = 0;
        myfontinfo->glyph[i].height = 0;
        myfontinfo->glyph[i].xoffset = 0;
        myfontinfo->glyph[i].yoffset = 0;
        myfontinfo->glyph[i].advance = glyph.gmCellIncX;
        myfontinfo->glyph[i].bitmap = NULL;

        if ((charWidth != 0) && (charHeight != 0)) {
            grabList[numToGrab] = i;

            str[0] = i;
            TextOut(hdc, 8 * maxSpanLength * numToGrab,
                         metrics.tmAscent, str, 1);

            numToGrab++;
        }

        if ((numToGrab >= glyphsPerGrab) || (i == numchars - 1)) {
            for (j = 0; j < numToGrab; j++) {
                thisglyph = grabList[j];
                (void) GetGlyphOutline(hdc, thisglyph, GGO_METRICS,
                                       &glyph, 0, NULL, &mat);

                charWidth = glyph.gmBlackBoxX;
                charHeight = glyph.gmBlackBoxY;

                if (format == TXF_FORMAT_BITMAP) {
                    spanLength = (charWidth + 7) / 8;
                    bitmapData = calloc(charHeight * spanLength, sizeof(char));
                } else {
                    bitmapData = calloc(charHeight * charWidth, sizeof(char));
                }
                if (!bitmapData) {
                    goto FreeFontAndReturn;
                }
                DEBUG_GLYPH4("index %d, glyph %d (%d by %d)\n",
                             j, thisglyph, charWidth, charHeight);
                DEBUG_GLYPH4("glyph bbox: (%d x %d) @ (%d,%d)\n",
                             glyph.gmBlackBoxX,
                             glyph.gmBlackBoxY,
                             glyph.gmptGlyphOrigin.x,
                             glyph.gmptGlyphOrigin.y);

                for (y = 0; y < charHeight; y++) {
                    int py = (charHeight - y - 1) +
                             (metrics.tmAscent - glyph.gmptGlyphOrigin.y);

                    for (x = 0; x < charWidth; x++) {
                        int px = x + glyph.gmptGlyphOrigin.x;
                        int pix;

                        /* XXX The algorithm used to suck across the font
                         * ensures that each glyph begins on a byte boundary.
                         * In theory this would make it convienent to copy the
                         * glyph into a byte oriented bitmap.  We actually use
                         * the GetPixel function to extract each pixel from
                         * the image which is not that efficient.  We could
                         * either do tighter packing in the pixmap or more
                         * efficient extraction from the image.  Oh well.
                         */
                        pix = GetPixel(hdc, j * maxSpanLength * 8 + px, py);
                        if (pix == CLR_INVALID) {
                            DEBUG_GLYPH("?");
                        } else if (pix) {
                            DEBUG_GLYPH("x");
                            if (format == TXF_FORMAT_BITMAP) {
                                bitmapData[y*spanLength + x/8] |= (1 << (x&7));
                            } else {
                                grey = GetRValue(pix)*0.30 +
                                       GetGValue(pix)*0.59 +
                                       GetBValue(pix)*0.11 +
                                       0.5f;
                                if (grey < 0) grey = 0;
                                if (grey > 255) grey = 255;
                                bitmapData[y*charWidth + x] = grey;
                            }
                        } else {
                            DEBUG_GLYPH(" ");
                        }
                    }
                    DEBUG_GLYPH("\n");
                }
                myfontinfo->glyph[thisglyph].width = charWidth;
                myfontinfo->glyph[thisglyph].height = charHeight;
                myfontinfo->glyph[thisglyph].xoffset = glyph.gmptGlyphOrigin.x;
                myfontinfo->glyph[thisglyph].yoffset =
                        (glyph.gmptGlyphOrigin.y - charHeight);
                myfontinfo->glyph[thisglyph].advance = glyph.gmCellIncX;
                myfontinfo->glyph[thisglyph].bitmap = bitmapData;
            }
            numToGrab = 0;
            /* do we need to clear the offscreen pixmap to get more? */
            if (i < numchars - 1) {
                Rectangle(hdc, 0, 0, pixwidth, height);
            }
        }
    }
    return myfontinfo;

FreeFontAndReturn:
    for (j = i - 1; j >= 0; j--) {
        if (myfontinfo->glyph[j].bitmap) {
            free(myfontinfo->glyph[j].bitmap);
        }
    }
    free(myfontinfo);
    return NULL;
}

void printGlyph(FontInfoPtr font, int c)
{
    PerGlyphInfoPtr glyph;
    unsigned char *bitmapData;
    int width, height, spanLength;
    int x, y;

    if (c < font->min_char || c > font->max_char) {
        fprintf(stderr, "out of range glyph\n");
        return;
    }
    glyph = &font->glyph[c - font->min_char];
    bitmapData = glyph->bitmap;
    if (bitmapData) {
        width = glyph->width;
        spanLength = (width + 7) / 8;
        height = glyph->height;

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (format == TXF_FORMAT_BITMAP) {
                    if (bitmapData[y * spanLength + x / 8] & (1 << (x & 7))) {
                        putchar('X');
                    } else {
                        putchar('.');
                    }
                } else {
                    if (bitmapData[y * width + x] > 127) {
                        putchar('X');
                    } else {
                        putchar('.');
                    }
                }
            }
            putchar('\n');
        }
    }
}

void getMetric(FontInfoPtr font, int c, TexGlyphInfo * tgi)
{
    PerGlyphInfoPtr glyph;
    unsigned char *bitmapData;

    tgi->c = c;
    if (c < font->min_char || c > font->max_char) {
        tgi->width = 0;
        tgi->height = 0;
        tgi->xoffset = 0;
        tgi->yoffset = 0;
        tgi->dummy = 0;
        tgi->advance = 0;
        return;
    }
    glyph = &font->glyph[c - font->min_char];
    bitmapData = glyph->bitmap;
    if (bitmapData) {
        tgi->width = glyph->width;
        tgi->height = glyph->height;
        tgi->xoffset = glyph->xoffset;
        tgi->yoffset = glyph->yoffset;
    } else {
        tgi->width = 0;
        tgi->height = 0;
        tgi->xoffset = 0;
        tgi->yoffset = 0;
    }
    tgi->dummy = 0;
    tgi->advance = glyph->advance;
}

int glyphCompare(const void *a, const void *b)
{
    unsigned char *c1 = (unsigned char *) a;
    unsigned char *c2 = (unsigned char *) b;
    TexGlyphInfo tgi1;
    TexGlyphInfo tgi2;

    getMetric(fontinfo, *c1, &tgi1);
    getMetric(fontinfo, *c2, &tgi2);
    return tgi2.height - tgi1.height;
}

int getFontel(unsigned char *bitmapData, int spanLength, int i, int j)
{
    return bitmapData[i * spanLength + j / 8] & (1 << (j & 7)) ? 255 : 0;
}

void placeGlyph(FontInfoPtr font, int c, unsigned char *texarea,
                int stride, int x, int y)
{
    PerGlyphInfoPtr glyph;
    unsigned char *bitmapData;
    int width, height, spanLength;
    int i, j;

    if (c < font->min_char || c > font->max_char) {
        fprintf(stderr, "out of range glyph\n");
        return;
    }
    glyph = &font->glyph[c - font->min_char];
    bitmapData = glyph->bitmap;
    if (bitmapData) {
        width = glyph->width;
        spanLength = (width + 7) / 8;
        height = glyph->height;

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                if (format == TXF_FORMAT_BITMAP) {
                    texarea[stride * (y + i) + x + j] =
                        getFontel(bitmapData, spanLength, i, j);
                } else {
                    texarea[stride * (y + i) + x + j] = bitmapData[i*width+j];
                }
            }
        }
    }
}

char *nodupstring(char *s)
{
    int len, i, p;
    char *new;

    len = (int) strlen(s);
    new = (char *) calloc(len + 1, 1);
    p = 0;
    for (i = 0; i < len; i++) {
        if (!strchr(new, s[i])) {
            new[p] = s[i];
            p++;
        }
    }
    new = realloc(new, p + 1);
    return new;
}

int isOption(char *optName, int optExtra, int i, int argc, char **argv)
{
    if (strcmp(argv[i], optName) != 0) return 0;
    if (i >= (argc - optExtra)) return 0;
    return 1;
}

void main(int argc, char *argv[])
{
    int texw, texh;
    unsigned char *texarea, *texbitmap;
    FILE *file;
    int len, stride;
    unsigned char *glist, static_glist[256];
    int width, height;
    int px, py, maxheight;
    TexGlyphInfo tgi;
    int usageError = 0;
    char *filename;
    int endianness;
    int i, j;
    int error;

    texw = texh = TEX_DEFAULT;

    for (i = 32, j = 0; i < 128; i++, j++) {
        static_glist[j] = i;
    }
    static_glist[j] = 0;
    glist = static_glist;

    filename = FILE_DEFAULT;

    for (i = 1; (i < argc) && !usageError; i++) {
        if (isOption(argc, argv, i,      "-w", 1)) {
            i++;
            texw = atoi(argv[i]);
        }
        else if (isOption(argc, argv, i, "-h", 1)) {
            i++;
            texh = atoi(argv[i]);
        }
        else if (isOption(argv, argv, i, "-gap", 1)) {
            i++;
            gap = atoi(argv[i]);
        }
        else if (isOption(argc, argv, i, "-byte", 0)) {
            format = TXF_FORMAT_BYTE;
        }
        else if (isOption(argc, argv, i, "-bitmap", 0)) {
            format = TXF_FORMAT_BITMAP;
        }
        else if (isOption(argc, argv, i, "-glist", 1)) {
            i++;
            glist = (unsigned char *) argv[i];
        }
        else if (isOption(argc, argv, i, "-file", 1)) {
            i++;
            filename = argv[i];
        }
        else if (isOption(argc, argv, i, "-font", 1)) {
            i++;
            fontName = argv[i];
        }
        else if (isOption(argc, argv, i, "-size", 1)) {
            i++;
            fontHeight = atoi(argv[i]);
        }
        else if (isOption(argc, argv, i, "-sdf", 1)) {
            i++;
            sdfSize = atoi(argv[i]);
        }
        else if (isOption(argc, argv, i, "-bold", 0)) {
            bold = 1;
        }
        else if (isOption(argc, argv, i, "-italic", 0)) {
            italic = 1;
        }
        else if (isOption(argc, argv, i, "-antialias", 0)) {
            antialias = 1;
            format = TXF_FORMAT_BYTE;
        }
        else {
            usageError = 1;
        }
    }

    if (!fontName) {
        usageError = 1;
    }

    if (usageError) {
        fputc('\n', stderr);
        fprintf(stderr, "usage: mktexfont [options] txf-file\n");
        fprintf(stderr, " -font fname   font name (no default - required)\n");
        fprintf(stderr, " -w #          textureWidth (def=%d)\n",
                TEX_DEFAULT);
        fprintf(stderr, " -h #          textureHeight (def=%d)\n",
                TEX_DEFAULT);
        fprintf(stderr, " -gap #        gap between glyphs (def=%d)\n",
                GAP_DEFAULT);
        fprintf(stderr, " -bitmap       use a bitmap encoding (default)\n");
        fprintf(stderr, " -byte         use a byte encoding (less compact)\n");
        fprintf(stderr, " -glist ABC    glyph list (def=ASCII)\n",)
        fprintf(stderr, " -file name    output file for tex font (def=%s)\n",
                FILE_DEFAULT);
        fprintf(stderr, " -size #       font size (def=%d)\n",
                HEIGHT_DEFAULT);
        fprintf(stderr, " -sdf #        signed distance field width (def=%d)\n",
                SDF_DEFAULT);
        fprintf(stderr, " -bold         synthetic bold face\n");
        fprintf(stderr, " -italic       synthetic italic\n");
        fprintf(stderr, " -antialias    antitalias the font\n");
        fputc('\n', stderr);
        exit(1);
    }

    error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "FreeType initialization error\n");
        exit(1);
    }

    error = FT_NewFace(library, fontName, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "%s - not a font file\n", fontName);
        exit(1);
    }
    else if (error) {
        fprintf(stderr, "Cannot open %s\n", fontName);
        exit(1);
    }

    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        fprintf(stderr,  "%s - not a scalable font\n");
        exit(1);
    }

    error = FT_Set_Char_Size(face, 0, fontHeight*64, 72, 72);
    if (error) {
        fprintf(stderr, "Cannot select height %d\n", fontHeight);
        exit(1);
    }

    texarea = calloc(texw * texh, sizeof(unsigned char));
    glist = (unsigned char *) nodupstring((char *) glist);

    fontinfo = getGlyphs(face);
    if (!fontinfo) {
        fprintf(stderr, "could not get font glyphs\n");
        exit(1);
    }
    len = (int) strlen((char *) glist);
    qsort(glist, len, sizeof(unsigned char), glyphCompare);

    file = fopen(filename, "wb");
    fwrite("\377txf", 1, 4, file);
    endianness = 0x12345678;
    /*CONSTANTCONDITION*/
    assert(sizeof(int) == 4);  /* Ensure external file format size. */
    fwrite(&endianness, sizeof(int), 1, file);
    fwrite(&format, sizeof(int), 1, file);
    fwrite(&texw, sizeof(int), 1, file);
    fwrite(&texh, sizeof(int), 1, file);
    fwrite(&fontinfo->max_ascent, sizeof(int), 1, file);
    fwrite(&fontinfo->max_descent, sizeof(int), 1, file);
    fwrite(&len, sizeof(int), 1, file);

    px = gap;
    py = gap;
    maxheight = 0;
    for (i = 0; i < len; i++) {
        if (glist[i] != 0) {  /* If not already processed... */

            /* Try to find a character from the glist that will fit on the
            remaining space on the current row. */

            int foundWidthFit = 0;
            int c;

            getMetric(fontinfo, glist[i], &tgi);
            width = tgi.width;
            height = tgi.height;
            if (height > 0 && width > 0) {
                for (j = i; j < len;) {
                    if (height > 0 && width > 0) {
                        if (px + width + gap < texw) {
                            foundWidthFit = 1;
                            if (j != i) {
                                /* Step back so i loop increment leaves us
                                 * at same character.
                                 */
                                i--;
                            }
                            break;
                        }
                    }
                    j++;
                    getMetric(fontinfo, glist[j], &tgi);
                    width = tgi.width;
                    height = tgi.height;
                }

                /* If a fit was found, use that character; otherwise,
                 * advance a line in  the texture.
                 */
                if (foundWidthFit) {
                    if (height > maxheight) {
                        maxheight = height;
                    }
                    c = j;
                } else {
                    getMetric(fontinfo, glist[i], &tgi);
                    width = tgi.width;
                    height = tgi.height;

                    py += maxheight + gap;
                    px = gap;
                    maxheight = height;
                    if (py + height + gap >= texh) {
                        fprintf(stderr, "Overflowed texture space.\n");
                        exit(1);
                    }
                    c = i;
                }

                /* Place the glyph in the texture image. */
                placeGlyph(fontinfo, glist[c], texarea, texw, px, py);

                /* Assign glyph's texture coordinate. */
                tgi.x = px;
                tgi.y = py;

                /* Advance by glyph width, remaining in the current line. */
                px += width + gap;
            } else {
                /* No texture image; assign invalid bogus coordinates. */
                tgi.x = -1;
                tgi.y = -1;
            }
            glist[c] = 0;     /* Mark processed; don't process again. */
            /*CONSTANTCONDITION*/
            assert(sizeof(tgi) == 12);  /* Ensure external file format size. */
            fwrite(&tgi, sizeof(tgi), 1, file);
        }
    }

    switch (format) {
    case TXF_FORMAT_BYTE:
        fwrite(texarea, texw * texh, 1, file);
        break;
    case TXF_FORMAT_BITMAP:
        stride = (texw + 7) >> 3;
        texbitmap = (unsigned char *) calloc(stride * texh, 1);
        for (i = 0; i < texh; i++) {
            for (j = 0; j < texw; j++) {
                if (texarea[i * texw + j] >= 128) {
                    texbitmap[i * stride + (j >> 3)] |= 1 << (j & 7);
                }
            }
        }
        fwrite(texbitmap, stride * texh, 1, file);
        free(texbitmap);
        break;
    default:
        fprintf(stderr, "Unknown texture font format.\n");
        exit(1);
    }
    free(texarea);
    fclose(file);
}
