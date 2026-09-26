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

#ifndef M_PI
#       define  M_PI    3.141592653589
#endif

#define deg     * (hwFloat)(M_PI / 180.0)

/* Internal definitions of spin-related flags */
#define HW_DATA         1
#define HW_INVISIBLE    2
#define HW_VISIBILITY   3

#define HW_POS_TYPE     4
#define HW_DIR_TYPE     5
#define HW_UP_TYPE      6
#define HW_SCALE_TYPE   7
#define HW_TRANS_TYPE   8
#define HW_COLOR_TYPE   9

#define HW_POS_VALS     10
#define HW_DIR_VALS     11
#define HW_UP_VALS      12
#define HW_SCALE_VALS   13
#define HW_TRANS_VALS   14
#define HW_COLOR_VALS   15

#define HW_POS_TIMES    16
#define HW_DIR_TIMES    17
#define HW_UP_TIMES     18
#define HW_SCALE_TIMES  19
#define HW_TRANS_TIMES  20
#define HW_COLOR_TIMES  21

#define HW_LIFETIME     22
#define HW_CYCLE        23
#define HW_OFFSET       24
#define HW_DELAY        25
#define HW_POS_OFFSET   26
#define HW_NUM_CYCLES   27

/* Params for interpolation */
#define SPIN_POS        0
#define SPIN_DIR        1
#define SPIN_UP         2
#define SPIN_SCALE      3
#define SPIN_TRANS      4
#define SPIN_COLOR      5

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to spin */
        hwStrChildren, hwStrInvisible, hwStrVisibility,
        hwStrPosType, hwStrDirType, hwStrUpType, hwStrScaleType,
        hwStrPosParams, hwStrDirParams, hwStrUpParams, hwStrScaleParams,
        hwStrPosTimes, hwStrDirTimes, hwStrUpTimes, hwStrScaleTimes,
        hwStrTransType, hwStrTransParams, hwStrTransTimes,
        hwStrColorType, hwStrColorParams, hwStrColorTimes,
        hwStrLifetime, hwStrOffset, hwStrDelay, hwStrPosOffset,
        hwStrCycleTime,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwSpinStruct = {
        0,              /* Parent - NULL */
        "hwSpinner",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwSpinner = &hwSpinStruct;

#define HW_SPIN_UNDEF   -1

typedef struct {
    hwInt32
        spinType,
        numVals,
        numTimes,
        numCooked;
    hwFloat
        *vals,
        *times,
        *cooked;
} spinParm;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        valid,                  /* Is it a valid spinner? */
        invisible,              /* Is it visible? */
        visibility,             /* Visibility mask */
        numObjs;                /* Size of the object list */
    hwObject
        *objects;               /* The list of objects */
    hwFloat
        objectDelay,            /* Delay in between objects */
        lifetime[2],            /* When is this object alive? */
        offsetTime,             /* Offset for this object */
        cycleTime,              /* How long for 1 cycle? */
        numCycles,              /* Number of cycles per existance */
        currTrans,              /* Current transparency */
        currColor[3],           /* Current color */
        posOffset[3];           /* Position offset for splines */
    double
        prevTime,               /* For SPIN_RANDOM */
        ranNum[3];              /* For SPIN_RANDOM */
    spinParm
        pos, dir, up, scale,    /* The spinner parameters */
        trans, color;           /*    Also for transparency & color */
} Spin;

/* The hash table for spin strings */
static void *spinTab = 0;

static hwObject create( hwObject proto )
{
    Spin
        *result;

    LOG_ENTRY
    if( proto != hwSpinner )    return 0;

    HW_HASH_SETUP(spinTab, 32)
        HW_INSERT( hwStrChildren,    HW_DATA,        spinTab );
        HW_INSERT( hwStrInvisible,   HW_INVISIBLE,   spinTab );
        HW_INSERT( hwStrVisibility,  HW_VISIBILITY,  spinTab );
        HW_INSERT( hwStrPosType,     HW_POS_TYPE,    spinTab );
        HW_INSERT( hwStrDirType,     HW_DIR_TYPE,    spinTab );
        HW_INSERT( hwStrUpType,      HW_UP_TYPE,     spinTab );
        HW_INSERT( hwStrScaleType,   HW_SCALE_TYPE,  spinTab );
        HW_INSERT( hwStrTransType,   HW_TRANS_TYPE,  spinTab );
        HW_INSERT( hwStrColorType,   HW_COLOR_TYPE,  spinTab );
        HW_INSERT( hwStrPosParams,   HW_POS_VALS,    spinTab );
        HW_INSERT( hwStrDirParams,   HW_DIR_VALS,    spinTab );
        HW_INSERT( hwStrUpParams,    HW_UP_VALS,     spinTab );
        HW_INSERT( hwStrScaleParams, HW_SCALE_VALS,  spinTab );
        HW_INSERT( hwStrTransParams, HW_TRANS_VALS,  spinTab );
        HW_INSERT( hwStrColorParams, HW_COLOR_VALS,  spinTab );
        HW_INSERT( hwStrPosTimes,    HW_POS_TIMES,   spinTab );
        HW_INSERT( hwStrDirTimes,    HW_DIR_TIMES,   spinTab );
        HW_INSERT( hwStrUpTimes,     HW_UP_TIMES,    spinTab );
        HW_INSERT( hwStrScaleTimes,  HW_SCALE_TIMES, spinTab );
        HW_INSERT( hwStrTransTimes,  HW_TRANS_TIMES, spinTab );
        HW_INSERT( hwStrColorTimes,  HW_COLOR_TIMES, spinTab );
        HW_INSERT( hwStrLifetime,    HW_LIFETIME,    spinTab );
        HW_INSERT( hwStrCycleTime,   HW_CYCLE,       spinTab );
        HW_INSERT( hwStrNumCycles,   HW_NUM_CYCLES,  spinTab );
        HW_INSERT( hwStrOffset,      HW_OFFSET,      spinTab );
        HW_INSERT( hwStrDelay,       HW_DELAY,       spinTab );
        HW_INSERT( hwStrPosOffset,   HW_POS_OFFSET,  spinTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Spin) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwSpinner;
    result->refCount = 1;

    /* Default spin */
    result->pos.spinType = HW_SPIN_UNDEF;
    result->dir.spinType = HW_SPIN_UNDEF;
    result->up.spinType = HW_SPIN_UNDEF;
    result->scale.spinType = HW_SPIN_UNDEF;
    result->trans.spinType = HW_SPIN_UNDEF;
    result->color.spinType = HW_SPIN_UNDEF;

    result->pos.numVals = 0;
    result->dir.numVals = 0;
    result->up.numVals = 0;
    result->scale.numVals = 0;
    result->trans.numVals = 0;
    result->color.numVals = 0;

    result->pos.numTimes = 0;
    result->dir.numTimes = 0;
    result->up.numTimes = 0;
    result->scale.numTimes = 0;
    result->trans.numTimes = 0;
    result->color.numTimes = 0;

    result->invisible = 0;
    result->visibility = 0xFFFFFFFFL;
    result->numObjs = 0;
    result->objects = 0;
    result->objectDelay = 0.0;
    result->lifetime[0] = 0.0;
    result->lifetime[1] = 1.0e38;
    result->offsetTime = 0.0;
    result->cycleTime = 0.0;
    result->numCycles = 0.0;
    result->posOffset[0] = 0.0;
    result->posOffset[1] = 0.0;
    result->posOffset[2] = 0.0;

    result->pos.vals = 0;
    result->dir.vals = 0;
    result->up.vals = 0;
    result->scale.vals = 0;
    result->trans.vals = 0;
    result->color.vals = 0;

    result->pos.times = 0;
    result->dir.times = 0;
    result->up.times = 0;
    result->scale.times = 0;
    result->trans.times = 0;
    result->color.times = 0;

    result->pos.cooked = 0;
    result->dir.cooked = 0;
    result->up.cooked = 0;
    result->scale.cooked = 0;
    result->trans.cooked = 0;
    result->color.cooked = 0;

    result->pos.numCooked = 0;
    result->dir.numCooked = 0;
    result->up.numCooked = 0;
    result->scale.numCooked = 0;
    result->trans.numCooked = 0;
    result->color.numCooked = 0;

    result->prevTime = 1e38;
    result->valid = 0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Spin
        *spin = (Spin *)obj;

    LOG_ENTRY
    spin->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Spin
        *spin = (Spin *)obj;
    int
        i;

    LOG_ENTRY
    if( --spin->refCount > 0 )  return;
    if( spin->objects ) {
        for( i = 0; i < spin->numObjs; i++ ) {
            spin->objects[i]->destroy( spin->objects[i] );
        }
        free( spin->objects );
    }
    if( spin->pos.vals ) free( spin->pos.vals );
    if( spin->dir.vals ) free( spin->dir.vals );
    if( spin->up.vals ) free( spin->up.vals );
    if( spin->scale.vals ) free( spin->scale.vals );
    if( spin->trans.vals ) free( spin->trans.vals );
    if( spin->color.vals ) free( spin->color.vals );

    if( spin->pos.times ) free( spin->pos.times );
    if( spin->dir.times ) free( spin->dir.times );
    if( spin->up.times ) free( spin->up.times );
    if( spin->scale.times ) free( spin->scale.times );
    if( spin->trans.times ) free( spin->trans.times );
    if( spin->color.times ) free( spin->color.times );

    if( spin->pos.cooked ) free( spin->pos.cooked );
    if( spin->dir.cooked ) free( spin->dir.cooked );
    if( spin->up.cooked ) free( spin->up.cooked );
    if( spin->scale.cooked ) free( spin->scale.cooked );
    if( spin->trans.cooked ) free( spin->trans.cooked );
    if( spin->color.cooked ) free( spin->color.cooked );

    free( spin );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Spin
        *spin = (Spin *)obj;
    int
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, spinTab ) ) {
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT ) {
            __hwIntSetError( HW_ERROR_BAD_TYPE );
            break;
        }
        if( spin->objects ) {
            n = spin->numObjs;
            for( i = 0; i < n; i++ ) {
                spin->objects[i]->destroy( spin->objects[i] );
            }
            free( spin->objects );
            spin->objects = 0;
        }
        n = HW_GET_COUNT( type );
        spin->objects = malloc( n * sizeof(hwObject) );
        if( !spin->objects ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)memcpy( spin->objects, val, n*sizeof(hwObject) );
        spin->numObjs = n;
        for( i = 0; i < n; i++ ) {
            spin->objects[i]->addref( spin->objects[i] );
        }
        break;
    case HW_INVISIBLE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )           spin->invisible = 1;
        else                            spin->invisible = 0;
        break;
    case HW_VISIBILITY :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->visibility = *(hwInt32 *)val;
        break;
    case HW_POS_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->pos.spinType = *(hwInt32 *)val;
        break;
    case HW_DIR_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->dir.spinType = *(hwInt32 *)val;
        break;
    case HW_UP_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->up.spinType = *(hwInt32 *)val;
        break;
    case HW_SCALE_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->scale.spinType = *(hwInt32 *)val;
        break;
    case HW_TRANS_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->trans.spinType = *(hwInt32 *)val;
        break;
    case HW_COLOR_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        spin->color.spinType = *(hwInt32 *)val;
        break;
    case HW_POS_VALS :
        if( HW_GET_BASE( type ) != HW_TYPE_FLOAT )      goto BadType;
        n = HW_GET_COUNT( type );
        spin->pos.numVals = n;
        spin->pos.vals = realloc( spin->pos.vals, n * sizeof(hwFloat) );
        if( !spin->pos.vals )           goto NoMem;
        (void)memcpy( spin->pos.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_DIR_VALS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->dir.numVals = n;
        spin->dir.vals = realloc( spin->dir.vals, n * sizeof(hwFloat) );
        if( !spin->dir.vals )           goto NoMem;
        (void)memcpy( spin->dir.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_UP_VALS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->up.numVals = n;
        spin->up.vals = realloc( spin->up.vals, n * sizeof(hwFloat) );
        if( !spin->up.vals )            goto NoMem;
        (void)memcpy( spin->up.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_SCALE_VALS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->scale.numVals = n;
        spin->scale.vals = realloc( spin->scale.vals, n * sizeof(hwFloat) );
        if( !spin->scale.vals )         goto NoMem;
        (void)memcpy( spin->scale.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_TRANS_VALS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->trans.numVals = n;
        spin->trans.vals = realloc( spin->trans.vals, n * sizeof(hwFloat) );
        if( !spin->trans.vals )         goto NoMem;
        (void)memcpy( spin->trans.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_COLOR_VALS :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->color.numVals = n;
        spin->color.vals = realloc( spin->color.vals, n * sizeof(hwFloat) );
        if( !spin->color.vals )         goto NoMem;
        (void)memcpy( spin->color.vals, val, n * sizeof(hwFloat) );
        break;
    case HW_POS_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->pos.numTimes = n;
        spin->pos.times = realloc( spin->pos.times, n * sizeof(hwFloat) );
        if( !spin->pos.times )          goto NoMem;
        (void)memcpy( spin->pos.times, val, n * sizeof(hwFloat) );
        break;
    case HW_DIR_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->dir.numTimes = n;
        spin->dir.times = realloc( spin->dir.times, n * sizeof(hwFloat) );
        if( !spin->dir.times )          goto NoMem;
        (void)memcpy( spin->dir.times, val, n * sizeof(hwFloat) );
        break;
    case HW_UP_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->up.numTimes = n;
        spin->up.times = realloc( spin->up.times, n * sizeof(hwFloat) );
        if( !spin->up.times )           goto NoMem;
        (void)memcpy( spin->up.times, val, n * sizeof(hwFloat) );
        break;
    case HW_SCALE_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->scale.numTimes = n;
        spin->scale.times = realloc( spin->scale.times, n * sizeof(hwFloat) );
        if( !spin->scale.times )                goto NoMem;
        (void)memcpy( spin->scale.times, val, n * sizeof(hwFloat) );
        break;
    case HW_TRANS_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->trans.numTimes = n;
        spin->trans.times = realloc( spin->trans.times, n * sizeof(hwFloat) );
        if( !spin->trans.times )                goto NoMem;
        (void)memcpy( spin->trans.times, val, n * sizeof(hwFloat) );
        break;
    case HW_COLOR_TIMES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        spin->color.numTimes = n;
        spin->color.times = realloc( spin->color.times, n * sizeof(hwFloat) );
        if( !spin->color.times )                goto NoMem;
        (void)memcpy( spin->color.times, val, n * sizeof(hwFloat) );
        break;
    case HW_LIFETIME :
        if( type != HW_TYPE_2F )        goto BadType;
        spin->lifetime[0] = ((hwFloat *)val)[0];
        spin->lifetime[1] = ((hwFloat *)val)[1];
        if( spin->numCycles > 0.0 ) {
            spin->cycleTime = 
                (spin->lifetime[1] - spin->lifetime[0] ) / spin->numCycles;
        }
        break;
    case HW_NUM_CYCLES :
        switch( type ) {
        case HW_TYPE_1F :
            spin->numCycles = *(hwFloat *)val;
            break;
        case HW_TYPE_1I :
            spin->numCycles = *(hwInt32 *)val;
            break;
        default :
            goto BadType;
        }
        if( spin->numCycles > 0.0 ) {
            spin->cycleTime = 
                (spin->lifetime[1] - spin->lifetime[0] ) / spin->numCycles;
        }
        break;
    case HW_CYCLE :
        switch( type ) {
        case HW_TYPE_1F :
            spin->cycleTime = *(hwFloat *)val;
            break;
        case HW_TYPE_1I :
            spin->cycleTime = *(hwInt32 *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_OFFSET :
        switch( type ) {
        case HW_TYPE_1F :
            spin->offsetTime = *(hwFloat *)val;
            break;
        case HW_TYPE_1I :
            spin->offsetTime = *(hwInt32 *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_DELAY :
        switch( type ) {
        case HW_TYPE_1F :
            spin->objectDelay = *(hwFloat *)val;
            break;
        case HW_TYPE_1I :
            spin->objectDelay = *(hwInt32 *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_POS_OFFSET :
        if( type != HW_TYPE_3F )        goto BadType;
        spin->posOffset[0] = ((hwFloat *)val)[0];
        spin->posOffset[1] = ((hwFloat *)val)[1];
        spin->posOffset[2] = ((hwFloat *)val)[2];
        break;
    default :
        if( spin->objects ) {
            n = spin->numObjs;
            for( i = 0; i < n; i++ ) {
                spin->objects[i]->modify( spin->objects[i], prop, type, val );
            }
        }
        break;
    }
    spin->valid = 0;
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
    Spin
        *spin = (Spin *)obj;
    int
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, spinTab ) ) {
    case HW_DATA :
        *val = spin->objects;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT,spin->numObjs);
    case HW_INVISIBLE :
        *val = &spin->invisible;
        return HW_TYPE_1B;
    case HW_VISIBILITY :
        *val = &spin->visibility;
        return HW_TYPE_1I;
    case HW_POS_TYPE :
        *val = &spin->pos.spinType;
        return HW_TYPE_1I;
    case HW_DIR_TYPE :
        *val = &spin->dir.spinType;
        return HW_TYPE_1I;
    case HW_UP_TYPE :
        *val = &spin->up.spinType;
        return HW_TYPE_1I;
    case HW_SCALE_TYPE :
        *val = &spin->scale.spinType;
        return HW_TYPE_1I;
    case HW_TRANS_TYPE :
        *val = &spin->trans.spinType;
        return HW_TYPE_1I;
    case HW_COLOR_TYPE :
        *val = &spin->color.spinType;
        return HW_TYPE_1I;
    case HW_POS_VALS :
        if( !spin->pos.vals )   return 0;
        n = spin->pos.numVals;
        *val = spin->pos.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_DIR_VALS :
        if( !spin->dir.vals )   return 0;
        n = spin->dir.numVals;
        *val = spin->dir.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_UP_VALS :
        if( !spin->up.vals )    return 0;
        n = spin->up.numVals;
        *val = spin->up.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_SCALE_VALS :
        if( !spin->scale.vals ) return 0;
        n = spin->scale.numVals;
        *val = spin->scale.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_TRANS_VALS :
        if( !spin->trans.vals ) return 0;
        n = spin->trans.numVals;
        *val = spin->trans.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_COLOR_VALS :
        if( !spin->color.vals ) return 0;
        n = spin->color.numVals;
        *val = spin->color.vals;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_POS_TIMES :
        if( !spin->pos.times )  return 0;
        n = spin->pos.numTimes;
        *val = spin->pos.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_DIR_TIMES :
        if( !spin->dir.times )  return 0;
        n = spin->dir.numTimes;
        *val = spin->dir.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_UP_TIMES :
        if( !spin->up.times )   return 0;
        n = spin->up.numTimes;
        *val = spin->up.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_SCALE_TIMES :
        if( !spin->scale.times )        return 0;
        n = spin->scale.numTimes;
        *val = spin->scale.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_TRANS_TIMES :
        if( !spin->trans.times )        return 0;
        n = spin->trans.numTimes;
        *val = spin->trans.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_COLOR_TIMES :
        if( !spin->color.times )        return 0;
        n = spin->color.numTimes;
        *val = spin->color.times;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,n);
    case HW_LIFETIME :
        *val = spin->lifetime;
        return HW_TYPE_2F;
    case HW_CYCLE :
        *val = &spin->cycleTime;
        return HW_TYPE_1F;
    case HW_OFFSET :
        *val = &spin->offsetTime;
        return HW_TYPE_1F;
    case HW_DELAY :
        *val = &spin->objectDelay;
        return HW_TYPE_1F;
    case HW_POS_OFFSET :
        *val = &spin->posOffset;
        return HW_TYPE_3F;
    default :
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}


extern void __hwIntSplinePos
(
    hwFloat *spl,
    hwFloat *times,
    hwInt32 n,
    hwFloat percent,
    hwFloat result[3]
);

static void interpParms
(
    Spin *spin,
    spinParm *parm,
    double Delta,
    int type,
    hwFloat prev[3],
    hwFloat result[3]
)
{
    hwFloat
        Mat[4][4],
        Delta1,
        Percent,
        Ang, *Ptr;
    int
        i;

    Delta1 = 1.0 - Delta;
    Percent = Delta * 100.0;

    switch( parm->spinType ) {
    case HW_SPIN_SMOOTH :
        __hwIntSplinePos( parm->cooked, parm->times,
                                parm->numCooked, Percent, result );
        break;
    case HW_SPIN_RANDOM :
        if( (type == SPIN_POS) || (type == SPIN_COLOR) ) {
            /* POS and COLOR are the only ones with three different
             * random numbers
             */
            Ptr = parm->vals;
            for( i = 0; i < 3; i++ ) {
                Delta = spin->ranNum[i];
                Delta1 = 1.0 - Delta;
                result[i] = Delta * Ptr[3+i] + Delta1 * Ptr[0+i];
            }
            break;
        }
        Delta = spin->ranNum[0];
        Delta1 = 1.0 - Delta;
        /* FALLTHROUGH! */
    case HW_SPIN_LINEAR :
        Ptr = parm->vals;
        switch( type ) {
        case SPIN_POS : /* Position */
        case SPIN_SCALE :       /* scale */
        case SPIN_COLOR :
            result[0] = Delta * Ptr[3] + Delta1 * Ptr[0];
            result[1] = Delta * Ptr[4] + Delta1 * Ptr[1];
            result[2] = Delta * Ptr[5] + Delta1 * Ptr[2];
            break;
        case SPIN_DIR : /* Direction */
            Ang = Delta * Ptr[1] + Delta1 * Ptr[0];
            result[0] = cos( (double)(Ang deg) );
            result[1] = 0.0;
            result[2] = sin( (double)(Ang deg) );
            break;
        case SPIN_UP :  /* Up */
            Ang = Delta * Ptr[1] + Delta1 * Ptr[0];
            hwRotateAxis( Mat, Ang deg, prev );
            result[0] = 0.0; result[1] = 1.0; result[2] = 0.0;
            VXFORM3( result, result, Mat );
        case SPIN_TRANS :        /* transparency */
            result[0] = Delta * Ptr[1] + Delta1 * Ptr[0];
            break;
        }
        break;
    case HW_SPIN_CONSTANT :
        result[0] = parm->vals[0];
        result[1] = parm->vals[1];
        result[2] = parm->vals[2];
        break;
    default :
        switch( type ) {
        case SPIN_POS : /* position */
            result[0] = 0.0; result[1] = 0.0; result[2] = 0.0;
            break;
        case SPIN_DIR : /* direction */
            result[0] = 1.0; result[1] = 0.0; result[2] = 0.0;
            break;
        case SPIN_UP :  /* up */
            result[0] = 0.0; result[1] = 1.0; result[2] = 0.0;
            break;
        case SPIN_SCALE :       /* scale */
            result[0] = 1.0; result[1] = 1.0; result[2] = 1.0;
            break;
        case SPIN_TRANS :       /* transparency */
            result[0] = 0.0; result[1] = 0.0; result[2] = 0.0;
            break;
        case SPIN_COLOR :       /* color */
            result[0] = 1.0; result[1] = 1.0; result[2] = 1.0;
            break;
        }
        break;
    }
}

static void MakeMat( hwDisplay disp, Spin *spin, hwFloat matrix[4][4] )
{
    hwFloat
        Pos[3], Up[3], Dir[3], Scale[3],
        Transp[3], Color[3],
        Mat[4][4], Mat1[4][4];
    double
        denom,
        Delta, DeltaT;

    /* How long since time 0? */
    DeltaT = disp->getElapsedTime(disp) - spin->lifetime[0];
    DeltaT += spin->offsetTime;

    if( spin->cycleTime > 0.0 ) {
        Delta = DeltaT / spin->cycleTime;
        if( Delta < 0.0 ) {
            Delta = 1.0 - fmod( -Delta, 1.0 );
        }
        else {
            Delta = fmod( Delta, 1.0 );
        }
    }
    else {
        denom = spin->lifetime[1] - spin->lifetime[0];
        Delta = DeltaT / denom;
        if( Delta < 0.0 ) {
            Delta = 0.0;
        }
        else if( Delta > 1.0 ) {
            Delta = 1.0;
        }
    }

    if( Delta < spin->prevTime ) {
        /* Transitioning from not-alive to alive - regenerate random numbers */
if( spin->hdr.name && (strcmp(spin->hdr.name, "SPIN_DEBUG") == 0)) {
printf("regen %g %g\n", Delta, spin->prevTime);
}
        spin->ranNum[0] = hwDrand48();
        spin->ranNum[1] = hwDrand48();
        spin->ranNum[2] = hwDrand48();
    }

    spin->prevTime = Delta;

    interpParms( spin, &spin->pos,   Delta, SPIN_POS, 0, Pos );
    if( spin->pos.spinType ==  HW_SPIN_SMOOTH ) {
        Pos[0] += spin->posOffset[0];
        Pos[1] += spin->posOffset[1];
        Pos[2] += spin->posOffset[2];
    }
    interpParms( spin, &spin->dir,   Delta, SPIN_DIR, 0, Dir );
    interpParms( spin, &spin->up,    Delta, SPIN_UP, Dir, Up );
    interpParms( spin, &spin->scale, Delta, SPIN_SCALE, 0, Scale );
    interpParms( spin, &spin->trans, Delta, SPIN_TRANS, 0, Transp );
    interpParms( spin, &spin->color, Delta, SPIN_COLOR, 0, Color );
    spin->currColor[0] = Color[0];
    spin->currColor[1] = Color[1];
    spin->currColor[2] = Color[2];
    spin->currTrans = Transp[0];

    /* OK, now create the matrix! */
    __hwIntVecToMat( Up, Dir, Mat );
    hwScale( Mat1, Scale[0], Scale[1], Scale[2] );
    hwMatMult( matrix, Mat1, Mat );
    matrix[3][0] = Pos[0];
    matrix[3][1] = Pos[1];
    matrix[3][2] = Pos[2];
}

static void validateParms( Spin *spin, spinParm *parm, int which )
{
    hwInt32
        n;

    switch( parm->spinType ) {
    case HW_SPIN_CONSTANT :
        if( parm->numVals != 3 )        parm->spinType = HW_SPIN_UNDEF;
        break;
    case HW_SPIN_RANDOM :
    case HW_SPIN_LINEAR :
        switch( which ) {
        case SPIN_POS : /* Position */
        case SPIN_SCALE :       /* Scale */
        case SPIN_COLOR : /* Color */
            if( parm->numVals != 6 )    parm->spinType = HW_SPIN_UNDEF;
            break;
        case SPIN_DIR : /* Direction */
        case SPIN_UP :  /* Up */
        case SPIN_TRANS :        /* Transparency */
            if( parm->numVals != 2 )    parm->spinType = HW_SPIN_UNDEF;
            break;
        }
        break;
    case HW_SPIN_SMOOTH :
        if( (parm->numVals % 3) != 0 )  parm->spinType = HW_SPIN_UNDEF;
        n = parm->numVals / 3;
        if( parm->numTimes != n ) {
            parm->numTimes = 0;
            if( parm->times ) {
                free( parm->times );
                parm->times = 0;
            }
        }
        parm->cooked = realloc( parm->cooked, n * 9 * sizeof(hwFloat) );
        if( parm->cooked ) {
            if(         (parm->vals[3*n-3] == parm->vals[0])
                &&      (parm->vals[3*n-2] == parm->vals[1])
                &&      (parm->vals[3*n-1] == parm->vals[2])    )
            {
                n = __hwIntCalcClosed( parm->vals, n-1, parm->cooked );
            }
            else {
                n = __hwIntCalcOpen( parm->vals, n, parm->cooked );
            }
            if( n == -1 )               parm->spinType = HW_SPIN_UNDEF;
            parm->numCooked = n;
        }
        else {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            parm->spinType = HW_SPIN_UNDEF;
        }
        break;
    }
}

static void draw( hwObject obj )
{
    Spin
        *spin = (Spin *)obj;
    int
        i, n;
    hwObject
        *objs;
    double
        elapsedTime;
    float
        matrix[4][4];
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( spin->invisible )                               return;

    elapsedTime = __hwDisp->getElapsedTime(__hwDisp);

    if( (elapsedTime < spin->lifetime[0]) ||
        (elapsedTime > spin->lifetime[1]) )
    {
        spin->prevTime = 1e38;
        return;
    }

#if 0
    if( spin->cycleTime <= 0.0 ) {
        spin->cycleTime = spin->lifetime[1] - spin->lifetime[0];
    }
#endif

    /* Validate the spinner */
    if( !spin->valid ) {
        validateParms( spin, &spin->pos,   SPIN_POS );
        validateParms( spin, &spin->dir,   SPIN_DIR );
        validateParms( spin, &spin->up,    SPIN_UP );
        validateParms( spin, &spin->scale, SPIN_SCALE );
        validateParms( spin, &spin->trans, SPIN_TRANS );
        validateParms( spin, &spin->color, SPIN_COLOR );
        spin->valid = 1;
    }

    /* Draw the objects */
    MakeMat( __hwDisp, spin, matrix );
    __hwDisp->pushMatrix( __hwDisp, matrix );

    n = spin->numObjs;
    objs = spin->objects;
    for( i = 0; i < n; i++ ) {
        __hwDisp->currentChild( __hwDisp, objs[i] );
        if( spin->trans.spinType != HW_SPIN_UNDEF ) {
            objs[i]->modify( objs[i], hwStrTransparency, HW_TYPE_1F,
                                &spin->currTrans );
        }
        if( spin->color.spinType != HW_SPIN_UNDEF ) {
            objs[i]->modify( objs[i], hwStrColor, HW_TYPE_3F,
                                spin->currColor );
        }
        objs[i]->draw( objs[i] );
    }

    __hwDisp->popMatrix( __hwDisp );
    /* All done! */
    LOG_EXIT
}

/*** EOF hwSpinner.c ***/
