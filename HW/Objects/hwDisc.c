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

/* Internal definitions of disc-related flags */
#define HW_GRAPHN       1
#define HW_RADIUS       2
#define HW_OPT_FLAGS    3
#define HW_RESOLUTION   4
#define HW_BBOX         5
#define HW_COOKED_FLAGS 6
#define HW_COOKED_DATA  7
#define HW_COOKED_COUNTS 8
#define HW_DIRTY        9

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to disc */
        hwStrGraphN, hwStrRadius, hwStrOptFlags,
        hwStrResolution,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwDiscStruct = {
        0,              /* Parent - NULL */
        "hwDisc",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwDisc = &hwDiscStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty,                  /* Is this disc cooked? */
        graphN,                 /* Tesselation parameters */
        cookedN,
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags;            /* Cooked by texture pipeline */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        radius,                 /* Creation parms */
        resolution,             /* Resolution magnifier */
        BBox[6],                /* Bounding box */
        *data;
} Disc;

/* The hash table for disc stdiscs */
static void *discTab = 0;

static hwObject create( hwObject proto )
{
    Disc
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwDisc )       return 0;

    HW_HASH_SETUP(discTab, 16)
        t = discTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrRadius,       HW_RADIUS,        t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Disc) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwDisc;
    result->refCount = 1;

    result->graphN = 9;
    result->dl = -1;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity disc */
    result->radius = 1.0;
    result->resolution = 1.0;
    result->data = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Disc
        *disc = (Disc *)obj;

    LOG_ENTRY
    disc->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Disc
        *disc = (Disc *)obj;;

    LOG_ENTRY
    if( --disc->refCount > 0 )  return;
    if( disc->dl >= 0 ) {
        __hwIntDestroyList( disc->dl );
    }
    if( disc->data )    free( disc->data );
    free( disc );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Disc
        *disc = (Disc *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, discTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        disc->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        disc->graphN = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        disc->optFlags = *(hwInt32 *)val;
        break;
    case HW_RADIUS :
        switch( type ) {
        case HW_TYPE_1I :
            disc->radius = ((hwInt32 *)val)[0];
            break;
        case HW_TYPE_1F :
            disc->radius = ((hwFloat *)val)[0];
            break;
        default :
            goto BadType;
        }
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &disc->surf, n, type, val );
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &disc->surf, n, type, val );
            if( disc->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &disc->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) disc->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Disc *disc, hwFloat **result )
{
    hwInt32
        dataFlags,
        n, vn;
    hwInt32
        dataSize;
    hwFloat
        *data;
    struct __hwDisplayInternal
        *intDisp;

    LOG_ENTRY

    /* Extract scratch data */
    intDisp = (struct __hwDisplayInternal *)disp;
    dataSize = intDisp->scratchDataSize;
    data = intDisp->scratchData;

    /* Clean up old data lying around */
    if( disc->dl >= 0 ) {
        __hwIntDestroyList( disc->dl );
    }
    disc->dl = -1;
    if( disc->data )    free( disc->data );
    disc->data = 0;

    /* Allocate data... */
    dataFlags = HW_DATA_NORMALS;

    dataFlags |= hwTextureFlags( &disc->surf );

    n = disc->graphN;
    n = (n * disc->resolution + 0.5f); if( n < 4 ) n = 4;
    if( disc->graphN & 1 ) n |= 1;
    disc->cookedN = n;

    vn = hwCalcWPV( dataFlags );

    if( n*vn > dataSize ) {
        dataSize = n*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate disc... */
    __hwIntMakeThinRing( n, dataFlags,
                    disc->radius,
                    data,
                    disc->surf.color );

    /* Transform data */
    __hwIntTransformData( &disc->orient, data, n, dataFlags );

    /* Update the bounding box */
    disc->BBox[0] = disc->BBox[1] = disc->BBox[2] = HW_MAX_FLOAT;
    disc->BBox[3] = disc->BBox[4] = disc->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( disc->BBox, data, n, vn );

    /* Do special texture mapping tricks: UV->RGB if not textudisc,
     * etc.
     */
    disc->cookedFlags = __hwIntTexturePipeline(
                                    &disc->surf,
                                    &data, &dataSize, &vn,
                                    n, dataFlags );

    *result = data;
ERROR :
    /* Replace scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Disc
        *disc = (Disc *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, discTab ) ) {
    case HW_RESOLUTION :
        *val = &disc->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &disc->graphN;
        return HW_TYPE_1I;
    case HW_RADIUS :
        *val = &disc->radius;
        return HW_TYPE_1F;
    case HW_OPT_FLAGS :
        *val = &disc->optFlags;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( disc->dirty ) cook( __hwDisp, disc, &dataPtr );
        *val = disc->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( disc->dirty ) cook( __hwDisp, disc, &dataPtr );
        *val = &disc->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = disc->data;
        if( disc->dirty || !dataPtr ) cook( __hwDisp, disc, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( disc->cookedFlags );
        vn *= disc->cookedN;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( disc->dirty ) cook( __hwDisp, disc, &dataPtr );
        *val = &disc->cookedN;
        return HW_MAKE_TYPE(HW_TYPE_INT,1);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &disc->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &disc->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Disc
        *disc = (Disc *)obj;
    hwInt32
        n, vn;
    hwFloat
        *data = NULL;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( disc->dirty ) {
        cook( __hwDisp, disc, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &disc->surf, disc->BBox, disc->cookedN ) )
    {
        return;
    }

    if( disc->dl >= 0 ) {
        if( !(disc->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &disc->surf );
        }
        __hwDisp->callList( __hwDisp, disc->dl );
    }
    else if( disc->data ) {
        hwSurfAttrs( &disc->surf );
        __hwDisp->drawPolygon( __hwDisp, disc->data,
                                disc->cookedFlags, disc->cookedN );
    }
    else {
        /* Draw it... */
        n = disc->cookedN;

        hwSurfAttrs( &disc->surf );

        if( disc->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            disc->dl = __hwDisp->openList( __hwDisp );
            if( disc->dl < 0 ) {
                disc->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                disc->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( disc->optFlags & HW_OPT_DL_ATTRS ) {
                /* Cook surface attributes into DL */
                hwSurfAttrs( &disc->surf );
            }
        }

        __hwDisp->drawPolygon( __hwDisp,
                                      data, disc->cookedFlags, n );

        if( disc->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, disc->dl );
        }
        else if( disc->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( disc->cookedFlags );
            disc->data = malloc( n*vn*sizeof(hwFloat) );
            if( disc->data ) {
                (void)memcpy( disc->data, data, n*vn*sizeof(hwFloat) );
            }
            else {
                disc->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( disc->optFlags & HW_OPT_SAVED_DATA ) {
            disc->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwDisc.c ***/
