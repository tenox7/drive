// The overall file header
static const char *fileHeader[] = {
    "%PDF-1.0",
    NULL
};

// First object is the Catalog
#define OBJ_CATALOG     1
static const char *objCatalog[] = {
    "1 0 obj <<",
    " /Type /Catalog",
    " /Pages 3 0 R",
    " /Outlines 2 0 R",
    ">> endobj",
    NULL
};

// Second object is the (empty) Outlines
#define OBJ_OUTLINES    2
static const char *objOutlines[] = {
    "2 0 obj <<",
    " /Type /Outlines",
    " /Count 0",
    ">> endobj",
    NULL
};

// Third object is the Pages - it refers to two objects,
// object 4 (the page count) and object 5 (the kids),
// which occur at the very end of the file
#define OBJ_PAGES       3
#define OBJ_PAGE_COUNT  4
#define OBJ_PAGE_KIDS   5
#define OBJ_FONT        6
static const char *objPages[] = {
    "3 0 obj <<",
    " /Type /Pages",
    " /Count 4 0 R",
    " /Kids 5 0 R",
    ">> endobj",
    NULL
};

// The page count object
static const char *objPageCount[] = {
    "4 0 obj",
    " %d\n",
    "endobj",
    NULL
};

// The page kids object prolog + epilog - these go at the end of the file
static const char *objPageKidsProlog[] = {
    "5 0 obj [",
    NULL
};
static const char *objPageKidsEpilog[] = {
    "] endobj",
    NULL
};

// The Font object
const char *objFont[] = {
    "6 0 obj <<",
    " /Type /Font",
    " /Subtype /Type1",
    " /Name /F1",
    " /BaseFont /Times-Bold",
    " /Encoding /WinAnsiEncoding",
    ">> endobj",
    NULL,
};

#define NUM_STATIC_OBJECTS      7

// xref goes between fileEpilog0 and fileEpilog1
// The first object on each page is a Page object.  It
// refers to the Font (the last object), and the next
// object, which is a stream
static const char *objPage[] = {
    "%d 0 obj <<\n",
    " /Type /Page",
    " /Parent 3 0 R",
    " /Resources <<",
    "  /Font << /F1 6 0 R >>",        // Use of OBJ_FONT
    "  /ProcSet [ /PDF /Text ]",
    " >>",
    " /MediaBox [ 0 0 %d %d ]\n",
    " /Contents %d 0 R\n",
    ">> endobj",
    NULL
};

// The second object on each page is a stream object.  It
// refers to the count, which is the next object.
static const char *objStream[] = {
    "%d 0 obj\n",
    "<< /Length %d 0 R >>\n",
    "stream",
    NULL,
};

static const char *endObjStream[] = {
    "endstream",
    "endobj",
    NULL
};
// The third object on the page is the stream count object
static const char *objStreamCount[] = {
    "%d 0 obj\n",
    "%d\n",
    "endobj",
    NULL
};

// The trailer refers both to the number of objects (which is
// the Size parameter) as well as the location of the xref
// table in the file.
static const char *fileEpilog[] = {
    "trailer",
    "<<",
    "/Size %d\n",
    "/Root 1 0 R",
    ">>",
    "startxref",
    "%d\n",
    "%%EOF",
    NULL
};
