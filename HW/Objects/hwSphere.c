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

/* Internal definitions of sphere-related flags */
#define HW_GRAPHN       1
#define HW_GRAPHM       2
#define HW_RADIUS       3
#define HW_LAT_RANGE    4
#define HW_LON_RANGE    5
#define HW_OPT_FLAGS    7
#define HW_RESOLUTION   8
#define HW_NORMALS      9
#define HW_BBOX         10
#define HW_COOKED_FLAGS 11
#define HW_COOKED_DATA  12
#define HW_COOKED_COUNTS 13
#define HW_TANGENT      14
#define HW_DIRTY        15

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to sphere */
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
    hwSphereStruct = {
        0,              /* Parent - NULL */
        "hwSphere",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwSphere = &hwSphereStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this sphere cooked? */
        graphN, graphM,         /* Tesselation parameters */
        cookedNM[2],            /* Cooked tess. parameters */
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataFlags;              /* Data format flags */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        radius, lat[2], lon[2], /* Creation parms */
        resolution,             /* Resolution magnifier */
        BBox[6],                /* Bounding box */
        *data;                  /* Cooked data */
} Sphere;

/* The hash table for sphere strings */
static void *sphereTab = 0;

static hwObject create( hwObject proto )
{
    Sphere
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwSphere )     return 0;

    HW_HASH_SETUP(sphereTab, 24)
        t = sphereTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrRadius,       HW_RADIUS,        t );
        HW_INSERT( hwStrLatRange,     HW_LAT_RANGE,     t );
        HW_INSERT( hwStrLonRange,     HW_LON_RANGE,     t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrHasNormals,   HW_NORMALS,       t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrHasTangent,   HW_TANGENT,       t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Sphere) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwSphere;
    result->refCount = 1;

    result->graphN = 9; result->graphM = 17;
    result->dl = -1;
    result->dataFlags = HW_DATA_NORMALS;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity sphere */
    result->radius = 1.0;
    result->lat[0] = -90.0;
    result->lat[1] = 90.0;
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
    Sphere
        *sphere = (Sphere *)obj;

    LOG_ENTRY
    sphere->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Sphere
        *sphere = (Sphere *)obj;;

    LOG_ENTRY
    if( --sphere->refCount > 0 )        return;
    if( sphere->dl >= 0 ) {
        __hwIntDestroyList( sphere->dl );
    }
    if( sphere->data )  free( sphere->data );
    free( sphere );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Sphere
        *sphere = (Sphere *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, sphereTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        sphere->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        sphere->graphN = *(hwInt32 *)val;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        sphere->graphM = *(hwInt32 *)val;
        break;
    case HW_RADIUS :
        switch( type ) {
        case HW_TYPE_1I :
            sphere->radius = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            sphere->radius = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        sphere->optFlags = *(hwInt32 *)val;
        break;
    case HW_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   sphere->dataFlags |= HW_DATA_NORMALS;
        else                    sphere->dataFlags &= ~HW_DATA_NORMALS;
        break;
    case HW_TANGENT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   sphere->dataFlags |= HW_DATA_TANGENT;
        else                    sphere->dataFlags &= ~HW_DATA_TANGENT;
        break;
    case HW_LAT_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        sphere->lat[0] = ((hwFloat *)val)[0];
        sphere->lat[1] = ((hwFloat *)val)[1];
        break;
    case HW_LON_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        sphere->lon[0] = ((hwFloat *)val)[0];
        sphere->lon[1] = ((hwFloat *)val)[1];
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &sphere->surf, n, type, val );
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &sphere->surf, n, type, val );
            if( sphere->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &sphere->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) sphere->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Sphere *sphere, hwFloat **result )
{
    hwInt32
        dataFlags,
        n, m, vn;
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

    if( sphere->dl >= 0 ) {
        __hwIntDestroyList( sphere->dl );
    }
    sphere->dl = -1;
    if( sphere->data )  free( sphere->data );
    sphere->data = 0;

    /* Allocate data... */
    dataFlags = sphere->dataFlags;

    dataFlags |= hwTextureFlags( &sphere->surf );

    n = sphere->graphN; m = sphere->graphM;

    n = (n * sphere->resolution + 0.5f); if( n < 3 ) n = 3;
    m = (m * sphere->resolution + 0.5f); if( m < 4 ) m = 4;
    if( sphere->graphN & 1 ) n |= 1;
    if( sphere->graphM & 1 ) m |= 1;
    sphere->cookedNM[0] = n;
    sphere->cookedNM[1] = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate sphere... */
    __hwIntMakeSphere( n, m, dataFlags, sphere->radius, data,
                    sphere->lat[0] deg, sphere->lat[1] deg,
                    sphere->lon[0] deg, sphere->lon[1] deg,
                    sphere->surf.color );

    /* Transform data */
    __hwIntTransformData( &sphere->orient, data, n*m, dataFlags );

    /* Update the bounding box */
    sphere->BBox[0] = sphere->BBox[1] = sphere->BBox[2] = HW_MAX_FLOAT;
    sphere->BBox[3] = sphere->BBox[4] = sphere->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( sphere->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    sphere->cookedFlags = __hwIntTexturePipeline(
                                    &sphere->surf,
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
    Sphere
        *sphere = (Sphere *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    switch( hwLookup( prop, sphereTab ) ) {
    case HW_RESOLUTION :
        *val = &sphere->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &sphere->graphN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &sphere->graphM;
        return HW_TYPE_1I;
    case HW_RADIUS :
        *val = &sphere->radius;
        return HW_TYPE_1F;
    case HW_LAT_RANGE :
        *val = sphere->lat;
        return HW_TYPE_2F;
    case HW_LON_RANGE :
        *val = sphere->lon;
        return HW_TYPE_2F;
    case HW_OPT_FLAGS :
        *val = &sphere->optFlags;
        return HW_TYPE_1I;
    case HW_NORMALS :
        intDisp->scratchInt[0] = (sphere->dataFlags & HW_DATA_NORMALS) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_TANGENT :
        intDisp->scratchInt[0] = (sphere->dataFlags & HW_DATA_TANGENT) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_BBOX :
        if( sphere->dirty ) cook( __hwDisp, sphere, &dataPtr );
        *val = sphere->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( sphere->dirty ) cook( __hwDisp, sphere, &dataPtr );
        *val = &sphere->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = sphere->data;
        if( sphere->dirty || !dataPtr ) cook( __hwDisp, sphere, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( sphere->cookedFlags );
        vn *= sphere->cookedNM[0] * sphere->cookedNM[1];
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( sphere->dirty ) cook( __hwDisp, sphere, &dataPtr );
        *val = sphere->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &sphere->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &sphere->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    __hwIntSetError( HW_ERROR_BAD_PROP );
    return 0;
}

static void draw( hwObject obj )
{
    Sphere
        *sphere = (Sphere *)obj;
    hwInt32
        n, m, vn;
    hwFloat
        *data = NULL;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( sphere->dirty ) {
        cook( __hwDisp, sphere, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &sphere->surf, sphere->BBox,
                    2*sphere->cookedNM[0]*sphere->cookedNM[1] ) )
    {
        return;
    }

    if( sphere->dl >= 0 ) {
        if( !(sphere->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &sphere->surf );
        }
        __hwDisp->callList( __hwDisp, sphere->dl );
    }
    else if( sphere->data ) {
        hwSurfAttrs( &sphere->surf );
        __hwDisp->drawMesh( __hwDisp,
                        sphere->data, sphere->cookedFlags,
                        sphere->cookedNM[0], sphere->cookedNM[1] );
    }
    else {
        /* Draw it... */
        n = sphere->cookedNM[0]; m = sphere->cookedNM[1];

        hwSurfAttrs( &sphere->surf );

        if( sphere->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            sphere->dl = __hwDisp->openList( __hwDisp );
            if( sphere->dl < 0 ) {
                sphere->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                sphere->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( sphere->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &sphere->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp,
                                   data, sphere->cookedFlags, n, m );

        if( sphere->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, sphere->dl );
        }
        else if( sphere->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( sphere->cookedFlags );
            sphere->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( sphere->data ) {
                (void)memcpy( sphere->data, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                sphere->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( sphere->optFlags & HW_OPT_SAVED_DATA ) {
            sphere->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwSphere.c ***/
