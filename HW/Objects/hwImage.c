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

#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"

#ifdef ANDROID
#include <android/log.h>
//#define ANDROID_LOG(a)  __android_log_print a
#define ANDROID_LOG(a)
#else
#define ANDROID_LOG(a)
#endif

/* Internal definitions of image-related flags */
#define HW_ROWS                 1
#define HW_COLUMNS              2
#define HW_COMPONENTS           3
#define HW_IMAGE                4
#define HW_FILENAME             5
#define HW_INT_IMAGE            6
#define HW_TRANS_COLOR          7
#define HW_MAX_DIMENSION        8
#define HW_POS_X                9
#define HW_POS_Y                10
#define HW_WIDTH                11
#define HW_HEIGHT               12
#define HW_COLOR                13
#define HW_BOUNDS               14
#define HW_PARENT               15
#define HW_ALIGN                16
#define HW_DIRTY                17

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to image */
        hwStrTransparentColor,
        hwStrRows, hwStrColumns, hwStrComponents,
        hwStrImage, hwStrFileName,
        hwStrMaxDimension,
        hwStrPosX, hwStrPosY,
        hwStrWidth, hwStrHeight,
        hwStrColor, hwStrAlign,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwImageDef = {
        0,              /* Parent - NULL */
        "hwImage",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwImage = &hwImageDef;

typedef struct {
    struct _hwObjectStruct
        hdr;                        /* Common stuff */
    hwInt32
        refCount,
        maxDimension,
        dirty,
        fileDirty,
        imageDirty,
        color,
        posX, posY,
        width, height,
        align,
        bounds[4],
        parentBounds[4];
    hwImageStruct
        scaledImg,
        *img;
    hwObject
        parent;
    char
        *filename;
    unsigned char
        hasTranspColor,
        transpColor[3];
    void
        *cookedBits;
    hwInt32
        tmId;
} Image;

static void cook( hwDisplay, Image * );

/* The hash table for image strings */
static void *imageTab = 0;

static hwObject create( hwObject proto )
{
    Image
        *result;
    void
        *t;
    LOG_ENTRY

    if( proto != hwImage )      return 0;

    HW_HASH_SETUP(imageTab, 32)
        t = imageTab;
        HW_INSERT( hwStrRows,             HW_ROWS,          t );
        HW_INSERT( hwStrColumns,          HW_COLUMNS,       t );
        HW_INSERT( hwStrComponents,       HW_COMPONENTS,    t );
        HW_INSERT( hwStrImage,            HW_IMAGE,         t );
        HW_INSERT( hwStr__Image,          HW_INT_IMAGE,     t );
        HW_INSERT( hwStrFileName,         HW_FILENAME,      t );
        HW_INSERT( hwStrTransparentColor, HW_TRANS_COLOR,   t );
        HW_INSERT( hwStrMaxDimension,     HW_MAX_DIMENSION, t );
        HW_INSERT( hwStrPosX,             HW_POS_X,         t );
        HW_INSERT( hwStrPosY,             HW_POS_Y,         t );
        HW_INSERT( hwStrWidth,            HW_WIDTH,         t );
        HW_INSERT( hwStrHeight,           HW_HEIGHT,        t );
        HW_INSERT( hwStrAlign,            HW_ALIGN,         t );
        HW_INSERT( hwStrColor,            HW_COLOR,         t );
        HW_INSERT( hwStrBounds,           HW_BOUNDS,        t );
        HW_INSERT( hwStrParent,           HW_PARENT,        t );
        HW_INSERT( hwStrDirty,            HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Image) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwImage;
    result->refCount = 1;

    result->img = 0;
    result->filename = 0;
    result->hasTranspColor = 0;
    result->maxDimension = 16384;
    result->posX = 0;
    result->posY = 0;
    result->width = 0;
    result->height = 0;
    result->align = 0;
    result->parent = NULL;
    result->scaledImg.width = 0;
    result->scaledImg.height = 0;
    result->scaledImg.data = 0;
    result->cookedBits = 0;
    result->tmId = -1;
    result->color = 0xFFFFFFFF;
    result->dirty = 1;
    result->fileDirty = 0;
    result->imageDirty = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Image
        *img = (Image *)obj;

    LOG_ENTRY
    img->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Image
        *image = (Image *)obj;

    LOG_ENTRY
    if( --image->refCount > 0 )     return;
    if( image->img )        __hwIntDestroyImage( image->img );
    if( image->filename )       free( image->filename );
    free( image );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Image
        *image = (Image *)obj;
    int
        rows, cols, comps, n;
    hwInt8
        *dst;
    const hwFloat
        *fsrc;
    const hwInt32
        *uisrc;
    const hwInt16
        *ussrc;
    hwInt32
        dirty = 1;      /* Default is, make it dirty */
    HW_USE_CURR_DISP;

#ifdef ANDROID_NDK
    LOG_ENTRY
#endif
    switch( hwLookup( prop, imageTab ) ) {
    case HW_ROWS :
        if( type != HW_TYPE_1I )        goto BadType;
        if( !image->img )       image->img = __hwIntCreateImage();
        image->img->width = *(hwInt32 *)val;
        image->imageDirty = 1;
        break;
    case HW_COLUMNS :
        if( type != HW_TYPE_1I )        goto BadType;
        if( !image->img )       image->img = __hwIntCreateImage();
        image->img->height = *(hwInt32 *)val;
        image->imageDirty = 1;
        break;
    case HW_COMPONENTS :
        if( type != HW_TYPE_1I )        goto BadType;
        if( !image->img )       image->img = __hwIntCreateImage();
        image->img->components = *(hwInt32 *)val;
        image->imageDirty = 1;
        break;
    case HW_MAX_DIMENSION :
        if( type != HW_TYPE_1I )        goto BadType;
        image->maxDimension = *(hwInt32 *)val;
        image->imageDirty = 1;
        break;
    case HW_POS_X :
        if( type == HW_TYPE_1F )      image->posX = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) image->posX = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_POS_Y :
        if( type == HW_TYPE_1F )      image->posY = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) image->posY = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_WIDTH :
        if( type == HW_TYPE_1F )      image->width = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) image->width = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_HEIGHT :
        if( type == HW_TYPE_1F )      image->height = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) image->height = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_ALIGN :
        if( type == HW_TYPE_1F )      image->align = *(hwFloat *)val;
        else if( type == HW_TYPE_1I ) image->align = *(hwInt32 *)val;
        else                          goto BadType;
        break;
    case HW_PARENT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        image->parent = (hwObject)val;
        break;
    case HW_FILENAME :
        if( type != HW_TYPE_STRING )        goto BadType;
        if( image->filename ) {
            free( image->filename );
            image->filename = 0;
        }
        if( image->img ) {
            __hwIntDestroyImage( image->img );
            image->img = 0;
        }
        image->filename = malloc( strlen( (char *)val ) + 1 );
        if( !image->filename ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        (void)strcpy( image->filename, val );
        image->fileDirty = 1;
        break;
    case HW_DIRTY :
        if( image->tmId != -1 ) {
            __hwDisp->destroyTexture( __hwDisp, image->tmId );
        }
        image->tmId = -1;
        break;
    case HW_TRANS_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        image->hasTranspColor = 1;
        fsrc = val;
        image->transpColor[0] = fsrc[0] * 255.0f;
        image->transpColor[1] = fsrc[1] * 255.0f;
        image->transpColor[2] = fsrc[2] * 255.0f;
        image->imageDirty = 1;
        break;
    case HW_COLOR :
        dirty = 0;      /* No recooking needed here */
        image->color = __hwIntGuiColor( type, val );
        break;
    case HW_IMAGE :
        /* Can't modify image of non-image */
        if( !image->img )               goto BadType;

        /* Can't modify shared image */
        if( image->img->refCount > 1 )      goto BadType;

        rows = image->img->width;
        cols = image->img->height;
        comps = image->img->components;
        n = rows * cols * comps;
        if( n != HW_GET_COUNT(type) )       goto BadType;

        if( image->img->data ) {
            free( image->img->data );
        }

        dst = malloc( n );
        if( !dst ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        image->img->data = (void *)dst;
        switch( HW_GET_BASE(type) ) {
        case HW_TYPE_BYTE :
            (void)memcpy( image->img->data, val, n );
            break;
        case HW_TYPE_SHORT :
            ussrc = val;
            while( n-- ) *dst++ = (*ussrc++ >> 8) & 0xFF;
            break;
        case HW_TYPE_INT :
            uisrc = val;
            while( n-- ) *dst++ = (*uisrc++ >> 24) & 0xFF;
            break;
        case HW_TYPE_FLOAT :
            fsrc = val;
            while( n-- ) *dst++ = *fsrc++ * 255;
            break;
        }
        image->imageDirty = 1;
        break;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if( dirty ) image->dirty = 1;

    LOG_EXIT
    return;

BadType:
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Image
        *image = (Image *)obj;
    hwInt32
        rows, cols, comps, n, tmp, x, y;
    unsigned char
        *ptr, *next, *dst;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    if( image->dirty ) {
        cook( __hwDisp, image );
    }

    switch( hwLookup( prop, imageTab ) ) {
    case HW_INT_IMAGE :
        /* HACK!  We'll resample the image here if it's bigger than
         * the user requested max dimension
         */
        if( !image->img ) return 0;
        comps = image->img->components;
        while( (image->img->width > image->maxDimension) ||
                (image->img->height > image->maxDimension) )
        {
            /* Scale it by half */
            dst = ptr = image->img->data;
            next = ptr + comps*image->img->width;
            for( y = 0; y < image->img->height; y += 2 ) {
                for( x = 0; x < image->img->width; x += 2 ) {
                    for( n = 0; n < comps; n++, ptr++, next++ ) {
                        tmp = (ptr[0] + ptr[comps] + next[0] + next[comps])/4;
                        *dst++ = tmp;
                    }
                    ptr += comps; next += comps;
                }
                ptr = next; next += comps*image->img->width;
            }
            image->img->width /= 2;
            image->img->height /= 2;
        }
        *val = image->img;
        return 1;
    case HW_ROWS :
        if( image->filename )       return 0;
        if( !image->img )       return 0;
        *val  = &image->img->width;
        return HW_TYPE_1I;
    case HW_COLUMNS :
        if( image->filename )       return 0;
        if( !image->img )       return 0;
        *val = &image->img->height;
        return HW_TYPE_1I;
    case HW_COMPONENTS :
        if( image->filename )       return 0;
        if( !image->img )       return 0;
        *val = &image->img->components;
        return HW_TYPE_1I;
    case HW_FILENAME :
        if( !image->filename )      return 0;
        *val = image->filename;
        return HW_TYPE_STRING;
    case HW_TRANS_COLOR :
        if( !image->hasTranspColor )        return 0;
        *val = intDisp->scratchFloat;
        intDisp->scratchFloat[0] = image->transpColor[0] * (1.0f / 255.0f);
        intDisp->scratchFloat[1] = image->transpColor[1] * (1.0f / 255.0f);
        intDisp->scratchFloat[2] = image->transpColor[2] * (1.0f / 255.0f);
        return HW_TYPE_3F;
    case HW_IMAGE :
        if( image->filename )       return 0;
        if( !image->img )       return 0;
        if( !image->img->data )     return 0;
        rows = image->img->width;
        cols = image->img->height;
        comps = image->img->components;
        n = rows * cols * comps;
        *val = image->img->data;
        return HW_MAKE_TYPE(HW_TYPE_BYTE,n);
    case HW_COLOR :
        *val = &image->color;
        return HW_TYPE_1I;
    case HW_POS_X :
        *val = &image->posX;
        return HW_TYPE_1I;
    case HW_POS_Y :
        *val = &image->posY;
        return HW_TYPE_1I;
    case HW_WIDTH :
        *val = &image->width;
        return HW_TYPE_1I;
    case HW_HEIGHT :
        *val = &image->height;
        return HW_TYPE_1I;
    case HW_ALIGN :
        *val = &image->align;
        return HW_TYPE_1I;
    case HW_PARENT :
        *val = image->parent;
        return HW_TYPE_OBJECT;
    case HW_BOUNDS :
        *val = image->bounds;
        return HW_TYPE_4I;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void cook( hwDisplay disp, Image *image )
{
    char
        filename[1024];
    int
        n, m;
    char
        *suff;
    hwImageStruct
        *(*imageReader)( char * );

    image->dirty = 0;

    __hwIntGetParentBounds( disp, image->parent, image->parentBounds );

    __hwIntPositionWidget( image->bounds, image->parentBounds,
                           image->posX, image->posY,
                           image->width, image->height,
                           image->align,
                           0.0 );

    if( image->fileDirty ) {
        /* If the user has set the hwDefaultDataFilePath, then prepend that
         * path to the path passed in datafile.  Unless, of course, the
         * datafile name starts with a '/', in which case we already have
         * a fully qualified file name.
         */
        if( (image->filename[0] != '/') && hwDefaultDataFilePath ) {
            n = strlen( hwDefaultDataFilePath );
            if( n > 1023 ) n = 1023;
            strncpy( filename, hwDefaultDataFilePath, n );
        }
        else {
            n = 0;
        }

        m = strlen( image->filename );
        if( (n + m) > 1023 ) m = 1023 - n;
        strncpy( filename+n, image->filename, m );
        filename[n+m] = 0;

        ANDROID_LOG((ANDROID_LOG_INFO,
                 "HB_CLIENT", "%s  ImageName = %s", __func__, filename));

        suff = strrchr( filename, '.' );
        imageReader = __hwIntReadPPM;
        if( suff ) {
            if(     ((suff[1] == 'j') || (suff[1] == 'J'))
                &&  ((suff[2] == 'p') || (suff[2] == 'P'))
                &&  ((suff[3] == 'g') || (suff[3] == 'G')) )
            {
                imageReader = __hwIntReadJPG;
            }
            else if( ((suff[1] == 'j') || (suff[1] == 'J'))
                &&   ((suff[2] == 'p') || (suff[2] == 'P'))
                &&   ((suff[3] == 'e') || (suff[3] == 'E'))
                &&   ((suff[3] == 'g') || (suff[3] == 'G')) )
            {
                imageReader = __hwIntReadJPG;
            }
            else if( ((suff[1] == 't') || (suff[1] == 'T'))
                &&   ((suff[2] == 'i') || (suff[2] == 'I'))
                &&   ((suff[3] == 'f') || (suff[3] == 'F')) )
            {
                imageReader = __hwIntReadTIF;
            }
            else if( ((suff[1] == 'p') || (suff[1] == 'P'))
                &&   ((suff[2] == 'n') || (suff[2] == 'N'))
                &&   ((suff[3] == 'g') || (suff[3] == 'G')) )
            {
                imageReader = __hwIntReadPNG;
            }
        }
        image->img = (*imageReader)( filename );

        image->fileDirty = 0;
        image->imageDirty = 1;
    }

    if( image->imageDirty && image->img ) {
        if( image->hasTranspColor ) {
            __hwIntModifyAlpha( image->img, image->transpColor );
        }
        if( image->tmId != -1 ) {
            disp->destroyTexture( disp, image->tmId );
        }
        image->tmId = -1;
    }
    image->imageDirty = 0;
}

static void draw( hwObject obj )
{
    Image
        *image = (Image  *)obj;
    hwInt32
        x, y, w, h;
    hwOrientType
        orient;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    if( image->dirty ) {
        cook( __hwDisp, image );
    }

    if( !image->img )  return;

    if( image->tmId == -1 ) {
        hwDefaultOrient( &orient );
        image->tmId = __hwDisp->createTexture( __hwDisp,
                                           image->img,
                                           HW_TM_TRILINEAR,
                                           HW_TM_MODULATE,
                                           HW_TM_CLAMP,
                                           HW_TM_EXPLICIT,
                                           &orient,
                                           0 );
    }

    x = image->bounds[0] + image->parentBounds[0];
    y = image->bounds[1] + image->parentBounds[1];
    w = image->bounds[2];
    h = image->bounds[3];

    if( !(w && h) ) return;     /* zero-size image draws nothing */

    __hwDisp->guiRaster( __hwDisp,
                                x, y, w, h, image->color, image->tmId );

    LOG_EXIT
}

/*** EOF hwImage.c ***/
