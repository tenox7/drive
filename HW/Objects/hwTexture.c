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
#include "hw.h"
#include "hw_internal.h"

#ifdef ANDROID
#include <android/log.h>
//#define ANDROID_LOG(a)  __android_log_print a
#define ANDROID_LOG(a)
#else
#define ANDROID_LOG(a)
#endif

/* Internal definitions of texture-related flags */
#define HW_APPLY                1
#define HW_BOUND                2
#define HW_COORD_MODE           3
#define HW_FILTER               4
#define HW_TEXTURE_ID           5
#define HW_INTERNAL_FORMAT      6
#define HW_DIRTY                7
#define HW_RELIEF_SCALE         8

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to texture */
        hwStrApply, hwStrBound, hwStrCoordMode, hwStrFilter,
        hwStrInternalFormat, hwStrReliefScale,

        /* Inherited from Image */
        hwStrRows, hwStrColumns, hwStrComponents,
        hwStrImage,
        hwStrFileName,

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwTextureStruct = {
        0,              /* Parent - NULL */
        "hwTexture",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwTexture = &hwTextureStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,
        tid,                    /* Texture ID to use... */
        intFormat,               /* Internal format, if any */
        filter,                 /* Is filtering on? */
        apply,                  /* Application mode */
        bound,                  /* Boundary mode */
        coordMode;              /* Coordinate generation mode */
    hwFloat
        reliefScale;            /* If bump mapping */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwImageStruct
        *cooked;                /* Cooked image, if needed */
    hwObject
        image;                  /* The texture image */
} Texture;

/* The hash table for texture strings */
static void *textureTab = 0;

static hwObject create( hwObject proto )
{
    Texture
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwTexture )    return 0;

    HW_HASH_SETUP(textureTab, 16)
        t = textureTab;
        HW_INSERT( hwStrApply,          HW_APPLY,           t );
        HW_INSERT( hwStrBound,          HW_BOUND,           t );
        HW_INSERT( hwStrCoordMode,      HW_COORD_MODE,      t );
        HW_INSERT( hwStrFilter,         HW_FILTER,          t );
        HW_INSERT( hwStrInternalFormat, HW_INTERNAL_FORMAT, t );
        HW_INSERT( hwStrTextureID,      HW_TEXTURE_ID,      t );
        HW_INSERT( hwStrDirty,          HW_DIRTY,           t );
        HW_INSERT( hwStrReliefScale,    HW_RELIEF_SCALE,    t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Texture) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwTexture;
    result->refCount = 1;

    result->image = hwImage->create( hwImage );
    if( !result->image ) {
        free( result );
        return 0;
    }
    result->cooked = NULL;

    result->dirty = 1;
    result->tid = -1;
    result->filter = 1;
    result->intFormat = HW_TM_COLOR;
    result->apply = HW_TM_MODULATE;
    result->bound = HW_TM_REPEAT;
    result->coordMode = HW_TM_EXPLICIT;
    result->reliefScale = 1.0;
    result->orient.scale[0] =
    result->orient.scale[1] =
    result->orient.scale[2] = 1.0;
    result->orient.rotate[0] =
    result->orient.rotate[1] =
    result->orient.rotate[2] = 0.0;
    result->orient.pos[0] =
    result->orient.pos[1] =
    result->orient.pos[2] = 0.0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Texture
        *texture = (Texture *)obj;

    LOG_ENTRY
    texture->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Texture
        *texture = (Texture *)obj;

    LOG_ENTRY
    if( --texture->refCount > 0 )       return;
    if( texture->image )        texture->image->destroy( texture->image );
    if( texture->cooked )       free( texture->cooked );
    free( texture );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Texture
        *texture = (Texture *)obj;
    hwInt32
        iVal;
    int
        dirty = 1, n;

    LOG_ENTRY

    switch( hwLookup( prop, textureTab ) ) {
    case HW_APPLY :
        if( type != HW_TYPE_1I )        goto BadType;

        switch( (iVal = *(hwInt32 *)val) ) {
        case HW_TM_MODULATE :
        case HW_TM_BUMP :
        case HW_TM_RELIEF :
        case HW_TM_GLOSS :
        case HW_TM_SHADOW :
        case HW_TM_SHADOW_VAR :
            break;
        default :
            goto LBadValue;
        }

        texture->apply = iVal;
        break;

    case HW_BOUND :
        if( type != HW_TYPE_1I )        goto BadType;

        switch( (iVal = *(hwInt32 *)val) ) {
        case HW_TM_REPEAT :
        case HW_TM_CLAMP :
            break;
        default :
            goto LBadValue;
        }

        texture->bound = iVal;
        break;

    case HW_COORD_MODE :
        if( type != HW_TYPE_1I )        goto BadType;

        switch( (iVal = *(hwInt32 *)val) ) {
        case HW_TM_EXPLICIT :
        case HW_TM_PLANAR :
        case HW_TM_SPHERE :
        case HW_TM_CYLINDER :
        case HW_TM_ENVMAP :
        case HW_TM_STAGE_0 :
            break;
        default :
            goto LBadValue;
        }

        texture->coordMode = iVal;
        break;

    case HW_FILTER :
        if( type != HW_TYPE_1I )        goto BadType;

        switch( (iVal = *(hwInt32 *)val) ) {
        case HW_TM_POINT :
        case HW_TM_LINEAR :
        case HW_TM_MIP :
        case HW_TM_TRILINEAR :
            break;
        default :
            goto LBadValue;
        }

        texture->filter = iVal;
        break;

    case HW_INTERNAL_FORMAT :
        if( type != HW_TYPE_1I )        goto BadType;

        switch( (iVal = *(hwInt32 *)val) ) {
        case HW_TM_COLOR :
        case HW_TM_ALPHA :
        case HW_TM_INTENSITY :
        case HW_TM_HEIGHT :
        case HW_TM_NORMAL :
        case HW_TM_NORMAL_HEIGHT :
        case HW_TM_DEPTH :
        case HW_TM_DEPTH_VAR :
            break;
        default :
            goto LBadValue;
        }

        texture->intFormat = iVal;
        break;

    case HW_RELIEF_SCALE :
        if( type != HW_TYPE_1F )        goto BadType;
        texture->reliefScale = *(hwFloat *)val;
        break;

    case HW_DIRTY :
        if( texture->image ) {
            texture->image->modify( texture->image, prop, type, val );
        }
        break;

    default :
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            __hwIntOrientModify( &texture->orient, n, type, val );
            dirty = 0;
        }
        else {
            texture->image->modify( texture->image, prop, type, val );
        }
        break;
    }
    if( dirty ) texture->dirty = 1;

    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    LOG_EXIT
    return;

LBadValue :
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return;
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Texture
        *texture = (Texture *)obj;
    int
        n;

    LOG_ENTRY
    switch( hwLookup( prop, textureTab ) ) {
    case HW_APPLY :
        *val = &texture->apply;
        return HW_TYPE_1I;
    case HW_BOUND :
        *val = &texture->bound;
        return HW_TYPE_1I;
    case HW_COORD_MODE :
        *val = &texture->coordMode;
        return HW_TYPE_1I;
    case HW_FILTER :
        *val = &texture->filter;
        return HW_TYPE_1I;
    case HW_TEXTURE_ID :
        *val = &texture->tid;
        return HW_TYPE_1I;
    case HW_INTERNAL_FORMAT :
        *val = &texture->intFormat;
        return HW_TYPE_1I;
    case HW_RELIEF_SCALE :
        *val = &texture->reliefScale;
        return HW_TYPE_1F;
    default :
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &texture->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    return texture->image->inquire( texture->image, prop, val );
}

static void draw( hwObject obj )
{
    Texture
        *texture = (Texture *)obj;
    hwImageStruct
        *img, *cooked;
    hwUint8
        *newRaster;
    int
        n;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    /* This routine is kind of funky.  It doesn't actually
     * draw anything; rather, it makes sure that the texture ID
     * is cooked properly to a given display.  A later inquiry
     * to the texture object will retrieve the texture ID.
     */
    if( !texture->dirty ) {
        LOG_EXIT
        return;
    }

    ANDROID_LOG((ANDROID_LOG_INFO, "HB_CLIENT", "Dirty texture"));

    /* Delete / destroy old contents */
    if( texture->tid >= 0 ) {
        __hwDisp->destroyTexture( __hwDisp, texture->tid );
    }
    if( texture->cooked ) {
        free( texture->cooked );
        texture->cooked = NULL;
    }
    texture->tid = -1;
    texture->dirty = 0;

    /* Get the image */
    if( !texture->image->inquire( texture->image,
                                hwStr__Image, (void **)&img) )
    {
        goto InternalError;
    }

    /* See if we're bump-mapping with a height field */
    if( texture->intFormat == HW_TM_HEIGHT) {
        if( img->components != 1 ) goto LBadValue;
        switch( texture->apply ) {
        case HW_TM_NORMAL :
        case HW_TM_RELIEF :
            break;
        default :
            /* Nothing useful to do with a height map other than
             * normal / relief map
             */
            goto LBadValue;
        }

        /* We need to create a normal map from the height field */
        n = img->width * img->height * 4;
        cooked = malloc( sizeof(hwSurfaceType) + n );
        if( !cooked ) goto NoMemory;

        newRaster = (hwUint8 *)(cooked + 1);

        /* TBD: Some day we should support other than 8BPP */
        hwMakeNormalMap8( newRaster, img->data,
                          img->width, img->height,
                          texture->bound, texture->bound,
                          texture->reliefScale );

        cooked->width = img->width;
        cooked->height = img->height;
        cooked->depth = img->depth;
        cooked->depth = img->depth;
        cooked->components = 4;
        cooked->type = HW_IMG_UBYTE;
        cooked->refCount = 1;
        cooked->tmId = -1;
        cooked->fileName = NULL;
        cooked->data = newRaster;
        cooked->next = NULL;

        /* Use the new cooked image */
        texture->cooked = img = cooked;
    }

    texture->tid = __hwDisp->createTexture( __hwDisp,
                            img, texture->filter, texture->apply, 
                            texture->bound,
                            texture->coordMode,
                            &texture->orient,
                            texture->intFormat );
    LOG_EXIT
    return;

LBadValue :
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return;

NoMemory :
    __hwIntSetError( HW_ERROR_NO_MEMORY );
    LOG_EXIT
    return;

InternalError :
    __hwIntSetError( HW_ERROR_INTERNAL );
    LOG_EXIT
    return;
}

/* This routine is a mondo hack.  For convenience, we violate
 * object-oriented design, and look directly at internal entries
 * in hwTexture.  Oh, well.  At least we check the class...
 */
hwInt32 __hwIntTexturePipeline
(
    hwSurfaceType *surf,
    hwFloat **data,
    int *dataSize,
    hwInt32 *vn,
    hwInt32 numVerts,
    hwInt32 dataFlags
)
{
    hwFloat
        *localData;
    Texture
        *tm;
    hwImageStruct
        *img[HW_MAX_TEXTURES];
    hwObject
        texture = 0;
    hwFloat
        *color;
    hwInt32
        valid = 1, ti, numTex,
        hasAlpha;

    LOG_ENTRY
    color = surf->color;
    numTex = surf->numTextures;
    if( !numTex ) valid = 0;

    for( ti = 0; ti < numTex; ti++ ) {
        texture = surf->textures[ti];

        if(    !texture
            || (texture->parent != hwTexture)
            || !(texture->inquire)(texture, hwStr__Image,( void **)&img[ti])
            || !(img[ti]->data) )
        {
            valid = 0;
        }
    }

    if( !valid )        goto RemoveTM;

    hasAlpha = 0;
    for( ti = 0; ti < numTex; ti++ ) {
        tm = (Texture *)surf->textures[ti];

        if( (img[ti]->components == 2) || (img[ti]->components == 4) ) {
            hasAlpha = 1;
        }

        /* We have a texture; let's sure hope we have coord gen! */
        switch( tm->coordMode ) {
        case HW_TM_ENVMAP :
        case HW_TM_STAGE_0 :
            /* Done by graphics pipeline */
            break;
        default :
            if( !(dataFlags & (HW_DATA_ST << (3*ti))) ) {
                localData = __hwIntAddUV( *vn, numVerts, *data,
                                    (hwInt32 *)dataSize );
                if( !localData )    return dataFlags;
                *vn += 2;
                dataFlags |= (HW_DATA_ST << (3*ti));
                *data = localData;
            }
            break;
        }

        switch( tm->coordMode ) {
        case HW_TM_EXPLICIT :
            /* Already there.  Use orientation to change coords */
            __hwIntOrientData( &tm->orient, *data, numVerts,
                                dataFlags, ti );
            break;
        case HW_TM_SPHERE :
            __hwIntSphereGen( &tm->orient, *data, numVerts,
                                dataFlags, ti );
            break;
        case HW_TM_CYLINDER :
            __hwIntCylGen( &tm->orient, *data, numVerts,
                                dataFlags, ti );
            break;
        case HW_TM_PLANAR :
            __hwIntPlaneGen( &tm->orient, *data, numVerts,
                                dataFlags, ti );
            break;
        case HW_TM_ENVMAP :
        case HW_TM_STAGE_0 :
            /* Done by graphics pipeline */
            break;
        }
    }

    if( surf->flags & HW_SURF_TEX_SAMP ) {
        if( !(dataFlags & HW_DATA_RGB) ) {
            if( hasAlpha ) {
                localData = __hwIntAddRGB( *vn, numVerts, color, *data,
                                        (hwInt32 *)dataSize, 1 );
                if( localData ) {
                    *vn += 4;
                    dataFlags |= HW_DATA_RGB | HW_DATA_ALPHA;
                    *data = localData;
                }
            }
            else {
                localData = __hwIntAddRGB( *vn, numVerts, color, *data,
                                        (hwInt32 *)dataSize, 0 );
                if( localData ) {
                    *vn += 3;
                    dataFlags |= HW_DATA_RGB;
                    *data = localData;
                }
            }
        }

        if( dataFlags & HW_DATA_RGB ) {
            __hwIntTextureData( img, numTex, tm->apply,
                            (tm->bound == HW_TM_CLAMP),
                            *data, numVerts, dataFlags );
        }

        goto RemoveTM;
    }

    LOG_EXIT
    return dataFlags;

RemoveTM :
    ti = hwCalcWPV( dataFlags & HW_DATA_TEXCOORD ) - 3;
    if( ti > 0 ) {
        __hwIntRemoveUV( *vn, numVerts, *data, ti );
        *vn -= ti;
    }
    dataFlags &= ~HW_DATA_TEXCOORD;

    return dataFlags;
}

/*** EOF hwTexture.c ***/
