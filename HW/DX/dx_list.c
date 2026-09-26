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
#include "hw_dx.h"

/* Here we plug in the display list functions */
hwInt32 __hwDxOpenList( hwDisplay disp )
{
#if 1
    return -1;
#else
    struct dxDispList
        *newList;
    int
        i, n;

    if( !__hwdxCurrentContext )                 return -1;
    if( __hwdxCurrentContext->openList )        return -1;

    newList = malloc( sizeof(struct dxDispList) );
    if( !newList )      return -1;

    newList->dlName = i = __hwdxCurrentContext->segNum++;
    newList->head = newList->tail = 0;

    n = i % DX_DL_HASH_SIZE;
    newList->nextHash = __hwdxCurrentContext->displayHash[n];
    __hwdxCurrentContext->displayHash[n] = newList;
    __hwdxCurrentContext->openList = newList;

    return i;
#endif
}

void __hwDxCloseList( hwDisplay disp )
{
    if( !__hwdxCurrentContext )                 return;
    if( !__hwdxCurrentContext->openList )       return;

    /* Not really a whole lot to do here :-) */
    __hwdxCurrentContext->openList = 0;
}

void __hwDxCallList( hwDisplay disp, hwInt32 dl )
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;
    int
        i, n, selecting, pick;
    hwSelectionType
        *sel;
    float
        *ptr;

    if( !__hwdxCurrentContext )                 return;

    if( __hwdxCurrentContext->openList ) {
        __hwdxAppendCallList( dl );
    }
    else {
        /* Find the list in the hash table */
        n = dl % DX_DL_HASH_SIZE;
        for(    currList = __hwdxCurrentContext->displayHash[n];
                currList && currList->dlName != dl;
                currList = currList->nextHash )
        {
            /* NOTHING */
        }
        if( !currList ) return;

        selecting = (__hwdxCurrentContext->renderMode & HW_RENDER_MASK)
                        == HW_RENDER_SELECT;
        sel = &__hwdxCurrentContext->sel;

        for(    currCall = currList->head;
                currCall;
                currCall = currCall->genericData.next )
        {
            switch( currCall->genericData.primType ) {
            case DX_DL_POLYGON :
                if( selecting ) {
                    pick = __hwIntPickPolygon(
                                currCall->stripData.verts,
                                3 + currCall->stripData.extData,
                                currCall->stripData.vertCount,
                                sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }
                }
                else {
                    /*polygon_with_data3d( gfd,
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                currCall->stripData.extData,
                                currCall->stripData.vertFlags,
                                CLOCKWISE );*/
                }
                break;
            case DX_DL_POLYLINE :
                if( selecting ) {
                    /* TBD: Move/draw flags */
                    pick = __hwIntPickPolyline(
                                currCall->stripData.verts,
                                3+currCall->stripData.extData,
                                currCall->stripData.vertCount,
                                0,
                                sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }
                }
                else {
                    /*polyline3d(       gfd,
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                currCall->stripData.vertFlags );*/
                }
                break;
            case DX_DL_TRISTRIP :
                if( selecting ) {
                    pick = __hwIntPickStrip(
                                currCall->stripData.verts,
                                3 + currCall->stripData.extData,
                                currCall->stripData.vertCount,
                                sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }
                }
                else {
                    /*triangular_strip_with_data( gfd,
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                (float *)0,
                                currCall->stripData.extData,
                                currCall->stripData.vertFlags,
                                CLOCKWISE );*/
                }
                break;
            case DX_DL_POLYMARKER :
                if( selecting ) {
                    pick = __hwIntPickMarkers(
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                3+currCall->stripData.extData, sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }
                }
                else {
                    if( currCall->stripData.extData ) {
                        n = currCall->stripData.vertCount;
                        ptr = currCall->stripData.verts;
                        for( i = 0; i < n; i++ ) {
                            /*marker_color( gfd, ptr[3], ptr[4], ptr[5] );
                            polymarker3d( gfd, ptr, 1, FALSE );*/
                            ptr += 6;
                        }
                    }
                    else {
                        /*polymarker3d( gfd,
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                FALSE );*/
                    }
                }
                break;
            case DX_DL_POLYHEDRON :
                if( selecting ) {
                    pick = __hwIntPickPolyhedron(
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                3 + currCall->stripData.extData,
                                (hwInt32 *)currCall->polyhedronData.indexList,
                                currCall->polyhedronData.primCount,
                                sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }
                }
                else {
                    /*polyhedron_with_data( gfd,
                                currCall->polyhedronData.verts,
                                currCall->polyhedronData.vertCount,
                                currCall->polyhedronData.extData,
                                currCall->polyhedronData.vertFlags,
                                currCall->polyhedronData.indexList,
                                (float *)0,
                                currCall->polyhedronData.primCount,
                                CLOCKWISE );*/
                }
                break;
            case DX_DL_TRIANGLES :
                if( selecting ) {
                    // TBD
                    /*pick = __hwIntPickStrip(
                                currCall->stripData.verts,
                                3 + currCall->stripData.extData,
                                currCall->stripData.vertCount,
                                sel );
                    if( pick ) {
                        sel->picked = 1;
                        sel->obj = __hwdxCurrentContext->currObj;
                    }*/
                }
                else {
                    /*triangular_strip_with_data( gfd,
                                currCall->stripData.verts,
                                currCall->stripData.vertCount,
                                (float *)0,
                                currCall->stripData.extData,
                                currCall->stripData.vertFlags,
                                CLOCKWISE );*/
                }
                break;
            case DX_DL_PUSH_MATRIX :
                __hwDxPushMat( disp, currCall->matrixData.mat );
                break;
            case DX_DL_POP_MATRIX :
                __hwDxPopMat( disp );
                break;
            case DX_DL_SURF_ATTRS :
                __hwDxSurfAttrs( disp, &currCall->surfData.surf );
                break;
            case DX_DL_CALL_LIST :
                __hwDxCallList( disp,
                                currCall->callListData.listIndex );
                break;
            }
        }
    }
}

void __hwDxDestroyList( hwDisplay disp, hwInt32 dl )
{
    struct dxDispList
        *prevList,
        *currList;
    union dxCallData
        *currCall,
        *nextCall;
    int
        n;

    if( !__hwdxCurrentContext )                 return;
    if( __hwdxCurrentContext->openList )        return;

    n = dl % DX_DL_HASH_SIZE;
    for(        prevList = 0, currList = __hwdxCurrentContext->displayHash[n];
                currList && currList->dlName != dl;
                prevList = currList, currList = currList->nextHash )
    {
        /* NOTHING */
    }
    if( !currList )     return;

    /* Delete all of the calls */
    for( currCall = currList->head; currCall; currCall = nextCall ) {
        nextCall = currCall->genericData.next;
        /* Because of the way we allocate, we free each call with
         * exactly ONE call to free.  So there.
         */
        free( currCall );
    }

    /* Delete this element from the list */
    if( prevList ) {
        prevList->nextHash = currList->nextHash;
    }
    else {
        __hwdxCurrentContext->displayHash[n] = currList->nextHash;
    }
    free( currList );
}

void __hwdxAppendStrip
(
    int primType, float *verts, int vertCount,
    int extData, int vertFlags
)
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;
    int
        n;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    n = vertCount * (extData + 3) * sizeof(float);
    currCall = malloc( sizeof(currCall->stripData) + n );
    if( !currCall )     return;

    /* Initialize result */
    currCall->stripData.primType = primType;
    currCall->stripData.next = 0;
    currCall->stripData.verts = (float *)(sizeof(currCall->stripData)
                                                + (char *)currCall);
    currCall->stripData.vertCount = vertCount;
    currCall->stripData.extData = extData;
    currCall->stripData.vertFlags = vertFlags;
    (void)memcpy( currCall->stripData.verts, verts, n );

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}

void __hwdxAppendPolyhedron
(
    float *verts, int vertCount,
    int extData, int vertFlags,
    int *indexList, int primCount,
    int primSize
)
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;
    int
        n, m;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    n = vertCount * (extData + 3) * sizeof(float);
    m = primCount * (primSize + 2) * sizeof(int);
    currCall = malloc( sizeof(currCall->polyhedronData) + n + m );
    if( !currCall )     return;

    /* Initialize result */
    currCall->polyhedronData.primType = DX_DL_POLYHEDRON;
    currCall->polyhedronData.next = 0;
    currCall->polyhedronData.verts = (float *)(sizeof(currCall->polyhedronData)
                                                + (char *)currCall);
    currCall->polyhedronData.vertCount = vertCount;
    currCall->polyhedronData.extData = extData;
    currCall->polyhedronData.vertFlags = vertFlags;
    currCall->polyhedronData.indexList= (int *)(sizeof(currCall->polyhedronData)
                                                + n + (char *)currCall);
    currCall->polyhedronData.primCount = primCount;
    (void)memcpy( currCall->polyhedronData.verts, verts, n );
    (void)memcpy( currCall->polyhedronData.indexList, indexList, m );

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}

void __hwdxAppendPushMat( float mat[4][4] )
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    currCall = malloc( sizeof(currCall->matrixData) );
    if( !currCall )     return;

    /* Initialize result */
    currCall->matrixData.primType = DX_DL_PUSH_MATRIX;
    currCall->matrixData.next = 0;
    (void)memcpy( currCall->matrixData.mat, mat, 4*4*sizeof(float) );

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}

void __hwdxAppendPopMat( void )
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    currCall = malloc( sizeof(currCall->genericData) );
    if( !currCall )     return;

    /* Initialize result */
    currCall->genericData.primType = DX_DL_POP_MATRIX;
    currCall->genericData.next = 0;

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}

void __hwdxAppendSurf( hwSurfaceType *surf )
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    currCall = malloc( sizeof(currCall->surfData) );
    if( !currCall )     return;

    /* Initialize result */
    currCall->surfData.primType = DX_DL_SURF_ATTRS;
    currCall->surfData.next = 0;
    currCall->surfData.surf = *surf;

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}

void __hwdxAppendCallList( int dl )
{
    struct dxDispList
        *currList;
    union dxCallData
        *currCall;

    if( !__hwdxCurrentContext )                 return;

    currList = __hwdxCurrentContext->openList;
    if( !currList )                             return;

    /* Allocate data */
    currCall = malloc( sizeof(currCall->callListData) );
    if( !currCall )     return;

    /* Initialize result */
    currCall->callListData.primType = DX_DL_CALL_LIST;
    currCall->callListData.next = 0;
    currCall->callListData.listIndex = dl;

    /* Stick it in the list */
    if( currList->tail )        currList->tail->genericData.next = currCall;
    else                        currList->head = currCall;
    currList->tail = currCall;
}
