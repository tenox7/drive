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

/* Internal definitions of poly-related flags */
#define HW_DATA         1
#define HW_OPT_FLAGS    2
#define HW_COLOR        3
#define HW_PRIMSIZE     4
#define HW_BBOX         5
#define HW_RGB          6
#define HW_COOKED_FLAGS 7
#define HW_COOKED_DATA  8
#define HW_COOKED_COUNTS 9
#define HW_DIRTY        10

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to polyline */
        hwStrData, hwStrOptFlags, hwStrColor,
        hwStrHasRGB, hwStrPrimSize,

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwPolyStruct = {
        0,              /* Parent - NULL */
        "hwPolymarker",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwPolymarker = &hwPolyStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this poly cooked? */
        graphN,                 /* Tesselation parameters */
        dl,                     /* Display list */
        hasRGB,                 /* RGB/vertex? */
        dataType,               /* User data type */
        optFlags;               /* Optimization level (0 default) */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwFloat
        color[3],               /* Color - lines don't have a surface */
        BBox[6],                /* The bounding box */
        primSize,               /* Point size */
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
    if( proto != hwPolymarker ) return 0;

    HW_HASH_SETUP(polyTab, 16)
        t = polyTab;
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrColor,        HW_COLOR,         t );
        HW_INSERT( hwStrPrimSize,     HW_PRIMSIZE,      t );
        HW_INSERT( hwStrHasRGB,       HW_RGB,           t );
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
    result->hdr.parent = hwPolymarker;
    result->refCount = 1;

    result->graphN = 0;
    result->dl = -1;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultOrient( &result->orient );

    /* Default color */
    result->color[0] = result->color[1] = result->color[2] = 1.;
    result->primSize = 1.0;

    /* Identity poly */
    result->data = 0;
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
        n;

    LOG_ENTRY
    switch( hwLookup( prop, polyTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        poly->optFlags = *(hwInt32 *)val;
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT(type);
        if( poly->optFlags & HW_OPT_SAFE_USER_DATA ) {
            poly->data = (float *)val;
        }
        else {
            poly->data = realloc( poly->data, n * sizeof(hwFloat) );
            if( !poly->data )   return;
            (void)memcpy( poly->data, val, n * sizeof(hwFloat) );
        }
        poly->dataType = type;
        break;
    case HW_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        poly->color[0] = ((hwFloat *)val)[0];
        poly->color[1] = ((hwFloat *)val)[1];
        poly->color[2] = ((hwFloat *)val)[2];
        break;
    case HW_RGB :
        if( type != HW_TYPE_1B )        goto BadType;
        poly->hasRGB = *(hwInt32 *)val;
        break;
    case HW_PRIMSIZE :
        if( type != HW_TYPE_1F )        goto BadType;
        poly->primSize = *(hwFloat *)val;
        break;
    case HW_DIRTY :
        break;
    default :
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            __hwIntOrientModify( &poly->orient, n, type, val );
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    poly->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Poly *poly, hwFloat **result )
{
    hwInt32
        n, vn;
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

    if( !poly->data ) goto ERROR;

    if( poly->dl >= 0 ) {
        __hwIntDestroyList( poly->dl );
    }
    poly->dl = -1;
    if( poly->cookedData )      free( poly->cookedData );
    poly->cookedData = 0;

    vn = poly->hasRGB ? 6 : 3;

    /* Allocate data... */
    n = poly->graphN = HW_GET_COUNT( poly->dataType ) / vn;
    if( (n*vn) > dataSize ) {
        dataSize = (n*vn);
        data = realloc( data, dataSize*vn*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;
    (void)memcpy( data, poly->data, n*vn*sizeof(hwFloat) );

    /* Transform data */
    __hwIntTransformData( &poly->orient, data, n, poly->hasRGB?HW_DATA_RGB:0 );

    /* Update the bounding box */
    poly->BBox[0] = poly->BBox[1] = poly->BBox[2] = HW_MAX_FLOAT;
    poly->BBox[3] = poly->BBox[4] = poly->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( poly->BBox, data, n, vn );

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
    hwInt32
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    switch( hwLookup( prop, polyTab ) ) {
    case HW_DATA :
        *val = poly->data;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,(3*poly->graphN));
    case HW_OPT_FLAGS :
        *val = &poly->optFlags;
        return HW_TYPE_1I;
    case HW_COLOR :
        *val = poly->color;
        return HW_TYPE_3F;
    case HW_RGB :
        *val = &poly->hasRGB;
        return HW_TYPE_1B;
        break;
    case HW_PRIMSIZE :
        *val = &poly->primSize;
        return (poly->primSize == 1.) ? (HW_TYPE_CLEAN|HW_TYPE_1F) : HW_TYPE_1F;
    case HW_BBOX :
        if( poly->dirty ) cook( __hwDisp, poly, &dataPtr );
        *val = poly->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( poly->dirty ) cook( __hwDisp, poly, &dataPtr );
        intDisp->scratchInt[0] = poly->hasRGB ? HW_DATA_RGB : 0;
        *val = intDisp->scratchInt;
        return HW_MAKE_TYPE(HW_TYPE_INT,1);
    case HW_COOKED_DATA :
        dataPtr = poly->cookedData;
        if( poly->dirty || !dataPtr ) cook( __hwDisp, poly, &dataPtr );
        *val = dataPtr;
        vn = poly->hasRGB ? 6 : 3;
        vn *= poly->graphN;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( poly->dirty ) cook( __hwDisp, poly, &dataPtr );
        *val = &poly->graphN;
        return HW_MAKE_TYPE(HW_TYPE_INT,1);
    default :
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
    hwInt32
        n, vn;
    hwFloat
        *data = NULL;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    if( !poly->data ) return;

    intDisp->scratchSurf.color[0] = poly->color[0];
    intDisp->scratchSurf.color[1] = poly->color[1];
    intDisp->scratchSurf.color[2] = poly->color[2];
    intDisp->scratchSurf.primSize = poly->primSize;
    intDisp->scratchSurf.flags = HW_SURF_WIREFRAME;
    intDisp->scratchSurf.visibility = 0xFFFFFFFF;       /* HACK */

    if( poly->dirty ) {
        cook( __hwDisp, poly, &data );
        if( !data ) return;
    }

    if( !__hwDisp->boundsVisible( __hwDisp,
                    &intDisp->scratchSurf, poly->BBox, poly->graphN ) )
    {
        return;
    }

    if( poly->dl >= 0 ) {
        if( !(poly->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &intDisp->scratchSurf );
        }
        __hwDisp->callList( __hwDisp, poly->dl );
    }
    else if( poly->cookedData ) {
        hwSurfAttrs( &intDisp->scratchSurf );
        __hwDisp->drawMarkers( __hwDisp,
                        poly->cookedData, poly->hasRGB ? HW_DATA_RGB : 0,
                        poly->graphN );
    }
    else {
        if( !poly->data )       return;
        n = poly->graphN;

        /* Draw it... */
        hwSurfAttrs( &intDisp->scratchSurf );

        if( poly->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            poly->dl = __hwDisp->openList( __hwDisp );
            if( poly->dl < 0 ) {
                poly->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                poly->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( poly->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &intDisp->scratchSurf );
            }
        }

        __hwDisp->drawMarkers( __hwDisp,
                                      data, poly->hasRGB ? HW_DATA_RGB : 0, n );

        if( poly->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, poly->dl );
        }
        else if( poly->optFlags & HW_OPT_CACHE_DATA ) {
            vn = poly->hasRGB ? 6 : 3;
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

/*** EOF hwPolymarker.c ***/
