#include <stdio.h>
#include <stdlib.h>

/* Structure of a fnt file:
 * First line:
 *   common lineHeight=N base=N scaleW=N scaleH=N pages=1
 * Per char:
 *   char id=N x=N y=N width=N height=N xoffset=N yoffset=N xadvance=N  page=N
 */

struct TgaHeader {
    unsigned char idLength;
    unsigned char cmapType;
    unsigned char imageType;
    unsigned char cmapSpec[5];
    unsigned short xOrigin;
    unsigned short yOrigin;
    unsigned short width;
    unsigned short height;
    unsigned char depth;
    unsigned char descriptor;
};

struct FntHeader {
    int lineHeight;
    int base;
    int scaleW;
    int scaleH;
    int pages;
};

struct CharDescriptor {
    int charId;
    int charX, charY;
    int charW, charH;
    int charXoffs, charYoffs;
    int charAdvance;
    int charPage;
};

struct TexFontHdr {
    char magic[4];
    int swap;
    int format;
    int texWidth;
    int texHeight;
    int maxAscent;
    int maxDescent;
    int numGlyphs;
};

struct TexGlyphInfo {
    unsigned short c;
    unsigned char width;
    unsigned char height;
    signed char xoffset;
    signed char yoffset;
    signed char advance;
    char dummy;
    short x;
    short y;
};

int readHeader(FILE *f, struct FntHeader *hdr)
{
    int n;

    n = fscanf(f, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d\n",
                &hdr->lineHeight, &hdr->base, &hdr->scaleW,
                &hdr->scaleH, &hdr->pages);
    if (n != 5) {
        return -1;
    }
    return 1;
}

int readChar(FILE *f, struct CharDescriptor *ch)
{
    int n;

    n = fscanf(f, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d "
                  "yoffset=%d xadvance=%d page=%d\n",
                  &ch->charId, &ch->charX, &ch->charY, &ch->charW, &ch->charH,
                  &ch->charXoffs, &ch->charYoffs,
                  &ch->charAdvance, &ch->charPage);
    if (n != 9) {
        if (feof(f)) {
            return 0;
        } else {
            return -1;
        }
    }
    return 1;
}

int readTgaHeader(FILE *f, struct TgaHeader *hdr)
{
    if (fread(hdr, sizeof(*hdr), 1, f) != 1) {
        return -1;
    }
    if (hdr->idLength) return 0;
    if (hdr->cmapType) return 0;
    if (hdr->imageType != 3) return 0;
    if (hdr->depth != 8) return 0;
    return 1;
}

int writeTxfHeader(FILE *f, struct FntHeader *hdr, int numGlyphs)
{
    struct TexFontHdr txf;

    txf.magic[0] = '\377';
    txf.magic[1] = 't';
    txf.magic[2] = 'x';
    txf.magic[3] = 'f';
    txf.swap = 0x12345678;
    txf.format = 0;
    txf.texWidth = hdr->scaleW;
    txf.texHeight = hdr->scaleH;
    txf.maxAscent = hdr->base;
    txf.maxDescent = hdr->lineHeight - hdr->base;
    txf.numGlyphs = numGlyphs;

    if (fwrite(&txf, sizeof(txf), 1, f) != 1) {
        return 0;
    }

    return 1;
}

int writeTxfGlyph(FILE *f, struct FntHeader *hdr, struct CharDescriptor *ch)
{
    struct TexGlyphInfo txi;

    txi.c = ch->charId;
    txi.width = ch->charW;
    txi.height = ch->charH;
    txi.xoffset = ch->charXoffs;
    txi.yoffset = hdr->lineHeight - (ch->charYoffs + ch->charH);
    txi.advance = ch->charAdvance;
    txi.x = ch->charX;
    txi.y = hdr->scaleH - (ch->charY + ch->charH);

    if (fwrite(&txi, sizeof(txi), 1, f) != 1) {
        return 0;
    }

    return 1;
}

main(int argc, char **argv)
{
    FILE *f;
    FILE *out;
    char fname[1024];
    int ok;
    struct FntHeader hdr;
    struct CharDescriptor ch[256];
    int i, numCh;
    struct TgaHeader tga;
    unsigned char *data, *ptr;

    /* Open font description input and read header */
    sprintf(fname, "%s.fnt", argv[1]);
    f = fopen(fname, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file %s\n", fname);
        exit(1);
    }

    if (readHeader(f, &hdr) < 0) {
        fprintf(stderr, "File read error\n");
        exit(1);
    }

    /* Read all glyphs */
    numCh = 0;
    do {
        ok = readChar(f, ch + numCh);
        if (ok < 0) {
            fprintf(stderr, "File read error\n");
            exit(1);
        } else if (ok) {
            numCh++;
        }
    } while (ok > 0);
    (void)fclose(f);

    /* Read the TGA header */
    sprintf(fname, "%s_00.tga", argv[1]);
    f = fopen(fname, "rb+");
    if (!f) {
        fprintf(stderr, "Cannot open file %s\n", fname);
        exit(1);
    }
    if (readTgaHeader(f, &tga) <= 0) {
        fprintf(stderr, "Error reading tga header\n");
        exit(1);
    }
    if ((tga.width != hdr.scaleW) || (tga.height != hdr.scaleH)) {
        fprintf(stderr, "TGA does not match FNT\n");
        exit(1);
    }

    /* Read TGA image data */
    data = malloc(tga.width * tga.height);
    if (!data) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    if (fread(data, tga.width*tga.height, 1, f) != 1) {
        fprintf(stderr, "Error reading tga data\n");
        exit(1);
    }
    (void)fclose(f);

    /* Open output and write output header */
    sprintf(fname, "%s.txf", argv[1]);
    out = fopen(fname, "wb");
    if (!out) {
        fprintf(stderr, "Error writing %s\n", fname);
        exit(1);
    }
    if (!writeTxfHeader(out, &hdr, numCh)) {
        fprintf(stderr, "File write error\n");
        exit(1);
    }

    /* Write all glyphs */
    for (i = 0; i < numCh; i++) {
        if (!writeTxfGlyph(out, &hdr, ch + i)) {
            fprintf(stderr, "File write error\n");
            exit(1);
        }
    }

    /* Write image data (upside-down) */
    for (i = 0, ptr = data + (tga.width*tga.height); i < tga.height; i++) {
        ptr -= tga.width;
        if (fwrite(ptr, tga.width, 1, out) != 1) {
            fprintf(stderr, "Error writing tga data\n");
            exit(1);
        }
    }
    (void)fclose(out);

    /* All done! */
    return 0;
}
