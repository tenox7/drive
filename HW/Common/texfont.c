/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"
#include "TexFont.h"

/* byte swap a 32-bit value */
#define SWAPL(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[3];\
                 ((char *) (x))[3] = n;\
                 n = ((char *) (x))[1];\
                 ((char *) (x))[1] = ((char *) (x))[2];\
                 ((char *) (x))[2] = n; }

/* byte swap a short */
#define SWAPS(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[1];\
                 ((char *) (x))[1] = n; }

static struct {
    hwFloat xPos, yPos;
    hwFloat scale, cookedScale;
    hwFloat color[4];
    hwInt32 rendering;
    hwInt32 coordMode;
} txfAttribs = {
    0.f, 0.f,
    24.f, 1.f,
    {1.f, 1.f, 1.f, 1.f},
    0,
    TXF_COORD_MODE_NDC
};

void txfSetPosition(hwFloat xPos, hwFloat yPos)
{
    txfAttribs.xPos = xPos;
    txfAttribs.yPos = yPos;
}

void txfSetScale(hwFloat scale)
{
    txfAttribs.scale = scale;
    txfAttribs.cookedScale = scale;
}

void txfSetColor(hwFloat color[4])
{
    txfAttribs.color[0] = color[0];
    txfAttribs.color[1] = color[1];
    txfAttribs.color[2] = color[2];
    txfAttribs.color[3] = color[3];
}

int txfBeginFontRendering(hwDisplay disp, TexFont *txf, hwInt32 coordMode)
{
    hwSurfaceType surf;
    hwObject txfTexture;
    hwFloat mat[4][4];
    hwInt32 winSize[2];

    if (txfAttribs.rendering) {
        return 0;
    }

    txfTexture = txfEstablishTexture(disp, txf);

    hwDefaultSurf(&surf);
    surf.flags |= HW_SURF_BACKFACE | HW_SURF_BLEND | HW_SURF_EMISSIVE | HW_SURF_BLEND_PREMULT;
    surf.numTextures = 1;
    surf.textures[0] = txfTexture;
    surf.color[0] = txfAttribs.color[0];
    surf.color[1] = txfAttribs.color[1];
    surf.color[2] = txfAttribs.color[2];
    surf.transp = 1.0 - txfAttribs.color[3];
    hwSurfAttrs(&surf);

    switch (coordMode) {
    case TXF_COORD_MODE_NDC :
        /* Nothing to do */
        break;
    case TXF_COORD_MODE_WIN :
        disp->getDrawSize(disp, winSize);

        mat[0][0] = 2.f / (float) winSize[0];
        mat[0][1] = 0.f;
        mat[0][2] = 0.f;
        mat[0][3] = 0.f;

        mat[1][0] = 0.f;
        mat[1][1] = 2.f / (float) winSize[1];
        mat[1][2] = 0.f;
        mat[1][3] = 0.f;

        mat[2][0] = 0.f;
        mat[2][1] = 0.f;
        mat[2][2] = 1.f;
        mat[2][3] = 0.f;

        mat[3][0] = -1.f;
        mat[3][1] = -1.f;
        mat[3][2] = 0.f;
        mat[3][3] = 1.f;

        disp->pushMatrix(disp, mat);
        break;
    default :
        /* Unrecognized coord mode */
        return 0;
    }

    txfAttribs.coordMode = coordMode;
    txfAttribs.rendering = 1;
    txfAttribs.cookedScale = txfAttribs.scale /
                 (float)(txf->max_ascent + txf->max_descent);
    return 1;
}

void txfEndFontRendering(hwDisplay disp)
{
    if (!txfAttribs.rendering) {
        return;
    }
    txfAttribs.rendering = 0;
    if (txfAttribs.coordMode == TXF_COORD_MODE_WIN) {
        disp->popMatrix(disp);
    }
}

static TexGlyphVertexInfo *getTCVI(TexFont * txf, int c)
{
    TexGlyphVertexInfo *tgvi;

    /* Automatically substitute uppercase letters with lowercase if not
       uppercase available (and vice versa). */
    if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
        tgvi = txf->lut[c - txf->min_glyph];
        if (tgvi) {
            return tgvi;
        }
        if (islower(c)) {
            c = toupper(c);
            if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
                return txf->lut[c - txf->min_glyph];
            }
        }
        if (isupper(c)) {
            c = tolower(c);
            if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
                return txf->lut[c - txf->min_glyph];
            }
        }
    }
    /* Punt and return first char */
    return txf->lut[0];
}

static char *lastError;

char *txfErrorString(void)
{
    return lastError;
}

TexFont *txfLoadFont(TxfReadFunc read, void *file)
{
    TexFont *txf;
    hwFloat w, h, xstep, ystep;
    hwFloat fudgeS, fudgeT;
    char fileid[4], tmp;
    unsigned char *texbitmap;
    int min_glyph, max_glyph;
    int endianness, swap, format, stride, width, height;
    int i, j, got;

    txf = NULL;

    if (!file) {
        lastError = "file open error.";
        goto error;
    }

    txf = (TexFont *) malloc(sizeof(TexFont));
    if (txf == NULL) {
        lastError = "out of memory.";
        goto error;
    }
    /* For easy cleanup in error case. */
    txf->tgi = NULL;
    txf->tgvi = NULL;
    txf->lut = NULL;
    txf->teximage = NULL;
    txf->texobj = NULL;

    got = (*read)(fileid, 1, 4, file);
    if (got != 4 || strncmp(fileid, "\377txf", 4)) {
        lastError = "not a texture font file.";
        goto error;
    }
    assert(sizeof(int) == 4);  /* Ensure external file format size. */
    got = (*read)(&endianness, sizeof(int), 1, file);
    if (got == 1 && endianness == 0x12345678) {
        swap = 0;
    } else if (got == 1 && endianness == 0x78563412) {
        swap = 1;
    } else {
        lastError = "not a texture font file.";
        goto error;
    }
#define EXPECT(n, m) \
    if (got != n) { \
        lastError = "premature end of file #" #m; \
        goto error; \
    }

    got = (*read)(&format, sizeof(int), 1, file);
    EXPECT(1,1);
    got = (*read)(&txf->tex_width, sizeof(int), 1, file);
    EXPECT(1,2);
    got = (*read)(&txf->tex_height, sizeof(int), 1, file);
    EXPECT(1,3);
    got = (*read)(&txf->max_ascent, sizeof(int), 1, file);
    EXPECT(1,4);
    got = (*read)(&txf->max_descent, sizeof(int), 1, file);
    EXPECT(1,5);
    got = (*read)(&txf->num_glyphs, sizeof(int), 1, file);
    EXPECT(1,6);

    if (swap) {
        SWAPL(&format, tmp);
        SWAPL(&txf->tex_width, tmp);
        SWAPL(&txf->tex_height, tmp);
        SWAPL(&txf->max_ascent, tmp);
        SWAPL(&txf->max_descent, tmp);
        SWAPL(&txf->num_glyphs, tmp);
    }
    txf->tgi = (TexGlyphInfo *) malloc(txf->num_glyphs * sizeof(TexGlyphInfo));
    if (txf->tgi == NULL) {
        lastError = "out of memory.";
        goto error;
    }
    assert(sizeof(TexGlyphInfo) == 12);  /* Ensure external file format size. */
    got = (*read)(txf->tgi, sizeof(TexGlyphInfo), txf->num_glyphs, file);
    EXPECT(txf->num_glyphs,7);

    if (swap) {
        for (i = 0; i < txf->num_glyphs; i++) {
            SWAPS(&txf->tgi[i].c, tmp);
            SWAPS(&txf->tgi[i].x, tmp);
            SWAPS(&txf->tgi[i].y, tmp);
        }
    }
    txf->tgvi = (TexGlyphVertexInfo *)
    malloc(txf->num_glyphs * sizeof(TexGlyphVertexInfo));
    if (txf->tgvi == NULL) {
        lastError = "out of memory.";
        goto error;
    }
    w = txf->tex_width;
    h = txf->tex_height;
    xstep = 0.5 / w;
    ystep = 0.5 / h;
    fudgeS = 0.;//xstep * 0.5;
    fudgeT = 0.;//ystep * 0.5;
    for (i = 0; i < txf->num_glyphs; i++) {
        TexGlyphInfo *tgi;

        tgi = &txf->tgi[i];
        txf->tgvi[i].t0[0] = tgi->x / w + xstep - fudgeS;
        txf->tgvi[i].t0[1] = tgi->y / h + ystep - fudgeT;
        txf->tgvi[i].v0[0] = tgi->xoffset;
        txf->tgvi[i].v0[1] = tgi->yoffset;
        txf->tgvi[i].t1[0] = (tgi->x + tgi->width) / w + xstep + fudgeS;
        txf->tgvi[i].t1[1] = tgi->y / h + ystep - fudgeT;
        txf->tgvi[i].v1[0] = tgi->xoffset + tgi->width;
        txf->tgvi[i].v1[1] = tgi->yoffset;
        txf->tgvi[i].t2[0] = (tgi->x + tgi->width) / w + xstep + fudgeS;
        txf->tgvi[i].t2[1] = (tgi->y + tgi->height) / h + ystep + fudgeT;
        txf->tgvi[i].v2[0] = tgi->xoffset + tgi->width;
        txf->tgvi[i].v2[1] = tgi->yoffset + tgi->height;
        txf->tgvi[i].t3[0] = tgi->x / w + xstep - fudgeS;
        txf->tgvi[i].t3[1] = (tgi->y + tgi->height) / h + ystep + fudgeT;
        txf->tgvi[i].v3[0] = tgi->xoffset;
        txf->tgvi[i].v3[1] = tgi->yoffset + tgi->height;
        txf->tgvi[i].advance = tgi->advance;
    }

    min_glyph = txf->tgi[0].c;
    max_glyph = txf->tgi[0].c;
    for (i = 1; i < txf->num_glyphs; i++) {
        if (txf->tgi[i].c < min_glyph) {
            min_glyph = txf->tgi[i].c;
        }
        if (txf->tgi[i].c > max_glyph) {
            max_glyph = txf->tgi[i].c;
        }
    }
    txf->min_glyph = min_glyph;
    txf->range = max_glyph - min_glyph + 1;

    txf->lut = (TexGlyphVertexInfo **)
    calloc(txf->range, sizeof(TexGlyphVertexInfo *));
    if (txf->lut == NULL) {
        lastError = "out of memory.";
        goto error;
    }
    for (i = 0; i < txf->num_glyphs; i++) {
        txf->lut[txf->tgi[i].c - txf->min_glyph] = &txf->tgvi[i];
    }

    width = txf->tex_width;
    height = txf->tex_height;

    switch (format) {
    case TXF_FORMAT_BYTE:
        texbitmap = (unsigned char *) malloc(width);
        if (texbitmap == NULL) {
            lastError = "out of memory.";
            goto error;
        }
        txf->teximage = (unsigned char *) malloc(width * height * 2);
        if (txf->teximage == NULL) {
            free(texbitmap);
            lastError = "out of memory.";
            goto error;
        }
        for (i = 0; i < height; i++) {
            got = (*read)(texbitmap, 1, width, file);
            EXPECT(width, 10);
            for (j = 0; j < width; j++) {
                txf->teximage[2*(i*width + j)  ] = texbitmap[j];
                txf->teximage[2*(i*width + j)+1] = texbitmap[j];
            }
        }
        free(texbitmap);
        break;
    case TXF_FORMAT_BITMAP:
        stride = (width + 7) >> 3;
        texbitmap = (unsigned char *) malloc(stride);
        if (texbitmap == NULL) {
            lastError = "out of memory.";
            goto error;
        }
        txf->teximage = (unsigned char *) calloc(2 * width * height, 1);
        if (txf->teximage == NULL) {
            lastError = "out of memory.";
            goto error;
        }
        for (i = 0; i < height; i++) {
            got = (*read)(texbitmap, 1, stride, file);
            EXPECT(stride, 11);
            for (j = 0; j < width; j++) {
                if (texbitmap[j >> 3] & (1 << (j & 7))) {
                    txf->teximage[2*(i*width + j)  ] = 255;
                    txf->teximage[2*(i*width + j)+1] = 255;
                }
            }
        }
        free(texbitmap);
        break;
    }

    return txf;

error:

    if (txf) {
        if (txf->tgi) {
            free(txf->tgi);
        }
        if (txf->tgvi) {
            free(txf->tgvi);
        }
        if (txf->lut) {
            free(txf->lut);
        }
        if (txf->teximage) {
            free(txf->teximage);
        }
        free(txf);
    }
    return NULL;
}

hwObject txfEstablishTexture(hwDisplay disp, TexFont *txf)
{
    hwObject tm;
    int sz;

    if (!txf->texobj) {
        txf->texobj = tm = hwTexture->create(hwTexture);
        if (!txf->texobj) {
            return NULL;
        }
        sz = 2 * txf->tex_width * txf->tex_height;

        HW_MODIFY_1I(tm, hwStrRows, txf->tex_height);
        HW_MODIFY_1I(tm, hwStrColumns, txf->tex_width);
        HW_MODIFY_1I(tm, hwStrComponents, 2);
        HW_MODIFY_1I(tm, hwStrType, HW_IMG_UBYTE);
        HW_MODIFY_1I(tm, hwStrApply, HW_TM_MODULATE);
        tm->modify(tm, hwStrImage, HW_MAKE_TYPE(HW_TYPE_BYTE,sz),
                   txf->teximage);
        HW_MODIFY_1I(tm, hwStrFilter, HW_TM_TRILINEAR);
    }
    return txf->texobj;
}

void txfBindFontTexture(hwDisplay disp, TexFont *txf)
{
    if (txf->texobj) {
        txf->texobj->draw(txf->texobj);
    }
}

void txfUnloadFont(TexFont * txf)
{
    if (txf->texobj) {
        txf->texobj->destroy(txf->texobj);
    }
    if (txf->teximage) {
        free(txf->teximage);
    }
    free(txf->tgi);
    free(txf->tgvi);
    free(txf->lut);
    free(txf);
}

void txfGetStringMetrics(TexFont * txf, unsigned char *string, int len, int *width,
                         int *max_ascent, int *max_descent)
{
    TexGlyphVertexInfo *tgvi;
    int w, i;

    w = 0;
    for (i = 0; i < len; i++) {
        if (string[i] == 27) {
            switch (string[i + 1]) {
            case 'M': i += 4; break;
            case 'T': i += 7; break;
            case 'L': i += 7; break;
            case 'F': i += 13; break;
            }
        } else {
            tgvi = getTCVI(txf, string[i]);
            w += tgvi->advance;
        }
    }
    *width = w;
    *max_ascent = txf->max_ascent;
    *max_descent = txf->max_descent;
}

static void glyphToQuad(hwFloat *quad, TexGlyphVertexInfo *tgvi,
                        hwFloat colors[][3])
{
    hwFloat px, py;
    hwFloat sz;

    px = txfAttribs.xPos;
    py = txfAttribs.yPos;
    sz = txfAttribs.cookedScale;

    if (colors) {
        quad[0*8 + 0] = sz * tgvi->v0[0] + px;
        quad[0*8 + 1] = sz * tgvi->v0[1] + py;
        quad[0*8 + 2] = 0.f;
        quad[0*8 + 3] = colors[0][0];
        quad[0*8 + 4] = colors[0][1];
        quad[0*8 + 5] = colors[0][2];
        quad[0*8 + 6] = tgvi->t0[0];
        quad[0*8 + 7] = tgvi->t0[1];

        quad[1*8 + 0] = sz * tgvi->v1[0] + px;
        quad[1*8 + 1] = sz * tgvi->v1[1] + py;
        quad[1*8 + 2] = 0.f;
        quad[1*8 + 3] = colors[1][0];
        quad[1*8 + 4] = colors[1][1];
        quad[1*8 + 5] = colors[1][2];
        quad[1*8 + 6] = tgvi->t1[0];
        quad[1*8 + 7] = tgvi->t1[1];

        quad[2*8 + 0] = sz * tgvi->v2[0] + px;
        quad[2*8 + 1] = sz * tgvi->v2[1] + py;
        quad[2*8 + 2] = 0.f;
        quad[2*8 + 3] = colors[2][0];
        quad[2*8 + 4] = colors[2][1];
        quad[2*8 + 5] = colors[2][2];
        quad[2*8 + 6] = tgvi->t2[0];
        quad[2*8 + 7] = tgvi->t2[1];

        quad[3*8 + 0] = sz * tgvi->v3[0] + px;
        quad[3*8 + 1] = sz * tgvi->v3[1] + py;
        quad[3*8 + 2] = 0.f;
        quad[3*8 + 3] = colors[3][0];
        quad[3*8 + 4] = colors[3][1];
        quad[3*8 + 5] = colors[3][2];
        quad[3*8 + 6] = tgvi->t3[0];
        quad[3*8 + 7] = tgvi->t3[1];
    } else {
        quad[0*5 + 0] = sz * tgvi->v0[0] + px;
        quad[0*5 + 1] = sz * tgvi->v0[1] + py;
        quad[0*5 + 2] = 0.f;
        quad[0*5 + 3] = tgvi->t0[0];
        quad[0*5 + 4] = tgvi->t0[1];

        quad[1*5 + 0] = sz * tgvi->v1[0] + px;
        quad[1*5 + 1] = sz * tgvi->v1[1] + py;
        quad[1*5 + 2] = 0.f;
        quad[1*5 + 3] = tgvi->t1[0];
        quad[1*5 + 4] = tgvi->t1[1];

        quad[2*5 + 0] = sz * tgvi->v2[0] + px;
        quad[2*5 + 1] = sz * tgvi->v2[1] + py;
        quad[2*5 + 2] = 0.f;
        quad[2*5 + 3] = tgvi->t2[0];
        quad[2*5 + 4] = tgvi->t2[1];

        quad[3*5 + 0] = sz * tgvi->v3[0] + px;
        quad[3*5 + 1] = sz * tgvi->v3[1] + py;
        quad[3*5 + 2] = 0.f;
        quad[3*5 + 3] = tgvi->t3[0];
        quad[3*5 + 4] = tgvi->t3[1];
    }
}

void txfRenderGlyph(hwDisplay disp, TexFont * txf, int c)
{
    TexGlyphVertexInfo *tgvi;
    hwFloat quad[4*5];

    tgvi = getTCVI(txf, c);

    glyphToQuad(quad, tgvi, NULL);

    disp->drawQuads(disp, quad, HW_DATA_ST, 1);

    txfAttribs.xPos += txfAttribs.cookedScale * tgvi->advance;
}

#define MAX_QUADS       32

void txfRenderString(hwDisplay disp, TexFont *txf, unsigned char *string, int len)
{
    TexGlyphVertexInfo *tgvi;
    hwFloat quads[MAX_QUADS*4*5];
    int i, n = 0;

    for (i = 0; i < len; i++) {
        tgvi = getTCVI(txf, string[i]);
        glyphToQuad(quads+n*4*5, tgvi, NULL);
        txfAttribs.xPos += txfAttribs.cookedScale * tgvi->advance;
        n++;

        if (n >= MAX_QUADS) {
            disp->drawQuads(disp, quads, HW_DATA_ST, n);
            n = 0;
        }
    }

    if (n) {
        disp->drawQuads(disp, quads, HW_DATA_ST, n);
    }
}

enum {
    MONO, TOP_BOTTOM, LEFT_RIGHT, FOUR
};

void txfRenderFancyString(hwDisplay disp, TexFont *txf, unsigned char *string, int len)
{
    TexGlyphVertexInfo *tgvi;
    hwFloat c[4][3];
    int i, n = 0, t;
    hwFloat quads[MAX_QUADS*4*8]; /* XYZ RGB ST */
    unsigned char *ptr;

    for (i = 0; i < 4; i++) {
        c[i][0] = txfAttribs.color[0];
        c[i][1] = txfAttribs.color[1];
        c[i][2] = txfAttribs.color[2];
    }

    for (i = 0; i < len; i++) {
        if (string[i] == 27) {
            ptr = (unsigned char *) string + i + 2;
            switch (string[i + 1]) {
            case 'M':
                for (t = 0; t < 3; t++) {
                    c[0][t] = ptr[t] * (1.f / 255.f);
                    c[1][t] = ptr[t] * (1.f / 255.f);
                    c[2][t] = ptr[t] * (1.f / 255.f);
                    c[3][t] = ptr[t] * (1.f / 255.f);
                }
                i += 4;
                break;
            case 'T':
                for (t = 0; t < 3; t++) {
                    c[0][t] = ptr[t  ] * (1.f / 255.f);
                    c[1][t] = ptr[t  ] * (1.f / 255.f);
                    c[2][t] = ptr[t+3] * (1.f / 255.f);
                    c[3][t] = ptr[t+3] * (1.f / 255.f);
                }
                i += 7;
                break;
            case 'L':
                for (t = 0; t < 3; t++) {
                    c[0][t] = ptr[t  ] * (1.f / 255.f);
                    c[1][t] = ptr[t+3] * (1.f / 255.f);
                    c[2][t] = ptr[t  ] * (1.f / 255.f);
                    c[3][t] = ptr[t+3] * (1.f / 255.f);
                }
                i += 7;
                break;
            case 'F':
                for (t = 0; t < 3; t++) {
                    c[0][t] = ptr[t  ] * (1.f / 255.f);
                    c[1][t] = ptr[t+3] * (1.f / 255.f);
                    c[2][t] = ptr[t+6] * (1.f / 255.f);
                    c[3][t] = ptr[t+9] * (1.f / 255.f);
                }
                i += 13;
                break;
            }
        } else {
            tgvi = getTCVI(txf, string[i]);
            glyphToQuad(quads+n*4*8, tgvi, c);
            txfAttribs.xPos += txfAttribs.cookedScale * tgvi->advance;
            n++;

            if (n >= MAX_QUADS) {
                disp->drawQuads(disp, quads, HW_DATA_RGB | HW_DATA_ST, n);
                n = 0;
            }
        }
    }
    if (n) {
        disp->drawQuads(disp, quads, HW_DATA_RGB | HW_DATA_ST, n);
    }
}

int txfInFont(TexFont * txf, int c)
{
    TexGlyphVertexInfo *tgvi;

    /* NOTE: No uppercase/lowercase substituion. */
    if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
        if (txf->lut[c - txf->min_glyph]) {
            return 1;
        }
    }
    return 0;
}

#include "fixed8.c"
#include "fixed12.c"
#include "fixed18.c"
#include "fixed24.c"

TexFont
    *__hwFixed8, *__hwFixed12, *__hwFixed18, *__hwFixed24;

typedef struct {
    unsigned char *ptr;
    size_t numBytes;
} TexStream;

size_t txfStreamRead(void *data, size_t size, size_t nelems, void *stream)
{
    TexStream *texStream = stream;
    size_t bytes, result;

    bytes = size * nelems;
    if (bytes > texStream->numBytes) {
        bytes = texStream->numBytes;
    }

    result = bytes / size;
    bytes = result * size;

    (void)memcpy(data, texStream->ptr, bytes);
    texStream->ptr += bytes;
    texStream->numBytes -= bytes;

    return result;
}

TexFont *txfLoadStaticFont(int size)
{
    TexStream texStream;

    if (!__hwFixed8) {
        texStream.ptr = __hwFixedData8;
        texStream.numBytes = __hwFixedData8_SIZE;
        __hwFixed8 = txfLoadFont(txfStreamRead, &texStream);

        texStream.ptr = __hwFixedData12;
        texStream.numBytes = __hwFixedData12_SIZE;
        __hwFixed12 = txfLoadFont(txfStreamRead, &texStream);

        texStream.ptr = __hwFixedData18;
        texStream.numBytes = __hwFixedData18_SIZE;
        __hwFixed18 = txfLoadFont(txfStreamRead, &texStream);

        texStream.ptr = __hwFixedData24;
        texStream.numBytes = __hwFixedData24_SIZE;
        __hwFixed24 = txfLoadFont(txfStreamRead, &texStream);
    }

    if (size < 12) {
        return __hwFixed8;
    } else if (size < 18) {
        return __hwFixed12;
    } else if (size < 24) {
        return __hwFixed18;
    } else {
        return __hwFixed24;
    }
}

void txfResetStaticFonts( void )
{
    if( !__hwFixed8 ) return;

    if( __hwFixed8->texobj ) {
        HW_MODIFY_1I(__hwFixed8->texobj, hwStrDirty, 1);
    }
    if( __hwFixed12->texobj ) {
        HW_MODIFY_1I(__hwFixed12->texobj, hwStrDirty, 1);
    }
    if( __hwFixed18->texobj ) {
        HW_MODIFY_1I(__hwFixed18->texobj, hwStrDirty, 1);
    }
    if( __hwFixed24->texobj ) {
        HW_MODIFY_1I(__hwFixed24->texobj, hwStrDirty, 1);
    }
}
