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

/* Internal definitions of group-related flags */
#define HW_DATA         1
#define HW_INVISIBLE    2
#define HW_BBOX         3
#define HW_VISIBILITY   4
#define HW_LOD          5
#define HW_MATRICES     6
#define HW_OPT_FLAGS    7
#define HW_DIRTY        8

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to group */
        hwStrChildren, hwStrInvisible, hwStrVisibility, hwStrLOD,
        hwStrMatrices, hwStrOptFlags,

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwGroupStruct = {
        0,              /* Parent - NULL */
        "hwGroup",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwGroup = &hwGroupStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount;
    hwFloat
        BBox[6],                /* Bounding box of the group */
        matrix[4][4],           /* Matrix of the group */
        (*matrices)[4][4],      /* Array of matrices if specified */
        *LODs;                  /* LOD control array */
    hwInt32
        invisible,              /* Is it visible? */
        bboxValid,              /* Is the BBox valid? */
        initialized,            /* Has it been initialized? */
        optFlags,               /* Optimized? */
        displayList,            /* Fully cooked DL */
        numObjs,                /* Size of the object list */
        numLODs,                /* Size of LOD control array */
        numMatrices;            /* Size of matrix array */
    hwInt32
        visibility;             /* Visibility mask */
    hwObject
        *objects;               /* The list of objects */
    hwOrientType
        orient;                 /* The orientation of the group */
    int
        defaultOrient;          /* Is the orient just a default? */
} Group;

/* The hash table for group strings */
static void *groupTab = 0;

static hwObject create( hwObject proto )
{
    Group
        *result;

    LOG_ENTRY
    if( proto != hwGroup )      return 0;

    HW_HASH_SETUP(groupTab, 16)
        HW_INSERT( hwStrChildren,   HW_DATA,       groupTab );
        HW_INSERT( hwStrBounds,     HW_BBOX,       groupTab );
        HW_INSERT( hwStrInvisible,  HW_INVISIBLE,  groupTab );
        HW_INSERT( hwStrVisibility, HW_VISIBILITY, groupTab );
        HW_INSERT( hwStrLOD,        HW_LOD,        groupTab );
        HW_INSERT( hwStrMatrices,   HW_MATRICES,   groupTab );
        HW_INSERT( hwStrOptFlags,   HW_OPT_FLAGS,  groupTab );
        HW_INSERT( hwStrDirty,      HW_DIRTY,      groupTab );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Group) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwGroup;
    result->refCount = 1;

    /* Default group */
    result->bboxValid = 0;
    result->numObjs = 0;
    result->initialized = 0;
    result->objects = 0;
    result->numLODs = 0;
    result->LODs = 0;
    result->numMatrices = 0;
    result->matrices = 0;
    result->optFlags = 0;
    result->displayList = -1;

    result->invisible = 0;
    result->visibility = 0xFFFFFFFFL;

    hwDefaultOrient( &result->orient );
    result->defaultOrient = 1;

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Group
        *group = (Group *)obj;

    LOG_ENTRY
    group->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    Group
        *group = (Group *)obj;
    int
        i;

    LOG_ENTRY
    if( --group->refCount > 0 ) return;
    if( group->objects ) {
        for( i = 0; i < group->numObjs; i++ ) {
            group->objects[i]->destroy( group->objects[i] );
        }
        free( group->objects );
    }
    if( group->LODs ) free( group->LODs );
    if( group->matrices ) free( group->matrices );
    free( group );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Group
        *group = (Group *)obj;
    int
        dirty = 1,
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, groupTab ) ) {
    case HW_DATA :
        if( HW_GET_BASE(type) != HW_TYPE_OBJECT ) {
            __hwIntSetError( HW_ERROR_BAD_TYPE );
            break;
        }
        if( group->objects ) {
            n = group->numObjs;
            for( i = 0; i < n; i++ ) {
                group->objects[i]->destroy( group->objects[i] );
            }
            free( group->objects );
            group->objects = 0;
        }
        n = HW_GET_COUNT( type );
        group->objects = malloc( n * sizeof(hwObject) );
        if( !group->objects ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)memcpy( group->objects, val, n*sizeof(hwObject) );
        group->numObjs = n;
        for( i = 0; i < n; i++ ) {
            group->objects[i]->addref( group->objects[i] );
        }
        break;
    case HW_INVISIBLE :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )           group->invisible = 1;
        else                            group->invisible = 0;
        break;
    case HW_VISIBILITY :
        if( type != HW_TYPE_1I )        goto BadType;
        group->visibility = *(hwInt32 *)val;
        break;
    case HW_LOD :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        if( n & 1 )     goto BadType;

        if( group->LODs ) free( group->LODs );
        group->LODs = malloc( n * sizeof(hwFloat) );
        if( !group->LODs ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)memcpy( group->LODs, val, n*sizeof(hwFloat) );
        group->numLODs = n / 2;
        break;
    case HW_MATRICES :
        if( HW_GET_BASE(type) != HW_TYPE_FLOAT )        goto BadType;
        n = HW_GET_COUNT( type );
        if( n % 16 )    goto BadType;

        if( group->matrices )   free( group->matrices );
        group->matrices = malloc( n * sizeof(hwFloat) );
        if( !group->matrices ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            break;
        }
        (void)memcpy( group->matrices, val, n*sizeof(hwFloat) );
        group->numMatrices = n / 16;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        n = *(hwInt32 *)val;
        if( n & HW_OPT_USE_DL ) {
            group->optFlags = HW_OPT_USE_DL;
        }
        else if( !n ) {
            group->optFlags = 0;
        }
        break;
    case HW_DIRTY :
        /* FALLTHROUGH! */
    default :
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            __hwIntOrientModify( &group->orient, n, type, val );

            /* Now for a bit of cleverness.  We turn around and
             * ask if the props are still default.  Only if they
             * are NOT do we set the defaultOrient to 0.
             */
            n = __hwIntOrientInquire( &group->orient, n, (void **)&val );
            if( !(n & HW_TYPE_CLEAN) ) {
                group->defaultOrient = 0;
            }
        }
        else if( group->objects ) {
            n = group->numObjs;
            for( i = 0; i < n; i++ ) {
                group->objects[i]->modify( group->objects[i], prop, type, val );
            }
        }
        break;
    }
    group->initialized = 0;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
}

static void cook( Group *group )
{
    hwInt32
        i, j, n;
    hwFloat
        *Ptr,
        Points[8*3],
        BBox[6];
    void
        *VPtr;
    hwObject
        *objs;

    LOG_ENTRY
    n = group->numObjs;
    objs = group->objects;
    if( !objs ) return;

    if( group->displayList >= 0 ) {
        __hwIntDestroyList( group->displayList );
        group->displayList = -1;
    }

    if( !group->defaultOrient ) {
        hwMakeMatrix( &group->orient, group->matrix );
    }

    group->BBox[0] = group->BBox[1] = group->BBox[2] = HW_MAX_FLOAT;
    group->BBox[3] = group->BBox[4] = group->BBox[5] = -HW_MAX_FLOAT;
    group->bboxValid = 1;
    for( i = 0; i < n; i++ ) {
        if( objs[i]->inquire( objs[i], hwStrBounds, &VPtr )
                    == HW_MAKE_TYPE(HW_TYPE_FLOAT,6) )
        {
            Ptr = VPtr;
            if( i < group->numMatrices ) {
                Points[ 0]=Ptr[0]; Points[ 1]=Ptr[1]; Points[ 2]=Ptr[2];
                Points[ 3]=Ptr[3]; Points[ 4]=Ptr[1]; Points[ 5]=Ptr[2];
                Points[ 6]=Ptr[0]; Points[ 7]=Ptr[4]; Points[ 8]=Ptr[2];
                Points[ 9]=Ptr[3]; Points[10]=Ptr[4]; Points[11]=Ptr[2];

                Points[12]=Ptr[0]; Points[13]=Ptr[1]; Points[14]=Ptr[5];
                Points[15]=Ptr[3]; Points[16]=Ptr[1]; Points[17]=Ptr[5];
                Points[18]=Ptr[0]; Points[19]=Ptr[4]; Points[20]=Ptr[5];
                Points[21]=Ptr[3]; Points[22]=Ptr[4]; Points[23]=Ptr[5];

                __hwIntMatXformData( group->matrices[i], Points, 8, 0 );

                Ptr = Points;
                for( j = 0; j < 8; j++, Ptr += 3 ) {
                    if( Ptr[0] < group->BBox[0] )       group->BBox[0] = Ptr[0];
                    if( Ptr[1] < group->BBox[1] )       group->BBox[1] = Ptr[1];
                    if( Ptr[2] < group->BBox[2] )       group->BBox[2] = Ptr[2];
                    if( Ptr[0] > group->BBox[3] )       group->BBox[3] = Ptr[0];
                    if( Ptr[1] > group->BBox[4] )       group->BBox[4] = Ptr[1];
                    if( Ptr[2] > group->BBox[5] )       group->BBox[5] = Ptr[2];
                }
            }
            else {
                if( Ptr[0] < group->BBox[0] )   group->BBox[0] = Ptr[0];
                if( Ptr[1] < group->BBox[1] )   group->BBox[1] = Ptr[1];
                if( Ptr[2] < group->BBox[2] )   group->BBox[2] = Ptr[2];
                if( Ptr[3] > group->BBox[3] )   group->BBox[3] = Ptr[3];
                if( Ptr[4] > group->BBox[4] )   group->BBox[4] = Ptr[4];
                if( Ptr[5] > group->BBox[5] )   group->BBox[5] = Ptr[5];
            }
        }
        else {
            group->bboxValid = 0;
        }
    }

    if( group->bboxValid ) {
        /* Now, create points with all of the corners of the bounding
         * box, then transform it, then get the bounds of *that* box.
         */
        (void)memcpy( BBox, group->BBox, 6*sizeof(hwFloat) );
        Points[ 0] = BBox[0]; Points[ 1] = BBox[1]; Points[ 2] = BBox[2];
        Points[ 3] = BBox[3]; Points[ 4] = BBox[1]; Points[ 5] = BBox[2];
        Points[ 6] = BBox[0]; Points[ 7] = BBox[4]; Points[ 8] = BBox[2];
        Points[ 9] = BBox[3]; Points[10] = BBox[4]; Points[11] = BBox[2];

        Points[12] = BBox[0]; Points[13] = BBox[1]; Points[14] = BBox[5];
        Points[15] = BBox[3]; Points[16] = BBox[1]; Points[17] = BBox[5];
        Points[18] = BBox[0]; Points[19] = BBox[4]; Points[20] = BBox[5];
        Points[21] = BBox[3]; Points[22] = BBox[4]; Points[23] = BBox[5];

        __hwIntTransformData( &group->orient, Points, 8, 0 );

        group->BBox[0] = group->BBox[1] = group->BBox[2] = HW_MAX_FLOAT;
        group->BBox[3] = group->BBox[4] = group->BBox[5] = -HW_MAX_FLOAT;

        __hwIntUpdateBounds( group->BBox, Points, 8, 3 );
    }

    group->initialized = 1;
    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Group
        *group = (Group *)obj;
    hwInt32
        result;
    int
        i, n;

    LOG_ENTRY
    switch( hwLookup( prop, groupTab ) ) {
    case HW_DATA :
        if( !group->numObjs ) return 0;
        *val = group->objects;
        return HW_MAKE_TYPE(HW_TYPE_OBJECT,group->numObjs);
    case HW_INVISIBLE :
        *val = &group->invisible;
        return HW_TYPE_1B;
    case HW_VISIBILITY :
        *val = &group->visibility;
        return HW_TYPE_1I;
    case HW_BBOX :
        if( !group->initialized ) cook( group );
        if( !group->bboxValid ) return 0;
        *val = group->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_LOD :
        if( !group->LODs )      return 0;
        *val = group->LODs;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,2*group->numLODs);
    case HW_MATRICES :
        if( !group->matrices )  return 0;
        *val = group->matrices;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,16*group->numMatrices);
        break;
    case HW_OPT_FLAGS :
        *val = &group->optFlags;
        return HW_TYPE_1I;
    default :
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &group->orient, n, val );
        }
        else {
            n = group->numObjs;
            for( i = 0; i < n; i++ ) {
                obj = group->objects[i];
                result = obj->inquire( obj, prop, val );
                if( result ) return result;
            }
        }
        break;
    }
    __hwIntSetError( HW_ERROR_BAD_PROP );
    LOG_EXIT
    return 0;
}

static void draw( hwObject obj )
{
    Group
        *group = (Group *)obj;
    hwInt32
        i, j, n, LODvalid = 0;
    hwObject
        *objs;
    hwFloat
        LODsize;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    if( !group->initialized )   cook( group );

    if( group->invisible )      return;

    intDisp->scratchSurf.visibility = group->visibility;
    intDisp->scratchSurf.flags = 0;

    if( group->bboxValid ) {
        if( !__hwDisp->boundsVisible( __hwDisp,
                    &intDisp->scratchSurf, group->BBox, group->numObjs*20 ) )
        {
            return;
        }
        if( group->LODs ) {
            LODvalid = 1;
            LODsize = __hwDisp->boundsSize( __hwDisp,
                                                   group->BBox );
        }
    }
    else {
        if( !__hwDisp->boundsVisible(__hwDisp,
                                            &intDisp->scratchSurf, 0, 0 ) )
        {
            return;
        }
    }

    if( LODvalid ) {
        group->optFlags &= ~HW_OPT_USE_DL;
    }

    if( group->optFlags & HW_OPT_USE_DL ) {
        if( group->displayList >= 0 ) {
            __hwDisp->callList( __hwDisp, group->displayList );
            return;
        }
        group->displayList = __hwDisp->openList( __hwDisp );
        if( group->displayList < 0 ) {
            group->optFlags &= ~HW_OPT_USE_DL;
        }
    }

    if( !group->defaultOrient ) {
        __hwDisp->pushMatrix( __hwDisp, group->matrix );
    }

    n = group->numObjs;
    objs = group->objects;

    if( LODvalid ) {
        for( i = 0; i < n; i++ ) {
            if( i < group->numLODs ) {
                if(        (LODsize >= group->LODs[2*i])
                        && (LODsize < group->LODs[2*i+1]) )
                {
                    if( i < group->numMatrices ) {
                        __hwDisp->pushMatrix( __hwDisp,
                                                     group->matrices[i] );
                    }
                    __hwDisp->currentChild( __hwDisp, objs[i] );
                    objs[i]->draw( objs[i] );
                    if( i < group->numMatrices ) {
                        __hwDisp->popMatrix( __hwDisp );
                    }
                }
            }
            else {
                if( i < group->numMatrices ) {
                    __hwDisp->pushMatrix( __hwDisp,
                                                 group->matrices[i] );
                }
                __hwDisp->currentChild( __hwDisp, objs[i] );
                objs[i]->draw( objs[i] );
                if( i < group->numMatrices ) {
                    __hwDisp->popMatrix( __hwDisp );
                }
            }
        }
    }
    else {
        for( i = 0; i < n; i++ ) {
            if( i < group->numMatrices ) {
                __hwDisp->pushMatrix( __hwDisp,
                                             group->matrices[i] );
            }
            __hwDisp->currentChild( __hwDisp, objs[i] );
            objs[i]->draw( objs[i] );
            if( i < group->numMatrices ) {
                __hwDisp->popMatrix( __hwDisp );
            }
        }
    }

    if( !group->defaultOrient ) {
        __hwDisp->popMatrix( __hwDisp );
    }

    if( group->optFlags & HW_OPT_USE_DL ) {
        __hwDisp->closeList( __hwDisp );
        __hwDisp->callList( __hwDisp, group->displayList );
    }

    LOG_EXIT
}

/*** EOF hwGroup.c ***/
