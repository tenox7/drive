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

/* Internal definitions of data-related flags */
#define HW_DATA         1

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to data */
        hwStrData,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwDataStruct = {
        0,              /* Parent - NULL */
        "hwData",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwData = &hwDataStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dataType;               /* Type of input array */
    void
        *data;                  /* User's input data */
} Data;

/* The hash table for data strings */
static void *dataTab = 0;

static hwObject create( hwObject proto )
{
    Data
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwData )   return 0;

    HW_HASH_SETUP(dataTab, 4)
        t = dataTab;
        HW_INSERT( hwStrData,         HW_DATA,          t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Data) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwData;
    result->refCount = 1;

    result->dataType = 0;
    result->data = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Data
        *data = (Data *)obj;

    LOG_ENTRY
    data->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Data
        *data = (Data *)obj;;

    LOG_ENTRY
    if( --data->refCount > 0 )  return;
    if( data->data ) {
        free( data->data );
    }
    free( data );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Data
        *data = (Data *)obj;
    int
        numBytes, vt, vn;

    LOG_ENTRY
    switch( hwLookup( prop, dataTab ) ) {
    case HW_DATA :
        vt = HW_GET_BASE( type );
        vn = HW_GET_COUNT( type );
        switch (vt) {
        case HW_TYPE_BYTE :  numBytes = vn * sizeof(hwInt8); break;
        case HW_TYPE_SHORT : numBytes = vn * sizeof(hwInt16); break;
        case HW_TYPE_INT :   numBytes = vn * sizeof(hwInt32); break;
        case HW_TYPE_BOOL :  numBytes = vn * sizeof(hwInt32); break;
        case HW_TYPE_FLOAT : numBytes = vn * sizeof(hwFloat); break;
        default :
            goto BadType;
        }
        data->data = realloc( data->data, numBytes );
        if( !data->data )   return;
        (void)memcpy( data->data, val, numBytes);
        data->dataType = type;
        break;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Data
        *data = (Data *)obj;
    int
        n, vn;
    hwFloat
        *dataPtr;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    switch( hwLookup( prop, dataTab ) ) {
    case HW_DATA :
        *val = data->data;
        return data->dataType;
    default :
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );

    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    /* Nothing to draw */
}

/*** EOF hwData.c ***/
