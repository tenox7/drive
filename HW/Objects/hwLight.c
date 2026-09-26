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

/* Internal definitions of light-related flags */
#define HW_COLOR        1
#define HW_POSITIONAL   2
#define HW_POS          3
#define HW_DIR          4
#define HW_CUTOFF       5
#define HW_EXP          6
#define HW_ATTEN        7

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void **val );
static void draw( hwObject );

static const char
    *propList[] = {
        hwStrColor, hwStrPositional, hwStrPos, hwStrDir,
        hwStrCutoff, hwStrLightExp, hwStrAtten,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwLightStruct = {
        0,              /* Parent - NULL */
        "hwLight",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwLight = &hwLightStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        positional;
    hwFloat
        color[3], pos[3], dir[3], cutoff, lightExp, atten;
} Light;

/* The hash table for light strings */
static void *lightTab = 0;

static hwObject create( hwObject proto )
{
    Light
        *result;

    LOG_ENTRY
    if( proto != hwLight )      return 0;

    HW_HASH_SETUP(lightTab, 16)
        HW_INSERT( hwStrColor,      HW_COLOR,      lightTab );
        HW_INSERT( hwStrPositional, HW_POSITIONAL, lightTab );
        HW_INSERT( hwStrPos,        HW_POS,        lightTab );
        HW_INSERT( hwStrDir,        HW_DIR,        lightTab );
        HW_INSERT( hwStrCutoff,     HW_CUTOFF,     lightTab );
        HW_INSERT( hwStrLightExp,   HW_EXP,        lightTab );
        HW_INSERT( hwStrAtten,      HW_ATTEN,      lightTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Light) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwLight;
    result->refCount = 1;

    /* Default light */
    result->positional = 0;
    result->color[0] = result->color[1] = result->color[2] = 1.0;
    result->pos[0] = 0.0; result->pos[1] = 0.0; result->pos[2] = -1.0;
    result->dir[0] = 0.0; result->dir[1] = 0.0; result->dir[2] = 1.0;
    result->cutoff = 0.0;
    result->lightExp = 0.0;
    result->atten = 0.0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Light
        *light = (Light *)obj;

    LOG_ENTRY
    light->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Light
        *light = (Light *)obj;;

    LOG_ENTRY
    if( --light->refCount > 0 ) return;
    free( light );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Light
        *light = (Light *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, lightTab ) ) {
    case HW_POSITIONAL :
        if( type != HW_TYPE_1B )        goto BadType;
        light->positional = *(hwInt32 *)val;
        break;
    case HW_CUTOFF :
        switch( type ) {
        case HW_TYPE_1I :
            light->cutoff = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            light->cutoff = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_EXP :
        switch( type ) {
        case HW_TYPE_1I :
            light->lightExp = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            light->lightExp = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_ATTEN :
        switch( type ) {
        case HW_TYPE_1I :
            light->atten = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            light->atten = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        light->color[0] = ((hwFloat *)val)[0];
        light->color[1] = ((hwFloat *)val)[1];
        light->color[2] = ((hwFloat *)val)[2];
        break;
    case HW_POS :
        if( type != HW_TYPE_3F )        goto BadType;
        light->pos[0] = ((hwFloat *)val)[0];
        light->pos[1] = ((hwFloat *)val)[1];
        light->pos[2] = ((hwFloat *)val)[2];
        break;
    case HW_DIR :
        if( type != HW_TYPE_3F )        goto BadType;
        light->dir[0] = ((hwFloat *)val)[0];
        light->dir[1] = ((hwFloat *)val)[1];
        light->dir[2] = ((hwFloat *)val)[2];
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
    Light
        *light = (Light *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, lightTab ) ) {
    case HW_POSITIONAL :
        *val = &light->positional;
        return HW_TYPE_1B;
    case HW_CUTOFF :
        *val = &light->cutoff;
        return HW_TYPE_1F;
    case HW_EXP :
        *val = &light->lightExp;
        return HW_TYPE_1F;
    case HW_ATTEN :
        *val = &light->atten;
        return HW_TYPE_1F;
    case HW_COLOR :
        *val = light->color;
        return HW_TYPE_3F;
    case HW_POS :
        *val = light->pos;
        return HW_TYPE_3F;
    case HW_DIR :
        *val = light->dir;
        return HW_TYPE_3F;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Light
        *light = (Light *)obj;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( light->positional ) {
        __hwDisp->positionalLight( __hwDisp,
                        light->color, light->pos, light->dir,
                        light->cutoff, light->lightExp, light->atten );
    }
    else {
        __hwDisp->directionalLight( __hwDisp,
                        light->color, light->dir );
    }
    LOG_EXIT
}

/*** EOF hwLight.c ***/
