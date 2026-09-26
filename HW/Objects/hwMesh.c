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

/* Internal definitions of mesh-related flags */
#define HW_GRAPHN       1
#define HW_GRAPHM       2
#define HW_DATA         3
#define HW_OPT_FLAGS    4
#define HW_RESOLUTION   5
#define HW_BBOX         6
#define HW_COOKED_FLAGS 7
#define HW_COOKED_DATA  8
#define HW_COOKED_COUNTS 9
#define HW_DIRTY        10

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to mesh */
        hwStrGraphN, hwStrGraphM, hwStrOptFlags,
        hwStrData, hwStrResolution,

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
    hwMeshStruct = {
        0,              /* Parent - NULL */
        "hwMesh",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwMesh = &hwMeshStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty,                  /* Is this mesh cooked? */
        graphN, graphM,         /* Tesselation parameters */
        cookedNM[2],            /* Cooked tess. parameters */
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataType,               /* Type of data passed in */
        dataFlags;              /* Data format flags */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwSurfaceType
        surf;                   /* Surface parameters */
    hwFloat
        BBox[6],                /* The bounding box */
        resolution,             /* Resolution magnifier */
        *data,                  /* User's input data */
        *cookedData;            /* Cooked data */
} Mesh;

/* The hash table for mesh strings */
static void *meshTab = 0;

static hwObject create( hwObject proto )
{
    Mesh
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwMesh )       return 0;

    HW_HASH_SETUP(meshTab, 16)
        t = meshTab;
        HW_INSERT( hwStrGraphN,       HW_GRAPHN,        t );
        HW_INSERT( hwStrGraphM,       HW_GRAPHM,        t );
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrResolution,   HW_RESOLUTION,    t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Mesh) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwMesh;
    result->refCount = 1;

    result->graphN = 0; result->graphM = 0;
    result->resolution = 1.0;
    result->dl = -1;
    result->dataFlags = 0;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;
    result->dataType = 0;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity mesh */
    result->data = 0;
    result->cookedData = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Mesh
        *mesh = (Mesh *)obj;

    LOG_ENTRY
    mesh->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Mesh
        *mesh = (Mesh *)obj;;

    LOG_ENTRY
    if( --mesh->refCount > 0 )  return;
    if( mesh->dl >= 0 ) {
        __hwIntDestroyList( mesh->dl );
    }
    if( mesh->data && !(mesh->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( mesh->data );
    }
    if( mesh->cookedData )      free( mesh->cookedData );
    free( mesh );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Mesh
        *mesh = (Mesh *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, meshTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        mesh->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        mesh->graphN = *(hwInt32 *)val;
        break;
    case HW_GRAPHM :
        if( type != HW_TYPE_1I )        goto BadType;
        mesh->graphM = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        mesh->optFlags = *(hwInt32 *)val;
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT ) goto BadType;
        n = HW_GET_COUNT(type);
        if( mesh->optFlags & HW_OPT_SAFE_USER_DATA ) {
            mesh->data = (float *)val;
        }
        else {
            mesh->data = realloc( mesh->data, n * sizeof(hwFloat) );
            if( !mesh->data )   return;
            (void)memcpy( mesh->data, val, n * sizeof(hwFloat) );
        }
        mesh->dataType = type;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &mesh->surf, n, type, val );
        break;
    default :
        if( (n = hwLookup( prop, __hwIntVtxTab )) >= 0 ) {
            dirty = __hwIntVtxModify( &mesh->dataFlags, n, type, val );
        }
        else if( (n = hwLookup( prop, __hwIntSurfTab )) >= 0 ) {
            dirty = __hwIntSurfModify( &mesh->surf, n, type, val );
            if( mesh->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        else if( (n = hwLookup( prop, __hwIntOrientTab )) >= 0 ) {
            __hwIntOrientModify( &mesh->orient, n, type, val );
        }
        else {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) mesh->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Mesh *mesh, hwFloat **result )
{
    hwInt32
        n, m, vn;
    int
        dataSize;
    struct __hwDisplayInternal
        *intDisp;
    hwFloat
        *data;
    hwFloat
        res;

    LOG_ENTRY

    /* Extract scratch data */
    intDisp = (struct __hwDisplayInternal *)disp;
    dataSize = intDisp->scratchDataSize;
    data = intDisp->scratchData;

    if( !mesh->data ) goto ERROR;

    if( mesh->dl >= 0 ) {
        __hwIntDestroyList( mesh->dl );
    }
    mesh->dl = -1;
    if( mesh->cookedData )      free( mesh->cookedData );
    mesh->cookedData = 0;

    vn = hwCalcWPV( mesh->dataFlags );
    n = mesh->graphN * mesh->graphM * vn;
    if( HW_GET_COUNT(mesh->dataType) != n ) goto ERROR;

    /* Allocate data... */
    n = mesh->graphN - 1;       m = mesh->graphM - 1;

    res = mesh->resolution;
    if( res < 1.0 ) res = 1.0;
    n = (n * res + 0.5f); if( n < 1 ) n = 1;
    m = (m * res + 0.5f); if( m < 1 ) m = 1;
    mesh->cookedNM[0] = ++n;
    mesh->cookedNM[1] = ++m;

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    if( (n == mesh->graphN) && (m == mesh->graphM) ) {
        (void)memcpy( data, mesh->data, n*m*vn*sizeof(hwFloat) );
    }
    else {
        __hwIntMeshInterp( mesh->data, mesh->graphN, mesh->graphM,
                            &data, n, m, mesh->dataFlags );
    }

    /* Transform data */
    __hwIntTransformData( &mesh->orient, data, n*m, mesh->dataFlags );

    /* Update the bounding box */
    mesh->BBox[0] = mesh->BBox[1] = mesh->BBox[2] = HW_MAX_FLOAT;
    mesh->BBox[3] = mesh->BBox[4] = mesh->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( mesh->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    mesh->cookedFlags = __hwIntTexturePipeline(
                                    &mesh->surf,
                                    &data, &dataSize, &vn,
                                    n*m, mesh->dataFlags );

    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}


static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Mesh
        *mesh = (Mesh *)obj;
    hwInt32
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, meshTab ) ) {
    case HW_RESOLUTION :
        *val = &mesh->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &mesh->graphN;
        return HW_TYPE_1I;
    case HW_GRAPHM :
        *val = &mesh->graphM;
        return HW_TYPE_1I;
    case HW_DATA :
        *val = mesh->data;
        return mesh->dataType;
    case HW_OPT_FLAGS :
        *val = &mesh->optFlags;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( mesh->dirty ) cook( __hwDisp, mesh, &dataPtr );
        *val = mesh->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( mesh->dirty ) cook( __hwDisp, mesh, &dataPtr );
        *val = &mesh->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = mesh->cookedData;
        if( mesh->dirty || !dataPtr ) cook( __hwDisp, mesh, &dataPtr );
        *val = mesh->cookedData;
        vn = hwCalcWPV( mesh->cookedFlags );
        vn *= mesh->cookedNM[0] * mesh->cookedNM[1];
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( mesh->dirty ) cook( __hwDisp, mesh, &dataPtr );
        *val = mesh->cookedNM;
        return HW_MAKE_TYPE(HW_TYPE_INT,2);
    default :
        n = hwLookup( prop, __hwIntVtxTab );
        if( n >= 0 ) {
            return __hwIntVtxInquire( &mesh->dataFlags, n, val );
        }
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &mesh->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &mesh->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Mesh
        *mesh = (Mesh *)obj;
    hwInt32
        n, m, vn;
    hwFloat
        *data = NULL;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !mesh->data )   return;

    if( mesh->dirty ) {
        cook( __hwDisp, mesh, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                &mesh->surf, mesh->BBox,
                2*mesh->cookedNM[0]*mesh->cookedNM[1] ) )
    {
        return;
    }

    if( mesh->dl >= 0 ) {
        if( !(mesh->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &mesh->surf );
        }
        __hwDisp->callList( __hwDisp, mesh->dl );
    }
    else if( mesh->cookedData ) {
        hwSurfAttrs( &mesh->surf );
        __hwDisp->drawMesh( __hwDisp,
                        mesh->cookedData, mesh->cookedFlags,
                        mesh->cookedNM[0], mesh->cookedNM[1] );
    }
    else {
        /* Draw it... */
        n = mesh->cookedNM[0];
        m = mesh->cookedNM[1];

        hwSurfAttrs( &mesh->surf );

        if( mesh->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            mesh->dl = __hwDisp->openList( __hwDisp );
            if( mesh->dl < 0 ) {
                mesh->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                mesh->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( mesh->optFlags & HW_OPT_DL_ATTRS ) {
                /* Cook surface attrs into DL */
                hwSurfAttrs( &mesh->surf );
            }
        }

        __hwDisp->drawMesh( __hwDisp,
                                   data, mesh->cookedFlags, n, m );

        if( mesh->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, mesh->dl );
        }
        else if( mesh->optFlags == HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( mesh->cookedFlags );
            mesh->cookedData = malloc( n*m*vn*sizeof(hwFloat) );
            if( mesh->cookedData ) {
                (void)memcpy( mesh->cookedData, data, n*m*vn*sizeof(hwFloat) );
            }
            else {
                mesh->optFlags &= HW_OPT_CACHE_DATA;
            }
        }

        if( mesh->optFlags & HW_OPT_SAVED_DATA ) {
            mesh->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwMesh.c ***/
