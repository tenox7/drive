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
#include <math.h>
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
        /* Specific to hwJoystick */
        hwStrID,
        hwStrKeyMapping,
        hwStrAxisMapping,
        hwStrCallback,

        /* End of list */
        0
    };

#define HW_ID           1
#define HW_KEY_MAPPING  2
#define HW_AXIS_MAPPING 3
#define HW_CALLBACK     4
#define HW_EVENT        5

static struct _hwObjectStruct
    hwJoystickStruct = {
        0,      /* Parent - NULL */
        "hwJoystick",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwJoystick = &hwJoystickStruct;

/* Hash table for joystick strings */
static void *joystickTab = 0;

typedef struct {
    hwInt32 source, dest;
} Mapping;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;               /* Reference count */
    hwObject
        parent;                 /* Parent container */
    Mapping
        *keyMap,
        *axisMap;
    hwInt32
        numKeyMap,
        numAxisMap;
    char
        *ID;                    /* Identifier, for easy use in switch stmts */
    hwJoystickCB
        callback;               /* Thing to call when joystick hit */
} HwJoystick;

static void cook( hwDisplay disp, HwJoystick *joystick );

static hwObject create( hwObject joystick )
{
    HwJoystick
        *result;
    void
        *t;

    if( joystick != hwJoystick )      return 0;

    HW_HASH_SETUP(joystickTab, 16)
        t = joystickTab;
        HW_INSERT( hwStrID,          HW_ID,           t );
        HW_INSERT( hwStrKeyMapping,  HW_KEY_MAPPING,  t );
        HW_INSERT( hwStrAxisMapping, HW_AXIS_MAPPING, t );
        HW_INSERT( hwStrEvent,       HW_EVENT,        t );
        HW_INSERT( hwStrCallback,    HW_CALLBACK,     t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(HwJoystick) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    memset(result, 0, sizeof(HwJoystick));

    /* Class-specific initialization goes here */

    result->hdr = *joystick;
    result->hdr.parent = hwJoystick;
    result->hdr.name = 0;

    result->refCount = 1;

    result->keyMap = NULL;
    result->numKeyMap = 0;
    result->axisMap = NULL;
    result->numAxisMap = 0;
    result->ID = NULL;
    result->callback = NULL;

    return (hwObject)result;
}

static void addref( hwObject obj )
{
    HwJoystick
        *joystick = (HwJoystick *)obj;;

    /* This method only rarely needs to be changed */
    joystick->refCount++;
}

static void destroy( hwObject obj )
{
    HwJoystick
        *joystick = (HwJoystick *)obj;
    int
        i, n;

    if( --joystick->refCount > 0 ) return;

    if( joystick->keyMap ) {
        free( joystick->keyMap );
        joystick->keyMap = NULL;
    }
    if( joystick->axisMap ) {
        free( joystick->axisMap );
        joystick->axisMap = NULL;
    }
    if( joystick->ID ) {
        free( joystick->ID );
        joystick->ID = NULL;
    }
    free( joystick );
}

static int mapCmp(const void *a, const void *b)
{
    const Mapping
        *ma, *mb;

    ma = (const Mapping *)a;
    mb = (const Mapping *)b;

    if( ma->source < mb->source )       return -1;
    else if( ma->source > mb->source )  return 1;
    else                                return 0;
}

static hwInt32 findMap(hwInt32 val, Mapping *map, hwInt32 numMap)
{
    hwInt32
        currVal,
        first, last, curr;

    first = -1;
    last = numMap;
    do {
        curr = (first + last) / 2;
        currVal = map[curr].source;
        if( currVal == val )     return map[curr].dest;
        else if( currVal < val ) first = curr;
        else                     last = curr;
    } while( first < (last - 1) );
    return -1;
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    HwJoystick
        *joystick = (HwJoystick *)obj;
    const char 
        *str;
    hwInt32
        base, i, n;
    const hwInt32
        *iptr;
    const hwFloat
        *fptr;
    hwWinEvent
        e;

    switch( hwLookup( prop, joystickTab ) ) {
    case HW_ID :
        if( type != HW_TYPE_STRING )    goto BadType;
        str = val;
        if( joystick->ID ) free( joystick->ID );
        joystick->ID = malloc(strlen(str) + 1);
        if( !joystick->ID ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        strcpy(joystick->ID, str);
        break;
    case HW_KEY_MAPPING :
        base = HW_GET_BASE(type);
        switch( base ) {
        case HW_TYPE_INT : case HW_TYPE_FLOAT : break;
        default :                               goto BadType;
        }

        n = HW_GET_COUNT(type);
        if( n & 1 ) goto BadType;
        n >>= 1;

        if( joystick->keyMap ) free( joystick->keyMap );
        joystick->keyMap = malloc(n * sizeof(Mapping));
        if( !joystick->keyMap ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        joystick->numKeyMap = n;

        if( base == HW_TYPE_INT ) {
            iptr = val;
            for( i = 0; i < n; i++ ) {
                joystick->keyMap[i].dest =   iptr[2*i  ];
                joystick->keyMap[i].source = iptr[2*i+1];
            }
        }
        else {
            fptr = val;
            for( i = 0; i < n; i++ ) {
                joystick->keyMap[i].dest =   (hwInt32)fptr[2*i  ];
                joystick->keyMap[i].source = (hwInt32)fptr[2*i+1];
            }
        }
        qsort(joystick->keyMap, n, sizeof(Mapping), mapCmp);
        break;
    case HW_AXIS_MAPPING :
        base = HW_GET_BASE(type);
        switch( base ) {
        case HW_TYPE_INT : case HW_TYPE_FLOAT : break;
        default :                               goto BadType;
        }

        n = HW_GET_COUNT(type);
        if( n & 1 ) goto BadType;
        n >>= 1;

        if( joystick->axisMap ) free( joystick->axisMap );
        joystick->axisMap = malloc(n * sizeof(Mapping));
        if( !joystick->axisMap ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        joystick->numAxisMap = n;

        if( base == HW_TYPE_INT ) {
            iptr = val;
            for( i = 0; i < n; i++ ) {
                joystick->axisMap[i].dest =   iptr[2*i  ];
                joystick->axisMap[i].source = iptr[2*i+1];
            }
        }
        else {
            fptr = val;
            for( i = 0; i < n; i++ ) {
                joystick->axisMap[i].dest =   (hwInt32)fptr[2*i  ];
                joystick->axisMap[i].source = (hwInt32)fptr[2*i+1];
            }
        }
        qsort(joystick->axisMap, n, sizeof(Mapping), mapCmp);
        break;
    case HW_CALLBACK :
        if( type == HW_TYPE_STRING ) {
            joystick->callback = (hwJoystickCB) hwFindCallback( (char *)val );
        }
        else if( type == HW_TYPE_CALLBACK ) {
            joystick->callback = (hwJoystickCB) val;
        }
        else {
            goto BadType;
        }
        break;
    case HW_EVENT :
        if( !joystick->callback ) break;
        if( type != HW_TYPE_EVENT ) goto BadType;

        e = *(hwWinEvent *) val;
        switch( e.type ) {
        case HW_INPUT_JOY_PRESS :
        case HW_INPUT_JOY_RELEASE :
            if( joystick->numKeyMap ) {
                n = findMap(e.joystickButton.value,
                            joystick->keyMap, joystick->numKeyMap);
                if( n >= 0 ) {
                    e.joystickButton.value = n;
                    (*joystick->callback)(obj, &e);
                }
            }
            else {
                // If no mapping, just pass raw event
                (*joystick->callback)(obj, &e);
            }
            break;
        case HW_INPUT_JOY_AXIS :
            if( joystick->numAxisMap ) {
                n = findMap(e.joystickAxis.axis,
                            joystick->axisMap, joystick->numAxisMap);
                if( n >= 0 ) {
                    e.joystickAxis.axis = n;
                    (*joystick->callback)(obj, &e);
                }
            }
            else {
                // If no mapping, just pass raw event
                (*joystick->callback)(obj, &e);
            }
            break;
        }
        break;
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    HwJoystick
        *joystick = (HwJoystick *)obj;

    switch( hwLookup( prop, joystickTab ) ) {
    case HW_ID :
        *val = joystick->ID;
        return HW_TYPE_STRING;
    case HW_KEY_MAPPING :
        *val = joystick->keyMap;
        return HW_MAKE_TYPE(HW_TYPE_INT, 2*joystick->numKeyMap);
    case HW_AXIS_MAPPING :
        *val = joystick->axisMap;
        return HW_MAKE_TYPE(HW_TYPE_INT, 2*joystick->numAxisMap);
    case HW_CALLBACK :
        *val = (void *)joystick->callback;
        return HW_TYPE_CALLBACK;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0;
    }

    /* NOTREACHED */
    return 0;
}

static void draw( hwObject obj )
{
    /* Nothing to do to draw a joystick */
}

/*** EOF hwJoystick.c ***/
