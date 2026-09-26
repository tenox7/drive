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

/* Internal definitions of cone-related flags */
#define HW_GRAPHN       1
#define HW_GRAPHM       2
#define HW_RADIUS       3
#define HW_HEIGHT       4
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
        /* Specific to cone */
        hwStrGraphN, hwStrGraphM, hwStrRadius, hwStrHeight,
        hwStrLonRange, hwStrOptFlags,
        hwStrHasNormals, hwStrResolution,
        hwStrHasTangent,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwConeStruct = {
        0,              /* Parent - NULL */
        "hwCone",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwCone = &hwConeStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty,                  /* Is this cone cooked? */
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
        height, rad[2], lon[2], /* Creation parms */
        BBox[6],                /* Bounding box */
        resolution,             /* Resolution magnifier */
        *data;
} Cone;

/* The hash table for cone strings */
static void *coneTab = 0;

static hwObject create( hwObject proto )
{
    Cone
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwCone )       return 0;

    HW_HASH_SETUP(coneTab, 16)
        t = coneTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrRadius,       HW_RADIUS,        t );
        HW_INSERT( hwStrHeight,       HW_HEIGHT,        t );
        HW_INSERT( hwStrLonRange,     HW_LON_RANGE,     t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrHasNormals,   HW_NORMALS,       t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrHasTangent,   HW_TANGENT,       t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Cone) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwCone;
    result->refCount = 1;

    result->graphN = 9; result->graphM = 17;
    result->dl = -1;
    result->dataFlags = HW_DATA_NORMALS;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity cone */
    result->height = 1.0;
    result->rad[0] = 1.0;
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
    Cone
        *cone = (Cone *)obj;

    LOG_ENTRY
    cone->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Cone
        *cone = (Cone *)obj;;

    LOG_ENTRY
    if( --cone->refCount > 0 )  return;
    if( cone->dl >= 0 ) {
        __hwIntDestroyList( cone->dl );
    }
    if( cone->data )    free( cone->data );
    free( cone );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Cone
        *cone = (Cone *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, coneTab ) ) {
    case HW_RADIUS :
        switch( type ) {
        case HW_TYPE_1I :
            cone->rad[0] = cone->rad[1] = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            cone->rad[0] = cone->rad[1] = *(hwFloat *)val;
            break;
        case HW_TYPE_2F :
            cone->rad[0] = ((hwFloat *)val)[0];
            cone->rad[1] = ((hwFloat *)val)[1];
            break;
        default :
            goto BadType;
        }
        break;
    case HW_HEIGHT :
        switch( type ) {
        case HW_TYPE_1I :
            cone->height = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            cone->height = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        cone->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        cone->graphN = *(hwInt32 *)val;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        cone->graphM = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        cone->optFlags = *(hwInt32 *)val;
        break;
    case HW_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   cone->dataFlags |= HW_DATA_NORMALS;
        else                    cone->dataFlags &= ~HW_DATA_NORMALS;
        break;
    case HW_TANGENT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   cone->dataFlags |= HW_DATA_TANGENT;
        else                    cone->dataFlags &= ~HW_DATA_TANGENT;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &cone->surf, n, type, val );
        break;
    case HW_LON_RANGE :
        if( type != HW_TYPE_2F )        goto BadType;
        cone->lon[0] = ((hwFloat *)val)[0];
        cone->lon[1] = ((hwFloat *)val)[1];
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            dirty = __hwIntSurfModify( &cone->surf, n, type, val );
            if( cone->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &cone->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }

    if( dirty ) cone->dirty = 1;
    return;

BadType:
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    LOG_EXIT
}

static void cook( hwDisplay disp, Cone *cone, hwFloat **result )
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

    /* Clean up old data lying around */
    if( cone->dl >= 0 ) {
        __hwIntDestroyList( cone->dl );
    }
    cone->dl = -1;
    if( cone->data )    free( cone->data );
    cone->data = 0;

    /* Allocate data... */
    dataFlags = cone->dataFlags;

    dataFlags |= hwTextureFlags( &cone->surf );

    n = cone->graphN;   m = cone->graphM;

    n = (n * cone->resolution + 0.5f); if( n < 2 ) n = 2;
    m = (m * cone->resolution + 0.5f); if( m < 4 ) m = 4;
    if( cone->graphN & 1 ) n |= 1;
    if( cone->graphM & 1 ) m |= 1;
    cone->cookedNM[0] = n;
    cone->cookedNM[1] = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    /* Tesselate cone... */
    __hwIntMakeCone( n, m, dataFlags,
                    cone->rad[0], cone->rad[1], cone->height,
                    data,
                    cone->lon[0] deg, cone->lon[1] deg,
                    cone->surf.color );

    /* Transform data */
    __hwIntTransformData( &cone->orient, data, n*m, dataFlags );

    /* Update the bounding box */
    cone->BBox[0] = cone->BBox[1] = cone->BBox[2] = HW_MAX_FLOAT;
    cone->BBox[3] = cone->BBox[4] = cone->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( cone->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    cone->cookedFlags = __hwIntTexturePipeline(
                                    &cone->surf,
                                    &data, &dataSize, &vn,
                                    n*m, dataFlags );
    *result = data;

ERROR :
    // Put the scratch data back
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Cone
        *cone = (Cone *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    switch( hwLookup( prop, coneTab ) ) {
    case HW_RESOLUTION :
        *val = &cone->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &cone->graphN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &cone->graphM;
        return HW_TYPE_1I;
    case HW_RADIUS :
        *val = cone->rad;
        return HW_TYPE_2F;
    case HW_HEIGHT :
        *val = &cone->height;
        return HW_TYPE_1F;
    case HW_LON_RANGE :
        *val = cone->lon;
        return HW_TYPE_2F;
    case HW_OPT_FLAGS :
        *val = &cone->optFlags;
        return HW_TYPE_1I;
    case HW_NORMALS :
        intDisp->scratchInt[0] = (cone->dataFlags & HW_DATA_NORMALS) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_TANGENT :
        intDisp->scratchInt[0] = (cone->dataFlags & HW_DATA_TANGENT) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_BBOX :
        if( cone->dirty ) cook( __hwDisp, cone, &dataPtr );
        *val = cone->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( cone->dirty ) cook( __hwDisp, cone, &dataPtr );
        *val = &cone->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = cone->data;
        if( cone->dirty || !dataPtr ) cook( __hwDisp, cone, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( cone->cookedFlags );
        vn *= cone->cookedNM[0] * cone->cookedNM[1];
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( cone->dirty ) cook( __hwDisp, cone, &dataPtr );
        *val = cone->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &cone->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &cone->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Cone
        *cone = (Cone *)obj;
    hwInt32
        n, m, vn;
    hwFloat
        *data = NULL;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( cone->dirty ) {
        cook( __hwDisp, cone, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                        &cone->surf, cone->BBox,
                        2*cone->cookedNM[0]*cone->cookedNM[1] ) )
    {
        return;
    }

    if( cone->dl >= 0 ) {
        if( !(cone->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &cone->surf );
        }
        __hwDisp->callList( __hwDisp, cone->dl );
    }
    else if( cone->data ) {
        hwSurfAttrs( &cone->surf );
        __hwDisp->drawMesh( __hwDisp, cone->data,
                        cone->cookedFlags,
                        cone->cookedNM[0], cone->cookedNM[1] );
    }
    else {
        /* Draw it... */
        n = cone->cookedNM[0];
        m = cone->cookedNM[1];

        hwSurfAttrs( &cone->surf );

        if( cone->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            cone->dl = __hwDisp->openList( __hwDisp );
            if( cone->dl < 0 ) {
                cone->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                cone->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( cone->optFlags & HW_OPT_DL_ATTRS ) {
                /* Cook surface attrs into DL */
                hwSurfAttrs( &cone->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp, data,
                                  cone->cookedFlags, n, m );

        if( cone->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, cone->dl );
        }
        else if( cone->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( cone->cookedFlags );
            cone->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( cone->data ) {
                (void)memcpy( cone->data, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                cone->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( cone->optFlags & HW_OPT_SAVED_DATA ) {
            cone->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwCone.c ***/
