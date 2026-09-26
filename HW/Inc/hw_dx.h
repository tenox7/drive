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

/* Common types, header files, etc. for the DirectX implementation of
 * HoverWare.  First, the header files:
 */

#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>


/* Now, the type definitions.  We need 2: the display type, and
 * the drawable type.  Here they be.
 */

#define MSD     8               /* Matrix stack depth */

typedef struct {
    struct __hwDisplayInternal disp;    /* Function table, etc. */
    hwInt32     w, h;           /* The size of this display */
    hwInt32     dbuffer;        /* Is it double buffered? */
    int         iPixelFormat;   /* The chosen pixel format index */
} dxDisplay;

typedef struct _dxTextureStruct {
    hwImageStruct *img;                 /* The image for this texture */
    hwInt32 filt, appl, bound;          /* Creation parameters */
    struct _dxTextureStruct *next;      /* Next texture in linked list */
    hwInt32 textureID;                  /* ID of this texture */
    hwInt32 fildesMask[4];              /* Which file descriptors defined? */
    hwInt32 coordMode;                  /* Coordinate generation mode */
    hwOrientType orient;                /* Texture orientation matrix */
} dxTexture;

#define SEL_SIZE        512

/* DX impementation of "display lists."  This is the complete list of
 * things supported in DLs:
 *
 *      drawIndexedTriangles
 *      drawQuads
 *              => polyhedron or polyline
 *      drawMesh
 *      drawStrip
 *              => tristrips or polyline
 *      drawPolygon
 *              => polygon or polyline
 *      drawPolyline
 *              => polyline
 *      drawMarkers
 *              => polymarker
 *      __hwIntText3d
 *          pushMatrix
 *          popMatrix
 *          surfAttrs
 *          drawPolygon
 *          drawIndexedTriangles
 *          drawMesh
 *
 * I suppose we could add callList with little or no penalty.
 * So, the data we need to save is:
 */
union dxCallData {
    /* Generic */
    struct {
        int primType;
        union dxCallData *next;
    } genericData;

    /* For polygon, polyline, tristrip, polymarker */
    struct {
        int primType;
        union dxCallData *next;
        float *verts;
        int vertCount;
        int extData;
        int vertFlags;
    } stripData;

    /* For polyhedron */
    struct {
        int primType;
        union dxCallData *next;
        float *verts;
        int vertCount;
        int extData;
        int vertFlags;
        int *indexList;
        int primCount;
    } polyhedronData;

    /* For matrix */
    struct {
        int primType;
        union sbCallData *next;
        float mat[4][4];
    } matrixData;

    /* For surfAttrs */
    struct {
        int primType;
        union dxCallData *next;
        hwSurfaceType surf;
    } surfData;

    /* For callList */
    struct {
        int primType;
        union dxCallData *next;
        int listIndex;
    } callListData;
}; /* 18 words */

/* Enumeration of prim types */
#define DX_DL_POLYGON           1
#define DX_DL_POLYLINE          2
#define DX_DL_TRISTRIP          3
#define DX_DL_POLYMARKER        4
#define DX_DL_POLYHEDRON        5
#define DX_DL_PUSH_MATRIX       6
#define DX_DL_POP_MATRIX        7
#define DX_DL_SURF_ATTRS        8
#define DX_DL_CALL_LIST         9
#define DX_DL_TRIANGLES         10

/* Structure of an entire display list */
struct dxDispList {
    int dlName;
    union dxCallData *head, *tail;
    struct dxDispList *nextHash;
};

/* Hash table size for display lists */
#define DX_DL_HASH_SIZE         1024

#define MAX_GUI_STRING_SIZE 16
#define GUI_ELEMENT_FREE_SIZE   16

#define GUI_TEXT        0x00000000
#define GUI_RECT        0x10000000
#define GUI_LINE        0x20000000
#define GUI_TEXTURE     0x30000000
#define GUI_TRIANGLE    0x40000000
#define GUI_LIST        0x50000000
#define GUI_INVALID     0xF0000000
#define GUI_TYPE_MASK   0xF0000000

typedef struct __dxGuiElement {
    struct __dxGuiElement *next;
    hwInt32 typeFlags;
    hwInt32 color;
    union {
        struct {
            TexFont *txf;
            hwInt32 pos[2];
            hwInt32 size;
            char string[MAX_GUI_STRING_SIZE+1];
        } guiText;
        struct {
            hwInt32 pointA[2];
            hwInt32 pointB[2];
            hwInt32 texture;
        } guiRect;
        struct {
            hwInt32 pointA[2];
            hwInt32 pointB[2];
            hwInt32 pointC[2];
        } guiTriangle;
        struct {
            hwInt32 id;
        } guiList;
    } u;
} *dxGuiElement;

typedef struct __dxGuiList {
    hwInt32 id;
    struct __dxGuiList *prevHash, *nextHash;
    dxGuiElement head, tail;
} *dxGuiList;

#define GUI_HT_SIZE     32      /* Don't expect very many of these */

typedef struct {
    dxDisplay   *disp;          /* The actual display this drawable is on */
    IDirect3DDevice9 *d3dDevice; /* The D3D device on which to render */
    HWND        win;            /* The window */
    HANDLE      hDC;            /* Handle to device context */
    D3DCOLOR    clearColor;     /* What color to clear the display to */

    dxGuiList   guiListHash[GUI_HT_SIZE];       /* List of GUI display lists */
    dxGuiList   activeGuiList;  /* If one is active and compiling */
    dxGuiList   freeGuiList;    /* List of freed DLs */
    hwInt32     numGuiLists;    /* Just for openList purposes */

    hwInt32     winW, winH;     /* The current size of the window */
    hwInt32     numLights;      /* The number of lights defined */
    hwInt32     camSet;         /* Was a camera established? */
    hwInt32     lightOn;        /* Lights are on or off? */
    hwInt32     buffer;         /* The buffer we're using */
    hwInt32     primFlags;      /* BACKFACE, TWOSIDED, etc. */
    hwInt32     animMask;       /* 32-bit animation bit mask */
    hwInt32     invisibleInclude;       /* 32-bit invisibility include mask */
    hwInt32     invisibleExclude;       /* 32-bit invisibility exclude mask */
    hwFloat     ambFact;        /* The ambient lighting factor */
    hwFloat     VDC_XMax,       /* The window size, */
                VDC_YMax;       /*      in Virtual Device Coordinates */
    hwFloat     WC_Planes[5][4];/* Plane equations for view frustum */
    hwInt32     planesValid;    /* True iff plane eqns valid */
    hwFloat     matStack[MSD][4][4];    /* The matrix stack */
    hwFloat     rawMatStack[MSD][4][4]; /* The raw matrix stack */
    hwFloat     camMat[4][4];   /* The camera matrix */
    hwFloat     camPos[3];      /* Camera position */
    hwFloat     camField;       /* Camera field-of-view, radians */
    hwInt32     stackDepth;     /* How deep is the matrix stack? */
    hwInt32     renderMode;     /* Current rendering mode */
    hwSurfaceType lastSurface;  /* The last surface we stuck in hardware */
    hwTextState textState;      /* How are we to draw text? */
    dxTexture   *textureList;   /* List of textures in this drawable */
    hwSelectionType sel;        /* Selection info */
    hwObject    currObj;        /* Current object for picking */

    /* List of display lists */
    struct dxDispList   *displayHash[DX_DL_HASH_SIZE];
    struct dxDispList   *openList;      /* An open list, if any */
    int         segNum;         /* Segment of last created list */

    /* Saved state */
    hwInt32     shadeFlat;      /* Are we flatshading? */
    hwInt32     wireframeState; /* Are we wireframe or solid? */
    hwInt32     transp;         /* Transparency index */
    hwInt32     tmId[HW_MAX_TEXTURES];  /* Texture ID */
    hwInt32     tmActive;       /* Active texture unit */
    hwInt32     maxTexNum;      /* Maximim # of multitextures */
    hwInt32     blend;          /* Are we Blending? */
    hwFloat     color[3];       /* Surface color */
    hwFloat     ambFactSave;    /* Ambient lighting factor */
    hwFloat     shininess;      /* Shininess factor */
    hwFloat     specColor[3];   /* Specular color */
    hwFloat     ambColor[4];    /* Ambient color */
    hwFloat     primSize;       /* Line width/point size */
    dxGuiElement guiList, guiTail, guiFree;
} dxDrawable;

typedef struct {
    float
        refx, refy, refz,
        camx, camy, camz,
        upx, upy, upz,
        field_of_view,
        front, back,
        projection,
        skewX, skewY,   /* Skew fractions in X and Y axes */
        jitterX, jitterY;       /* Jitter fractions in X and Y axes */
} CameraArg;
extern void __dxCamera( CameraArg *cam, hwSelectionType *, int Width, int Height );

/* Finally, the global private data */
extern dxDrawable
    *__hwdxCurrentContext;
extern hwInt32
    __hwdxDispGfd;

/* Visual manipulation functions */
extern hwDisplay __hwDxInit( hwDisplay, hwDisplay, OS_DISPLAY_TYPE);
extern hwInt32 __hwDxChooseVisual( hwDisplay, hwInt32, hwInt32 );
extern OS_VISUAL_TYPE __hwDxExtractVisual( hwDisplay );
extern OS_DRAWABLE_TYPE __hwDxExtractWin( hwDisplay disp, hwDrawable draw );
extern hwDrawable __hwDxCreateWin
(
    hwDisplay, char *, hwInt32, hwInt32, hwInt32, hwInt32, hwInt32
);
extern hwDrawable __hwDxCreateChildWin
(
    hwDisplay, char *, HWND, hwInt32, hwInt32, hwInt32, hwInt32, hwInt32
);
extern void __hwDxCheckInput(hwDisplay,void (*)(hwDrawable, hwWinEvent *));
extern void __hwDxGetMousePos(hwDisplay, hwInt32, hwInt32 *);
extern hwDrawable __hwDxInitDrawable( hwDisplay, OS_DRAWABLE_TYPE );
extern void __hwDxMakeCurrent( hwDisplay, hwDrawable );
extern const char *__hwDxGetInfo( hwDisplay, hwInt32 );
extern void __hwDxViewport( hwDisplay, hwInt32, hwInt32, hwInt32, hwInt32 );

/* Misc API functions */
extern void __hwDxBackground( hwDisplay, hwFloat Color[3] );
extern void __hwDxLighting( hwDisplay, hwInt32 OnOff );
extern void __hwDxFog( hwDisplay, hwInt32 OnOff, hwFloat Color[3] );
extern void __hwDxFogParams( hwDisplay, hwInt32, hwFloat[2], hwFloat );
extern void __hwDxAmbient( hwDisplay, hwFloat Factor, hwFloat Color[3] );
extern void __hwDxPushMat( hwDisplay, hwFloat mat[4][4] );
extern void __hwDxPopMat( hwDisplay );
extern void __hwDxXformPoint( hwDisplay, hwFloat point[3] );
extern void __hwDxUpdate( hwDisplay, hwInt32 );
extern void __hwDxSetVisibility( hwDisplay, hwInt32 );
extern void __hwDxSetInvisibility( hwDisplay, hwInt32, hwInt32 );
extern hwInt32  __hwDxGetVisibility( hwDisplay );
extern void __hwDxSetDrawBuffer( hwDisplay, hwInt32 );
extern void __hwDxAccumOp( hwDisplay, hwInt32, hwFloat );

/* Selection/render mode functions */
extern hwInt32 __hwDxRenderMode( hwDisplay, hwInt32 );
extern void __hwDxSelectionInfo( hwDisplay, hwInt32, hwFloat, hwFloat, hwFloat);
extern hwInt32 __hwDxWasSelected( hwDisplay, hwObject *, hwInt32 * );
extern void __hwDxCurrObject( hwDisplay, hwObject );
extern void __hwDxCurrChild( hwDisplay, hwObject );

/* API display list functions */
extern hwInt32 __hwDxOpenList( hwDisplay );
extern void __hwDxCloseList( hwDisplay );
extern void __hwDxCallList( hwDisplay, hwInt32 dl );
extern void __hwDxDestroyList( hwDisplay, hwInt32 dl );

/* API primitive functions */
extern void __hwDxSurfAttrs( hwDisplay, hwSurfaceType *surf );
extern void __hwDxDrawMesh(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n, hwInt32 m
);
extern void __hwDxDrawStrip(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwDxDrawPolygon(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwDxDrawPolyline( hwDisplay, hwFloat *data, hwInt32 fl, hwInt32 n );
extern void __hwDxDrawMarkers(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwDxGuiText
(
    hwDisplay disp, TexFont *txf,
    hwInt32 color, hwInt32 halign, hwInt32 valign,
    hwInt32 height, hwInt32 x, hwInt32 y, char *text
);
void __hwDxGuiRaster
(
    hwDisplay disp,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 color,
    hwInt32 texId
);
void __hwDxGuiRectangle
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 radius
);
void __hwDxGuiPolyline
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);

void __hwDxGuiLines
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);
void __hwDxGuiPolygon
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);
extern hwInt32 __hwDxOpenGuiList( hwDisplay );
extern void __hwDxCloseGuiList( hwDisplay );
extern void __hwDxCallGuiList( hwDisplay, hwInt32 dl );
extern void __hwDxDestroyGuiList( hwDisplay, hwInt32 dl );

extern void __hwDxDrawQuads( hwDisplay, hwFloat *, hwInt32, hwInt32 );
extern void __hwDxIndexedTris(
    hwDisplay,
    hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
    hwInt32 *indexList, hwInt32 numTris
);

/* API information functions */
extern int __hwDxBoundsVisible(
    hwDisplay, hwSurfaceType *surf, hwFloat Bounds[6], hwInt32 complexity
);
extern hwFloat __hwDxBoundsSize( hwDisplay, hwFloat BBox[6] );

/* Misc API functions */
extern void __hwDxPosLight
(
    hwDisplay, hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
    hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
);
extern void __hwDxDirLight( hwDisplay, hwFloat Color[3], hwFloat Dir[3] );

extern void __hwDxCamera( hwDisplay, hwCamStruct * );

extern void __hwDxDestroyTexture( hwDisplay, hwInt32 tid );
extern hwInt32 __hwDxCreateTexture(
    hwDisplay, hwImageStruct *img, hwInt32 filt, hwInt32 appl, hwInt32 bound,
    hwInt32 coordMode, hwOrientType *orient, hwInt32 internalFormat
);
extern void __hwDxCurrentTexture( hwDisplay, hwInt32 tid, hwInt32 texNum );

extern hwTextState *__hwDxGetTextState( hwDisplay );
extern void __hwDxGetDrawSize( hwDisplay disp, hwInt32 *retSize );

extern void __hwDxDrawBBox( hwDisplay, hwFloat [] );
extern dxGuiElement __hwDxAllocGuiElem( void );
extern void __hwGlDrawElements( hwDisplay );

/* Internal display list functions */
extern void __hwdxAppendStrip( int, float *, int, int, int );
extern void __hwdxAppendPolyhedron( float *, int, int, int, int *, int, int );
extern void __hwdxAppendPushMat( float [4][4] );
extern void __hwdxAppendPopMat( void );
extern void __hwdxAppendSurf( hwSurfaceType * );
extern void __hwdxAppendCallList( int );

/*** EOF hw_dx.h ***/
