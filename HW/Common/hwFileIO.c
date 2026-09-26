#include <stdio.h>

#if defined(ANDROID_NDK) /* [ */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <android/log.h>
#include <android/asset_manager_jni.h>

#define ANDROID_LOG(a)  __android_log_print a

/* A consumer of this module must initialize these variables */
AAssetManager *hwAssetManager;
char *hwSdcardDir;

struct AssetStruct {
    AAsset *asset;
    FILE *sysFile;
    int lastChar;
};


void *hwFopen(const char *filename, const char *mode)
{
    AAsset *asset = NULL;
    FILE *f = NULL;
    struct AssetStruct *as;
    char sdcardPath[1024];

    if (*mode != 'r') return NULL;      // Only support read access

    if (hwSdcardDir) {
        sprintf(sdcardPath, "/sdcard/%s/%s", hwSdcardDir, filename);
        filename = sdcardPath;
    }

    f = fopen(filename, mode);

    if (!f) {
        if (hwAssetManager) {
            asset = AAssetManager_open(hwAssetManager,
                                       filename, AASSET_MODE_STREAMING);
        }
        if (!asset) {
            ANDROID_LOG((ANDROID_LOG_INFO, "HW", "Can't open asset %s\n", filename));
            return NULL;
        }
        ANDROID_LOG((ANDROID_LOG_INFO, "HW", "Found asset %s\n", filename));
    }
    else {
        ANDROID_LOG((ANDROID_LOG_INFO, "HW", "Found file %s\n", filename));
    }

    as = malloc(sizeof(struct AssetStruct));
    if (!as) {
        if (asset) AAsset_close(asset);
        if (f) fclose(f);
        return NULL;
    }

    as->asset = asset;
    as->sysFile = f;
    as->lastChar = EOF;

    return as;
}

void hwFclose(void *f)
{
    struct AssetStruct *as = f;

    if (as->asset) AAsset_close(as->asset);
    if (as->sysFile) fclose(as->sysFile);
    free(as);
}

int hwFread(void *ptr, int size, int nitems, void *f)
{
    struct AssetStruct *as = f;
    int n, remain, total;
    char *cptr;

    if (as->sysFile) {
        return fread(ptr, size, nitems, as->sysFile);
    }

    remain = nitems*size;
    total = 0;
    cptr = ptr;

    do {
        n = AAsset_read(as->asset, cptr, remain);
        if (n > 0) {
            remain -= n;
            total += n;
            cptr += n;
        }
    } while ((n > 0) && (remain > 0));

    return total / size;
}

int hwFwrite(void *ptr, int size, int nitems, void *f)
{
    return 0;
}

int hwFgetc(void *f)
{
    struct AssetStruct *as = f;
    int n;
    unsigned char c;

    if (as->sysFile) {
        return getc(as->sysFile);
    }

    if (as->lastChar >= 0) {
        c = as->lastChar;
        as->lastChar = EOF;
        return c;
    }

    n = AAsset_read(as->asset, &c, 1);
    if (n != 1) return EOF;

    return c;
}

int hwFungetc(int c, void *f)
{
    struct AssetStruct *as = f;

    if (as->sysFile) {
        return ungetc(c, as->sysFile);
    }

    as->lastChar = c;
    return c;
}

int hwFscanf(void *f, char *fmt, void *ptr)
{
    struct AssetStruct *as = f;
    int c;
    char *s;
    int n;

    if (as->sysFile) {
        return fscanf(as->sysFile, fmt, ptr);
    }

    if (strcmp(fmt, "%s") == 0) {
        do {
            c = hwFgetc(f);
        } while (isspace(c) && (c != EOF));
        s = ptr;
        while (!(isspace(c) || (c == EOF))) {
            *s++ = c;
            c = hwFgetc(f);
        }
        *s = 0;
        hwFungetc(c, f);
        return 1;
    }
    else if (strcmp(fmt, "%d") == 0) {
        do {
            c = hwFgetc(f);
        } while (isspace(c) && (c != EOF));

        n = 0;
        while (isdigit(c)) {
            n = 10*n + (c - '0');
            c = hwFgetc(f);
        }
        hwFungetc(c, f);
        *(int *)ptr = n;
        return 1;
    }
    else {
        return 0;
    }
}

char *hwFgets(char *str, int size, void *f)
{
    struct AssetStruct *as = f;
    int c, i;
    char *ptr;

    if (as->sysFile) {
        return fgets(str, size, as->sysFile);
    }

    size--;
    ptr = str;

    for (i = 0; i < size; i++) {
        c = hwFgetc(f);
        if (c != EOF) {
            *ptr++ = c;
        }
        if ((c == '\n') || (c == EOF)) {
            break;
        }
    }

    if (c == EOF) return NULL;

    *ptr = 0;
    return str;
}

int hwFseek(void *f, long offs, int whence)
{
    struct AssetStruct *as = f;
    if (as->sysFile) {
        return fseek(as->sysFile, offs, whence);
    }
    return EOF;
}

long hwFtell(void *f)
{
    struct AssetStruct *as = f;
    if (as->sysFile) {
        return ftell(as->sysFile);
    }
    return (long)EOF;
}

void hwFrewind(void *f)
{
    struct AssetStruct *as = f;

    if (as->sysFile) {
        rewind(as->sysFile);
        return;
    }

    AAsset_seek(as->asset, 0, SEEK_SET);
}

#else /* ] [ */

void *hwFopen(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

void hwFclose(void *f)
{
    fclose(f);
}

int hwFread(void *ptr, int size, int nitems, void *f)
{
    return fread(ptr, size, nitems, f);
}

int hwFwrite(void *ptr, int size, int nitems, void *f)
{
    return fwrite(ptr, size, nitems, f);
}

int hwFgetc(void *f)
{
    return fgetc(f);
}

int hwFungetc(int c, void *f)
{
    return ungetc(c, f);
}

int hwFscanf(void *f, char *fmt, void *ptr)
{
    return fscanf(f, fmt, ptr);
}

char *hwFgets(char *str, int size, void *f)
{
    return fgets(str, size, f);
}

int hwFseek(void *f, long offs, int whence)
{
    return fseek(f, offs, whence);
}

long hwFtell(void *f)
{
    return ftell(f);
}

void hwFrewind(void *f)
{
    rewind(f);
}

#endif /* ] */
