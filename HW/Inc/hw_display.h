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

#include "TexFont.h"

/* A HoverWare drawable object (opaque) */
typedef struct _hwDrawStruct *hwDrawable;

struct __hwDisplayStruct {
    const char *name;   /* Name of the hwDisplay */

    /********* Initialization and visual selection stuff *********/
    hwDisplay (*create)( hwDisplay proto, hwDisplay shared, OS_DISPLAY_TYPE );

    hwInt32 (*chooseVisual)
    (
        hwDisplay display, hwInt32 required, hwInt32 desired
    );

    OS_VISUAL_TYPE (*extractVisual)( hwDisplay display );

    OS_DRAWABLE_TYPE (*extractWindow)( hwDisplay disp, hwDrawable draw );

    /* Create a stoopid window of the given characteristics */
    hwDrawable (*createWindow) 
    (
        hwDisplay display,              /* Display to use */
        const char *winName,            /* Name of the window to create */
        hwInt32 winX, hwInt32 winY,     /*Placement*/
        hwInt32 winW, hwInt32 winH,     /* Size */
        hwInt32 flags                   /* Window flags */
    );

    /* Create a child window of the given characteristics */
    hwDrawable (*createChildWindow) 
    (
        hwDisplay display,              /* Display to use */
        const char *winName,            /* Name of the window to create */
        OS_WINDOW_TYPE parent,          /* Name of the parent */
        hwInt32 winX, hwInt32 winY,     /* Placement*/
        hwInt32 winW, hwInt32 winH,     /* Size */
        hwInt32 flags                   /* Window flags */
    );

    hwDrawable (*initDrawable)( hwDisplay display, OS_DRAWABLE_TYPE draw );

    /******** Window system events ****************************/
    void (*inputHandler)( hwDisplay disp,
                    void (*callback)( hwDrawable draw, hwWinEvent *event ) );
    void (*getMousePos)( hwDisplay disp, hwInt32 *x, hwInt32 *y );

    /******** Prepare to draw/finish drawing functions ********/
    void (*makeCurrent)( hwDisplay disp, hwDrawable draw );

    /* Get info about current drawable */
    const char *(*getInfo)( hwDisplay disp, hwInt32 whichInfo );

    void (*viewport)(
        hwDisplay disp,
        hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height
    );

    void (*scissor)(
        hwDisplay disp,
        hwInt32 x, hwInt32 y, hwInt32 width, hwInt32 height
    );

    void (*setCamera)( hwDisplay disp, hwCamStruct *cam );

    void (*pushMatrix)( hwDisplay disp, hwFloat Mat[4][4] );

    void (*popMatrix)( hwDisplay disp );

    void (*xformPoint)( hwDisplay disp, hwFloat point[3] );

    void (*update)( hwDisplay disp, hwInt32 doSwap );

    void (*setDrawBuffer)( hwDisplay disp, hwInt32 whichBuffer );

    /********* Selection *********/
    hwInt32 (*renderMode)( hwDisplay disp, hwInt32 mode );

    void (*selectionInfo)( hwDisplay disp, hwInt32 flags,
                        hwFloat posx, hwFloat posy, hwFloat aperture );

    hwInt32 (*wasSelected)( hwDisplay disp, hwObject *obj, hwInt32 *vert );

    void (*currentObject)( hwDisplay disp, hwObject obj );

    void (*currentChild)( hwDisplay disp, hwObject obj );

    /********* Lighting state *********/
    void (*positionalLight)
    (
        hwDisplay disp,
        hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
        hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
    );

    void (*directionalLight)
    (
        hwDisplay disp,
        hwFloat Color[3], hwFloat Dir[3]
    );

    void (*doFog)
    (
        hwDisplay disp,
        hwInt32 OnOff, hwFloat Color[3]
    );

    void (*fogParams)
    (
        hwDisplay disp,
        hwInt32 fogType, hwFloat fogPlanes[2], hwFloat fogDensity
    );

    void (*ambientLight)
    (
        hwDisplay disp,
        hwFloat Factor, hwFloat Color[3]
    );

    void (*enableLighting)
    (
        hwDisplay disp,
        hwInt32 OnOff
    );

    void (*backgroundColor)( hwDisplay disp, hwFloat Color[3] );

    /********* Texture state *********/
    hwInt32 (*createTexture)
    (
        hwDisplay disp,
        hwImageStruct *img,
        hwInt32 filt,
        hwInt32 appl,
        hwInt32 bound,
        hwInt32 coordMode,
        hwOrientType *orient,
        hwInt32 internalFormat
    );

    void (*destroyTexture)( hwDisplay disp, hwInt32 tid );

    void (*currentTexture)( hwDisplay disp, hwInt32 tid, hwInt32 texNum );

    /* Surface attribute setting */
    void (*surfAttrs)( hwDisplay disp, hwSurfaceType *surf );

    void (*setVisibility)( hwDisplay disp, hwInt32 mask );

    void (*setInvisibility)( hwDisplay disp, hwInt32 incl, hwInt32 excl );

    hwInt32 (*getVisibility)( hwDisplay disp );

    /********* Bounding box/visibility stuff *********/
    void (*drawBBox)( hwDisplay disp, hwFloat Points[24] );

    int (*boundsVisible)
    (
        hwDisplay disp, hwSurfaceType *surf, hwFloat BBox[6], hwInt32 complexity
    );

    hwFloat (*boundsSize)
    (
        hwDisplay disp, hwFloat BBox[6]
    );

    /********* Primitives *********/
    void (*drawMesh)
    (
        hwDisplay disp,
        hwFloat *data,
        hwInt32 dataFlags, hwInt32 n, hwInt32 m
    );

    void (*drawStrip)
    (
        hwDisplay disp,
        hwFloat *data,
        hwInt32 dataFlags, hwInt32 n
    );

    void (*drawPolygon)
    (
        hwDisplay disp,
        hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts
    );

    void (*drawPolyline)
    (
        hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n
    );

    void (*drawQuads)
    (
        hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n
    );

    void (*drawIndexedTriangles)
    (
        hwDisplay disp,
        hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
        hwInt32 *indexList, hwInt32 numTris
    );

    void (*drawMarkers)
    (
        hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n
    );

    /*** GUI utilities ***/
    void (*guiText)
    (
        hwDisplay disp,
        TexFont *txf,
        hwInt32 color, hwInt32 halign, hwInt32 valign, hwInt32 height,
        hwInt32 x, hwInt32 y, unsigned char *text
    );

    void (*guiRaster)
    (
        hwDisplay disp,
        hwInt32 x, hwInt32 y,
        hwInt32 w, hwInt32 h,
        hwInt32 color,
        hwInt32 texId
    );

    void (*guiRectangle)
    (
        hwDisplay disp,
        hwInt32 flags,
        hwInt32 color,
        hwInt32 x, hwInt32 y,
        hwInt32 w, hwInt32 h,
        hwInt32 radius /* Radius of corners */
    );

    void (*guiPolyline)
    (
        hwDisplay disp,
        hwInt32 flags,
        hwInt32 color,
        hwInt32 numPts,
        hwInt32 *pts
    );

    void (*guiLines)
    (
        hwDisplay disp,
        hwInt32 flags,
        hwInt32 color,
        hwInt32 numPts,
        hwInt32 *pts
    );

    void (*guiPolygon)
    (
        hwDisplay disp,
        hwInt32 flags,
        hwInt32 color,
        hwInt32 numPts,
        hwInt32 *pts
    );

    hwInt32 (*openGuiList)( hwDisplay disp );
    void (*closeGuiList)( hwDisplay disp );
    void (*callGuiList)( hwDisplay disp, hwInt32 id );
    void (*destroyGuiList)( hwDisplay disp, hwInt32 id );

    /********* Display list functions *********/
    hwInt32 (*openList)( hwDisplay disp );

    void (*closeList)( hwDisplay disp );

    void (*callList)( hwDisplay disp, hwInt32 dl );

    void (*destroyList)( hwDisplay disp, hwInt32 dl );

    /********* Stoopid architecture function *********/
    hwTextState *(*getTextState)( hwDisplay disp );

    /********* State inquiry ***********************/
    void (*getDrawSize)( hwDisplay disp, hwInt32 *retSize );

    /********* Time functions **********************/
    double (*getCurrTime)( hwDisplay disp );
    double (*getStartTime)( hwDisplay disp );
    double (*getElapsedTime)( hwDisplay disp );
    void (*setCurrTime)( hwDisplay disp, double tm );
    void (*setStartTime)( hwDisplay disp, double tm );
    void (*setElapsedTime)( hwDisplay disp, double tm );
};

extern const hwDisplay
    hwGlDisplay,
    hwNullDisplay,
    hwDefaultDisplay;
