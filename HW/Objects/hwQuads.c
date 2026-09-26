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
#include "math.h"
#include "hw_vecmath.h"

/* Internal definitions of quad-related flags */
#define HW_DATA         1
#define HW_OPT_FLAGS    2
#define HW_GRAPHN       3
#define HW_GRAPHM       4
#define HW_BBOX         5
#define HW_COOKED_FLAGS 6
#define HW_COOKED_DATA  7
#define HW_COOKED_COUNTS 8
#define HW_DIRTY        9

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to quadgon */
        hwStrOptFlags, hwStrData, hwStrGraphN, hwStrGraphM,

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
    hwQuadStruct = {
        0,              /* Parent - NULL */
        "hwQuads",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwQuads = &hwQuadStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this quad cooked? */
        dl,                     /* Display list */
        tessN, tessM,           /* Tesselation parameters */
        meshSize,               /* Size of one mesh, in words */
        numVerts,               /* Number of vertices */
        graphN,                 /* Number of points in the quadgon */
        dataType,               /* What type is Data? */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataFlags;              /* Data format flags */
    hwOrientType
        orient;                 /* Orientation maquadx */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        BBox[6],                /* The bounding box */
        *data,                  /* User's input data */
        *cookedData;            /* Cooked data */
} Quad;

/* The hash table for quad squadngs */
static void *quadTab = 0;

static hwObject create( hwObject proto )
{
    Quad
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwQuads )      return 0;

    HW_HASH_SETUP(quadTab, 16)
        t = quadTab;
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Quad) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwQuads;
    result->refCount = 1;

    result->dl = -1;
    result->dataFlags = 0;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;
    result->tessN = result->tessM = 1;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity quad */
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
    Quad
        *quad = (Quad *)obj;

    LOG_ENTRY
    quad->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Quad
        *quad = (Quad *)obj;;

    LOG_ENTRY
    if( --quad->refCount > 0 )  return;
    if( quad->dl >= 0 ) {
        __hwIntDestroyList( quad->dl );
    }
    if( quad->data && ~(quad->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( quad->data );
    }
    if( quad->cookedData )      free( quad->cookedData );
    free( quad );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Quad
        *quad = (Quad *)obj;
    int
        dirty = 1, vn, n;

    LOG_ENTRY
    switch( hwLookup( prop, quadTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        quad->optFlags = *(hwInt32 *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        quad->tessN = *(hwInt32 *)val;
        if( quad->tessN < 1 )   quad->tessN = 1;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        quad->tessM = *(hwInt32 *)val;
        if( quad->tessM < 1 )   quad->tessM = 1;
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        vn = HW_GET_COUNT( type );
        if( quad->optFlags & HW_OPT_SAFE_USER_DATA ) {
            quad->data = (float *)val;
        }
        else {
            quad->data = realloc( quad->data, vn * sizeof(hwFloat) );
            if( !quad->data )   return;
            (void)memcpy( quad->data, val, vn * sizeof(hwFloat) );
        }
        quad->dataType = type;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &quad->surf, n, type, val );
        break;
    default :
        if( (n = hwLookup( prop, __hwIntVtxTab )) >= 0 ) {
            dirty = __hwIntVtxModify( &quad->dataFlags, n, type, val );
        }
        else if( (n = hwLookup( prop, __hwIntSurfTab )) >= 0 ) {
            dirty = __hwIntSurfModify( &quad->surf, n, type, val );
            if( quad->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        else if( (n = hwLookup( prop, __hwIntOrientTab )) >= 0 ) {
            __hwIntOrientModify( &quad->orient, n, type, val );
        }
        else {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) quad->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void InterpPoint
(
    hwFloat *p0, hwFloat *p1, hwFloat *dst, hwInt32 WPV, double frac
)
{
    double
        iFrac = 1.0 - frac;

    while( WPV-- > 0 ) {
        *dst++ = iFrac * *p0++ + frac * *p1++;
    }
}

static void cook( hwDisplay disp, Quad *quad, hwFloat **result )
{
    hwInt32
        rowSize, meshSize, newSize,
        flags,
        q, i, j,
        n, vn, count;
    hwFloat
        *src, *dst,
        *point0, *point1, *point2, *point3,
        *left, *right;
    double
        frac;
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

    if( quad->dl >= 0 ) {
        __hwIntDestroyList( quad->dl );
    }
    quad->dl = -1;
    if( quad->cookedData )      free( quad->cookedData );
    quad->cookedData = 0;

    if( !quad->data ) goto ERROR;

    /* Allocate data... */
    count = HW_GET_COUNT(quad->dataType);
    flags = quad->dataFlags;
    vn = hwCalcWPV( flags );
    if( !count || (count % (4*vn)) )    goto ERROR;
    n = count / vn;
    quad->graphN = n / 4;

    if( count > dataSize ) {
        dataSize = count;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;
    (void)memcpy( data, quad->data, count*sizeof(hwFloat) );

    /* Transform data */
    __hwIntTransformData( &quad->orient, data, n, flags );

    /* Update the bounding box */
    quad->BBox[0] = quad->BBox[1] = quad->BBox[2] = HW_MAX_FLOAT;
    quad->BBox[3] = quad->BBox[4] = quad->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( quad->BBox, data, n, vn );

    /* Tesselate it, if needed.  We do this before the TexturePipeline
     * so that we get a prettier picture but after TransformData and
     * UpdateBounds since the internal data doesn't need to be there
     * for those stages.
     */
    if( (quad->tessN > 1) || (quad->tessM > 1) ) {
        rowSize = (quad->tessM + 1) * vn;
        meshSize = quad->meshSize = rowSize * (quad->tessN + 1);
        newSize = quad->graphN * meshSize;
        if( newSize > dataSize ) {
            dataSize = newSize;
            data = realloc( data, dataSize*sizeof(hwFloat) );
            if( !data ) goto ERROR;
        }

        /* First, distribute quads appropriately.  Remember,
         * source quad goes:    quad mesh goes:
         *    0--1                        0--2
         *    |  |                        |  |
         *    3--2                        1--3
         */
        src = data + 4*(quad->graphN - 1) * vn;
        dst = data + (quad->graphN - 1) * meshSize;
        for( q = 0; q < quad->graphN; q++ ) {
            point0 = dst;
            point1 = dst + rowSize - vn;
            point2 = dst + meshSize - rowSize;
            point3 = dst + meshSize - vn;
            (void)memcpy( point3, src + 2*vn, vn*sizeof(float) );
            (void)memcpy( point2, src + 1*vn, vn*sizeof(float) );
            (void)memcpy( point1, src + 3*vn, vn*sizeof(float) );
            if( point0 != src ) {
                (void)memcpy( point0, src   , vn*sizeof(float) );
            }
            src -= 4*vn;
            dst -= meshSize;
        }

        /* Now, interpolate */
        dst = data;
        for( q = 0; q < quad->graphN; q++ ) {
            point0 = dst;
            point1 = dst + rowSize - vn;
            point2 = dst + meshSize - rowSize;
            point3 = dst + meshSize - vn;
            for( i = 0; i <= quad->tessN; i++ ) {
                left = dst;     right = dst + rowSize - vn;
                if( (i > 0) && (i < quad->tessN) ) {
                    /* Interpolate left & right columns */
                    frac = i / (double)quad->tessN;
                    InterpPoint( point0, point2, left, vn, frac );
                    InterpPoint( point1, point3, right, vn, frac );
                }

                /* Skip first point */
                dst += vn;

                /* Interpolate middle points */
                for( j = 1; j < quad->tessM; j++ ) {
                    frac = j / (double)quad->tessM;
                    InterpPoint( left, right, dst, vn, frac );
                    dst += vn;
                }

                /* Skip last point */
                dst += vn;
            }
        }

        /* Recalc # of vertices */
        n = quad->graphN * (quad->tessN + 1) * (quad->tessM + 1);
    }

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    quad->cookedFlags = __hwIntTexturePipeline(
                                    &quad->surf,
                                    &data, &dataSize, &vn,
                                    n, flags );

    /* Gotta recalc mesh size */
    if( (quad->tessN > 1) || (quad->tessM > 1) ) {
        rowSize = (quad->tessM + 1) * vn;
        quad->meshSize = rowSize * (quad->tessN + 1);
    }

    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Quad
        *quad = (Quad *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, quadTab ) ) {
    case HW_DATA :
        *val = quad->data;
        return quad->dataType;
    case HW_OPT_FLAGS :
        *val = &quad->optFlags;
        return HW_TYPE_1I;
    case HW_GRAPHN :
        *val = &quad->tessN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &quad->tessM;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( quad->dirty ) cook( __hwDisp, quad, &dataPtr );
        *val = quad->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        *val = &quad->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = quad->cookedData;
        if( quad->dirty || !dataPtr ) cook( __hwDisp, quad, &dataPtr );
        *val = dataPtr;

        /* Figure out size of cooked data array */
        vn = hwCalcWPV( quad->cookedFlags );
        if( (quad->tessN > 1) || (quad->tessM > 1) ) {
            vn *= quad->graphN*(quad->tessN+1)*(quad->tessM+1);
        }
        else {
            vn *= quad->graphN * 4;
        }
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( quad->dirty ) cook( __hwDisp, quad, &dataPtr );
        dataPtr[0] = quad->graphN;
        if( (quad->tessN > 1) || (quad->tessM > 1) ) {
            dataPtr[1] = quad->tessN+1;
            dataPtr[2] = quad->tessM+1;
            return HW_MAKE_TYPE(HW_TYPE_INT,3);
        }
        else {
            dataPtr[1] = 4;
            return HW_MAKE_TYPE(HW_TYPE_INT,2);
        }
    default :
        n = hwLookup( prop, __hwIntVtxTab );
        if( n >= 0 ) {
            return __hwIntVtxInquire( &quad->dataFlags, n, val );
        }
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &quad->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &quad->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}


static void draw( hwObject obj )
{
    Quad
        *quad = (Quad *)obj;
    hwFloat
        *src, *data = NULL;
    hwInt32
        i, n, meshSize, rowSize;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !quad->data )   return;

    if( quad->dirty ) {
        cook( __hwDisp, quad, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                        &quad->surf, quad->BBox, 4*quad->graphN ) )
    {
        return;
    }

    if( quad->dl >= 0 ) {
        if( !(quad->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &quad->surf );
        }
        __hwDisp->callList( __hwDisp, quad->dl );
    }
    else if( quad->cookedData ) {
        hwSurfAttrs( &quad->surf );
        if( (quad->tessM > 1) || (quad->tessN > 1) ) {
            src = quad->cookedData;
            meshSize = quad->meshSize;
            for( i = 0; i < quad->graphN; i++ ) {
                __hwDisp->drawMesh( __hwDisp,
                        src, quad->cookedFlags, quad->tessN+1, quad->tessM+1 );
                src += meshSize;
            }
        }
        else {
            __hwDisp->drawQuads( __hwDisp,
                        quad->cookedData, quad->cookedFlags,quad->graphN );
        }
    }
    else {
        /* Draw it... */
        hwSurfAttrs( &quad->surf );

        if( quad->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            quad->dl = __hwDisp->openList( __hwDisp );
            if( quad->dl < 0 ) {
                quad->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                quad->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( quad->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &quad->surf );
            }
        }

        if( (quad->tessN > 1) || (quad->tessM > 1) ) {
            src = data;
            meshSize = quad->meshSize;
            for( i = 0; i < quad->graphN; i++ ) {
                __hwDisp->drawMesh( __hwDisp,
                        src, quad->cookedFlags, quad->tessN+1, quad->tessM+1 );
                src += meshSize;
            }
            n = quad->graphN * meshSize;
        }
        else {
            __hwDisp->drawQuads( __hwDisp,
                                        data, quad->cookedFlags, quad->graphN );
            n = quad->graphN * 4 * hwCalcWPV( quad->cookedFlags );
        }

        if( quad->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, quad->dl );
        }
        else if( quad->optFlags & HW_OPT_CACHE_DATA ) {
            quad->cookedData = malloc( n*sizeof(hwFloat) );
            if( quad->cookedData ) {
                (void)memcpy( quad->cookedData, data, n*sizeof(hwFloat) );
            }
            else {
                quad->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( quad->optFlags
                & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS|HW_OPT_CACHE_DATA))
        {
            quad->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwQuads.c ***/
