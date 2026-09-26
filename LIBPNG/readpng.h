#ifndef PNG_IO_FUNCS
typedef int (*PngReadFunc)(void *, size_t, size_t, void *);
typedef int (*PngWriteFunc)(void *, size_t, size_t, void *);
typedef int (*PngSeekFunc)(void *, long, int);
typedef long (*PngTellFunc)(void *);
#define PNG_IO_FUNCS 1
#endif

typedef struct {
    PngReadFunc read;
    PngSeekFunc seek;
    PngTellFunc tell;
    void *f;
    int offs, size;
    unsigned char *ptr;
} PngInStruct;

#define PNG_FLIP_Y      0x00000001
#define PNG_READ_16     0x00000002

int ReadPNG(PngInStruct *io, int flags,
            int *w, int *h, int *comps, unsigned char **img,
            unsigned char **pPall);

int PngInitStdIn(PngInStruct *io, char *filename);
void PngTermStdIn(PngInStruct *io);
int PngInitMemIn(PngInStruct *io, unsigned char *data, int dataSize);
