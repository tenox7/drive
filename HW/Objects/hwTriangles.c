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

/* Internal definitions of tri-related flags */
#define HW_DATA         1
#define HW_OPT_FLAGS    2
#define HW_COOKED_FLAGS 3
#define HW_COOKED_DATA  4
#define HW_BBOX         5
#define HW_INDICES      6
#define HW_DIRTY        7

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to trigon */
        hwStrOptFlags, hwStrData, hwStrIndices,

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
    hwTriStruct = {
        0,              /* Parent - NULL */
        "hwTriangles",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwTriangles = &hwTriStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this tri cooked? */
        dl,                     /* Display list */
        numVerts,               /* Number of vertices */
        numTris,                /* Number of triangles */
        graphN,                 /* Number of points in the trigon */
        dataType,               /* What type is Data? */
        indicesType,            /* What type is indices? */
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
    hwInt32
        *indices;               /* If indexed... */
} Tri;

/* The hash table for tri strings */
static void *triTab = 0;

static hwObject create( hwObject proto )
{
    Tri
        *result;

    LOG_ENTRY
    if( proto != hwTriangles )  return 0;

    HW_HASH_SETUP(triTab, 16)
        HW_INSERT( hwStrData,        HW_DATA,         triTab );
        HW_INSERT( hwStrOptFlags,    HW_OPT_FLAGS,    triTab );
        HW_INSERT( hwStrCookedFlags, HW_COOKED_FLAGS, triTab );
        HW_INSERT( hwStrCookedData,  HW_COOKED_DATA,  triTab );
        HW_INSERT( hwStrBounds,      HW_BBOX,         triTab );
        HW_INSERT( hwStrIndices,     HW_INDICES,      triTab );
        HW_INSERT( hwStrDirty,       HW_DIRTY,        triTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Tri) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwTriangles;
    result->refCount = 1;

    result->dl = -1;
    result->dataFlags = 0;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity tri */
    result->data = 0;
    result->indices = 0;
    result->graphN = 0;
    result->dataType = 0;
    result->indicesType = 0;
    result->cookedData = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Tri
        *tri = (Tri *)obj;

    LOG_ENTRY
    tri->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Tri
        *tri = (Tri *)obj;;

    LOG_ENTRY
    if( --tri->refCount > 0 )   return;
    if( tri->dl >= 0 ) {
        __hwIntDestroyList( tri->dl );
    }
    if( tri->cookedData )       free( tri->cookedData );
    if( tri->data && !(tri->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( tri->data );
    }
    if( tri->indices && !(tri->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( tri->indices );
    }
    free( tri );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Tri
        *tri = (Tri *)obj;
    int
        dirty = 1, vn, n;

    LOG_ENTRY
    switch( hwLookup( prop, triTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        tri->optFlags = *(hwInt32 *)val;
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        vn = HW_GET_COUNT( type );
        if( tri->optFlags & HW_OPT_SAFE_USER_DATA ) {
            tri->data = (float *)val;
        }
        else {
            tri->data = realloc( tri->data, vn * sizeof(hwFloat) );
            if( !tri->data )    return;
            (void)memcpy( tri->data, val, vn * sizeof(hwFloat) );
        }
        tri->dataType = type;
        break;
    case HW_INDICES :
        if( HW_GET_BASE(type) != HW_TYPE_INT )          goto BadType;
        vn = HW_GET_COUNT( type );
        if( tri->optFlags & HW_OPT_SAFE_USER_DATA ) {
            tri->indices = (hwInt32 *)val;
        }
        else {
            tri->indices = realloc( tri->indices, vn * sizeof(hwInt32) );
            if( !tri->indices )   return;
            (void)memcpy( tri->indices, val, vn * sizeof(hwInt32) );
        }
        tri->indicesType = type;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &tri->surf, n, type, val );
        break;
    default :
        if( (n = hwLookup( prop, __hwIntVtxTab )) >= 0 ) {
            dirty = __hwIntVtxModify( &tri->dataFlags, n, type, val );
        }
        else if( (n = hwLookup( prop, __hwIntSurfTab )) >= 0 ) {
            dirty = __hwIntSurfModify( &tri->surf, n, type, val );
            if( tri->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        else if( (n = hwLookup( prop, __hwIntOrientTab )) >= 0 ) {
            __hwIntOrientModify( &tri->orient, n, type, val );
        }
        else {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) tri->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Tri *tri, hwFloat **result )
{
    hwInt32
        n, vn, count, numVerts;
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

    if( tri->dl >= 0 ) {
        __hwIntDestroyList( tri->dl );
    }
    tri->dl = -1;
    if( tri->cookedData )       free( tri->cookedData );
    tri->cookedData = 0;

    *result = 0;
    if( !tri->data ) goto ERROR;

    /* Allocate data... */
    count = HW_GET_COUNT(tri->dataType);
    vn = hwCalcWPV( tri->dataFlags );
    if( !count ) goto ERROR;
    numVerts = count / vn;

    if( tri->indices ) {
        n = HW_GET_COUNT(tri->indicesType);
        if( n % 3 ) {
            goto ERROR;
        }
        tri->graphN = n / 3;
    }
    else {
        if( count % (3*vn) ) {
            goto ERROR;
        }
        tri->graphN = numVerts/3;
    }

    if( count > dataSize ) {
        dataSize = count;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;
    (void)memcpy( data, tri->data, count*sizeof(hwFloat) );

    /* Transform data */
    __hwIntTransformData( &tri->orient, data, numVerts, tri->dataFlags );

    /* Update the bounding box */
    tri->BBox[0] = tri->BBox[1] = tri->BBox[2] = HW_MAX_FLOAT;
    tri->BBox[3] = tri->BBox[4] = tri->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( tri->BBox, data, numVerts, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    tri->cookedFlags = __hwIntTexturePipeline(
                                    &tri->surf,
                                    &data, &dataSize, &vn,
                                    numVerts, tri->dataFlags );
    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Tri
        *tri = (Tri *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, triTab ) ) {
    case HW_DATA :
        *val = tri->data;
        return tri->dataType;
    case HW_INDICES :
        *val = tri->indices;
        return tri->indicesType;
    case HW_OPT_FLAGS :
        *val = &tri->optFlags;
        return HW_TYPE_1I;
    case HW_COOKED_FLAGS :
        *val = &tri->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        *val = tri->cookedData;
        /* Figure out how many triangles */
        vn = hwCalcWPV( tri->dataFlags );
        n = HW_GET_COUNT(tri->dataType) / vn;

        /* Figure out final size of cooked data */
        vn = hwCalcWPV( tri->cookedFlags );
        vn *= n;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_BBOX :
        if( tri->dirty ) cook( __hwDisp, tri, &dataPtr );
        *val = tri->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    default :
        n = hwLookup( prop, __hwIntVtxTab );
        if( n >= 0 ) {
            return __hwIntVtxInquire( &tri->dataFlags, n, val );
        }
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &tri->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &tri->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    __hwIntSetError( HW_ERROR_BAD_PROP );
    return 0;
}

static void draw( hwObject obj )
{
    Tri
        *tri = (Tri *)obj;
    hwFloat
        *data = NULL;
    hwInt32
        n, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !tri->data )    return;

    if( tri->dirty ) {
        cook( __hwDisp, tri, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &tri->surf, tri->BBox,
                    3*tri->graphN ) )
    {
        return;
    }

    if( tri->dl >= 0 ) {
        if( !(tri->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &tri->surf );
        }
        __hwDisp->callList( __hwDisp, tri->dl );
    }
    else if( tri->cookedData ) {
        hwSurfAttrs( &tri->surf );
        __hwDisp->drawIndexedTriangles( __hwDisp,
                        tri->cookedData, 3*tri->graphN, tri->cookedFlags,
                        tri->indices, tri->graphN );
    }
    else {
        /* Draw it... */
        hwSurfAttrs( &tri->surf );

        if( tri->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            tri->dl = __hwDisp->openList( __hwDisp );
            if( tri->dl < 0 ) {
                tri->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                tri->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( tri->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &tri->surf );
            }
        }

        __hwDisp->drawIndexedTriangles( __hwDisp,
                                data, 3*tri->graphN, tri->cookedFlags,
                                tri->indices, tri->graphN );

        if( tri->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, tri->dl );
        }
        else if( tri->optFlags & HW_OPT_CACHE_DATA ) {
            n = 3*tri->graphN;
            vn = hwCalcWPV( tri->cookedFlags );

            tri->cookedData = malloc( n*vn*sizeof(hwFloat) );
            if( tri->cookedData ) {
                (void)memcpy( tri->cookedData, data, n*vn*sizeof(hwFloat) );
            }
            else {
                tri->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( tri->optFlags & HW_OPT_SAVED_DATA ) {
            tri->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwTriangles.c ***/
