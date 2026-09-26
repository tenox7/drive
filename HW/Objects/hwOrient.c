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
#include "hw.h"
#include "hw_internal.h"

#define ORIENT_SCALE    1
#define ORIENT_ROTATE   2
#define ORIENT_POS      3
#define ORIENT_ORIENT   4

void
    *__hwIntOrientTab;

int __hwIntInitOrient( void )
{
    HW_HASH_SETUP(__hwIntOrientTab, 8)

        HW_INSERT( hwStrScale, ORIENT_SCALE, __hwIntOrientTab );
        HW_INSERT( hwStrRotate,ORIENT_ROTATE,__hwIntOrientTab );
        HW_INSERT( hwStrPos,   ORIENT_POS,   __hwIntOrientTab );
        HW_INSERT( hwStrOrient,ORIENT_ORIENT,__hwIntOrientTab );

    HW_HASH_CLEANUP

    return 1;
}

void hwDefaultOrient( hwOrientType *orient )
{
    orient->scale[0] = 1.; orient->scale[1] = 1.; orient->scale[2] = 1.;
    orient->rotate[0] = 0.; orient->rotate[1] = 0.; orient->rotate[2] = 0.;
    orient->pos[0] = 0.; orient->pos[1] = 0.; orient->pos[2] = 0.;
}

void __hwIntOrientModify
(
    hwOrientType *orient,
    int propno,
    hwInt32 type,
    const void *val
)
{
    hwObject
        obj;
    void
        *tmp;

    LOG_ENTRY
    switch( propno ) {
    case ORIENT_SCALE :
        switch( type ) {
        case HW_TYPE_1I :
            orient->scale[0] = *(hwInt32 *)val;
            orient->scale[1] = *(hwInt32 *)val;
            orient->scale[2] = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            orient->scale[0] = *(hwFloat *)val;
            orient->scale[1] = *(hwFloat *)val;
            orient->scale[2] = *(hwFloat *)val;
            break;
        case HW_TYPE_3F :
            orient->scale[0] = ((hwFloat *)val)[0];
            orient->scale[1] = ((hwFloat *)val)[1];
            orient->scale[2] = ((hwFloat *)val)[2];
            break;
        default :
            goto BadType;
        }
        break;
    case ORIENT_ROTATE :
        if( type != HW_TYPE_3F )        goto BadType;
        orient->rotate[0] = ((hwFloat *)val)[0];
        orient->rotate[1] = ((hwFloat *)val)[1];
        orient->rotate[2] = ((hwFloat *)val)[2];
        break;
    case ORIENT_POS :
        if( type != HW_TYPE_3F )        goto BadType;
        orient->pos[0] = ((hwFloat *)val)[0];
        orient->pos[1] = ((hwFloat *)val)[1];
        orient->pos[2] = ((hwFloat *)val)[2];
        break;
    case ORIENT_ORIENT :
        if( type != HW_TYPE_OBJECT )    goto BadType;
        obj = (hwObject)val;
        if( !obj->inquire( obj, hwStrOrient, &tmp ) )   goto BadType;
        *orient = *(hwOrientType *)tmp;
        break;
    }
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

hwInt32 __hwIntOrientInquire( hwOrientType *orient, int propno, void **val )
{
    hwInt32
        clean = 0;

    LOG_ENTRY
    switch( propno ) {
    case ORIENT_SCALE :
        if( (orient->scale[0] == 1.0) && (orient->scale[1] == 1.0)
                && (orient->scale[2] == 1.0) )
        {
            clean = HW_TYPE_CLEAN;
        }
        *val = orient->scale;
        return HW_TYPE_3F | clean;
    case ORIENT_ROTATE :
        if( (orient->rotate[0] == 0.0) && (orient->rotate[1] == 0.0)
                && (orient->rotate[2] == 0.0) )
        {
            clean = HW_TYPE_CLEAN;
        }
        *val = orient->rotate;
        return HW_TYPE_3F | clean;
    case ORIENT_POS :
        if( (orient->pos[0] == 0.0) && (orient->pos[1] == 0.0)
                && (orient->pos[2] == 0.0) )
        {
            clean = HW_TYPE_CLEAN;
        }
        *val = orient->pos;
        return HW_TYPE_3F | clean;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

/* The class which implements all of this... */
static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwOrientDef = {
        0,              /* Parent - NULL */
        "hwOrient",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwOrient = &hwOrientDef;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;
    hwOrientType
        orient;                 /* The orientace itself */
} Orient;


static hwObject create( hwObject parent )
{
    Orient
        *result;

    LOG_ENTRY
    if( parent != hwOrient )    return 0;
    result = malloc( sizeof(Orient) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }

    result->hdr = *hwOrient;
    result->hdr.parent = hwOrient;
    result->refCount = 1;

    hwDefaultOrient( &result->orient );

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Orient
        *orient = (Orient *)obj;

    LOG_ENTRY
    orient->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Orient
        *orient = (Orient *)obj;

    LOG_ENTRY
    if( --orient->refCount > 0 )        return;
    free( orient );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    int
        n;
    Orient
        *orient = (Orient *)obj;

    LOG_ENTRY
    n = hwLookup( prop, __hwIntOrientTab );
    if( n >= 0 ) {
        __hwIntOrientModify( &orient->orient, n, type, val );
    }
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    int
        n;
    Orient
        *orient = (Orient *)obj;

    LOG_ENTRY
    n = hwLookup( prop, __hwIntOrientTab );
    if( n == ORIENT_ORIENT ) {
        *val = &orient->orient;
        return 1;
    }
    else if( n >= 0 ) {
        return __hwIntOrientInquire( &orient->orient, n, val );
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    LOG_ENTRY
    /* No sense in drawing an orientation:-) */
    LOG_EXIT
}

/*** EOF hwOrient.c ***/
