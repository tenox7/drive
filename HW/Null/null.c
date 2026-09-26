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

#define HW_NULL
#include "hw.h"
#include "hw_internal.h"

void __hwNullPosLight
(
    hwDisplay disp,
    hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
    hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
)
{
}

void __hwNullDirLight( hwDisplay disp, hwFloat Color[3], hwFloat Dir[3] )
{
}


void __hwNullDestroyTexture( hwDisplay disp, hwInt32 tid )
{
}

hwInt32 __hwNullCreateTexture( hwDisplay disp, hwImageStruct *img, hwInt32 filt, hwInt32 appl, hwInt32 bound, hwInt32 coordMode, hwOrientType *orient, hwInt32 internalFormat )
{
    return 0;
}

void __hwNullCurrentTexture( hwDisplay disp, hwInt32 tid, hwInt32 texNum )
{
}

void __hwNullDrawBBox( hwDisplay disp, hwFloat Point32s[24] )
{
}

int __hwNullBoundsVisible( hwDisplay disp, hwSurfaceType *surf, hwFloat BBox[6], hwInt32 complexity )
{
    return 1;
}

hwFloat __hwNullBoundsSize( hwDisplay disp, hwFloat BBox[6] )
{
    return 1.0;
}


void __hwNullCamera( hwDisplay disp, hwCamStruct *cam )
{
}

void __hwNullSurfAttrs( hwDisplay disp, hwSurfaceType *surf )
{
}

void __hwNullDrawMesh( hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m )
{
}

void __hwNullDrawStrip( hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n )
{
}

void __hwNullBackground( hwDisplay disp, hwFloat Color[3] )
{
}

void __hwNullLighting( hwDisplay disp, hwInt32 OnOff )
{
}

void __hwNullFog( hwDisplay disp, hwInt32 OnOff, hwFloat Color[3] )
{
}

void __hwNullFogParams( hwDisplay disp, hwInt32 t, hwFloat p[2], hwFloat d )
{
}

void __hwNullAmbient( hwDisplay disp, hwFloat Factor, hwFloat Color[3] )
{
}

void __hwNullDrawPolygon( hwDisplay disp, hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts )
{
}

void __hwNullDrawPolyline( hwDisplay disp, hwFloat *data, hwInt32 fl, hwInt32 n )
{
}

void __hwNullDrawMarkers( hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n )
{
}

void __hwNullDrawQuads( hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n )
{
}

hwDisplay __hwNullCreate( hwDisplay disp, hwDisplay shared,
                        OS_DISPLAY_TYPE display)
{
    struct __hwDisplayInternal
        *result;

    result = malloc(sizeof(struct __hwDisplayInternal));
    if( !result ) return 0;
    memset(result, 0, sizeof(struct __hwDisplayInternal));
    result->vtab = *disp;
    __hwInternalInitDisplay((hwDisplay)result, shared);

    return (hwDisplay)result;
}

hwInt32 __hwNullChooseVisual( hwDisplay display, hwInt32 required, hwInt32 desired )
{
    return desired | required;
}

/* Create a stoopid window of the given characteristics */
hwDrawable __hwNullCreateWin
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return (hwDrawable)1;
}

hwDrawable __hwNullCreateChildWin
(
    hwDisplay display,  /* Display to use */
    const char *winName,      /* Name of the window to create */
    OS_WINDOW_TYPE parent,  /* Name of parent window */
    hwInt32 winX, hwInt32 winY, /* Placement */
    hwInt32 winW, hwInt32 winH, /* Size */
    hwInt32 flags
)
{
    return (hwDrawable)1;
}

hwDrawable __hwNullInitDrawable( hwDisplay disp, OS_DRAWABLE_TYPE draw )
{
    return (hwDrawable)1;
}

void __hwNullMakeCurrent( hwDisplay disp, hwDrawable draw )
{
    __hwInternalMakeCurrent( disp );
}

const char *__hwNullGetInfo( hwDisplay disp, hwInt32 whichInfo )
{
    switch( whichInfo ) {
    case HW_INFO_HW :
        return "Null";
    case HW_INFO_VENDOR :
        return "Hoverware";
    case HW_INFO_RENDERER :
        return "HW Null Renderer";
    case HW_INFO_VERSION :
        return "1.0";
    default :
        return 0;
    }
}

void __hwNullViewport( hwDisplay disp, hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height )
{
}

void __hwNullScissor( hwDisplay disp, hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height )
{
}

void __hwNullPushMat( hwDisplay disp, hwFloat Mat[4][4] )
{
}

void __hwNullPopMat( hwDisplay disp )
{
}

void __hwNullXformPoint( hwDisplay disp, hwFloat point[3] )
{
}

void __hwNullUpdate( hwDisplay disp, hwInt32 flags )
{
}


hwInt32 __hwNullOpenList( hwDisplay disp )
{
    return -1;
}

void __hwNullCloseList( hwDisplay disp )
{
}

void __hwNullCallList( hwDisplay disp, hwInt32 dl )
{
}

void __hwNullDestroyList( hwDisplay disp, hwInt32 dl )
{
}

hwTextState *__hwNullGetTextState( hwDisplay disp )
{
    return 0;
}

void __hwNullIndexedTris
(
    hwDisplay disp,
    hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
    hwInt32 *indexList, hwInt32 numTris
)
{
}

void __hwNullSetVisibility( hwDisplay disp, hwInt32 bits )
{
}

void __hwNullSetInvisibility( hwDisplay disp, hwInt32 incl, hwInt32 excl )
{
}

hwInt32 __hwNullGetVisibility( hwDisplay disp )
{
    return 0;
}


OS_VISUAL_TYPE __hwNullExtractVisual( hwDisplay display )
{
    return 0;
}

OS_DRAWABLE_TYPE __hwNullExtractWin( hwDisplay disp, hwDrawable draw )
{
    return 0;
}

hwInt32 __hwNullSelectionMode( hwDisplay disp, hwInt32 onOff )
{
    return 0;
}

void __hwNullSelectionInfo( hwDisplay disp, hwInt32 f, hwFloat x, hwFloat y, hwFloat a )
{
}

hwInt32 __hwNullWasSelected( hwDisplay disp, hwObject *obj, hwInt32 *v )
{
    return 0;
}

void __hwNullCurrentObject( hwDisplay disp, hwObject obj )
{
}

void __hwNullCurrentChild( hwDisplay disp, hwObject obj )
{
}

void __hwNullSetDrawBuffer( hwDisplay disp, hwInt32 buff )
{
}

void __hwNullGuiText
(
    hwDisplay disp,
    TexFont *txf,
    hwInt32 color, hwInt32 halign, hwInt32 valign, hwInt32 height,
    hwInt32 x, hwInt32 y, unsigned char *text
)
{
}

extern void __hwNullGuiRaster
(
    hwDisplay disp,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 color,
    hwInt32 texId 
)
{
}

void __hwNullGuiRectangle
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 radius
)
{
}

void __hwNullGuiPolyline
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
}

void __hwNullCheckInput
(
    hwDisplay disp,
    void (*callback)( hwDrawable, hwWinEvent * )
)
{
}

void __hwNullGetMousePos
(
    hwDisplay disp,
    hwInt32 *x, hwInt32 *y
)
{
}

void __hwNullGetDrawSize( hwDisplay disp, hwInt32 *retSize )
{
}

double __hwNullGetCurrTime( hwDisplay disp )
{
    return 0.;
}

double __hwNullGetStartTime( hwDisplay disp )
{
    return 0.;
}

double __hwNullGetElapsedTime( hwDisplay disp )
{
    return 0.;
}

void __hwNullSetCurrTime( hwDisplay disp, double tm )
{
}

void __hwNullSetStartTime( hwDisplay disp, double tm )
{
}

void __hwNullSetElapsedTime( hwDisplay disp, double tm )
{
}

static const struct __hwDisplayStruct
    __hwNullInitFunc = {
        "NULL",
        __hwNullCreate,
        __hwNullChooseVisual,
        __hwNullExtractVisual,
        __hwNullExtractWin,
        __hwNullCreateWin,
        __hwNullCreateChildWin,
        __hwNullInitDrawable,
        __hwNullCheckInput,
        __hwNullGetMousePos,

        __hwNullMakeCurrent,
        __hwNullGetInfo,
        __hwNullViewport,
        __hwNullScissor,
        __hwNullCamera,
        __hwNullPushMat,
        __hwNullPopMat,
        __hwNullXformPoint,
        __hwNullUpdate,
        __hwNullSetDrawBuffer,

        __hwNullSelectionMode,
        __hwNullSelectionInfo,
        __hwNullWasSelected,
        __hwNullCurrentObject,
        __hwNullCurrentChild,

        __hwNullPosLight,
        __hwNullDirLight,
        __hwNullFog,
        __hwNullFogParams,
        __hwNullAmbient,
        __hwNullLighting,
        __hwNullBackground,

        __hwNullCreateTexture,
        __hwNullDestroyTexture,
        __hwNullCurrentTexture,

        __hwNullSurfAttrs,
        __hwNullSetVisibility,
        __hwNullSetInvisibility,
        __hwNullGetVisibility,
        __hwNullDrawBBox,
        __hwNullBoundsVisible,
        __hwNullBoundsSize,

        __hwNullDrawMesh,
        __hwNullDrawStrip,
        __hwNullDrawPolygon,
        __hwNullDrawPolyline,
        __hwNullDrawQuads,
        __hwNullIndexedTris,
        __hwNullDrawMarkers,

        __hwNullGuiText,
        __hwNullGuiRaster,
        __hwNullGuiRectangle,
        __hwNullGuiPolyline,
        __hwNullGuiPolyline, /* Lines == polyline == don't care :-) */
        __hwNullGuiPolyline, /* Polygon == polyline == don't care :-) */
        /* GUI DL routines have same protos as non-GUI DL routines */
        __hwNullOpenList,
        __hwNullCloseList,
        __hwNullCallList,
        __hwNullDestroyList,

        __hwNullOpenList,
        __hwNullCloseList,
        __hwNullCallList,
        __hwNullDestroyList,

        __hwNullGetTextState,
        __hwNullGetDrawSize,

        __hwNullGetCurrTime,
        __hwNullGetStartTime,
        __hwNullGetElapsedTime,
        __hwNullSetCurrTime,
        __hwNullSetStartTime,
        __hwNullSetElapsedTime,
    };

const hwDisplay
    hwNullDisplay = (const hwDisplay)&__hwNullInitFunc;
    // NULL does not define a default display.  If you use libhwnull,
    // you know what you are doing.

/*** EOF null.c ***/
