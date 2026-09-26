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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to box */
        hwStrData, hwStrOptFlags,
        hwStrGraphN, hwStrGraphM,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwBoxStruct = {
        0,              /* Parent - NULL */
        "hwBox",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwBox = &hwBoxStruct;
    
typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty;                  /* Dirty flag */
    hwObject
        quads;                  /* The hwQuads child object */
    hwFloat
        corners[6];             /* Corners of the box */
} Box;

static hwObject create( hwObject proto )
{
    Box
        *result;

    LOG_ENTRY
    if( proto != hwBox )        return 0;

    result = malloc( sizeof(Box) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }

    result->quads = hwQuads->create( hwQuads );
    if( !result->quads ) {
        free( result );
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }

    result->hdr = *proto;
    result->hdr.parent = hwBox;

    result->refCount = 1;
    result->dirty = 1;

    result->corners[0] = result->corners[1] = result->corners[2] = -1;
    result->corners[3] = result->corners[4] = result->corners[5] =  1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Box
        *box = (Box *)obj;;

    LOG_ENTRY
    box->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Box
        *box = (Box *)obj;;

    LOG_ENTRY
    if( --box->refCount > 0 )   return;
    box->quads->destroy( box->quads );
    free( box );
    LOG_EXIT
}

static void cook( Box *box )
{
    hwObject
        quads = box->quads;
    hwFloat
        color[3] = {0.,0.,0.},
        data[24*8];     /* Max 6*4*xyznnnst */

    LOG_ENTRY
    if( !box->dirty ) return;

    /* TBD: At some point, the user should be able to
     * request RGB, tangent, etc. per-vertex.  For now,
     * we force it to XYZNNNST
     */
    __hwIntMakeBox( HW_DATA_NORMALS | HW_DATA_ST, box->corners, data, color );

    /* Stick it in the quads object */
    HW_MODIFY_1B( quads, hwStrHasRGB, HW_FALSE );
    HW_MODIFY_1B( quads, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_1B( quads, hwStrHasST0, HW_TRUE );
    quads->modify( quads, hwStrData,
                        HW_MAKE_TYPE(HW_TYPE_FLOAT,24*8), data );
    box->dirty = 0;
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Box
        *box = (Box *)obj;
    hwInt32
        tmp;

    LOG_ENTRY
    if( strcmp( prop, hwStrData ) == 0 ) {
        if( type != HW_MAKE_TYPE(HW_TYPE_FLOAT,6) ) {
            __hwIntSetError( HW_ERROR_BAD_TYPE );
            return;
        }
        (void)memcpy( box->corners, val, 6*sizeof(hwFloat) );
        box->dirty = 1;
    }
    else if( strcmp( prop, hwStrOptFlags ) == 0 ) {
        if( type != HW_TYPE_1I ) {
            __hwIntSetError( HW_ERROR_BAD_TYPE );
            return;
        }
        tmp = *(hwInt32 *)val;
        tmp &= ~HW_OPT_SAFE_USER_DATA;
        box->quads->modify( box->quads, prop, type, (void *)&tmp );
    }
    else {
        box->quads->modify( box->quads, prop, type, val );
    }
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Box
        *box = (Box *)obj;

    LOG_ENTRY
    if( strcmp( prop, hwStrData ) == 0 ) {
        *val = box->corners;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    }
    else {
        cook( box );
        return box->quads->inquire( box->quads, prop, val );
    }
    LOG_EXIT
}

static void draw( hwObject obj )
{
    Box
        *box = (Box *)obj;

    LOG_ENTRY
    cook( box );
    box->quads->draw( box->quads );
    LOG_EXIT
}

/*** EOF hwBox.c ***/
