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

/* Internal definitions of strip-related flags */
#define HW_DATA         1
#define HW_OPT_FLAGS    2
#define HW_BBOX         3
#define HW_COOKED_FLAGS 4
#define HW_COOKED_DATA  5
#define HW_COOKED_COUNTS 6
#define HW_DIRTY        7

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to strip */
        hwStrOptFlags, hwStrData,

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
    hwStripStruct = {
        0,              /* Parent - NULL */
        "hwStrip",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwStrip = &hwStripStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty,                  /* Is this strip cooked? */
        graphN,                 /* Number of vertices in strip */
        dataType,               /* Passed in by user */
        dl,                     /* Display list */
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
} Strip;

/* The hash table for strip strings */
static void *stripTab = 0;

static hwObject create( hwObject proto )
{
    Strip
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwStrip )      return 0;

    HW_HASH_SETUP(stripTab, 32)
        t = stripTab;
        HW_INSERT( hwStrData,         HW_DATA,          t );
        HW_INSERT( hwStrOptFlags,     HW_OPT_FLAGS,     t );
        HW_INSERT( hwStrBounds,       HW_BBOX,          t );
        HW_INSERT( hwStrCookedFlags,  HW_COOKED_FLAGS,  t );
        HW_INSERT( hwStrCookedData,   HW_COOKED_DATA,   t );
        HW_INSERT( hwStrCookedCounts, HW_COOKED_COUNTS, t );
        HW_INSERT( hwStrDirty,        HW_DIRTY,         t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Strip) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwStrip;
    result->refCount = 1;

    result->graphN = 0; 
    result->dl = -1;
    result->dataFlags = 0;
    result->dirty = 1;
    result->optFlags = hwDefaultOptFlags;

    hwDefaultSurf( &result->surf );
    hwDefaultOrient( &result->orient );

    /* Identity strip */
    result->data = 0;
    result->dataType = 0;
    result->cookedData = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Strip
        *strip = (Strip *)obj;

    LOG_ENTRY
    strip->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Strip
        *strip = (Strip *)obj;;

    LOG_ENTRY
    if( --strip->refCount > 0 ) return;
    if( strip->dl >= 0 ) {
        __hwIntDestroyList( strip->dl );
    }
    if( strip->data && !(strip->optFlags & HW_OPT_SAFE_USER_DATA) ) {
        free( strip->data );
    }
    if( strip->cookedData )     free( strip->cookedData );
    free( strip );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Strip
        *strip = (Strip *)obj;
    int
        dirty = 1, n;

    LOG_ENTRY
    switch( hwLookup( prop, stripTab ) ) {
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        strip->optFlags = *(hwInt32 *)val;
        break;
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT(type);
        if( strip->optFlags & HW_OPT_SAFE_USER_DATA ) {
            strip->data = (float *)val;
        }
        else {
            strip->data = realloc( strip->data, n*sizeof(hwFloat) );
            if( !strip->data )  return;
            (void)memcpy( strip->data, val, n * sizeof(hwFloat) );
        }
        strip->dataType = type;
        break;
    case HW_DIRTY :
        n = hwLookup( prop, __hwIntSurfTab );
        __hwIntSurfModify( &strip->surf, n, type, val );
        break;
    default :
        if( (n = hwLookup( prop, __hwIntVtxTab )) >= 0 ) {
            dirty = __hwIntVtxModify( &strip->dataFlags, n, type, val );
        }
        else if( (n = hwLookup( prop, __hwIntSurfTab )) >= 0 ) {
            dirty = __hwIntSurfModify( &strip->surf, n, type, val );
            if( strip->optFlags & HW_OPT_DL_ATTRS ) dirty = 1;
        }
        else if( (n = hwLookup( prop, __hwIntOrientTab )) >= 0 ) {
            __hwIntOrientModify( &strip->orient, n, type, val );
        }
        else {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }
    if( dirty ) strip->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( hwDisplay disp, Strip *strip, hwFloat **result )
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

    if( strip->dl >= 0 ) {
        __hwIntDestroyList( strip->dl );
    }
    strip->dl = -1;
    if( strip->cookedData )     free( strip->cookedData );
    strip->cookedData = 0;

    *result = 0;
    if( !strip->data )  goto ERROR;

    vn = hwCalcWPV( strip->dataFlags );
    n = HW_GET_COUNT( strip->dataType );
    if( n % vn ) goto ERROR;
    n /= vn;
    strip->graphN = n;

    /* Allocate data... */
    if( n*vn > dataSize ) {
        dataSize = n*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;
    (void)memcpy( data, strip->data, n*vn*sizeof(hwFloat) );

    /* Transform data */
    __hwIntTransformData( &strip->orient, data, n, strip->dataFlags );

    /* Update the bounding box */
    strip->BBox[0] = strip->BBox[1] = strip->BBox[2] = HW_MAX_FLOAT;
    strip->BBox[3] = strip->BBox[4] = strip->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( strip->BBox, data, n, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    strip->cookedFlags = __hwIntTexturePipeline(
                                    &strip->surf,
                                    &data, &dataSize, &vn,
                                    n, strip->dataFlags );

    *result = data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchData = data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Strip
        *strip = (Strip *)obj;
    hwInt32
        n, vn;
    hwFloat
        *dataPtr;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    switch( hwLookup( prop, stripTab ) ) {
    case HW_DATA :
        *val = strip->data;
        return strip->dataType;
    case HW_OPT_FLAGS :
        *val = &strip->optFlags;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( strip->dirty ) cook( __hwDisp, strip, &dataPtr );
        *val = strip->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_COOKED_FLAGS :
        if( strip->dirty ) cook( __hwDisp, strip, &dataPtr );
        *val = &strip->cookedFlags;
        return HW_TYPE_1I;
    case HW_COOKED_DATA :
        dataPtr = strip->cookedData;
        if( strip->dirty || !dataPtr ) cook( __hwDisp, strip, &dataPtr );
        *val = dataPtr;
        vn = hwCalcWPV( strip->cookedFlags );
        vn *= strip->graphN;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,vn);
    case HW_COOKED_COUNTS :
        if( strip->dirty ) cook( __hwDisp, strip, &dataPtr );
        *val = &strip->graphN;
        return HW_TYPE_1I;
    default :
        n = hwLookup( prop, __hwIntVtxTab );
        if( n >= 0 ) {
            return __hwIntVtxInquire( &strip->dataFlags, n, val );
        }
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &strip->surf, n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &strip->orient, n, val );
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Strip
        *strip = (Strip *)obj;
    hwInt32
        n, vn;
    hwFloat
        *data = NULL;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !strip->data ) return;

    if( strip->dirty ) {
        cook( __hwDisp, strip, &data );
        if( !data ) return;
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &strip->surf, strip->BBox, strip->graphN ) )
    {
        return;
    }

    if( strip->dl >= 0 ) {
        if( !(strip->optFlags & HW_OPT_DL_ATTRS) ) {
            hwSurfAttrs( &strip->surf );
        }
        __hwDisp->callList( __hwDisp, strip->dl );
    }
    else if( strip->cookedData ) {
        hwSurfAttrs( &strip->surf );
        __hwDisp->drawStrip( __hwDisp,
                        strip->cookedData, strip->cookedFlags,
                        strip->graphN );
    }
    else {
        n = strip->graphN;

        /* Draw it... */
        hwSurfAttrs( &strip->surf );

        if( strip->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            strip->dl = __hwDisp->openList( __hwDisp );
            if( strip->dl < 0 ) {
                strip->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                strip->optFlags |= HW_OPT_CACHE_DATA;
            }
            if( strip->optFlags & HW_OPT_DL_ATTRS ) {
                hwSurfAttrs( &strip->surf );
            }
        }

        __hwDisp->drawStrip( __hwDisp, data, strip->cookedFlags, n );

        if( strip->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, strip->dl );
        }
        else if( strip->optFlags & HW_OPT_CACHE_DATA ) {
            vn = hwCalcWPV( strip->cookedFlags );
            strip->cookedData = malloc( n*vn*sizeof(hwFloat) );
            if( strip->cookedData ) {
                (void)memcpy( strip->cookedData, data, n*vn*sizeof(hwFloat) );
            }
            else {
                strip->optFlags &= ~HW_OPT_CACHE_DATA;
            }
        }

        if( strip->optFlags & HW_OPT_SAVED_DATA ) {
            strip->dirty = 0;
        }
    }
    LOG_EXIT
}

/*** EOF hwStrip.c ***/
