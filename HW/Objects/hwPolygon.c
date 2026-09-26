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
#include "hw_vecmath.h"
#include <math.h>

/* Internal definitions of poly-related flags */
#define HW_DATA         1
#define HW_OPT_FLAGS    2
#define HW_BBOX         3
#define HW_COOKED_FLAGS 4
#define HW_COOKED_DATA  5
#define HW_COOKED_COUNTS 6
#define HW_DIRTY        7

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to polygon */
        hwStrOptFlags,
        hwStrData,

        /* From the vertex */
        STR_VERTEX

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwPolyStruct = {
        0,              /* Parent - NULL */
        "hwPolygon",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwPolygon = &hwPolyStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this poly cooked? */
        dl,                     /* Display list */
        graphN,                 /* Number of points in the polygon */
        dataType,               /* What type is Data? */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataFlags;              /* Data format flags */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        BBox[6],                /* The bounding box */
        *data,                  /* User's input data */
        *cookedData;            /* Cooked data */
} Poly;

/* The hash table for poly strings */
static void *polyTab = 0;

static hwObject create( hwObject proto )
{
    Poly
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwPolygon )    return 0;

    HW_HASH_SETUP(polyTab, 16)
        t = polyTab;
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Poly) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwPolygon;
    result->refCount = 1;

    result->dl = -1;
    result->dataFlags = 0;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity poly */
    result->data = 0;
    result->graphN = 0;
    result->dataType = 0;
    result->cookedData = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Poly
        *poly = (Poly *)obj;

    LOG_ENTRY
    poly->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Poly
        *poly = (Poly *)obj;;

    LOG_ENTRY
    if( --poly->refCount > 0 )  return;
    if( poly->dl >= 0 ) {
        __hwIntDestroyList( poly->dl );
    }
    if( poly->data && !(poly->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( poly->data );
    }
    if( poly->cookedData )      free( poly->cookedData );
    free( poly );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Poly
        *poly = (Poly *)obj;
    int
        dirty = 1, vn, n;

    LOG_ENTRY
    switch( hwLookup( prop, polyTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        poly->optFlags = *(hwInt32 *)val;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &poly->surf, n, type, val );
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        vn = HW_GET_COUNT( type );
        if( poly->optFlags & HW_OPT_SAFE_USER_DATA ) {
            poly->data = (float *)val;
        }
        else {
            poly->data = realloc( poly->data, vn * sizeof(hwFloat) );
            if( !poly->data )   return;
            (void)memcpy( poly->data, val, vn * sizeof(hwFloat) );
        }
        poly->dataType = type;
        break;
    default :
        if( (n = hwLookup( prop, __hwIntVtxTab )) >= 0 ) {
            dirty = __hwIntVtxModify( &poly->dataFlags, n, type, val );
        }
        else if( (n = hwLookup( prop, __hwIntSurfTab )) >= 0 ) {
            dirty = __hwIntSurfModify( &poly->surf, n, type, val );
            if( poly->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        else if( (n = hwLookup( prop, __hwIntOrientTab )) >= 0 ) {
            __hwIntOrientModify( &poly->orient, n, type, val );
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) poly->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Poly *poly, hwFloat **result )
{
    hwInt32
        i, n, vn, count,
        preNorm, postNorm;
    hwFloat
        *src, *dst,
        vecA[3], vecB[3], polyNorm[3];
    struct __hwDisplayInternal
        *intDisp;
    int
        dataSize;
    hwFloat
        *data;

    LOG_ENTRY

    /* Extract scratch data */
    intDisp = (struct __hwDisplayInternal *)disp;
    dataSize = intDisp->scratchDataSize;
    data = intDisp->scratchData;

    if( !poly->data )   goto ERROR;

    if( poly->dl >= 0 ) {
        __hwIntDestroyList( poly->dl );
    }
    poly->dl = -1;
    if( poly->cookedData )      free( poly->cookedData );
    poly->cookedData = 0;

    /* Allocate data... */
    count = HW_GET_COUNT(poly->dataType);
    vn = hwCalcWPV( poly->dataFlags );
    if( !count || (count % vn) )        goto ERROR;
    n = count / vn;
    poly->graphN = n;
    poly->cookedFlags = poly->dataFlags;

    if( poly->cookedFlags & HW_DATA_NORMALS ) {
        if( count > dataSize ) {
            dataSize = count;
            data = realloc( data, dataSize*sizeof(hwFloat) );
        }
        if( !data )     goto ERROR;

        (void)memcpy( data, poly->data, count*sizeof(hwFloat) );
    }
    else {
        count += n * 3;
        if( count > dataSize ) {
            dataSize = count;
            data = realloc( data, dataSize*sizeof(hwFloat) );
        }
        if( !data )     goto ERROR;

        /* Insert the normals by hand */
        if( poly->cookedFlags & HW_DATA_RGB )   preNorm = 6;
        else                                    preNorm = 3;
        postNorm = vn - preNorm;

        /* Better not be colinear... */
        VSUB( vecA, poly->data, poly->data + vn );
        VSUB( vecB, poly->data + 2*vn, poly->data + vn );
        VNORM( vecA, vecA );
        VNORM( vecB, vecB );
        VCROSS( polyNorm, vecB, vecA );
        VNORM( polyNorm, polyNorm );
        src = poly->data;
        dst = data;
        for( i = 0; i < n; i++ ) {
            (void)memcpy( dst, src, preNorm*sizeof(hwFloat) );
            src += preNorm; dst += preNorm;
            (void)memcpy( dst, polyNorm, 3*sizeof(hwFloat) );
            dst += 3;
            (void)memcpy( dst, src, postNorm*sizeof(hwFloat) );
            src += postNorm; dst += postNorm;
        }
        poly->cookedFlags |= HW_DATA_NORMALS;
        vn += 3;
    }

    /* Transform data */
    __hwIntTransformData( &poly->orient, data, n, poly->cookedFlags );

    /* Update the bounding box */
    poly->BBox[0] = poly->BBox[1] = poly->BBox[2] = HW_MAX_FLOAT;
    poly->BBox[3] = poly->BBox[4] = poly->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( poly->BBox, data, n, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    poly->cookedFlags = __hwIntTexturePipeline(
                                    &poly->surf,
                                    &data, &dataSize, &vn,
                                    n, poly->cookedFlags );

    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Poly
        *poly = (Poly *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, polyTab ) ) {
    case HW_DATA :
        *val = poly->data;
        return poly->dataType;
    case HW_OPT_FLAGS :
        *val = &poly->optFlags;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( poly->dirty ) cook( __hwDisp, poly, &dataPtr );
        *val = poly->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( poly->dirty ) cook( __hwDisp, poly, &dataPtr );
        *val = &poly->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = poly->cookedData;
        if( poly->dirty || !dataPtr ) cook( __hwDisp, poly, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( poly->cookedFlags );
        vn *= poly->graphN;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        *val = &poly->graphN;
        return HW_MAKE_TYPE(HW_TYPE_INT,1);
    default :
        n = hwLookup( prop, __hwIntVtxTab );
        if( n >= 0 ) {
            return __hwIntVtxInquire( &poly->dataFlags, n, val );
        }
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &poly->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &poly->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Poly
        *poly = (Poly *)obj;
    hwFloat
        *data = NULL;
    hwInt32
        n, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !poly->data ) return;

    if( poly->dirty ) {
        cook( __hwDisp, poly, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &poly->surf, poly->BBox, poly->graphN ) )
    {
        return;
    }

    if( poly->dl >= 0 ) {
        if( !(poly->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &poly->surf );
        }
        __hwDisp->callList( __hwDisp, poly->dl );
    }
    else if( poly->cookedData ) {
        hwSurfAttrs( &poly->surf );
        __hwDisp->drawPolygon( __hwDisp, poly->cookedData,
                        poly->cookedFlags, poly->graphN );
    }
    else {
        n = poly->graphN;

        /* Draw it... */
        hwSurfAttrs( &poly->surf );

        if( poly->optFlags & (HW_OPT_DL_ATTRS|HW_OPT_USE_DL) ) {
            poly->dl = __hwDisp->openList( __hwDisp );
            if( poly->dl < 0 ) {
                poly->optFlags &= ~(HW_OPT_DL_ATTRS|HW_OPT_USE_DL);
                poly->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( poly->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &poly->surf );
            }
        }

        __hwDisp->drawPolygon( __hwDisp,
                                      data, poly->cookedFlags, n );

        if( poly->optFlags & (HW_OPT_DL_ATTRS|HW_OPT_USE_DL) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, poly->dl );
        }
        else if( poly->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( poly->cookedFlags );
            poly->cookedData = malloc( n*vn*sizeof(hwFloat) );
            if( poly->cookedData ) {
                (void)memcpy( poly->cookedData, data, n*vn*sizeof(hwFloat) );
            }
            else {
                poly->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( poly->optFlags & HW_OPT_SAVED_DATA ) {
            poly->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwPolygon.c ***/
