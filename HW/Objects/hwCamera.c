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

/* Internal definitions of camera-related flags */
#define HW_POS          1
#define HW_DIR          2
#define HW_UP           3
#define HW_FIELD        4
#define HW_ASPECT       5
#define HW_PERSPECTIVE  6
#define HW_PLANES       7
#define HW_JITTER       8
#define HW_SKEW         9
#define HW_MIRROR       10

#define deg     * (3.141592653589 / 180.0)

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        hwStrPos, hwStrDir, hwStrUp, hwStrField, hwStrAspect,
        hwStrPerspective, hwStrPlanes, hwStrJitter, hwStrSkew,
        hwStrMirror,

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwCameraStruct = {
        0,              /* Parent - NULL */
        "hwCamera",     /* The name */
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwCamera = &hwCameraStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;
    hwCamStruct
        cam;
} Camera;

/* The hash table for camera strings */
static void *cameraTab = 0;

static hwObject create( hwObject proto )
{
    Camera
        *result;

    LOG_ENTRY
    if( proto != hwCamera )     return 0;

    HW_HASH_SETUP(cameraTab, 16 )
        HW_INSERT( hwStrPos,         HW_POS,         cameraTab );
        HW_INSERT( hwStrDir,         HW_DIR,         cameraTab );
        HW_INSERT( hwStrUp,          HW_UP,          cameraTab );
        HW_INSERT( hwStrField,       HW_FIELD,       cameraTab );
        HW_INSERT( hwStrAspect,      HW_ASPECT,      cameraTab );
        HW_INSERT( hwStrPlanes,      HW_PLANES,      cameraTab );
        HW_INSERT( hwStrJitter,      HW_JITTER,      cameraTab );
        HW_INSERT( hwStrSkew,        HW_SKEW,        cameraTab );
        HW_INSERT( hwStrMirror,      HW_MIRROR,      cameraTab );
        HW_INSERT( hwStrPerspective, HW_PERSPECTIVE, cameraTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Camera) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwCamera;
    result->refCount = 1;

    /* Default camera */
    result->cam.perspective = 0;
    result->cam.mirror = 0;
    result->cam.pos[0] = 0.; result->cam.pos[1] = 0.; result->cam.pos[2] = -1.4;
    result->cam.dir[0] = 0.; result->cam.dir[1] = 0.; result->cam.dir[2] = 1.;
    result->cam.up[0] = 0.; result->cam.up[1] = 1.0; result->cam.up[2] = 0.0;
    result->cam.field = 90.0;
    result->cam.aspect = 1.0;
    result->cam.planes[0] = 0.4; result->cam.planes[1] = 2.4;
    result->cam.jitter[0] = 0.0;
    result->cam.jitter[1] = 0.0;
    result->cam.skew[0] = 0.0;
    result->cam.skew[1] = 0.0;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Camera
        *cam = (Camera *)obj;

    LOG_ENTRY
    cam->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Camera
        *camera = (Camera *)obj;;

    LOG_ENTRY
    if( --camera->refCount > 0 )        return;
    free( camera );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Camera
        *camera = (Camera *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, cameraTab ) ) {
    case HW_PERSPECTIVE :
        if( type != HW_TYPE_1B )        goto BadType;
        camera->cam.perspective = *(hwInt32 *)val;
        break;
    case HW_MIRROR :
        if( type != HW_TYPE_1B )        goto BadType;
        camera->cam.mirror = *(hwInt32 *)val;
        break;
    case HW_FIELD :
        switch( type ) {
        case HW_TYPE_1I :
            camera->cam.field = *(hwInt32 *)val;
            break;
        case HW_TYPE_1F :
            camera->cam.field = *(hwFloat *)val;
            break;
        default :
            goto BadType;
        }
        break;
    case HW_JITTER :
        if( type != HW_TYPE_2F )        goto BadType;
        camera->cam.jitter[0] = ((hwFloat *)val)[0];
        camera->cam.jitter[1] = ((hwFloat *)val)[1];
        break;
    case HW_SKEW :
        if( type != HW_TYPE_2F )        goto BadType;
        camera->cam.skew[0] = ((hwFloat *)val)[0];
        camera->cam.skew[1] = ((hwFloat *)val)[1];
        break;
    case HW_POS :
        if( type != HW_TYPE_3F )        goto BadType;
        camera->cam.pos[0] = ((hwFloat *)val)[0];
        camera->cam.pos[1] = ((hwFloat *)val)[1];
        camera->cam.pos[2] = ((hwFloat *)val)[2];
        break;
    case HW_DIR :
        if( type != HW_TYPE_3F )        goto BadType;
        camera->cam.dir[0] = ((hwFloat *)val)[0];
        camera->cam.dir[1] = ((hwFloat *)val)[1];
        camera->cam.dir[2] = ((hwFloat *)val)[2];
        break;
    case HW_UP :
        if( type != HW_TYPE_3F )        goto BadType;
        camera->cam.up[0] = ((hwFloat *)val)[0];
        camera->cam.up[1] = ((hwFloat *)val)[1];
        camera->cam.up[2] = ((hwFloat *)val)[2];
        break;
    case HW_PLANES :
        if( type != HW_TYPE_2F )        goto BadType;
        camera->cam.planes[0] = ((hwFloat *)val)[0];
        camera->cam.planes[1] = ((hwFloat *)val)[1];
        break;
    default :
        __hwIntSetError( HW_ERROR_BAD_PROP );
        break;
    }
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Camera
        *camera = (Camera *)obj;

    LOG_ENTRY
    switch( hwLookup( prop, cameraTab ) ) {
    case HW_JITTER :
        *val = &camera->cam.jitter;
        return HW_TYPE_2F;
    case HW_SKEW :
        *val = &camera->cam.skew;
        return HW_TYPE_2F;
    case HW_PERSPECTIVE :
        *val = &camera->cam.perspective;
        return HW_TYPE_1B;
    case HW_MIRROR :
        *val = &camera->cam.mirror;
        return HW_TYPE_1B;
    case HW_FIELD :
        *val = &camera->cam.field;
        return HW_TYPE_1F;
    case HW_POS :
        *val = camera->cam.pos;
        return HW_TYPE_3F;
    case HW_DIR :
        *val = camera->cam.dir;
        return HW_TYPE_3F;
    case HW_UP :
        *val = camera->cam.up;
        return HW_TYPE_3F;
    case HW_PLANES :
        *val = camera->cam.planes;
        return HW_TYPE_2F;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    HW_USE_CURR_DISP;
    Camera
        *camera = (Camera *)obj;

    LOG_ENTRY
    __hwDisp->setCamera( __hwDisp, &camera->cam );
    LOG_EXIT
}

/*** EOF hwCamera.c ***/
