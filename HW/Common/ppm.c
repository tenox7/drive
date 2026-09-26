/* (c) Copyright Hewlett-Packard Company 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hw.h"
#include "hw_internal.h"

#if defined(ANDROID_NDK)
#  define ANDROID_LOG(a) __android_log_print a
#else
#  define ANDROID_LOG(a)
#endif

#define HT_SIZE         512
static hwImageStruct
    *hashTable[HT_SIZE];

static unsigned long hash( char *s )
{
    unsigned long
        Res, t;

    Res = 0;
    for( /* NO INIT */; *s; s++ ) {
        Res = (Res << 4) + *s;
        t = Res & 0xF0000000;
        if( t ) {
            Res ^= (t >> 24);
            Res ^= t;
        }
    }
    return Res % HT_SIZE;
}

hwImageStruct *__hwIntCreateImage( void )
{
    hwImageStruct
        *result;

    result = malloc( sizeof(struct __hwImageStruct) );
    if( !result ) return 0;
    result->width = 0;
    result->height = 0;
    result->depth = 0;
    result->components = 0;
    result->type = 0;
    result->refCount = 1;       /* Exactly 1 of me */
    result->tmId = -1;          /* No texture ever created from me */
    result->fileName = 0;       /* Anonymous */
    result->data = 0;           /* With no data */
    result->next = 0;           /* And not in the cache */
    return result;
}

void __hwIntDestroyImage( hwImageStruct *img )
{
    img->refCount--;
    if( img->refCount <= 0 ) {
        /* Free it! */
        if( img->data ) free( img->data );
        img->data = 0;

        /* if( img->tmId ) currDisp->destroyTmId( img->tmId ); TBD */

        /* We should never have to mesh with the hash table, since
         * they always start with a ref count of 1 (for the hash
         * table itself) and should never get to 0
         */
        free( img );
    }
}

void __hwIntModifyAlpha( hwImageStruct *img, unsigned char key[3] )
{
    unsigned char
        *src, *dst;
    int
        i, n, t;

    LOG_ENTRY
    n = img->width * img->height;
    switch( img->components ) {
    case 1 :
        src = img->data;
        dst = malloc( n * 2 );
        if( !dst ) return;
        t = (key[0] + key[1] + key[2]) / 3;
        for( i = 0; i < n; i++ ) {
            dst[2*i] = src[i];
            if( src[i] <= t ) {
                dst[2*i+1] = 0;
            }
            else {
                dst[2*i+1] = 255;
            }
        }
        free( src );
        img->components = 2;
        img->data = dst;
        break;
    case 2 :
        src = img->data;
        t = (key[0] + key[1] + key[2]) / 3;
        for( i = 0; i < n; i++ ) {
            if( src[2*i] <= t ) {
                src[2*i+1] = 0;
            }
            else {
                src[2*i+1] = 255;
            }
        }
        break;
    case 3 :
        dst = malloc( n * 4 );
        if( !dst ) return;
        src = img->data;
        for( i = 0; i < n; i++ ) {
            dst[4*i  ] = src[3*i  ];
            dst[4*i+1] = src[3*i+1];
            dst[4*i+2] = src[3*i+2];
            if(        (src[3*i  ] <= key[0])
                && (src[3*i+1] <= key[1])
                && (src[3*i+2] <= key[2])
                )
            {
                dst[4*i+3] = 0;
            }
            else {
                dst[4*i+3] = 255;
            }
        }
        img->components = 4;
        img->data = dst;
        break;
    case 4 :
        src = img->data;
        for( i = 0; i < n; i++ ) {
            if(        (src[4*i  ] <= key[0])
                && (src[4*i+1] <= key[1])
                && (src[4*i+2] <= key[2])
                )
            {
                src[4*i+3] = 0;
            }
            else {
                src[4*i+3] = 255;
            }
        }
        break;
    }
    LOG_EXIT
}

#ifdef JPEGLIB /* [ */
#if defined(CYGWIN) || defined(MINGW) || defined(LINUX)
#include "jpeglib.h"
#else
#include "cdjpeg.h"             /* Common decls for cjpeg/djpeg applications */
#include "jversion.h"           /* for version message */
#endif
#endif /* ] */

hwImageStruct *__hwIntReadJPG( char *filename )
{
#ifdef JPEGLIB /* [ */
    int
        n, w, h, i, comps, num_scanlines;
    FILE
        *f;
    hwImageStruct
        *result;
    unsigned char
        *img = 0, *dst,
        *buff = 0, *src,
        **buffptr = 0;
    struct jpeg_decompress_struct
        cinfo;
    struct jpeg_error_mgr
        jerr;

    /* See if this image is already loaded */
    n = hash( filename );
    for( result = hashTable[n]; result; result = result->next ) {
        if( strcmp( result->fileName, filename ) == 0 ) {
            /* Found it! */
            result->refCount++;
            return result;
        }
    }

    /* Try to open & read the file */
    f = hwFopen( filename, "rb" );
    if( !f )    return 0;

    /* Standard jpeg init stuff */
    (void)memset( &cinfo, 0, sizeof(cinfo) );
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress( &cinfo );

    jpeg_stdio_src( &cinfo, f );
    (void)jpeg_read_header( &cinfo, TRUE );
    (void)jpeg_start_decompress( &cinfo );

    w = cinfo.output_width;
    h = cinfo.output_height;
    comps = cinfo.out_color_components;
    if( comps > 4 ) comps = 4;

    buff = malloc( w * cinfo.out_color_components * cinfo.rec_outbuf_height );
    img = malloc( w * h * comps );
    buffptr = malloc( cinfo.rec_outbuf_height * sizeof(char *) );
    if( !buff || !img || !buffptr ) goto error;

    for( i = 0; i < cinfo.rec_outbuf_height; i++ ) {
        buffptr[i] = buff + i*w*cinfo.out_color_components;
    }

    dst = img;
    while( cinfo.output_scanline < cinfo.output_height ) {
        num_scanlines = jpeg_read_scanlines( &cinfo, buffptr,
                                                cinfo.rec_outbuf_height );
        src = buff;
        switch( cinfo.out_color_components ) {
        case 1 :
            while( num_scanlines-- ) {
                for( i = 0; i < w; i++ ) {
                    *dst++ = *src++;
                }
            }
            break;
        case 2 :
            while( num_scanlines-- ) {
                for( i = 0; i < w; i++ ) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
            break;
        case 3 :
            while( num_scanlines-- ) {
                for( i = 0; i < w; i++ ) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
            break;
        case 4 :
            while( num_scanlines-- ) {
                for( i = 0; i < w; i++ ) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
            break;
        default :
            while( num_scanlines-- ) {
                for( i = 0; i < w; i++ ) {
                    *dst++ = src[0];
                    *dst++ = src[1];
                    *dst++ = src[2];
                    *dst++ = src[3];
                    src += cinfo.out_color_components;
                }
            }
            break;
        }
    }
    (void)jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );

    free( buffptr );    buffptr = 0;
    free( buff );       buff = 0;
    (void)hwFclose( f );  f = 0;

    result = __hwIntCreateImage();
    result->fileName = malloc( strlen( filename ) + 1 );
    if( !result->fileName ) return 0;
    (void)strcpy( result->fileName, filename );

    n = hash( filename );
    result->next = hashTable[n];
    hashTable[n] = result;

    result->width = w;
    result->height = h;
    result->depth = 1;
    result->components = comps;
    result->type = HW_IMG_UBYTE;
    result->data = img;

    result->refCount++;
    return result;
error:
    if( buffptr )       free( buffptr );
    if( buff )          free( buff );
    if( img )           free( img );
    if( f )             (void)hwFclose( f );
    return 0;
#else /* ] [ */
    return 0;
#endif /* ] */
}

#include "readpng.h"

hwImageStruct *__hwIntReadPNG( char *filename )
{
    unsigned char
        *img;
    int
        n, w, h, comps, res;
    PngInStruct
        pin;
    hwImageStruct
        *result;

    /* See if this image is already loaded */
    n = hash( filename );
    for( result = hashTable[n]; result; result = result->next ) {
        if( strcmp( result->fileName, filename ) == 0 ) {
            /* Found it! */
            result->refCount++;
            return result;
        }
    }

    if (!PngInitStdIn(&pin, filename)) return 0;

    res = ReadPNG(&pin, 0, &w, &h, &comps, &img, NULL);

    PngTermStdIn(&pin);

    if (res < 0) {
        ANDROID_LOG((ANDROID_LOG_INFO, "FCMOD", "ReadPNG: error %d\n", res));
        return 0;
    }

    result = __hwIntCreateImage();
    result->fileName = malloc( strlen( filename ) + 1 );
    if( !result->fileName ) return 0;
    (void)strcpy( result->fileName, filename );

    n = hash( filename );
    result->next = hashTable[n];
    hashTable[n] = result;

    result->width = w;
    result->height = h;
    result->depth = 1;
    result->components = comps;
    result->type = HW_IMG_UBYTE;
    result->data = img;

    result->refCount++;
    return result;
}

hwImageStruct *__hwIntReadTIF( char *Name )
{
    /* TBD */
    return 0;
}

hwImageStruct *__hwIntReadPPM( char *Name )
{
    int
        Type,
        WantMono, BytesPerTexel,
        r, g, b, ThreeD = 0,
        c, w, h, d, i, j, k;
    FILE
        *f;
    char
        *s;
    unsigned char
        *Img = 0,
        *Buff = 0,
        *Curr;
    hwImageStruct
        *result;

    /* See if this image is already loaded */
    i = hash( Name );
    for( result = hashTable[i]; result; result = result->next ) {
        if( strcmp( result->fileName, Name ) == 0 ) {
            /* Found it! */
            result->refCount++;
            return result;
        }
    }

    /* Try to open & read the file */
    f = hwFopen( Name, "rb" );
    if( !f )    return 0;

    /* Read magic number */
    c = hwFgetc( f );
    if( c == 'V' )      ThreeD = 1;
    else if( c != 'P' ) goto Error;

    Type = hwFgetc( f );
    switch( Type ) {
    case '1' :  /* ASCII PBM */
    case '4' :  /* Binary PBM */
    case '2' :  /* ASCII PGM */
    case '5' :  /* Binary PGM */
        WantMono = 1;
        break;
    case '3' :  /* ASCII PPM */
    case '6' :  /* Binary PPM */
        WantMono = 0;
        break;
    default :
        goto Error;
    }

    /* Skip comments */
    c = hwFgetc( f );
    while( c == '\n' ) {
        do {
            c = hwFgetc( f );
        } while( isspace( c ) );
        if( c == '#' ) {
            do {
                c = hwFgetc( f );
            } while( (c != EOF) && (c != '\n') );
        }
        else {
            (void)hwFungetc( c, f );
        }
    }

    /* Read size */
    if( ThreeD ) {
        if( hwFscanf( f, "%d", &w ) != 1 )  goto Error;
        if( hwFscanf( f, "%d", &h ) != 1 )  goto Error;
        if( hwFscanf( f, "%d", &d ) != 1 )  goto Error;
    }
    else {
        if( hwFscanf( f, "%d", &w ) != 1 ) goto Error;
        if( hwFscanf( f, "%d", &h ) != 1 ) goto Error;
        d = 1;
    }
    c = hwFgetc( f );

    /* Read max sample value. TBD: We may someday want to support
     * bigger samples...
     */
    if( hwFscanf( f, "%d", &c ) != 1 )    goto Error;
    if( c > 255 )       goto Error;

    /* Read the EOL after the max sample value */
    (void)hwFgetc( f );

    /* Read the bitmap */
    BytesPerTexel = WantMono ? 1 : 3;
    Img = malloc( w*h*d*BytesPerTexel );

    /* Allocate temporary scanline buffer */
    Buff = malloc( w * 3 );
    if( !Img || !Buff ) goto Error;

    /* Read each scanline */
    switch( Type ) {
    case '1' :  /* ASCII PBM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &g ) != 1 )        goto Error;
                        g = g ? 255 : 0;
                        Curr[BytesPerTexel*j] = g;
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &g ) != 1 )        goto Error;
                        g = g ? 255 : 0;
                        Curr[BytesPerTexel*j  ] = g;
                        Curr[BytesPerTexel*j+1] = g;
                        Curr[BytesPerTexel*j+2] = g;
                    }
                }
            }
        }
        break;

    case '4' :  /* Binary PBM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                c = hwFread( Buff, (w+7)/8, 1, f );
                if( c != 1 )    goto Error;
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        c = (Buff[j/8] & (1<<(j%8))) ? 255 : 0;
                        Curr[BytesPerTexel*j] = c;
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        c = (Buff[j/8] & (1<<(j%8))) ? 255 : 0;
                        Curr[BytesPerTexel*j  ] = c;
                        Curr[BytesPerTexel*j+1] = c;
                        Curr[BytesPerTexel*j+2] = c;
                    }
                }
            }
        }
        break;

    case '2' :  /* ASCII PGM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &g ) != 1 )        goto Error;
                        Curr[BytesPerTexel*j] = g;
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &g ) != 1 )        goto Error;
                        Curr[BytesPerTexel*j  ] = g;
                        Curr[BytesPerTexel*j+1] = g;
                        Curr[BytesPerTexel*j+2] = g;
                    }
                }
            }
        }
        break;

    case '5' :  /* Binary PGM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                c = hwFread( Buff, w, 1, f );
                if( c != 1 )    goto Error;
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        Curr[BytesPerTexel*j] = Buff[j];
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        Curr[BytesPerTexel*j  ] = Buff[j];
                        Curr[BytesPerTexel*j+1] = Buff[j];
                        Curr[BytesPerTexel*j+2] = Buff[j];
                    }
                }
            }
        }
        break;

    case '3' :  /* ASCII PPM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &r ) != 1 ) goto Error;
                        if( hwFscanf( f, "%d", &g ) != 1 ) goto Error;
                        if( hwFscanf( f, "%d", &b ) != 1 ) goto Error;
                        c = (r + g + b) / 3;
                        Curr[BytesPerTexel*j] = c;
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        if( hwFscanf( f, "%d", &r ) != 1 ) goto Error;
                        if( hwFscanf( f, "%d", &g ) != 1 ) goto Error;
                        if( hwFscanf( f, "%d", &b ) != 1 ) goto Error;
                        Curr[BytesPerTexel*j  ] = r;
                        Curr[BytesPerTexel*j+1] = g;
                        Curr[BytesPerTexel*j+2] = b;
                    }
                }
            }
        }
        break;

    case '6' :  /* Binary PPM */
        for( k = 0, Curr = Img; k < d; k++ ) {
            for( i = 0; i < h; i++, Curr += BytesPerTexel*w ) {
                c = hwFread( Buff, w*3, 1, f );
                if( c != 1 )    goto Error;
                if( WantMono ) {
                    for( j = 0; j < w; j++ ) {
                        c = (int)(Buff[3*j] + Buff[3*j+1] + Buff[3*j+2])/3;
                        Curr[BytesPerTexel*j] = c;
                    }
                }
                else {
                    for( j = 0; j < w; j++ ) {
                        Curr[BytesPerTexel*j  ] = Buff[3*j  ];
                        Curr[BytesPerTexel*j+1] = Buff[3*j+1];
                        Curr[BytesPerTexel*j+2] = Buff[3*j+2];
                    }
                }
            }
        }
        break;
    }

    free( Buff );               Buff = 0;
    (void)hwFclose( f );          f = 0;

    /* Allocate the result and stick it in the hash table */
    result = __hwIntCreateImage();
    if( !result )       return 0;

    result->fileName = malloc( strlen( Name ) + 1 );
    if( !result->fileName )     return 0;
    (void)strcpy( result->fileName, Name );

    i = hash( Name );
    result->next = hashTable[i];
    hashTable[i] = result;

    /* Stash away other important info */
    result->width = w;
    result->height = h;
    result->depth = d;
    result->components = BytesPerTexel;
    result->type = HW_IMG_UBYTE;
    result->data = Img;

    /* Return the result */
    result->refCount++;
    return result;

Error  :
    if( Buff )          free( Buff );
    if( Img )           free( Img );
    if( f )             (void)hwFclose( f );
    return 0;
}

/*** EOF ppm.c ***/
