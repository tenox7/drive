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

/* Internal definitions of environ-related flags */
#define HW_BACKGROUND           1
#define HW_AMBIENT_FACTOR       2
#define HW_AMBIENT_COLOR        3
#define HW_FOG                  4
#define HW_FOG_COLOR            5
#define HW_FOG_PLANES           6
#define HW_FOG_TYPE             7
#define HW_FOG_DENSITY          8
#define HW_LIGHTING             9

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        hwStrBackgroundColor, hwStrAmbientFactor, hwStrAmbientColor,
        hwStrFog, hwStrFogColor, hwStrLighting,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwEnvironStruct = {
        0,              /* Parent - NULL */
        "hwEnviron",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwEnviron = &hwEnvironStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        lighting, fog, fogType;
    hwFloat
        backgroundColor[3],
        ambientFactor,
        ambientColor[3],
        fogColor[3],
        fogDensity,
        fogPlanes[2];
} Environ;

/* The hash table for environ strings */
static void *environTab = 0;

static hwObject create( hwObject proto )
{
    Environ
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwEnviron )    return 0;

    HW_HASH_SETUP(environTab, 16)
        t = environTab;
        HW_INSERT(hwStrBackgroundColor, HW_BACKGROUND,     t);
        HW_INSERT(hwStrAmbientFactor,   HW_AMBIENT_FACTOR, t);
        HW_INSERT(hwStrAmbientColor,    HW_AMBIENT_COLOR,  t);
        HW_INSERT(hwStrFog,             HW_FOG,            t);
        HW_INSERT(hwStrFogColor,        HW_FOG_COLOR,      t);
        HW_INSERT(hwStrFogPlanes,       HW_FOG_PLANES,     t);
        HW_INSERT(hwStrFogType,         HW_FOG_TYPE,       t);
        HW_INSERT(hwStrFogDensity,      HW_FOG_DENSITY,    t);
        HW_INSERT(hwStrLighting,        HW_LIGHTING,       t);
    HW_HASH_CLEANUP

    result = malloc( sizeof(Environ) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwEnviron;
    result->refCount = 1;

    /* Default environ */
    result->ambientFactor = 0.5;
    result->ambientColor[0] = result->ambientColor[1]
                = result->ambientColor[2] = 1.0;
    result->fog = 0;
    result->fogType = HW_FOG_EXP;
    result->fogDensity = 1.0f;
    result->fogPlanes[0] = 0.f;
    result->fogPlanes[1] = 1.f;
    result->fogColor[0] = result->fogColor[1]
                = result->fogColor[2] = 0.0;
    result->lighting = 1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Environ
        *env = (Environ *)obj;

    LOG_ENTRY
    env->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Environ
        *env = (Environ *)obj;

    LOG_ENTRY
    if( --env->refCount > 0 )       return;
    free( env );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Environ
        *env = (Environ *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, environTab ) ) {
    case HW_LIGHTING :
        if( type != HW_TYPE_1B )        goto BadType;
        env->lighting = *(hwInt32 *)val;
        break;
    case HW_FOG :
        if( type != HW_TYPE_1B )        goto BadType;
        env->fog = *(hwInt32 *)val;
        break;
    case HW_FOG_TYPE :
        if( type != HW_TYPE_1I )        goto BadType;
        env->fogType = *(hwInt32 *)val;
        break;
    case HW_FOG_DENSITY :
        if( type != HW_TYPE_1F )        goto BadType;
        env->fogDensity = *(hwFloat *)val;
        break;
    case HW_FOG_PLANES :
        if( type != HW_TYPE_2F )        goto BadType;
        env->fogPlanes[0] = ((hwFloat *)val)[0];
        env->fogPlanes[1] = ((hwFloat *)val)[1];
        break;
    case HW_AMBIENT_FACTOR :
        if( type != HW_TYPE_1F )        goto BadType;
        env->ambientFactor = *(hwFloat *)val;
        break;
    case HW_BACKGROUND :
        if( type != HW_TYPE_3F )        goto BadType;
        env->backgroundColor[0] = ((hwFloat *)val)[0];
        env->backgroundColor[1] = ((hwFloat *)val)[1];
        env->backgroundColor[2] = ((hwFloat *)val)[2];
        break;
    case HW_AMBIENT_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        env->ambientColor[0] = ((hwFloat *)val)[0];
        env->ambientColor[1] = ((hwFloat *)val)[1];
        env->ambientColor[2] = ((hwFloat *)val)[2];
        break;
    case HW_FOG_COLOR :
        if( type != HW_TYPE_3F )        goto BadType;
        env->fogColor[0] = ((hwFloat *)val)[0];
        env->fogColor[1] = ((hwFloat *)val)[1];
        env->fogColor[2] = ((hwFloat *)val)[2];
        break;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }
    LOG_EXIT
    return;

BadType:
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Environ
        *env = (Environ *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, environTab ) ) {
    case HW_LIGHTING :
        *val = &env->lighting;
        return HW_TYPE_1B;
    case HW_FOG :
        *val = &env->fog;
        return HW_TYPE_1B;
    case HW_FOG_TYPE :
        *val = &env->fogType;
        return HW_TYPE_1I;
    case HW_FOG_DENSITY :
        *val = &env->fogDensity;
        return HW_TYPE_1F;
    case HW_FOG_PLANES :
        *val = env->fogPlanes;
        return HW_TYPE_2F;
    case HW_AMBIENT_FACTOR :
        *val = &env->ambientFactor;
        return HW_TYPE_1F;
    case HW_BACKGROUND :
        *val = env->backgroundColor;
        return HW_TYPE_3F;
    case HW_AMBIENT_COLOR :
        *val = env->ambientColor;
        return HW_TYPE_3F;
    case HW_FOG_COLOR :
        *val = env->fogColor;
        return HW_TYPE_3F;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Environ
        *env = (Environ *)obj;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    __hwDisp->backgroundColor( __hwDisp,
                env->backgroundColor );
    __hwDisp->enableLighting( __hwDisp,  env->lighting );
    if( env->lighting ) {
        __hwDisp->doFog( __hwDisp,
                        env->fog, env->fogColor );
        __hwDisp->fogParams( __hwDisp,
                        env->fogType,
                        env->fogPlanes, env->fogDensity );
        __hwDisp->ambientLight( __hwDisp,
                        env->ambientFactor, env->ambientColor );
    }
    LOG_EXIT
}

/*** EOF hwEnviron.c ***/
