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

/* Internal definitions of surfRev-related flags */
#define HW_GRAPHN       1
#define HW_DATA         2
#define HW_LON_RANGE    3
#define HW_OPT_FLAGS    4
#define HW_NORMALS      5
#define HW_RESOLUTION   6
#define HW_BBOX         7
#define HW_COOKED_FLAGS 8
#define HW_COOKED_DATA  9
#define HW_COOKED_COUNTS 10
#define HW_TANGENT      11
#define HW_DIRTY        12

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject);

static const char
    *propList[] = {
        /* Specific to surfRev */
        hwStrGraphN, hwStrData,
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
    hwSurfRevStruct = {
        0,              /* Parent - NULL */
        "hwSurfRev",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwSurfRev = &hwSurfRevStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this surfRev cooked? */
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
        lon[2],                 /* Creation parms */
        *path,                  /* The path along which to sweep */
        BBox[6],                /* Bounding box */
        resolution,             /* Resolution magnifier */
        *data;
} SurfRev;

/* The hash table for surfRev strings */
static void *surfRevTab = 0;

static hwObject create( hwObject proto )
{
    SurfRev
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwSurfRev )    return 0;

    HW_HASH_SETUP(surfRevTab, 16)
        t = surfRevTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrLonRange,     HW_LON_RANGE,     t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrHasNormals,   HW_NORMALS,       t );
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrHasTangent,   HW_TANGENT,       t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(SurfRev) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwSurfRev;
    result->refCount = 1;

    result->graphN = 9; result->graphM = 0;
    result->dl = -1;
    result->dataFlags = HW_DATA_NORMALS;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity surfRev */
    result->lon[0] = 0.0;
    result->lon[1] = 360.0;
    result->resolution = 1.0;
    result->data = 0;
    result->path = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    SurfRev
        *surfRev = (SurfRev *)obj;

    LOG_ENTRY
    surfRev->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    SurfRev
        *surfRev = (SurfRev *)obj;

    LOG_ENTRY
    if( --surfRev->refCount > 0 )       return;
    if( surfRev->dl >= 0 ) {
        __hwIntDestroyList( surfRev->dl );
    }
    if( surfRev->data ) free( surfRev->data );
    free( surfRev );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    SurfRev
        *surfRev = (SurfRev *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, surfRevTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        surfRev->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        surfRev->graphN = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        surfRev->optFlags = *(hwInt32 *)val;
        break;
    case HW_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surfRev->dataFlags |= HW_DATA_NORMALS;
        else                    surfRev->dataFlags &= ~HW_DATA_NORMALS;
        break;
    case HW_TANGENT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   surfRev->dataFlags |= HW_DATA_TANGENT;
        else                    surfRev->dataFlags &= ~HW_DATA_TANGENT;
        break;
    case HW_LON_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        surfRev->lon[0] = ((hwFloat *)val)[0];
        surfRev->lon[1] = ((hwFloat *)val)[1];
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT(type);
        if( !n || (n & 1) )                             goto BadType;
        surfRev->graphM = n / 2;
        if( type != HW_MAKE_TYPE(HW_TYPE_FLOAT,n) )     goto BadType;
        surfRev->path = realloc( surfRev->path, n*sizeof(hwFloat) );
        (void)memcpy( surfRev->path, val, n*sizeof(hwFloat) );
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &surfRev->surf, n, type, val );
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &surfRev->surf, n, type, val );
            if( surfRev->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &surfRev->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) surfRev->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, SurfRev *surfRev, hwFloat **result )
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

    if( surfRev->dl >= 0 ) {
        __hwIntDestroyList( surfRev->dl );
    }
    surfRev->dl = -1;
    if( surfRev->data ) free( surfRev->data );
    surfRev->data = 0;

    *result = 0;
    if( !surfRev->path )        goto ERROR;

    /* Allocate data... */
    dataFlags = surfRev->dataFlags;

    dataFlags |= hwTextureFlags( &surfRev->surf );

    n = surfRev->graphN;        m = surfRev->graphM;

    n = (n * surfRev->resolution + 0.5f); if( n < 4 ) n = 4;
    if( surfRev->graphN & 1 ) n |= 1;
    surfRev->cookedNM[0] = n;
    surfRev->cookedNM[1] = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate surfRev... */
    __hwIntMakeSurfRev( n, m, dataFlags,
                    surfRev->lon[0] deg, surfRev->lon[1] deg,
                    surfRev->path, data,
                    surfRev->surf.color );
    if( dataFlags & HW_DATA_NORMALS ) {
        __hwIntSmoothMesh( data, n, m, dataFlags );
    }

    /* Transform data */
    __hwIntTransformData( &surfRev->orient, data, n*m, dataFlags );

    /* Update the bounding box */
    surfRev->BBox[0] = surfRev->BBox[1] = surfRev->BBox[2] = HW_MAX_FLOAT;
    surfRev->BBox[3] = surfRev->BBox[4] = surfRev->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( surfRev->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    surfRev->cookedFlags = __hwIntTexturePipeline(
                                    &surfRev->surf,
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
    SurfRev
        *surfRev = (SurfRev *)obj;
    hwInt32
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    switch( hwLookup( prop, surfRevTab ) ) {
    case HW_RESOLUTION :
        *val = &surfRev->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &surfRev->graphN;
        return HW_TYPE_1I;
    case HW_LON_RANGE :
        *val = surfRev->lon;
        return HW_TYPE_2F;
    case HW_OPT_FLAGS :
        *val = &surfRev->optFlags;
        return HW_TYPE_1I;
    case HW_NORMALS :
        intDisp->scratchInt[0] = (surfRev->dataFlags & HW_DATA_NORMALS) ?1:0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_TANGENT :
        intDisp->scratchInt[0] = (surfRev->dataFlags & HW_DATA_TANGENT) ?1:0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_DATA :
        if( !surfRev->path )    return 0;
        *val = surfRev->path;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,2*surfRev->graphM );
    case HW_BBOX :
        if( surfRev->dirty ) cook( __hwDisp, surfRev, &dataPtr );
        *val = surfRev->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( surfRev->dirty ) cook( __hwDisp, surfRev, &dataPtr );
        *val = &surfRev->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = surfRev->data;
        if( surfRev->dirty || !dataPtr ) cook( __hwDisp, surfRev, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( surfRev->cookedFlags );
        vn *= surfRev->cookedNM[0] * surfRev->graphM;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( surfRev->dirty ) cook( __hwDisp, surfRev, &dataPtr );
        *val = surfRev->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &surfRev->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &surfRev->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    __hwIntSetError( HW_ERROR_BAD_PROP );
    return 0;
}

static void draw( hwObject obj )
{
    SurfRev
        *surfRev = (SurfRev *)obj;
    hwFloat
        *data = NULL;
    hwInt32
        n, m, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !surfRev->path ) return;

    if( surfRev->dirty ) {
        cook( __hwDisp, surfRev, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &surfRev->surf, surfRev->BBox,
                    2*surfRev->cookedNM[0]*surfRev->graphM ) )
    {
        return;
    }

    if( surfRev->dl >= 0 ) {
        if( !(surfRev->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &surfRev->surf );
        }
        __hwDisp->callList( __hwDisp, surfRev->dl );
    }
    else if( surfRev->data ) {
        hwSurfAttrs( &surfRev->surf );
        __hwDisp->drawMesh( __hwDisp,
                        surfRev->data, surfRev->cookedFlags,
                        surfRev->cookedNM[0], surfRev->graphM );
    }
    else {
        n = surfRev->cookedNM[0];
        m = surfRev->cookedNM[1];

        /* Draw it... */
        hwSurfAttrs( &surfRev->surf );

        if( surfRev->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            surfRev->dl = __hwDisp->openList( __hwDisp );
            if( surfRev->dl < 0 ) {
                surfRev->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                surfRev->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( surfRev->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &surfRev->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp, data, surfRev->cookedFlags, n, m );

        if( surfRev->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, surfRev->dl );
        }
        else if( surfRev->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( surfRev->cookedFlags );
            surfRev->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( surfRev->data ) {
                (void)memcpy( surfRev->data, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                surfRev->optFlags = ~HW_OPT_CACHE_DATA;
            }
        }

        if( surfRev->optFlags & HW_OPT_SAVED_DATA ) {
            surfRev->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwSurfRev.c ***/
