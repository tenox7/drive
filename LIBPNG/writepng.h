#ifndef PNG_IO_FUNCS
typedef int (*PngReadFunc)(void *, size_t, size_t, void *);
typedef int (*PngWriteFunc)(void *, size_t, size_t, void *);
typedef int (*PngSeekFunc)(void *, long, int);
typedef long (*PngTellFunc)(void *);
#define PNG_IO_FUNCS    1
#endif

typedef struct {
    PngWriteFunc write;
    PngSeekFunc seek;
    PngTellFunc tell;
    void *f;
    int offs, size;
    unsigned char *ptr;
} PngOutStruct;

#define PNG_FLAG_SKIP_ALPHA     1
#define PNG_FLAG_GAMMA          2

int WritePNG(PngOutStruct *io, int flags,
            int w, int h, int comps, int bpp, void *img,
            unsigned char *pall);

int PngInitStdOut(PngOutStruct *io, char *filename);
void PngTermStdOut(PngOutStruct *io);
int PngInitMemOut(PngOutStruct *io, unsigned char *data, int dataSize);
