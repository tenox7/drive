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

#define SURF_COLOR      1
#define SURF_TRANSP     2
#define SURF_SPEC_COLOR 3
#define SURF_SHININESS  4
#define SURF_BACKFACE   5
#define SURF_TWOSIDED   6
#define SURF_EMISSIVE   7
#define SURF_INVISIBLE  8
#define SURF_WIREFRAME  9
#define SURF_TEXTURE    10
#define SURF_VISIBILITY 11
#define SURF_SURFACE    12
#define SURF_PRUNE_PRIM 13
#define SURF_SAMP_VERT  14
#define SURF_BLEND      15
#define SURF_FLIP_NORMALS       16
#define SURF_UNCOLORED  17
#define SURF_PRIMSIZE   18
#define SURF_DIRTY      19
#define SURF_PRE_MULT   20

void
    *__hwIntSurfTab;

int __hwIntInitSurf( void )
{
    LOG_ENTRY

    HW_HASH_SETUP(__hwIntSurfTab, 24)
        HW_INSERT( hwStrColor,        SURF_COLOR,        __hwIntSurfTab);
        HW_INSERT( hwStrTransparency, SURF_TRANSP,       __hwIntSurfTab);
        HW_INSERT( hwStrSpecColor,    SURF_SPEC_COLOR,   __hwIntSurfTab);
        HW_INSERT( hwStrShininess,    SURF_SHININESS,    __hwIntSurfTab);
        HW_INSERT( hwStrBackface,     SURF_BACKFACE,     __hwIntSurfTab);
        HW_INSERT( hwStrTwoSided,     SURF_TWOSIDED,     __hwIntSurfTab);
        HW_INSERT( hwStrBright,       SURF_EMISSIVE,     __hwIntSurfTab);
        HW_INSERT( hwStrInvisible,    SURF_INVISIBLE,    __hwIntSurfTab);
        HW_INSERT( hwStrWireframe,    SURF_WIREFRAME,    __hwIntSurfTab);
        HW_INSERT( hwStrVisibility,   SURF_VISIBILITY,   __hwIntSurfTab);
        HW_INSERT( hwStrTexture,      SURF_TEXTURE,      __hwIntSurfTab);
        HW_INSERT( hwStrSurface,      SURF_SURFACE,      __hwIntSurfTab);
        HW_INSERT( hwStrPrunePrim,    SURF_PRUNE_PRIM,   __hwIntSurfTab);
        HW_INSERT( hwStrSampVert,     SURF_SAMP_VERT,    __hwIntSurfTab);
        HW_INSERT( hwStrBlend,        SURF_BLEND,        __hwIntSurfTab);
        HW_INSERT( hwStrPreMult,      SURF_PRE_MULT,     __hwIntSurfTab);
        HW_INSERT( hwStrFlipNormals,  SURF_FLIP_NORMALS, __hwIntSurfTab);
        HW_INSERT( hwStrUncolored,    SURF_UNCOLORED,    __hwIntSurfTab);
        HW_INSERT( hwStrPrimSize,     SURF_PRIMSIZE,     __hwIntSurfTab);
        HW_INSERT( hwStrDirty,        SURF_DIRTY,        __hwIntSurfTab);
    HW_HASH_CLEANUP

    LOG_EXIT

    return 1;
}

int __hwIntSurfModify
(
    hwSurfaceType *surf,
    int propno,
    hwInt32 type,
    const void *val
)
{
    int
        i, n, result = 0;
    hwObject
        obj, *texPtr;
    void
        *tmp;

    LOG_ENTRY
    switch( propno ) {
    case SURF_TRANSP :
        switch( type ) {
        case HW_TYPE_1I :
            surf->transp = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            surf->transp = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        if( surf->flags & HW_SURF_TEX_SAMP ) {
            /* This can cause the transparency to bleed into the data */
            result = 1;
        }
        break;
    case SURF_SHININESS :
        switch( type ) {
        case HW_TYPE_1I :
            surf->shininess = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            surf->shininess = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case SURF_BACKFACE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_BACKFACE;
        else                    surf->flags &= ~HW_SURF_BACKFACE;
        result = 1;
        break;
    case SURF_TWOSIDED :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_TWOSIDED;
        else                    surf->flags &= ~HW_SURF_TWOSIDED;
        result = 1;
        break;
    case SURF_EMISSIVE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_EMISSIVE;
        else                    surf->flags &= ~HW_SURF_EMISSIVE;
        break;
    case SURF_INVISIBLE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_INVISIBLE;
        else                    surf->flags &= ~HW_SURF_INVISIBLE;
        break;
    case SURF_WIREFRAME :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_WIREFRAME;
        else                    surf->flags &= ~HW_SURF_WIREFRAME;
        result = 1;
        break;
    case SURF_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        surf->color[0] = ((hwFloat *)val)[0];
        surf->color[1] = ((hwFloat *)val)[1];
        surf->color[2] = ((hwFloat *)val)[2];
        if( surf->flags & HW_SURF_TEX_SAMP ) {
            /* This can cause the color to bleed into the data */
            result = 1;
        }
        break;
    case SURF_SPEC_COLOR:
        if( type != HW_TYPE_3F )        goto BadType;
        surf->specColor[0] = ((hwFloat *)val)[0];
        surf->specColor[1] = ((hwFloat *)val)[1];
        surf->specColor[2] = ((hwFloat *)val)[2];
        break;
    case SURF_TEXTURE :

        if( HW_GET_BASE(type) != HW_TYPE_OBJECT )       goto BadType;
        n = surf->numTextures;

        for( i = 0; i < n; i++ ) {
            surf->textures[i]->destroy( surf->textures[i] );
        }
        n = HW_GET_COUNT(type);
        if( n == 0 ) {
            surf->textures[0] = (hwObject)val;
            if( surf->textures[0] )     n = 1;
            else                        n = 0;
        }
        else {
            texPtr = (hwObject *)val;
            for( i = 0; i < n; i++ ) {
                surf->textures[i] = texPtr[i];
            }
        }
        for( i = 0; i < n; i++ ) {
            surf->textures[i]->addref( surf->textures[i] );
        }
        surf->numTextures = n;

        result = 1;
        break;
    case SURF_DIRTY :
        n = surf->numTextures;
        for( i = 0; i < n; i++ ) {
            HW_MODIFY_1I( surf->textures[i], hwStrDirty, 1 );
        }
        break;
    case SURF_VISIBILITY :
        if( type != HW_TYPE_1I )        goto BadType;
        surf->visibility = *(hwInt32 *)val;
        break;
    case SURF_SURFACE :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        obj = (hwObject)val;
        if( !obj->inquire( obj, hwStrSurface, &tmp ) )  goto BadType;
        *surf = *(hwSurfaceType *)tmp;
        result = 1;
        break;
    case SURF_PRUNE_PRIM :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_PRUNE_PRIM;
        else                    surf->flags &= ~HW_SURF_PRUNE_PRIM;
        result = 1;
        break;
    case SURF_BLEND :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_BLEND;
        else                    surf->flags &= ~HW_SURF_BLEND;
        break;
    case SURF_PRE_MULT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_BLEND_PREMULT;
        else                    surf->flags &= ~HW_SURF_BLEND_PREMULT;
        break;
    case SURF_UNCOLORED :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_UNCOLORED;
        else                    surf->flags &= ~HW_SURF_UNCOLORED;
        result = 1;
        break;
    case SURF_FLIP_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_FLIP_NORMALS;
        else                    surf->flags &= ~HW_SURF_FLIP_NORMALS;
        result = 1;
        break;
    case SURF_SAMP_VERT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surf->flags |= HW_SURF_TEX_SAMP;
        else                    surf->flags &= ~HW_SURF_TEX_SAMP;
        result = 1;
        break;
    case SURF_PRIMSIZE :
        if( type != HW_TYPE_1F )        goto BadType;
        surf->primSize = *(hwFloat *)val;
        result = 1;
        break;
    }
    LOG_EXIT
    return result;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    return 0;
}


hwInt32 __hwIntSurfInquire( hwSurfaceType *surf, int propno, void **val )
{
    hwInt32
        clean = 0;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    switch( propno ) {
    case SURF_TRANSP :
        if( surf->transp == 0.0 ) clean = HW_TYPE_CLEAN;
        *val = &surf->transp;
        return HW_TYPE_1F | clean;
    case SURF_SHININESS :
        if( surf->shininess == 0.0 ) clean = HW_TYPE_CLEAN;
        *val = &surf->shininess;
        return HW_TYPE_1F | clean;
    case SURF_BACKFACE :
        if( !(surf->flags & HW_SURF_BACKFACE) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_BACKFACE) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_TWOSIDED :
        if( !(surf->flags & HW_SURF_TWOSIDED) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_TWOSIDED) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_EMISSIVE :
        if( !(surf->flags & HW_SURF_EMISSIVE) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_EMISSIVE) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_INVISIBLE :
        if( !(surf->flags & HW_SURF_INVISIBLE) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_INVISIBLE) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_WIREFRAME :
        if( !(surf->flags & HW_SURF_WIREFRAME) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_WIREFRAME) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_BLEND :
        if( !(surf->flags & HW_SURF_BLEND) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_BLEND) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_PRE_MULT :
        if( !(surf->flags & HW_SURF_BLEND_PREMULT) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_BLEND_PREMULT) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_UNCOLORED :
        if( !(surf->flags & HW_SURF_UNCOLORED) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_UNCOLORED) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_FLIP_NORMALS :
        if( !(surf->flags & HW_SURF_FLIP_NORMALS) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_FLIP_NORMALS) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_PRUNE_PRIM :
        if( !(surf->flags & HW_SURF_PRUNE_PRIM) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_PRUNE_PRIM) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_SAMP_VERT :
        if( !(surf->flags & HW_SURF_TEX_SAMP) ) clean = HW_TYPE_CLEAN;
        intDisp->scratchInt[0] = (surf->flags & HW_SURF_TEX_SAMP) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B | clean;
    case SURF_COLOR :
        if( (surf->color[0] == 1.0) && (surf->color[1] == 1.0) &&
                (surf->color[2] == 1.0) )
        {
            clean = HW_TYPE_CLEAN;
        }
        *val = surf->color;
        return HW_TYPE_3F | clean;
    case SURF_SPEC_COLOR:
        if( (surf->specColor[0] == 1.0) && (surf->specColor[1] == 1.0) &&
                (surf->specColor[2] == 1.0) )
        {
            clean = HW_TYPE_CLEAN;
        }
        *val = surf->specColor;
        return HW_TYPE_3F | clean;
    case SURF_TEXTURE :
        if( surf->numTextures == 1 ) {
            *val = surf->textures[0];
            return HW_TYPE_OBJECT;
        }
        else if( surf->numTextures ) {
            *val = surf->textures;
            return HW_MAKE_TYPE( HW_TYPE_OBJECT, surf->numTextures );
        }
        else {
            return 0;
        }
    case SURF_VISIBILITY :
        if( surf->visibility == 0xFFFFFFFF )    clean = HW_TYPE_CLEAN;
        *val = &surf->visibility;
        return HW_TYPE_1I | clean;
    case SURF_PRIMSIZE :
        if( surf->primSize == 1.0f )            clean = HW_TYPE_CLEAN;
        *val = &surf->primSize;
        return HW_TYPE_1F | clean;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

/* The class which implements all of this... */
static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        STR_SURF

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwSurfaceDef = {
        0,              /* Parent - NULL */
        "hwSurface",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwSurface = &hwSurfaceDef;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;
    hwSurfaceType
        surf;                   /* The surface itself */
} Surf;


static hwObject create( hwObject parent )
{
    Surf
        *result;

    LOG_ENTRY
    if( parent != hwSurface )   return 0;
    result = malloc( sizeof(Surf) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }

    result->hdr  = *hwSurface;
    result->refCount = 1;

    result->hdr.parent = hwSurface;
    hwDefaultSurf( &result->surf );

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Surf
        *surf = (Surf *)obj;

    LOG_ENTRY
    surf->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Surf
        *surf = (Surf *)obj;
    int
        i, n;

    LOG_ENTRY
    if( --surf->refCount > 0 )  return;
    n = surf->surf.numTextures;
    for( i = 0; i < n; i++ ) {
        surf->surf.textures[i]->destroy( surf->surf.textures[i] );
    }
    free( surf );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    int
        n;
    Surf
        *surf = (Surf *)obj;

    LOG_ENTRY
    n = hwLookup( prop, __hwIntSurfTab );
    if( n >= 0 ) {
        (void)__hwIntSurfModify( &surf->surf, n, type, val );
    }
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    int
        n;
    Surf
        *surf = (Surf *)obj;

    LOG_ENTRY
    n = hwLookup( prop, __hwIntSurfTab );
    if( n == SURF_SURFACE ) {
        *val = &surf->surf;
        return 1;
    }
    else if( n >= 0 ) {
        return __hwIntSurfInquire( &surf->surf, n, val );
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Surf
        *surf = (Surf *)obj;

    LOG_ENTRY
    hwSurfAttrs( &surf->surf );
    LOG_EXIT
}

/*** EOF hwSurface.c ***/
