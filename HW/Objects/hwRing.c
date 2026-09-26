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

/* Internal definitions of ring-related flags */
#define HW_GRAPHN       1
#define HW_GRAPHM       2
#define HW_RADIUS       3
#define HW_LON_RANGE    4
#define HW_OPT_FLAGS    5
#define HW_RESOLUTION   6
#define HW_BBOX         7
#define HW_COOKED_FLAGS 8
#define HW_COOKED_DATA  9
#define HW_COOKED_COUNTS 10
#define HW_DIRTY        11

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to ring */
        hwStrGraphN, hwStrGraphM, hwStrRadius,
        hwStrLonRange, hwStrOptFlags,
        hwStrResolution,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwRingStruct = {
        0,              /* Parent - NULL */
        "hwRing",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwRing = &hwRingStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this ring cooked? */
        graphN, graphM,         /* Tesselation parameters */
        cookedNM[2],            /* Cooked tess. params */
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags;            /* Cooked by texture pipeline */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        rad[2], lon[2],         /* Creation parms */
        resolution,             /* Resolution magnifier */
        BBox[6],                /* Bounding box */
        *data;
} Ring;

/* The hash table for ring strings */
static void *ringTab = 0;

static hwObject create( hwObject proto )
{
    Ring
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwRing )       return 0;

    HW_HASH_SETUP(ringTab, 16)
        t = ringTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrRadius,       HW_RADIUS,        t );
        HW_INSERT( hwStrLonRange,     HW_LON_RANGE,     t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Ring) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwRing;
    result->refCount = 1;

    result->graphN = 9; result->graphM = 17;
    result->dl = -1;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity ring */
    result->rad[0] = 0.5;
    result->rad[1] = 1.0;
    result->lon[0] = 0.0;
    result->lon[1] = 360.0;
    result->resolution = 1.0;
    result->data = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Ring
        *ring = (Ring *)obj;

    LOG_ENTRY
    ring->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Ring
        *ring = (Ring *)obj;;

    LOG_ENTRY
    if( --ring->refCount > 0 )  return;
    if( ring->dl >= 0 ) {
        __hwIntDestroyList( ring->dl );
    }
    if( ring->data )    free( ring->data );
    free( ring );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Ring
        *ring = (Ring *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, ringTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        ring->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        ring->graphN = *(hwInt32 *)val;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        ring->graphM = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        ring->optFlags = *(hwInt32 *)val;
        break;
    case HW_LON_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        ring->lon[0] = ((hwFloat *)val)[0];
        ring->lon[1] = ((hwFloat *)val)[1];
        break;
    case HW_RADIUS :
        switch( type ) {
        case HW_TYPE_1I :
            ring->rad[0] = ((hwInt32 *)val)[0];
            ring->rad[1] = ((hwInt32 *)val)[0];
            break;
        case HW_TYPE_1F :
            ring->rad[0] = ((hwFloat *)val)[0];
            ring->rad[1] = ((hwFloat *)val)[0];
            break;
        case HW_TYPE_2F :
            ring->rad[0] = ((hwFloat *)val)[0];
            ring->rad[1] = ((hwFloat *)val)[1];
            break;
        default :
            goto BadType;
        }
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &ring->surf, n, type, val );
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &ring->surf, n, type, val );
            if( ring->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &ring->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) ring->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Ring *ring, hwFloat **result )
{
    hwInt32
        dataFlags,
        n, m, vn;
    struct __hwDisplayInternal
        *intDisp;
    hwInt32
        dataSize;
    hwFloat
        *data;

    LOG_ENTRY

    /* Extract scratch data */
    intDisp = (struct __hwDisplayInternal *)disp;
    dataSize = intDisp->scratchDataSize;
    data = intDisp->scratchData;

    if( ring->dl >= 0 ) {
        __hwIntDestroyList( ring->dl );
    }
    ring->dl = -1;
    if( ring->data )    free( ring->data );
    ring->data = 0;

    /* Allocate data... */
    dataFlags = HW_DATA_NORMALS;

    dataFlags = hwTextureFlags( &ring->surf );

    n = ring->graphN;   m = ring->graphM;

    n = (n * ring->resolution + 0.5f); if( n < 2 ) n = 2;
    m = (m * ring->resolution + 0.5f); if( m < 4 ) m = 4;
    if( ring->graphN & 1 ) n |= 1;
    if( ring->graphM & 1 ) m |= 1;
    ring->cookedNM[0] = n;
    ring->cookedNM[1] = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate ring... */
    __hwIntMakeRing( n, m, dataFlags,
                    ring->rad[0], ring->rad[1],
                    data,
                    ring->lon[0] deg, ring->lon[1] deg,
                    ring->surf.color );

    /* Transform data */
    __hwIntTransformData( &ring->orient, data, n*m, dataFlags );

    /* Update the bounding box */
    ring->BBox[0] = ring->BBox[1] = ring->BBox[2] = HW_MAX_FLOAT;
    ring->BBox[3] = ring->BBox[4] = ring->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( ring->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    ring->cookedFlags = __hwIntTexturePipeline(
                                    &ring->surf,
                                    &data, &dataSize, &vn,
                                    n*m, dataFlags );

    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Ring
        *ring = (Ring *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, ringTab ) ) {
    case HW_RESOLUTION :
        *val = &ring->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &ring->graphN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &ring->graphM;
        return HW_TYPE_1I;
    case HW_RADIUS :
        *val = ring->rad;
        return HW_TYPE_2F;
    case HW_LON_RANGE :
        *val = ring->lon;
        return HW_TYPE_2F;
    case HW_OPT_FLAGS :
        *val = &ring->optFlags;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( ring->dirty ) cook( __hwDisp, ring, &dataPtr );
        *val = ring->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( ring->dirty ) cook( __hwDisp, ring, &dataPtr );
        *val = &ring->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = ring->data;
        if( ring->dirty || !dataPtr ) cook( __hwDisp, ring, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( ring->cookedFlags );
        vn *= ring->cookedNM[0] * ring->cookedNM[1];
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( ring->dirty ) cook( __hwDisp, ring, &dataPtr );
        *val = ring->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &ring->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &ring->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Ring
        *ring = (Ring *)obj;
    hwFloat
        *data = NULL;
    hwInt32
        n, m, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( ring->dirty ) {
        cook( __hwDisp, ring, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &ring->surf, ring->BBox,
                    2*ring->cookedNM[0]*ring->cookedNM[1] ) )
    {
        return;
    }

    if( ring->dl >= 0 ) {
        if( !(ring->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &ring->surf );
        }
        __hwDisp->callList( __hwDisp, ring->dl );
    }
    else if( ring->data ) {
        hwSurfAttrs( &ring->surf );
        __hwDisp->drawMesh( __hwDisp,
                        ring->data, ring->cookedFlags,
                        ring->cookedNM[0], ring->cookedNM[1] );
    }
    else {
        n = ring->cookedNM[0];
        m = ring->cookedNM[1];

        /* Draw it... */
        hwSurfAttrs( &ring->surf );

        if( ring->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            ring->dl = __hwDisp->openList( __hwDisp );
            if( ring->dl < 0 ) {
                ring->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                ring->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( ring->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &ring->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp,
                                   data, ring->cookedFlags, n, m );

        if( ring->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, ring->dl );
        }
        else if( ring->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( ring->cookedFlags );
            ring->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( ring->data ) {
                (void)memcpy( ring->data, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                ring->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( ring->optFlags & HW_OPT_SAVED_DATA ) {
            ring->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwRing.c ***/
