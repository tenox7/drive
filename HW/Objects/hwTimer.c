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
#include <math.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"
#include "hw_vecmath.h"

/* Internal definitions of timer-related flags */
#define HW_DATA         1
#define HW_INVISIBLE    2
#define HW_VISIBILITY   3
#define HW_LIFETIME     4
#define HW_CYCLE        5
#define HW_DELAY        6
#define HW_EQUATION     7
#define HW_OFFSET       8

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to timer */
        hwStrChildren, hwStrInvisible, hwStrVisibility,
        hwStrLifetime, hwStrCycleTime, hwStrEquation,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwTimerStruct = {
        0,              /* Parent - NULL */
        "hwTimer",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwTimer = &hwTimerStruct;


typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        invisible,              /* Is it visible? */
        visibility,             /* Visibility mask */
        numObjs,                /* Size of the object list */
        equationSize;           /* Size of the polynomial */
    hwObject
        *objects;               /* The list of objects */
    hwFloat
        lifetime[2],            /* When is this object alive? */
        offset,                 /* Placement of polynomial equation */
        *equation,              /* Time polynomial equation */
        cycleTime;              /* How long for 1 cycle? */
} Timer;

/* The hash table for timer strings */
static void *timerTab = 0;

static hwObject create( hwObject proto )
{
    Timer
        *result;

    LOG_ENTRY
    if( proto != hwTimer )      return 0;

    HW_HASH_SETUP(timerTab, 16)
        HW_INSERT( hwStrChildren,   HW_DATA,       timerTab );
        HW_INSERT( hwStrInvisible,  HW_INVISIBLE,  timerTab );
        HW_INSERT( hwStrVisibility, HW_VISIBILITY, timerTab );
        HW_INSERT( hwStrLifetime,   HW_LIFETIME,   timerTab );
        HW_INSERT( hwStrCycleTime,  HW_CYCLE,      timerTab );
        HW_INSERT( hwStrEquation,   HW_EQUATION,   timerTab );
        HW_INSERT( hwStrOffset,     HW_OFFSET,     timerTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Timer) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwTimer;
    result->refCount = 1;

    /* Default timer */
    result->invisible = 0;
    result->visibility = 0xFFFFFFFFL;
    result->numObjs = 0;
    result->objects = 0;
    result->equationSize = 0;
    result->equation = 0;
    result->lifetime[0] = 0.0;
    result->lifetime[1] = 1.0e38;
    result->cycleTime = 0.0;
    result->offset = 0.0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Timer
        *timer = (Timer *)obj;

    LOG_ENTRY
    timer->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Timer
        *timer = (Timer *)obj;
    int
        i;

    LOG_ENTRY
    if( --timer->refCount > 0 ) return;
    if( timer->objects ) {
        for( i = 0; i < timer->numObjs; i++ ) {
            timer->objects[i]->destroy( timer->objects[i] );
        }
        free( timer->objects );
    }
    if( timer->equation ) {
        free( timer->equation );
    }
    free( timer );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Timer
        *timer = (Timer *)obj;
    int
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, timerTab ) ) {
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT ) {
            __hwIntSetError( HW_ERROR_BAD_TYPE );
            break;
        }
        if( timer->objects ) {
            n = timer->numObjs;
            for( i = 0; i < n; i++ ) {
                timer->objects[i]->destroy( timer->objects[i] );
            }
            free( timer->objects );
            timer->objects = 0;
        }
        n = HW_GET_COUNT( type );
        timer->objects = malloc( n * sizeof(hwObject) );
        if( !timer->objects ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)memcpy( timer->objects, val, n*sizeof(hwObject) );
        timer->numObjs = n;
        for( i = 0; i < n; i++ ) {
            timer->objects[i]->addref( timer->objects[i] );
        }
        break;
    case HW_INVISIBLE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )           timer->invisible = 1;
        else                            timer->invisible = 0;
        break;
    case HW_VISIBILITY :
        if( type != HW_TYPE_1I )        goto BadType;
        timer->visibility = *(hwInt32 *)val;
        break;
    case HW_LIFETIME :
        if( type != HW_TYPE_2F )        goto BadType;
        timer->lifetime[0] = ((hwFloat *)val)[0];
        timer->lifetime[1] = ((hwFloat *)val)[1];
        break;
    case HW_CYCLE :
        switch( type ) {
        case HW_TYPE_1F :
            timer->cycleTime = *(hwFloat *)val;
            break;
        case HW_TYPE_1I :
            timer->cycleTime = *(hwInt32 *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_OFFSET :
        if( type != HW_TYPE_1F )        goto BadType;
        timer->offset= ((hwFloat *)val)[0];
        break;
    case HW_EQUATION :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT(type);
        timer->equation = realloc( timer->equation, n*sizeof(hwFloat) );
        if( !timer->equation ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }
        (void)memcpy( timer->equation, val, n*sizeof(hwFloat) );
        timer->equationSize = n;
        break;
    default :
        if( timer->objects ) {
            n = timer->numObjs;
            for( i = 0; i < n; i++ ) {
                timer->objects[i]->modify( timer->objects[i], prop, type, val );
            }
        }
        break;
    }
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    return;

NoMem :
    __hwIntSetError( HW_ERROR_NO_MEMORY );
    return;
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Timer
        *timer = (Timer *)obj;
    int
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, timerTab ) ) {
    case HW_DATA :
        *val = timer->objects;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT,timer->numObjs);
    case HW_INVISIBLE :
        *val = &timer->invisible;
        return HW_TYPE_1B;
    case HW_VISIBILITY :
        *val = &timer->visibility;
        return HW_TYPE_1I;
    case HW_LIFETIME :
        *val = timer->lifetime;
        return HW_TYPE_2F;
    case HW_CYCLE :
        *val = &timer->cycleTime;
        return HW_TYPE_1F;
    case HW_EQUATION :
        *val = timer->equation;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,timer->equationSize);
    default :
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}


static void draw( hwObject obj )
{
    Timer
        *timer = (Timer *)obj;
    int
        i, n;
    hwObject
        *objs;
    float
        matrix[4][4];
    double
        elapsedTime,
        sum, currTime,
        saveTime;
    HW_USE_CURR_DISP;

    elapsedTime = __hwDisp->getElapsedTime(__hwDisp);

    LOG_ENTRY
    if( timer->equationSize < 1 )                       return;
    if( timer->invisible )                              return;
    if( elapsedTime < timer->lifetime[0] )       return;
    if( elapsedTime > timer->lifetime[1] )       return;

    saveTime = elapsedTime;
    if( timer->cycleTime > 0.0 ) {
        currTime = fmod( saveTime - timer->lifetime[0], timer->cycleTime );
    }
    else {
        currTime = saveTime - timer->lifetime[0];
    }

    /* Evaluate the polynomial */
    sum = 0.0;
    for( i = timer->equationSize - 1; i > 0; i-- ) {
        sum += timer->equation[i];
        sum *= (currTime - timer->offset);
    }
    sum += timer->equation[0];
    sum += timer->lifetime[0];

    __hwDisp->setElapsedTime(__hwDisp, sum);

    n = timer->numObjs;
    objs = timer->objects;
    for( i = 0; i < n; i++ ) {
        __hwDisp->currentChild( __hwDisp, objs[i] );
        objs[i]->draw( objs[i] );
    }

    __hwDisp->setElapsedTime(__hwDisp, saveTime);
    LOG_EXIT
}

/*** EOF hwTimer.c ***/
