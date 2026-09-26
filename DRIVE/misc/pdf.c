#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "pdf.h"
#include "pdfstrings.h"

typedef struct {
    char *filename;
    FILE *output;

    int objNum;
    int allocObjects;
    long *objOffsets;

    long pageStart;

    int *pages;
    int numPages;
    int allocPages;

    int pageObj;
    int streamObj;
    int endStreamObj;

    int stackDepth;
    int inText;

    int pageWidth, pageHeight;
} PDF;

static long putLines(PDF *pdf, const char *lines[], ...);
static int allocObject(PDF *pdf);
static int addPage(PDF *pdf);

void *pdfInit(const char *filename, int width, int height)
{
    PDF *pdf = NULL;

    pdf = malloc(sizeof(PDF));
    if (!pdf) {
        return NULL;
    }

    pdf->filename = NULL;
    pdf->objOffsets = NULL;
    pdf->pages = NULL;
    pdf->output = NULL;
    pdf->stackDepth = 0;
    pdf->inText = 0;
    pdf->pageWidth = width;
    pdf->pageHeight = height;

    pdf->filename = malloc(strlen(filename)+1);
    if (!pdf->filename) {
        goto error;
    }
    (void)strcpy(pdf->filename, filename);

    pdf->objNum = 0;
    pdf->allocObjects = 128;
    pdf->objOffsets = malloc(pdf->allocObjects*sizeof(long));
    if (!pdf->objOffsets) {
        goto error;
    }

    pdf->numPages = 0;
    pdf->allocPages = 16;
    pdf->pages = malloc(pdf->allocPages*sizeof(int));
    if (!pdf->pages) {
        goto error;
    }

    pdf->output = fopen(filename, "wb");
    if (!pdf->output) {
        goto error;
    }

    // Object 0 is the NULL object
    pdf->objOffsets[0] = 0;

    // Put out the main file header
    (void) putLines(pdf, fileHeader);

    // Object 1 is the Catalog
    pdf->objOffsets[OBJ_CATALOG] = putLines(pdf, objCatalog);

    // Object 2 is the Outlines (there are 0 of them)
    pdf->objOffsets[OBJ_OUTLINES] = putLines(pdf, objOutlines);

    // Object 3 is the Pages.
    pdf->objOffsets[OBJ_PAGES] = putLines(pdf, objPages);

    // Object 6 is the Font
    pdf->objOffsets[OBJ_FONT] = putLines(pdf, objFont);

    // All done with the static header objects.
    pdf->objNum = NUM_STATIC_OBJECTS;

    // Put out the header for the first page
    pdf->pageObj = allocObject(pdf);
    pdf->streamObj = allocObject(pdf);
    pdf->endStreamObj = allocObject(pdf);

    pdf->objOffsets[pdf->pageObj] = putLines(pdf, objPage,
                                             pdf->pageObj,
                                             pdf->pageWidth,
                                             pdf->pageHeight,
                                             pdf->streamObj);

    pdf->objOffsets[pdf->streamObj] = putLines(pdf, objStream,
                                               pdf->streamObj,
                                               pdf->endStreamObj);

    if (addPage(pdf) < 0) {
        goto error;
    }

    // All done with init - ready to write the first page
    return pdf;

error:
    if (pdf->pages) free(pdf->pages);
    if (pdf->objOffsets) free(pdf->objOffsets);
    if (pdf->filename) free(pdf->filename);
    if (pdf->output) fclose(pdf->output);
    free(pdf);
    return NULL;
}

int pdfNextPage(void *pPdf)
{
    PDF *pdf = pPdf;
    long pageSize;

    if (!pdf)   return -1;

    // Close the previous page.  Gotta figure out
    // how many bytes are in the stream, so that we can...
    pageSize = ftell(pdf->output) - pdf->pageStart;
    (void) putLines(pdf, endObjStream);

    // ... put out the stream count object, third on the page
    pdf->objOffsets[pdf->endStreamObj] = putLines(pdf, objStreamCount,
                                                  pdf->endStreamObj,
                                                  (int) pageSize);

    pdf->pageObj = allocObject(pdf);
    pdf->streamObj = allocObject(pdf);
    pdf->endStreamObj = allocObject(pdf);

    pdf->objOffsets[pdf->pageObj] = putLines(pdf, objPage,
                                             pdf->pageObj,
                                             pdf->pageWidth,
                                             pdf->pageHeight,
                                             pdf->streamObj);

    pdf->objOffsets[pdf->streamObj] = putLines(pdf, objStream,
                                               pdf->streamObj,
                                               pdf->endStreamObj);

    return addPage(pdf);
}

int pdfFinish(void *pPdf)
{
    PDF *pdf = pPdf;
    long pageSize;
    long xrefOffset;
    int i;

    if (!pdf)   return -1;

    // Close the final page.  Gotta figure out
    // how many bytes are in the stream, so that we can...
    pageSize = ftell(pdf->output) - pdf->pageStart;
    (void) putLines(pdf, endObjStream);

    // ... put out the stream count object, third on the page
    pdf->objOffsets[pdf->endStreamObj] = putLines(pdf, objStreamCount,
                                                  pdf->endStreamObj,
                                                  (int) pageSize);

    // Finally, put out all the pages.
    pdf->objOffsets[OBJ_PAGE_COUNT] = putLines(pdf, objPageCount,
                                               pdf->numPages);
    pdf->objOffsets[OBJ_PAGE_KIDS] = putLines(pdf, objPageKidsProlog);
    for (i = 0; i < pdf->numPages; i++) {
        (void) fprintf(pdf->output, " %d 0 R\n", pdf->pages[i]);
    }
    pdf->objOffsets[OBJ_PAGE_KIDS] = putLines(pdf, objPageKidsEpilog);

    // Here is the xref.  We cleverly kept track of the
    // byte offset of every object, so we can print them
    // out here
    xrefOffset = ftell(pdf->output);
    (void) fprintf(pdf->output, "xref\n");
    (void) fprintf(pdf->output, "0 %d\n0000000000 65535 f\n", pdf->objNum);
    for (i = 1; i < pdf->objNum; i++) {
        (void) fprintf(pdf->output, "%010d 00000 n\n", pdf->objOffsets[i]);
    }

    // Put the final epilog, which has the # of objects and
    // the position of the xref
    (void) putLines(pdf, fileEpilog,
                    pdf->objNum, (int)xrefOffset);

    free(pdf->pages);
    free(pdf->objOffsets);
    free(pdf->filename);
    fclose(pdf->output);
    free(pdf);

    return 0;
}

// Styles
int pdfRgbColor(void *pPdf, int paintType, float r, float g, float b)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;

    if (paintType & PDF_PAINT_STROKE) {
        (void) fprintf(pdf->output, "%6.4f %6.4f %6.4f RG\n", r, g, b);
    }
    if (paintType & PDF_PAINT_FILL) {
        (void) fprintf(pdf->output, "%6.4f %6.4f %6.4f rg\n", r, g, b);
    }
    return 0;
}

int pdfLineWidth(void *pPdf, float w)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;

    (void) fprintf(pdf->output, "%14.6f w\n", w);
    return 0;
}

int pdfLineDash(void *pPdf, int nDash, float *dash, float phase)
{
    PDF *pdf = pPdf;
    int i;

    if (!pdf) return -1;

    (void) fprintf(pdf->output, "[ ");
    for (i = 0; i < nDash; i++) {
        (void) fprintf(pdf->output, "%8.4f ", dash[i]);
    }
    (void) fprintf(pdf->output, "] %8.4f d\n", phase);
    return 0;
}

int pdfConcatMatrix(void *pPdf, float a, float b,
                                float c, float d,
                                float e, float f)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;

    (void) fprintf(pdf->output, "%14.6f %14.6f %14.6f %14.6f %14.6f %14.6f cm\n", a, b, c, d, e, f);
    return 0;
}

// State operations
int pdfSaveState(void *pPdf)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;

    (void) fprintf(pdf->output, "q\n");
    pdf->stackDepth++;
    return 0;
}

int pdfRestoreState(void *pPdf)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (!pdf->stackDepth) return -1;

    (void) fprintf(pdf->output, "Q\n");
    pdf->stackDepth--;
    return 0;
}

// Path drawing operations
int pdfMoveTo(void *pPdf, float x, float y)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    (void) fprintf(pdf->output, "%14.6f %14.6f m\n", x, y);
    return 0;
}

int pdfLineTo(void *pPdf, float x, float y)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    (void) fprintf(pdf->output, "%14.6f %14.6f l\n", x, y);
    return 0;
}

int pdfCurveTo(void *pPdf, float x0, float y0,
                           float x1, float y1,
                           float x2, float y2)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    (void) fprintf(pdf->output, "%14.6f %14.6f %14.6f %14.6f %14.6f %14.6f c\n",
                                x0, y0, x1, y1, x2, y2);
    return 0;
}

int pdfClosePath(void *pPdf)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    (void) fprintf(pdf->output, "h\n");
    return 0;
}

int pdfPaintPath(void *pPdf, int paintType)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    switch (paintType) {
    case PDF_PAINT_STROKE :
        (void) fprintf(pdf->output, "S\n");
        break;
    case PDF_PAINT_FILL :
        (void) fprintf(pdf->output, "f\n");
        break;
    case PDF_PAINT_STROKE | PDF_PAINT_FILL :
        (void) fprintf(pdf->output, "B\n");
        break;
    default :
        (void) fprintf(pdf->output, "n\n");
        break;
    }
    return 0;
}

// Text operations
int pdfBeginText(void *pPdf)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (pdf->inText) return -1;

    (void) fprintf(pdf->output, "BT\n");
    pdf->inText = 1;
    return 0;
}

int pdfTextSize(void *pPdf, float size)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (!pdf->inText) return -1;

    (void) fprintf(pdf->output, "/F1 %g Tf\n", size);
    return 0;
}

int pdfNewLine(void *pPdf, float dx, float dy)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (!pdf->inText) return -1;

    (void) fprintf(pdf->output, "%g %g Td\n", dx, dy);
    return 0;
}

int pdfDrawText(void *pPdf, const char *txt)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (!pdf->inText) return -1;

    (void) fputc('(', pdf->output);
    for (; *txt; txt++) {
        switch (*txt) {
        case '(' :
        case ')' :
        case '\\' :
        case '\n' :
            (void) fputc('\\', pdf->output);
        default :
            (void) fputc(*txt, pdf->output);
            break;
        }
    }
    (void) fprintf(pdf->output, ") Tj\n");
    return 0;
}

int pdfEndText(void *pPdf)
{
    PDF *pdf = pPdf;

    if (!pdf) return -1;
    if (!pdf->inText) return -1;

    (void) fprintf(pdf->output, "ET\n");
    pdf->inText = 0;
    return 0;
}

static long putLines(PDF *pdf, const char *lines[], ...)
{
    va_list va;
    long result;
    int i, j;
    char *fmt, *fmt1;

    result = ftell(pdf->output);

    va_start(va, lines);
    while (*lines) {
        fmt = strchr(*lines, '%');
        if (fmt && (strncmp(fmt, "%d", 2) == 0)) {
            fmt1 = strchr(fmt+1, '%');
            if (fmt1 && (strncmp(fmt1, "%d", 2) == 0)) {
                i = va_arg(va, int);
                j = va_arg(va, int);
                fprintf(pdf->output, *lines, i, j);
            } else {
                i = va_arg(va, int);
                fprintf(pdf->output, *lines, i);
            }
        } else {
            fprintf(pdf->output, "%s\n", *lines);
        }
        lines++;
    }
    va_end(va);

    return result;
}

static int allocObject(PDF *pdf)
{
    if (pdf->objNum >= pdf->allocObjects) {
        pdf->allocObjects *= 2;
        pdf->objOffsets = realloc(pdf->objOffsets,
                                  pdf->allocObjects*sizeof(long));
        if (!pdf->objOffsets) {
            return -1;
        }
    }
    return pdf->objNum++;
}

static int addPage(PDF *pdf)
{
    if (pdf->numPages >= pdf->allocPages) {
        pdf->allocPages *= 2;
        pdf->pages = realloc(pdf->pages, pdf->allocPages*sizeof(int));
        if (!pdf->pages) {
            return -1;
        }
    }

    pdf->pages[pdf->numPages] = pdf->pageObj;

    pdf->pageStart = ftell(pdf->output);

    return pdf->numPages++;
}

// EOF
