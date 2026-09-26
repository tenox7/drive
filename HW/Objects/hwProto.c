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

/* This is the minimum set of code required to create a new HW class.
 * Copy this to a new file, change "Proto" to your class name (and
 * "proto" to your class name too) then add your code.
 */

#include <stdio.h>
#include <stdlib.h>
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
        /* Specific to proto */
        /* End of list */
        0
    };

/* Here, you should #define integer constants equivalent
 * to all the string properties you care about; things
 * like:
 *
 * #define HW_COLOR     1
 * #define HW_NOSTRIL   2
 *
 * and so on.  Then, in the create method, if the
 * static string-to-int hash table is not created, you
 * initialize it with the strings and the number
 */

static struct _hwObjectStruct
    hwProtoStruct = {
        0,              /* Parent - NULL */
        "hwProto",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwProto = &hwProtoStruct;

/* Hash table for proto strings */
static void *protoTab = 0;
    
typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,               /* Reference count */
        dirty;                  /* Dirty flag */
    /* Add your class-specific data here */
} Proto;

static hwObject create( hwObject proto )
{
    Proto
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwProto )      return 0;

    /* Create the string-to-integer mapping table.
     * Change 24 to be appropriate relative to the
     * number of strings you care about.
     */
    HW_HASH_SETUP(protoTab, 24)
        t = protoTab;
        /* Here you insert the strings / numbers into the table, like
         * this:
         *
         * HW_INSERT( hwStrColor,   HW_COLOR,   t );
         * HW_INSERT( hwStrNostril, HW_NOSTRIL, t );
         *
         */
    HW_HASH_CLEANUP

    result = malloc( sizeof(Proto) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }

    /* Class-specific initialization goes here */

    result->hdr = *proto;
    result->hdr.parent = hwProto;

    result->refCount = 1;
    result->dirty = 1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Proto
        *proto = (Proto *)obj;;

    LOG_ENTRY
    /* This method only rarely needs to be changed */
    proto->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Proto
        *proto = (Proto *)obj;;

    LOG_ENTRY
    if( --proto->refCount > 0 ) return;

    /* If you allocate dynamic components of the object,
     * destroy/free them here
     */
    free( proto );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Proto
        *proto = (Proto *)obj;
    int
        dirty = 1;      /* Presume that mods make the object dirty */

    LOG_ENTRY
    switch( hwLookup( prop, protoTab ) ) {
    /* These cases will be your #defines:
     *
     * case HW_COLOR :
     *     if( type != HW_TYPE_3F ) goto BadType;
     *     proto->color[0] = ((hwFloat *)val)[0];
     *     proto->color[1] = ((hwFloat *)val)[1];
     *     proto->color[2] = ((hwFloat *)val)[2];
     *     break;
     * case HW_NOSTRIL :
     *     if( type != HW_TYPE_1I ) goto BadType;
     *     proto->nostril = *(hwInt32 *)val;
     *     break;
     *
     * If you allocate graphics resources (display lists, textures)
     * make sure you implement hwStrDirty:
     * case HW_DIRTY:
     *      break;
     */
    default :
        /* If you don't recognize a string, make sure to set an error */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }

    if (dirty) proto->dirty = 1;
    LOG_EXIT
    return;

BadType:
    /* You should check the types of properties, and "goto BadType"
     * if they don't match what you expect
     */
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Proto
        *proto = (Proto *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, protoTab ) ) {
    /* Just like "modify", these cases will be your #defines:
     *
     * case HW_COLOR :
     *     *val = proto->color;
     *     return HW_TYPE_3F;
     * case HW_NOSTRIL :
     *     *val = &proto->nostril;
     *     return HW_TYPE_1I;
     */
    default :
        /* If you don't recognize a property, set that error and
         * return 0 as the type
         */
        __hwIntSetError( HW_ERROR_BAD_PROP );
        return 0; 
    }

    /* NOTREACHED */
    LOG_EXIT
    return 0; 
}

static void cook( hwDisplay disp, Proto *proto )
{
    /* You'd put your code to cook the user input into drawable goo here */
    proto->dirty = 0;
}

static void draw( hwObject obj )
{
    Proto
        *proto = (Proto *)obj;
    /* You are probably going to need __hwDisp to draw anything.
     * This is how you get it:
     */
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( proto->dirty ) {
        cook( __hwDisp, proto );
    }

    /* Drawing code goes here */
    LOG_EXIT
}

/*** EOF hwProto.c ***/
