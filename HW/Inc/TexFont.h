
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#ifndef __TEXFONT_H__
#define __TEXFONT_H__

#include <stdlib.h>

/* Structure of a TXF file:
 *     offs    size             notes
 *      0        4               "\377txf"
 *      4        4               0x12, 0x34, 0x56, 0x78 - byte swap indicator
 *      8        4               format (from TXF_FORMAT_*, below)
 *      12       4               tex_width
 *      16       4               tex_height
 *      20       4               max_ascent
 *      24       4               max_descent
 *      28       4               num_glyphs (ng)
 *      32       ng*12           glyphs (see texGlyphInfo, below)
 *      32+ng*12 w*h             (or ((w+7)/8)*h) texture
 */

#define TXF_FORMAT_BYTE         0
#define TXF_FORMAT_BITMAP       1

typedef struct {
    unsigned short c;       /* Potentially support 16-bit glyphs. */
    unsigned char width;
    unsigned char height;
    signed char xoffset;
    signed char yoffset;
    signed char advance;
    char dummy;           /* Space holder for alignment reasons. */
    short x;
    short y;
} TexGlyphInfo;

typedef struct {
    hwFloat t0[2];
    hwInt16 v0[2];
    hwFloat t1[2];
    hwInt16 v1[2];
    hwFloat t2[2];
    hwInt16 v2[2];
    hwFloat t3[2];
    hwInt16 v3[2];
    hwFloat advance;
} TexGlyphVertexInfo;

typedef struct {
    hwObject texobj;
    int tex_width;
    int tex_height;
    int max_ascent;
    int max_descent;
    int num_glyphs;
    int min_glyph;
    int range;
    unsigned char *teximage;
    TexGlyphInfo *tgi;
    TexGlyphVertexInfo *tgvi;
    TexGlyphVertexInfo **lut;
} TexFont;

typedef size_t (*TxfReadFunc)(void *, size_t, size_t, void *);

extern char *txfErrorString(void);

extern void txfResetStaticFonts(void);

extern TexFont *txfLoadFont(TxfReadFunc read, void *file);

extern void txfUnloadFont(TexFont * txf);

extern hwObject txfEstablishTexture(hwDisplay disp, TexFont * txf);

extern void txfBindFontTexture(hwDisplay disp, TexFont * txf);

extern void txfGetStringMetrics(TexFont * txf, unsigned char *string, int len,
                                int *width, int *max_ascent, int *max_descent);

extern void txfRenderGlyph(hwDisplay disp, TexFont * txf, int c);

extern void txfRenderString(hwDisplay disp, TexFont * txf,
                            unsigned char *string, int len);

extern void txfRenderFancyString(hwDisplay disp, TexFont * txf,
                                 unsigned char *string, int len);

extern void txfSetPosition(hwFloat xPos, hwFloat yPos);

extern void txfSetScale(hwFloat pixSize);

extern void txfSetColor(hwFloat color[4]);

#define TXF_COORD_MODE_NDC      0       /* Scaling is in NDCs, default */
#define TXF_COORD_MODE_WIN      1       /* Scaling is in window mode */

extern int txfBeginFontRendering(hwDisplay disp, TexFont *txf, hwInt32 mode);

extern void txfEndFontRendering(hwDisplay disp);

extern TexFont *txfLoadStaticFont(int size);

#endif /* __TEXFONT_H__ */
