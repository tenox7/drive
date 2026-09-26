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

/* Internal definitions of torus-related flags */
#define HW_GRAPHN       1
#define HW_GRAPHM       2
#define HW_RADIUS       3
#define HW_LAT_RANGE    4
#define HW_LON_RANGE    5
#define HW_OPT_FLAGS    6
#define HW_NORMALS      7
#define HW_RESOLUTION   8
#define HW_BBOX         9
#define HW_COOKED_FLAGS 10
#define HW_COOKED_DATA  11
#define HW_COOKED_COUNTS 12
#define HW_TANGENT      13
#define HW_DIRTY        14

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to torus */
        hwStrGraphN, hwStrGraphM, hwStrRadius, hwStrLatRange,
        hwStrLonRange, hwStrOptFlags, hwStrHasNormals,
        hwStrHasTangent,
        hwStrResolution,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwTorusStruct = {
        0,              /* Parent - NULL */
        "hwTorus",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwTorus = &hwTorusStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this torus cooked? */
        graphN, graphM,         /* Tesselation parameters */
        cookedNM[2],
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataFlags;              /* Data format flags */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        rad[2], lat[2], lon[2], /* Creation parms */
        BBox[6],                /* Bounding box */
        resolution,             /* Resolution magnifier */
        *data;
} Torus;

/* The hash table for torus strings */
static void *torusTab = 0;

static hwObject create( hwObject proto )
{
    Torus
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwTorus )      return 0;

    HW_HASH_SETUP(torusTab, 24)
        t = torusTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrRadius,       HW_RADIUS,        t );
        HW_INSERT( hwStrLatRange,     HW_LAT_RANGE,     t );
        HW_INSERT( hwStrLonRange,     HW_LON_RANGE,     t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrHasNormals,   HW_NORMALS,       t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrHasTangent,   HW_TANGENT,       t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Torus) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwTorus;
    result->refCount = 1;

    result->graphN = 17; result->graphM = 17;
    result->dl = -1;
    result->dataFlags = HW_DATA_NORMALS;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity torus */
    result->rad[0] = 0.5;
    result->rad[1] = 1.0;
    result->lat[0] = 0.0;
    result->lat[1] = 360.0;
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
    Torus
        *torus = (Torus *)obj;

    LOG_ENTRY
    torus->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Torus
        *torus = (Torus *)obj;;

    LOG_ENTRY
    if( --torus->refCount > 0 ) return;
    if( torus->dl >= 0 ) {
        __hwIntDestroyList( torus->dl );
    }
    if( torus->data )   free( torus->data );
    free( torus );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Torus
        *torus = (Torus *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, torusTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        torus->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        torus->graphN = *(hwInt32 *)val;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        torus->graphM = *(hwInt32 *)val;
        break;
    case HW_RADIUS :
        switch( type ) {
        case HW_TYPE_1I :
            torus->rad[0] = *(hwInt32 *)val * 0.5;
            torus->rad[1] = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            torus->rad[0] = *(hwFloat *)val * 0.5;
            torus->rad[1] = *(hwFloat *)val;
            break;
        case HW_TYPE_2F :
            torus->rad[0] = ((hwFloat *)val)[0];
            torus->rad[1] = ((hwFloat *)val)[1];
            break;
        default :
            goto BadType;
        }
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        torus->optFlags = *(hwInt32 *)val;
        break;
    case HW_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   torus->dataFlags |= HW_DATA_NORMALS;
        else                    torus->dataFlags &= ~HW_DATA_NORMALS;
        break;
    case HW_TANGENT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   torus->dataFlags |= HW_DATA_TANGENT;
        else                    torus->dataFlags &= ~HW_DATA_TANGENT;
        break;
    case HW_LAT_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        torus->lat[0] = ((hwFloat *)val)[0];
        torus->lat[1] = ((hwFloat *)val)[1];
        break;
    case HW_LON_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        torus->lon[0] = ((hwFloat *)val)[0];
        torus->lon[1] = ((hwFloat *)val)[1];
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &torus->surf, n, type, val );
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &torus->surf, n, type, val );
            if( torus->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &torus->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) torus->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Torus *torus, hwFloat **result )
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

    *result = 0;
    if( torus->dl >= 0 ) {
        __hwIntDestroyList( torus->dl );
    }
    torus->dl = -1;
    if( torus->data )   free( torus->data );
    torus->data = 0;

    /* Allocate data... */
    dataFlags = torus->dataFlags;
    dataFlags |= hwTextureFlags( &torus->surf );

    n = torus->graphN;  m = torus->graphM;

    n = (n * torus->resolution + 0.5f); if( n < 4 ) n = 4;
    m = (m * torus->resolution + 0.5f); if( m < 4 ) m = 4;
    if( torus->graphN & 1 ) n |= 1;
    if( torus->graphM & 1 ) m |= 1;
    torus->cookedNM[0] = n;
    torus->cookedNM[1] = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate torus... */
    __hwIntMakeTorus( n, m, dataFlags, torus->rad[0], torus->rad[1],
                    data,
                    torus->lon[0] deg, torus->lon[1] deg,
                    torus->lat[0] deg, torus->lat[1] deg,
                    torus->surf.color );

    /* Transform data */
    __hwIntTransformData( &torus->orient, data, n*m, dataFlags );

    /* Update the bounding box */
    torus->BBox[0] = torus->BBox[1] = torus->BBox[2] = HW_MAX_FLOAT;
    torus->BBox[3] = torus->BBox[4] = torus->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( torus->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    torus->cookedFlags = __hwIntTexturePipeline(
                                    &torus->surf,
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
    Torus
        *torus = (Torus *)obj;
    hwInt32
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    switch( hwLookup( prop, torusTab ) ) {
    case HW_RESOLUTION :
        *val = &torus->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &torus->graphN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &torus->graphM;
        return HW_TYPE_1I;
    case HW_RADIUS :
        *val = torus->rad;
        return HW_TYPE_2F;
    case HW_LAT_RANGE :
        *val = torus->lat;
        return HW_TYPE_2F;
    case HW_LON_RANGE :
        *val = torus->lon;
        return HW_TYPE_2F;
    case HW_OPT_FLAGS :
        *val = &torus->optFlags;
        return HW_TYPE_1I;
    case HW_NORMALS :
        intDisp->scratchInt[0] = (torus->dataFlags & HW_DATA_NORMALS) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_TANGENT :
        intDisp->scratchInt[0] = (torus->dataFlags & HW_DATA_TANGENT) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_BBOX :
        if( torus->dirty ) cook( __hwDisp, torus, &dataPtr );
        *val = torus->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( torus->dirty ) cook( __hwDisp, torus, &dataPtr );
        *val = &torus->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = torus->data;
        if( torus->dirty || !dataPtr ) cook( __hwDisp, torus, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( torus->cookedFlags );
        vn *= torus->cookedNM[0] * torus->cookedNM[1];
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( torus->dirty ) cook( __hwDisp, torus, &dataPtr );
        *val = torus->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &torus->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &torus->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    __hwIntSetError( HW_ERROR_BAD_PROP );
    return 0;
}

static void draw( hwObject obj )
{
    Torus
        *torus = (Torus *)obj;
    hwFloat
        *data = NULL;
    hwInt32
        n, m, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( torus->dirty ) {
        cook( __hwDisp, torus, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &torus->surf, torus->BBox,
                    2*torus->cookedNM[0]*torus->cookedNM[1] ) )
    {
        return;
    }

    if( torus->dl >= 0 ) {
        if( !(torus->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &torus->surf );
        }
        __hwDisp->callList( __hwDisp, torus->dl );
    }
    else if( torus->data ) {
        hwSurfAttrs( &torus->surf );
        __hwDisp->drawMesh( __hwDisp,
                        torus->data, torus->cookedFlags,
                        torus->cookedNM[0], torus->cookedNM[1] );
    }
    else {
        /* Draw it... */
        n = torus->cookedNM[0];
        m = torus->cookedNM[1];

        hwSurfAttrs( &torus->surf );

        if( torus->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            torus->dl = __hwDisp->openList( __hwDisp );
            if( torus->dl < 0 ) {
                torus->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                torus->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( torus->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &torus->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp, data, torus->cookedFlags, n, m );

        if( torus->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, torus->dl );
        }
        else if( torus->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( torus->cookedFlags );
            torus->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( torus->data ) {
                (void)memcpy( torus->data, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                torus->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( torus->optFlags & HW_OPT_SAVED_DATA ) {
            torus->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwTorus.c ***/
