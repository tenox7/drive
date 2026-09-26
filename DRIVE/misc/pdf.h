#define PDF_PAINT_STROKE      0x1
#define PDF_PAINT_FILL        0x2

void *pdfInit(const char *filename, int pageWidth, int pageHeight);
int pdfNextPage(void *pPdf);
int pdfFinish(void *pPdf);

int pdfRgbColor(void *pPdf, int paintType, float r, float g, float b);
int pdfLineWidth(void *pPdf, float w);
int pdfLineDash(void *pPdf, int nDash, float *dash, float phase);
int pdfConcatMatrix(void *pPdf, float a, float b,
                                float c, float d,
                                float e, float f);

int pdfSaveState(void *pPdf);
int pdfRestoreState(void *pPdf);

int pdfMoveTo(void *pPdf, float x, float y);
int pdfLineTo(void *pPdf, float x, float y);
int pdfCurveTo(void *pPdf, float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);
int pdfClosePath(void *pPdf);
int pdfPaintPath(void *pPdf, int paintType);

int pdfBeginText(void *pPdf);
int pdfTextSize(void *pPdf, float size);
int pdfNewLine(void *pPdf, float dx, float dy);
int pdfDrawText(void *pPdf, const char *txt);
int pdfEndText(void *pPdf);

// EOF
