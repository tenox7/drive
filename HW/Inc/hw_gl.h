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

/*
 * Includes code originally Copyright (c) 2023 Ross Cunniff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Common types, header files, etc. for the OpenGL implementation of
 * HoverWare.  First, the header files:
 */

#if defined(CYGWIN)               /* [ */

#include <w32api/GL/gl.h>

#  if defined(GLFW)
#    include "GLFW/glfw3.h"
#  endif

#elif defined(WIN32)              /* ] [ */

#include <GL/gl.h>

#  if defined(GLFW)
#    include "GLFW/glfw3.h"
#  endif

#elif defined(MAC)              /* ] [ */

#include <OpenGL/OpenGL.h>

#elif defined(GLFW)             /* ] [ */

#include "GLFW/glfw3.h"

#elif defined(ANDROID_NDK)      /* ] [ */

#  ifdef USE_GLES_1_1
#    include <GLES/gl.h>
#  else
#    include <GLES2/gl2.h>
#  endif

#else                           /* ] [ */

#include <GL/glx.h>
#include <GL/gl.h>

#endif                          /* ] */

#include "hwglu.h"

#include "hw_glext.h"

/* Various enables we need to track */
#define ENABLE_FOG      0x00000001

/* Now, the type definitions.  We need 2: the display type, and
 * the drawable type.  Here they be.
 */

#define MSD     8               /* Matrix stack depth */

/* Globals */
typedef struct {
    double
        startTime,
        currTime,
        elapsedTime;
    int
        extensions,
        numTextures;
    int
        doExtensions,
        doPrune,
        doVbo,
        dumpProgs,
        pruneEye,
        showBoxes,
        disableCull;
    char
        *extensionString;
} glGlobals;

extern const struct __hwDisplayStruct
    __hwGlInitFunc;

#if defined(WIN32) && !defined(GLFW) /* [ */

typedef struct {
    struct __hwDisplayInternal disp;    /* Function table */
    hwInt32     w, h;           /* The size of this display */
    hwInt32     dbuffer;        /* Is it double buffered? */
    int         iPixelFormat;   /* The chosen pixel format index */
    struct __glDrawable *draw;  /* What we're drawing to */
    glGlobals glob;
} glDisplay;

#elif defined(MAC)      /* ] [ */

typedef struct {
    struct __hwDisplayInternal disp;    /* Function table */
    hwInt32     w, h;           /* The size of this display */
    hwInt32     dbuffer;        /* Is it double buffered? */
    AGLPixelFormat pixelFormat; /* The chosen pixel format */
    struct __glDrawable *draw;  /* What we're drawing to */
    glGlobals glob;
} glDisplay;

#elif defined(GLFW) /* ] [ */

typedef struct {
    struct __hwDisplayInternal disp;      /* Function table */
    GLFWmonitor *primaryMonitor;        /* Primary display */
    hwInt32     w, h;           /* The size of this display */
    hwInt32     dbuffer;        /* Is it double buffered? */
    struct __glDrawable *draw;  /* What we're drawing to */
    glGlobals glob;
} glDisplay;

#elif defined(ANDROID_NDK)   /* ] [ */

typedef struct {
    struct __hwDisplayInternal disp;    /* Function table */
    hwInt32     w, h;           /* The size of this display */
    void        *vis;           /* The visual to use */
    hwInt32     dbuffer;        /* Is it double buffered? */
    struct __glDrawable *draw;  /* What we're drawing to */
    glGlobals glob;
} glDisplay;

#else  /* ] [ */

typedef struct {
    struct __hwDisplayInternal disp;    /* Function table */
    Display     *xdisp;         /* The X11 display connection */
    hwInt32     screen;         /* The screen */
    hwInt32     w, h;           /* The size of this display */
    XVisualInfo vis;            /* The visual to use */
    hwInt32     dbuffer;        /* Is it double buffered? */
    struct __glDrawable *draw;  /* What we're drawing to */
    glGlobals glob;
} glDisplay;

#endif /* ]  WIN32/MAC/GLFW/ANDROID/GLX */

typedef struct _glTextureStruct {
    hwImageStruct *img;                 /* The image for this texture */
    hwInt32 filt, appl, bound;          /* Creation parameters */
    struct _glTextureStruct *next;      /* Next texture in linked list */
    hwInt32 textureID;                  /* ID of this texture */
    hwInt32 texName;                    /* Name for glBindTexture */
    hwInt32 coordMode;                  /* Coordinate generation mode */
    hwOrientType orient;                /* Texture orientation matrix */
    hwInt32 internalFormat;             /* Internal format, if any */
} glTexture;

#define SEL_SIZE        512

/*****************************************************************************
 * GUI display list structures
 ****************************************************************************/

#define MAX_GUI_STRING_SIZE 16
#define GUI_ELEMENT_FREE_SIZE   16

/* Use top nibble to avoid flags in bottom nibbles */
#define GUI_TEXT        0x00000000
#define GUI_RECT        0x10000000
#define GUI_LINE        0x20000000
#define GUI_TEXTURE     0x30000000
#define GUI_TRIANGLE    0x40000000
#define GUI_LIST        0x50000000
#define GUI_INVALID     0xF0000000
#define GUI_TYPE_MASK   0xF0000000

typedef struct __glGuiElement {
    struct __glGuiElement *next;
    hwInt32 typeFlags;
    hwInt32 color;
    union {
        struct {
            TexFont *txf;
            hwInt32 pos[2];
            hwInt32 size;
            unsigned char string[MAX_GUI_STRING_SIZE+1];
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
} *glGuiElement;

typedef struct __glGuiList {
    hwInt32 id;
    struct __glGuiList *prevHash, *nextHash;
    glGuiElement head, tail;
} *glGuiList;

#define GUI_HT_SIZE     32      /* Don't expect very many of these */

/*****************************************************************************
 * OGL ES display list datatypes
 ****************************************************************************/

#define GL_DL_SET_POINTERS      0
#define GL_DL_DRAW_ELEMENTS     1
#define GL_DL_PUSH_MATRIX       2
#define GL_DL_POP_MATRIX        3
#define GL_DL_SURF_ATTRS        4
#define GL_DL_CALL_LIST         5
#define GL_DL_WIREFRAME         6
#define GL_DL_SOLID             7
#define GL_DL_VISIBILITY        8
#define GL_DL_BUFF_OBJS         9
#define GL_DL_BUFF_VERTS        10
#define GL_DL_BUFF_IDX          11


#define VBO_THRESH              8      /* Below this, don't bother */

typedef struct {
    hwInt32 elemType;
    union __glDlElement *next;
} glDlElemHdr;

typedef struct {
    glDlElemHdr hdr;
    hwVertexOffsets vo;
    hwInt32 wpv;
    hwInt32 dataFlags;
    hwInt32 numVerts;
    hwFloat *data;
} glDlVertexes;

typedef struct {
    glDlElemHdr hdr;
    hwVertexOffsets vo;
    hwInt32 wpv;
    hwInt32 dataFlags;
    hwInt32 numVerts;
    hwInt32 offset;
} glDlBuffVerts;

typedef struct {
    glDlElemHdr hdr;
    hwInt32 primType;
    hwInt32 primCount;
    hwInt16 *indices;
} glDlIndexes;

typedef struct {
    glDlElemHdr hdr;
    hwInt32 primType;
    hwInt32 primCount;
    hwInt32 idxOffs;
} glDlBuffIdx;

typedef struct {
    glDlElemHdr hdr;
    hwFloat mat[4][4];
} glDlMatrix;

typedef struct {
    glDlElemHdr hdr;
    hwSurfaceType surf;
} glDlSurface;

typedef struct {
    glDlElemHdr hdr;
    hwInt32 listNum;
} glDlCallList;

typedef struct {
    glDlElemHdr hdr;
    hwInt32 visMask;
} glDlVisibility;

typedef struct {
    glDlElemHdr hdr;
    hwInt32 vBO;
    hwInt32 iBO;
} glDlBuffObjs;

typedef union __glDlElement {
    glDlElemHdr hdr;
    glDlVertexes vtx;
    glDlIndexes idx;
    glDlMatrix mat;
    glDlSurface surf;
    glDlCallList call;
    glDlVisibility vis;
    glDlBuffObjs vbo;
    glDlBuffVerts bvtx;
    glDlBuffIdx bidx;
} *glDlElement;

typedef struct __glDispList {
    hwInt32 dlName;
    struct __glDispList *prevHash, *nextHash;
    glDlElement head, tail;
} *glDispList;

/******************************************************************
 * GLES 2.0 state tracking
 *****************************************************************/
#define MAX_LIGHTS      8
#define MAX_TEX         8
#define MAX_VTX_ATTRIB  12
#define HWGL_PROG_HASH_SIZE     32 // TBD?

#define HWGL_VA_VERTEX          0
#define HWGL_VA_COLOR           1
#define HWGL_VA_NORMAL          2
#define HWGL_VA_TANGENT         3
#define HWGL_VA_TEXTURE         4

typedef enum {
    DIRTY_LIGHT0, DIRTY_LIGHT1,
    DIRTY_LIGHT2, DIRTY_LIGHT3,
    DIRTY_LIGHT4, DIRTY_LIGHT5,
    DIRTY_LIGHT6, DIRTY_LIGHT7,
    DIRTY_FOG,
    DIRTY_MATERIAL,
    DIRTY_MODELVIEW,
    DIRTY_PROJECTION,
    DIRTY_LIGHT_MODEL,
    DIRTY_COLOR,

    DIRTY_COUNT
} DirtyBitNames;

#define ALL_DIRTY_BITS  ((1 << DIRTY_COUNT) - 1)
#define ANY_LIGHT_DIRTY ((1 << (DIRTY_LIGHT0+MAX_LIGHTS)) - 1)

#define HWGL_ENUM_8(t) \
    ENA_##t##0, ENA_##t##1, ENA_##t##2, ENA_##t##3, \
    ENA_##t##4, ENA_##t##5, ENA_##t##6, ENA_##t##7,

#define HWGL_ENUM_SUFF_8(t,s) \
    ENA_##t##0_##s, ENA_##t##1_##s, ENA_##t##2_##s, ENA_##t##3_##s, \
    ENA_##t##4_##s, ENA_##t##5_##s, ENA_##t##6_##s, ENA_##t##7_##s,

/* In this enum, care must be taken so that the groups of 8/16
 * don't straddle 32-bit boudaries.  This is to ensure that the *ANY*
 * macros below will work.
 */
typedef enum {
    /***************** Word 0 *****************/
    HWGL_ENUM_8(LIGHT)
    HWGL_ENUM_SUFF_8(LIGHT, POS)
    HWGL_ENUM_SUFF_8(LIGHT, SPOT)
    HWGL_ENUM_SUFF_8(LIGHT, ATT)

    /***************** Word 1 *****************/
    HWGL_ENUM_8(TEX)
    HWGL_ENUM_SUFF_8(TEX,CUBE)
    HWGL_ENUM_SUFF_8(TEX,PROJ)
    HWGL_ENUM_SUFF_8(TEX,GLOSS)

    /***************** Word 2 *****************/
    HWGL_ENUM_SUFF_8(TEX,NORMAL)        // Only one NORMAL and/or RELIEF map
    HWGL_ENUM_SUFF_8(TEX,RELIEF)        // is enabled at any time
    HWGL_ENUM_SUFF_8(TEXGEN,STAGE_0)
    HWGL_ENUM_SUFF_8(TEXGEN,REFLECT)

    /***************** ...and beyond  *****************/
    ENA_LIGHTING, ENA_LIGHT_PER_PIXEL,

    ENA_FOG, ENA_FOG_LINEAR, ENA_FOG_EXP, ENA_FOG_EXP2,

    ENA_COLOR_SUM,
    ENA_NORMALIZE,
    ENA_COLOR_MATERIAL,
    ENA_VERTEX_COLOR,

    ENA_COUNT
} EnableBitNames;

#define NUM_ENABLE_WORDS        ((ENA_COUNT+31)/32)

typedef struct __hwProgHash {
    struct __hwProgHash *next;
    hwUint32 cookedEnables[NUM_ENABLE_WORDS];
    hwInt32 vtxShader, fragShader, program;
} *hwProgHash;

typedef struct {
    hwFloat pos[4];
    hwFloat spotDir[3];
    hwFloat spotCutoff;
    hwFloat spotExp;
    hwFloat linearAtten;
    hwFloat ambient[4];
    hwFloat diffuse[4];
    hwFloat specular[4];
} hwLightInfo;

typedef struct {
    hwInt32 mode;
    hwFloat density;
    hwFloat fogStart;
    hwFloat fogEnd;
    hwFloat color[4];
} hwFogInfo;

typedef struct {
    hwFloat specular[4];
    hwFloat emission[4];
    hwFloat specExp;
} hwMaterialInfo;

#define PROJ_STACK_DEPTH        2
#define MODELVIEW_STACK_DEPTH   16

typedef struct {
    char *startPtr;
    char *currPtr;
    char *endPtr;
    char **lines;
    int numLines;
    int allocSize;
    int allocLines;
} GlProgSource;

typedef struct {
    hwUint32 enables[NUM_ENABLE_WORDS];
    hwUint32 vtxAttribEnables;
    hwUint32 cookedEnables[NUM_ENABLE_WORDS];
    hwLightInfo lights[MAX_LIGHTS];
    hwMaterialInfo material;
    hwFogInfo fog;
    hwFloat globalAmbient[4];
    hwInt32 colorMaterial;
    hwInt32 lmColorControl;
    hwInt32 matrixMode;
    hwInt32 activeTexture;
    hwInt32 clientActiveTexture;
    hwInt32 texEnv[MAX_TEX];
    hwFloat *prMat, *prTOS;
    hwFloat *mvMat, *mvTOS;
    hwFloat prStack[PROJ_STACK_DEPTH*4*4];
    hwFloat mvStack[MODELVIEW_STACK_DEPTH*4*4];
    hwFloat mvpMatrix[16];
    hwFloat mvInvXpose[9];
    hwFloat color[4];
    hwInt32 lastError;
    hwInt32 progDirty;
    hwInt32 constDirty;

    GlProgSource vtxProg;       /* Vertex program in-progress */
    GlProgSource fragProg;      /* Fragment program in-progress */

    hwProgHash progHash[HWGL_PROG_HASH_SIZE];
    hwInt32 program;
} hwProgInfo;

#define DO_ENABLE_BITS(a,ena)   ((a)[(ena)>>5] |=  (1U << ((ena)&31)))
#define DO_DISABLE_BITS(a,ena)  ((a)[(ena)>>5] &= ~(1U << ((ena)&31)))
#define IS_ENABLED_BITS(a,ena)  ((a)[(ena)>>5] &   (1U << ((ena)&31)))
#define DO_ENABLE(ena)          DO_ENABLE_BITS(ppi->enables, ena)
#define DO_DISABLE(ena)         DO_DISABLE_BITS(ppi->enables, ena)
#define IS_ENABLED(ena)         IS_ENABLED_BITS(ppi->enables, ena)
#define IS_COOKED(ena)          IS_ENABLED_BITS(ppi->cookedEnables, ena)
// Note: the *ANY* macros only work if e0 and e1 are located in the
// same 32-bit word.  Care is taken in the definition of the
// ENA bits to makesure this is true for things which go together -
// e.g. lights, textures, vertex arrays, etc.
#define IS_ANY_BIT_ENABLED(a,e0,e1) \
        ((a)[(e0)>>5] & ((1<<(((e1)+1)&31))-1) & ~((1<<((e0)&31))-1))
#define IS_ANY_COOKED(e0,e1)    IS_ANY_BIT_ENABLED(ppi->cookedEnables, e0, e1)

#define IS_DIRTY(flag)          (ppi->constDirty & (1 << (flag)))
#define DO_DIRTY(flag)          (ppi->constDirty |= (1 << (flag)))

#define JOY_MAX_AXES    8
#define JOY_MAX_BUTTONS 24
typedef struct __glJoystick {
    char *name;                                 // Name of the joystick
    int numButtons;                             // How many buttons it has
    unsigned char buttons[JOY_MAX_BUTTONS];     // What their current value is
} *glJoystick;

#define GL_DL_HASH_SIZE 256     /* Medium-sized default */

typedef struct __glDrawable {
    glDisplay   *disp;          /* The actual display this drawable is on */
    void        (*eventCB)( hwDrawable, hwWinEvent * ); /* Callback */

#if defined(WIN32) && !defined(GLFW) /* [ */
    HGLRC       ctx;            /* The WGL context on which to render */
    HWND        win;            /* The window */
    HANDLE      hDC;            /* Handle to device context */
#elif defined(MAC) /* ] [ */
    AGLContext  ctx;            /* The AGL context on which to render */
    WindowRef   win;            /* The window we're drawing in */
    Rect        contentRegion, growRegion; /* Important bounds */
    /* TBD: GC? */
#elif defined(GLFW) /* ] [ */
    GLFWwindow  *win;           /* The GLFW window/context */
    int         mouseX, mouseY; /* The current mouse position */
    int         winX, winY;     /* The current window position */
    hwInt32     buttonState;    /* Current state of mouse buttons */
    hwInt32     kbdMods;        /* Current state of keyboard mods */
#elif defined(ANDROID_NDK) /* ] [ */
    int         ctx;            /* The GLX context on which to render */
    int         win;            /* The window */
    int         gc;             /* Context, for X graphics */
#else /* ] [ */
    GLXContext  ctx;            /* The GLX context on which to render */
    Window      win;            /* The window */
    GC          gc;             /* Context, for X graphics */
#endif /* ] */
    hwInt32     scissor[4];     /* x y w h */
    glJoystick  joystick[HW_MAX_JOYSTICKS]; /* Joystick state */
    glGuiList   guiListHash[GUI_HT_SIZE]; /* List of GUI display lists */
    glGuiList   activeGuiList;  /* If one is open and compiling */
    glGuiList   freeGuiList;    /* List of freed DLs */
    hwInt32     numGuiLists;    /* Just for openList purposes */
    hwInt32     enableBits;     /* Track certain things for enable/disable */

    hwInt16     *idxPool;       /* Temp pool of indices */
    hwInt32     idxPoolSize;    /* How big it is */
    hwFloat     *vtxPool;       /* Temp pool of vertices */
    hwInt32     vtxPoolSize;    /* How big it is */
    hwFloat     *auxPool;       /* Aux temp pool of vertices */
    hwInt32     auxPoolSize;    /* How big it is */

    glDispList  displayHash[GL_DL_HASH_SIZE]; /* List of GL display lists */
    glDispList  openList;       /* List currently being compiled */
    hwInt32     numLists;       /* Count of display lists */

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
    hwFloat     projMat[4][4];  /* The projection matrix */
    hwFloat     camPos[3];      /* Camera position */
    hwFloat     camField;       /* Camera field-of-view, radians */
    hwInt32     stackDepth;     /* How deep is the matrix stack? */
    hwInt32     renderMode;     /* Current rendering mode */
    hwSurfaceType lastSurface;  /* The last surface we stuck in hardware */
    hwTextState textState;      /* How are we to draw text? */
    glTexture   *textureList;   /* List of textures in this drawable */
    hwSelectionType sel;        /* Selection info */
    hwObject    currObj;        /* Current object for picking */
    GLuint      selMem[SEL_SIZE];       /* GL_SELECT memory */

    /* Saved state */
    hwInt32     shadeFlat;      /* Are we flatshading? */
    hwInt32     wireframeState; /* Are we wireframe or solid? */
    hwInt32     tmId[HW_MAX_TEXTURES];  /* Texture ID */
    hwInt32     tmActive;       /* Active texture unit */
    hwInt32     maxTexNum;      /* Maximim # of multitextures */
    hwInt32     blend;          /* Are we Blending? */
    hwInt32     blendSrc;       /* GL_ONE or GL_SRC_ALPHA */
    hwFloat     color[3];       /* Surface color */
    hwFloat     transp;         /* Transparency index */
    hwFloat     ambFactSave;    /* Ambient lighting factor */
    hwFloat     shininess;      /* Shininess factor */
    hwFloat     specColor[3];   /* Specular color */
    hwFloat     ambColor[4];    /* Ambient color */
    hwFloat     primSize;       /* Line width/point size */
    glGuiElement guiList, guiTail, guiFree;
    hwInt32     lastElemType;   /* Last GUI element drawn */

    hwProgInfo  progInfo;       /* Stuff for fixed-function emulation */
} glDrawable;

/* Values for wireframeState */
#define WFS_INVALID     -1
#define WFS_SOLID       0
#define WFS_WIREFRAME   1

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
    int
        mirror;
} CameraArg;
extern void __glCamera( CameraArg *cam, hwSelectionType *,
                        int Width, int Height );

/* Visual manipulation functions */
extern hwDisplay __hwGlCreate( hwDisplay, hwDisplay, OS_DISPLAY_TYPE);
extern hwInt32 __hwGlChooseVisual( hwDisplay, hwInt32, hwInt32 );
extern OS_VISUAL_TYPE __hwGlExtractVisual( hwDisplay );
extern OS_DRAWABLE_TYPE __hwGlExtractWin( hwDisplay disp, hwDrawable draw );

extern hwDrawable __hwGlCreateChildWin
(
    hwDisplay, const char *, OS_WINDOW_TYPE,
    hwInt32, hwInt32, hwInt32, hwInt32, hwInt32
);

extern hwDrawable __hwGlCreateWin
(
    hwDisplay, const char *, hwInt32, hwInt32, hwInt32, hwInt32, hwInt32
);
extern void __hwGlInputHandler
(
    hwDisplay, void (*)( hwDrawable, hwWinEvent * )
);
extern void __hwGlGetMousePos( hwDisplay, hwInt32 *, hwInt32 * );
extern hwDrawable __hwGlInitDrawable( hwDisplay, OS_DRAWABLE_TYPE );
extern void __hwGlMakeCurrent( hwDisplay, hwDrawable );
extern const char *__hwGlGetInfo( hwDisplay, hwInt32 );
extern void __hwGlViewport( hwDisplay, hwInt32, hwInt32, hwInt32, hwInt32 );
extern void __hwGlScissor( hwDisplay, hwInt32, hwInt32, hwInt32, hwInt32 );

/* Misc API functions */
extern void __hwGlBackground( hwDisplay, hwFloat Color[3] );
extern void __hwGlLighting( hwDisplay, hwInt32 OnOff );
extern void __hwGlFog( hwDisplay, hwInt32 OnOff, hwFloat Color[3] );
extern void __hwGlFogParams( hwDisplay, hwInt32, hwFloat[2], hwFloat );
extern void __hwGlAmbient( hwDisplay, hwFloat Factor, hwFloat Color[3] );
extern void __hwGlPushMat( hwDisplay, hwFloat mat[4][4] );
extern void __hwGlPopMat( hwDisplay );
extern void __hwGlXformPoint( hwDisplay, hwFloat point[3] );
extern void __hwGlUpdate( hwDisplay, hwInt32 );
extern void __hwGlSetVisibility( hwDisplay, hwInt32 );
extern void __hwGlSetInvisibility( hwDisplay, hwInt32, hwInt32 );
extern hwInt32  __hwGlGetVisibility( hwDisplay );
extern void __hwGlSetDrawBuffer( hwDisplay, hwInt32 );
extern void __hwGlAccumOp( hwDisplay, hwInt32, hwFloat );

/* Selection/render mode functions */
extern hwInt32 __hwGlRenderMode( hwDisplay, hwInt32 );
extern void __hwGlSelectionInfo( hwDisplay, hwInt32, hwFloat, hwFloat, hwFloat);
extern hwInt32 __hwGlWasSelected( hwDisplay, hwObject *, hwInt32 * );
extern void __hwGlCurrObject( hwDisplay, hwObject );
extern void __hwGlCurrChild( hwDisplay, hwObject );
extern void __hwgl_SetWireframeState( glDrawable * );
extern void __hwgl_SetSolidState( glDrawable * );

/* API display list functions */
extern hwInt32 __hwGlOpenList( hwDisplay );
extern void __hwGlCloseList( hwDisplay );
extern void __hwGlCallList( hwDisplay, hwInt32 dl );
extern void __hwGlDestroyList( hwDisplay, hwInt32 dl );

/* API primitive functions */
extern void __hwGlSurfAttrs( hwDisplay, hwSurfaceType *surf );
extern void __hwGlDrawMesh(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n, hwInt32 m
);
extern void __hwGlDrawStrip(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwGlDrawPolygon(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwGlDrawPolyline( hwDisplay, hwFloat *data, hwInt32 fl, hwInt32 n );
extern void __hwGlDrawMarkers(
    hwDisplay, hwFloat *data, hwInt32 flags, hwInt32 n
);
extern void __hwGlGuiText
(
    hwDisplay disp, TexFont *txf,
    hwInt32 color, hwInt32 halign, hwInt32 valign,
    hwInt32 height, hwInt32 x, hwInt32 y, unsigned char *text
);
void __hwGlGuiRaster
(
    hwDisplay disp,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 color,
    hwInt32 texId
);
void __hwGlGuiRectangle
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 radius
);
void __hwGlGuiPolyline
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);
void __hwGlGuiLines
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);
void __hwGlGuiPolygon
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
);
extern hwInt32 __hwGlOpenGuiList( hwDisplay );
extern void __hwGlCloseGuiList( hwDisplay );
extern void __hwGlCallGuiList( hwDisplay, hwInt32 dl );
extern void __hwGlDestroyGuiList( hwDisplay, hwInt32 dl );

extern void __hwGlDrawQuads( hwDisplay, hwFloat *, hwInt32, hwInt32 );
extern void __hwGlIndexedTris(
    hwDisplay,
    hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
    hwInt32 *indexList, hwInt32 numTris
);

/* API information functions */
extern int __hwGlAnimVisible( hwDisplay, hwInt32 );
extern int __hwGlBoundsVisible(
    hwDisplay, hwSurfaceType *surf, hwFloat Bounds[6], hwInt32 complexity
);
extern hwFloat __hwGlBoundsSize( hwDisplay, hwFloat BBox[6] );

/* Misc API functions */
extern void __hwGlPosLight
(
    hwDisplay, hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
    hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
);
extern void __hwGlDirLight( hwDisplay, hwFloat Color[3], hwFloat Dir[3] );

extern void __hwGlCamera( hwDisplay, hwCamStruct * );

extern void __hwGlDestroyTexture( hwDisplay, hwInt32 tid );
extern hwInt32 __hwGlCreateTexture(
    hwDisplay, hwImageStruct *img, hwInt32 filt, hwInt32 appl, hwInt32 bound,
    hwInt32 coordMode, hwOrientType *orient, hwInt32 internalFormat
);
extern void __hwGlCurrentTexture( hwDisplay, hwInt32 tid, hwInt32 texNum );

extern hwTextState *__hwGlGetTextState( hwDisplay );
extern void __hwGlGetDrawSize( hwDisplay disp, hwInt32 *retSize );

/* Time functions */
extern double __hwGlGetCurrTime( hwDisplay disp );
extern double __hwGlGetStartTime( hwDisplay disp );
extern double __hwGlGetElapsedTime( hwDisplay disp );
extern void __hwGlSetCurrTime( hwDisplay disp, double tm );
extern void __hwGlSetStartTime( hwDisplay disp, double tm );
extern void __hwGlSetElapsedTime( hwDisplay disp, double tm );

/* Miscellany */
extern void __hwGlDrawBBox( hwDisplay, hwFloat [] );
extern glGuiElement __hwGlAllocGuiElem( glDrawable *glctx );
extern void __hwGlDrawElements( hwDisplay );
extern void __hwGlInitState( glDrawable *draw );
extern void __hwglSetProcAddrs( glDisplay *gldisp );
extern void __hwGlGetEnvVars( glDisplay *gldisp );
extern void __hwGlGetExtensions( glDisplay *gldisp, glDrawable *draw );
extern void __hwGlInitDrawableVars( glDrawable *draw );
extern void __hwGlCheckInput( hwDisplay disp );
extern void __hwGlSwapBuffers( glDrawable *glctx );

/* Display list routines */
void __hwGlAppendVertices( glDrawable *glctx,
                           hwFloat *data, hwInt32 dataFlags, hwInt32 nv );
void __hwGlAppendDrawElements( glDrawable *glctx,
                               hwInt32 primType, hwInt32 primCount,
                               hwInt16 *indices );
void __hwGlAppendDrawArrays( glDrawable *glctx,
                             hwInt32 primType, hwInt32 primStart,
                             hwInt32 primCount );
void __hwGlAppendPushMat( glDrawable *glctx, hwFloat mat[4][4] );
void __hwGlAppendPopMat( glDrawable *glctx );
void __hwGlAppendSurf( glDrawable *glctx, hwSurfaceType *surf );
void __hwGlAppendCallList( glDrawable *glctx, hwInt32 dl );
void __hwGlAppendWireframe( glDrawable *glctx );
void __hwGlAppendSolid( glDrawable *glctx );
void __hwGlAppendVisibility( glDrawable *glctx, hwInt32 );
glDlElement __hwGlAllocBuffVerts( hwInt32 df, hwInt32 nv, hwInt32 offs );
glDlElement __hwGlAllocBuffIdx( hwInt32 pt, hwInt32 np, hwInt32 idxOffs );
glDlElement __hwGlAllocBuffObjs( hwInt32 iBO, hwInt32 vBO );

/* Temporary vertex/index pool management */
void __hwGlSetDataPointers(glDrawable *glctx, float *vptr, int Flags, int nv);
void __hwGlSetOffsPointers(glDrawable *glctx, float *vptr,
                           hwVertexOffsets *offs, int nv, int wpv,
                           int optimized);
hwInt16 *__hwGlGrowIdxPool( glDrawable *glctx, hwInt32 newSize );
hwFloat *__hwGlGrowVtxPool( glDrawable *glctx, hwInt32 newSize );
hwFloat *__hwGlGrowAuxPool( glDrawable *glctx, hwInt32 newSize );

/* Graphics programs */
int __hwGlInitProgSource(GlProgSource *ps);
int __hwGlCreateVtxProgram(hwProgInfo *ppi);
int __hwGlCreateFragProgram(hwProgInfo *ppi);

#define FETCH_GL_CTX \
    hwDisplay __hwDisp = HW_GET_CURR_DISP(); \
    glDrawable *glctx = __hwDisp ? ((glDisplay *)__hwDisp)->draw : NULL

#define USE_GL_CTX(disp) \
    glDrawable *glctx = ((glDisplay *)(disp))->draw

#define USE_PROG_INFO \
    FETCH_GL_CTX; \
    hwProgInfo *ppi = glctx ? &glctx->progInfo : NULL

/* GLES2.0 fixed-function emulation stuff */
/* These are set to the same as the gl.h equivalents. */

/* Enable */
#define HWGL_LIGHT0                     0x4000
#define HWGL_LIGHT1                     0x4001
#define HWGL_LIGHT2                     0x4002
#define HWGL_LIGHT3                     0x4003
#define HWGL_LIGHT4                     0x4004
#define HWGL_LIGHT5                     0x4005
#define HWGL_LIGHT6                     0x4006
#define HWGL_LIGHT7                     0x4007
#define HWGL_LIGHTING                   0x0B50
#define HWGL_TEXTURE_2D                 0x0DE1
#define HWGL_FOG                        0x0B60
#define HWGL_COLOR_SUM                  0x8458
#define HWGL_NORMALIZE                  0x0BA1
#define HWGL_COLOR_MATERIAL             0x0B57
#define HWGL_POINT_SMOOTH               0x0B10
#define HWGL_LINE_SMOOTH                0x0B20
#define HWGL_TEXTURE_GEN                0x7FFF0007

/* TexGen */
#define HWGL_TEXTURE_GEN_MODE           0x2500
#define HWGL_REFLECTION_MAP             0x7FFF0008
#define HWGL_STAGE_0                    0x7FFF0009
#define HWGL_NONE                       0

/* TextureUnit */
#define HWGL_TEXTURE0                   0x84C0
#define HWGL_TEXTURE1                   0x84C1
#define HWGL_TEXTURE2                   0x84C2
#define HWGL_TEXTURE3                   0x84C3
#define HWGL_TEXTURE4                   0x84C4
#define HWGL_TEXTURE5                   0x84C5
#define HWGL_TEXTURE6                   0x84C6
#define HWGL_TEXTURE7                   0x84C7

/* ShadingModel */
#define HWGL_SMOOTH                     0x1D01

/* LightParameter */
#define HWGL_AMBIENT                    0x1200
#define HWGL_DIFFUSE                    0x1201
#define HWGL_SPECULAR                   0x1202
#define HWGL_POSITION                   0x1203
#define HWGL_SPOT_DIRECTION             0x1204
#define HWGL_SPOT_EXPONENT              0x1205
#define HWGL_SPOT_CUTOFF                0x1206
#define HWGL_LINEAR_ATTENUATION         0x1208

/* FogParameter */
#define HWGL_FOG_DENSITY                0x0B62
#define HWGL_FOG_START                  0x0B63
#define HWGL_FOG_END                    0x0B64
#define HWGL_FOG_MODE                   0x0B65
#define HWGL_FOG_COLOR                  0x0B66

/* FogMode */
#define HWGL_LINEAR                     0x2601
#define HWGL_EXP                        0x0800
#define HWGL_EXP2                       0x0801

/* MaterialParameter */
#define HWGL_EMISSION                   0x1600
#define HWGL_SHININESS                  0x1601
#define HWGL_AMBIENT_AND_DIFFUSE        0x1602

/* LightModelParameter */
#define HWGL_LIGHT_MODEL_AMBIENT        0x0B53
#define HWGL_LIGHT_MODEL_COLOR_CONTROL  0x81F8
#define HWGL_SEPARATE_SPECULAR_COLOR    0x81FA

/* ColorMaterialFace */
#define HWGL_FRONT_AND_BACK             0x0408

/* EnableClientAttrib */
#define HWGL_VERTEX_ARRAY               0x8074
#define HWGL_NORMAL_ARRAY               0x8075
#define HWGL_COLOR_ARRAY                0x8076
#define HWGL_TEXTURE_COORD_ARRAY        0x8078
#define HWGL_TANGENT_ARRAY              0x7FFF0000

/* AttribPointer */
#define HWGL_FLOAT                      0x1406

/* TextureEnvMode */
#define HWGL_MODULATE                   0x2100
#define HWGL_NORMAL_MAP                 0x7FFF0002
#define HWGL_RELIEF_MAP                 0x7FFF0003
#define HWGL_GLOSS_MAP                  0x7FFF0004
#define HWGL_SHADOW_MAP                 0x7FFF0005
#define HWGL_SHADOW_VAR_MAP             0x7FFF0006

/* TextureEnvParameter */
#define HWGL_TEXTURE_ENV_MODE           0x2200

/* MatrixMode */
#define HWGL_MODELVIEW                  0x1700
#define HWGL_PROJECTION                 0x1701

/* GetFloat */
#define HWGL_MODELVIEW_MATRIX           0x0BA6
#define HWGL_PROJECTION_MATRIX          0x0BA7

/* Errors */
#define HWGL_INVALID_ENUM               0x0500
#define HWGL_INVALID_VALUE              0x0501
#define HWGL_STACK_OVERFLOW             0x0503
#define HWGL_STACK_UNDERFLOW            0x0504

/* API functions */
extern void
    hwGlEnable( hwInt32 pname ),
    hwGlDisable( hwInt32 pname ),
    hwGlActiveTexture( hwInt32 tex ),
    hwGlLightfv( hwInt32 light, hwInt32 pname, const hwFloat *param ),
    hwGlLightf( hwInt32 light, hwInt32 pname, hwFloat param ),
    hwGlFogfv( hwInt32 pname, const hwFloat *param ),
    hwGlFogf( hwInt32 pname, hwFloat param ),
    hwGlFogi( hwInt32 pname, hwInt32 param ),
    hwGlMaterialfv( hwInt32 face, hwInt32 pname, const hwFloat *param ),
    hwGlMateriali( hwInt32 face, hwInt32 pname, hwInt32 param ),
    hwGlColorMaterial( hwInt32 face, hwInt32 mode ),
    hwGlLightModelfv( hwInt32 pname, const hwFloat *param ),
    hwGlLightModeli( hwInt32 pname, hwInt32 param ),
    hwGlTexEnvi( hwInt32 pname, hwInt32 parameter ),
    hwGlTexGeni( hwInt32 pname, hwInt32 parameter ),
    hwGlMatrixMode( hwInt32 mode ),
    hwGlPushMatrix( void ),
    hwGlPopMatrix( void ),
    hwGlLoadMatrixf( const hwFloat *mat ),
    hwGlGetFloatv( hwInt32 pname, hwFloat *result ),
    hwGlTranslatef( hwFloat x, hwFloat y, hwFloat z ),
    hwGlScalef( hwFloat x, hwFloat y, hwFloat z ),
    hwGlFrustum( hwFloat x0, hwFloat x1, 
                 hwFloat y0, hwFloat y1, 
                 hwFloat z0, hwFloat z1 ),
    hwGlOrtho( hwFloat x0, hwFloat x1,
               hwFloat y0, hwFloat y1,
               hwFloat z0, hwFloat z1 ),
    hwGlMultMatrixf( const hwFloat *mat ),
    hwGlShadeModel( hwInt32 pname ),
    hwGlPointSize(  hwFloat psize ),
    hwGlLoadIdentity( void ),
    hwGlColor4f(hwFloat r, hwFloat g, hwFloat b, hwFloat a),
    hwGlColor4ub(hwInt32 r, hwInt32 g, hwInt32 b, hwInt32 a),
    hwGlEnableClientState( hwInt32 ),
    hwGlDisableClientState( hwInt32 ),
    hwGlClientActiveTexture( hwInt32 ),
    hwGlVertexPointer( hwInt32, hwInt32, hwInt32, const void * ),
    hwGlNormalPointer( hwInt32, hwInt32, const void * ),
    hwGlTangentPointer( hwInt32, hwInt32, const void * ),
    hwGlTexCoordPointer( hwInt32, hwInt32, hwInt32, const void * ),
    hwGlColorPointer( hwInt32, hwInt32, hwInt32, const void * );
extern hwInt32
    hwGlGetError( void );

/* HWGL-specific functions */
extern void
    __hwGlBeginSelect( glDrawable *glctx ),
    __hwGlEndSelect( glDrawable *glctx ),
    hwGlInitProgState( hwProgInfo *ppi ),
    hwGlBeginRendering( void );

#if !defined(HWGL_LIGHT_IMP) /* [ */
    /* Make it so use of these things is an error unless in the
     * implementation of gl_lighting.c
     */
#define glEnable                syntax error
#define glDisable               syntax error

#ifndef WIN32
// This gets confusing with extension pointers
#define glActiveTexture         syntax error
#define glClientActiveTexture   syntax error
#endif

#define glLightfv               syntax error
#define glLightiv               syntax error
#define glLightf                syntax error
#define glLighti                syntax error

#define glFogfv                 syntax error
#define glFogiv                 syntax error
#define glFogf                  syntax error
#define glFogi                  syntax error

#define glMaterialfv            syntax error
#define glMaterialiv            syntax error
#define glMaterialf             syntax error
#define glMateriali             syntax error

#define glColorMaterial         syntax error

#define glLightModelfv          syntax error
#define glLightModeliv          syntax error
#define glLightModelf           syntax error
#define glLightModeli           syntax error

#define glShadeModel            syntax error
#define glPointSize             syntax error

#define glTexEnvfv              syntax error
#define glTexEnviv              syntax error
#define glTexEnvf               syntax error
#define glTexEnvi               syntax error

#define glMatrixMode            syntax error
#define glPushMatrix            syntax error
#define glPopMatrix             syntax error

#define glLoadMatrixf           syntax error
#define glLoadMatrixd           syntax error
#define glTranslatef            syntax error
#define glTranslated            syntax error
#define glScalef                syntax error
#define glScaled                syntax error
#define glMultMatrixf           syntax error
#define glMultMatrixd           syntax error

#define glGetFloatv             syntax error
#define glFrustum               syntax error
#define glOrtho                 syntax error
#define glLoadIdentity          syntax error

#define glEnableClientState     syntax error
#define glDisableClientState    syntax error
#define glVertexPointer         syntax error
#define glNormalPointer         syntax error
#define glTexCoordPointer       syntax error
#define glColorPointer          syntax error

#define glColor4f               syntax error
#define glColor4ub              syntax error

#endif /* ] */

/* We never want to use these for GLES */
#define glBegin                 syntax error
#define glEnd                   syntax error

#define glVertex2d              syntax error
#define glVertex2f              syntax error
#define glVertex2i              syntax error
#define glVertex2s              syntax error
#define glVertex2dv             syntax error
#define glVertex2fv             syntax error
#define glVertex2iv             syntax error
#define glVertex2sv             syntax error
#define glVertex3d              syntax error
#define glVertex3f              syntax error
#define glVertex3i              syntax error
#define glVertex3s              syntax error
#define glVertex3dv             syntax error
#define glVertex3fv             syntax error
#define glVertex3iv             syntax error
#define glVertex3sv             syntax error
#define glVertex4d              syntax error
#define glVertex4f              syntax error
#define glVertex4i              syntax error
#define glVertex4s              syntax error
#define glVertex4dv             syntax error
#define glVertex4fv             syntax error
#define glVertex4iv             syntax error
#define glVertex4sv             syntax error

#define glNormal3b              syntax error
#define glNormal3d              syntax error
#define glNormal3f              syntax error
#define glNormal3i              syntax error
#define glNormal3s              syntax error
#define glNormal3bv             syntax error
#define glNormal3dv             syntax error
#define glNormal3fv             syntax error
#define glNormal3iv             syntax error
#define glNormal3sv             syntax error


#define glColor3b               syntax error
#define glColor3d               syntax error
#define glColor3f               syntax error
#define glColor3i               syntax error
#define glColor3s               syntax error
#define glColor3ub              syntax error
#define glColor3ui              syntax error
#define glColor3us              syntax error
#define glColor3bv              syntax error
#define glColor3dv              syntax error
#define glColor3fv              syntax error
#define glColor3iv              syntax error
#define glColor3sv              syntax error
#define glColor3ubv             syntax error
#define glColor3uiv             syntax error
#define glColor3usv             syntax error
#define glColor4b               syntax error
#define glColor4d               syntax error
//#define glColor4f             syntax error  In a different section
#define glColor4i               syntax error
#define glColor4s               syntax error
//#define glColor4ub            syntax error  In a different section
#define glColor4ui              syntax error
#define glColor4us              syntax error
#define glColor4bv              syntax error
#define glColor4dv              syntax error
#define glColor4fv              syntax error
#define glColor4iv              syntax error
#define glColor4sv              syntax error
#define glColor4ubv             syntax error
#define glColor4uiv             syntax error
#define glColor4usv             syntax error

#define glTexCoord1d            syntax error
#define glTexCoord1f            syntax error
#define glTexCoord1i            syntax error
#define glTexCoord1s            syntax error
#define glTexCoord1dv           syntax error
#define glTexCoord1fv           syntax error
#define glTexCoord1iv           syntax error
#define glTexCoord1sv           syntax error
#define glTexCoord2d            syntax error
#define glTexCoord2f            syntax error
#define glTexCoord2i            syntax error
#define glTexCoord2s            syntax error
#define glTexCoord2dv           syntax error
#define glTexCoord2fv           syntax error
#define glTexCoord2iv           syntax error
#define glTexCoord2sv           syntax error
#define glTexCoord3d            syntax error
#define glTexCoord3f            syntax error
#define glTexCoord3i            syntax error
#define glTexCoord3s            syntax error
#define glTexCoord3dv           syntax error
#define glTexCoord3fv           syntax error
#define glTexCoord3iv           syntax error
#define glTexCoord3sv           syntax error
#define glTexCoord4d            syntax error
#define glTexCoord4f            syntax error
#define glTexCoord4i            syntax error
#define glTexCoord4s            syntax error
#define glTexCoord4dv           syntax error
#define glTexCoord4fv           syntax error
#define glTexCoord4iv           syntax error
#define glTexCoord4sv           syntax error

#define glGenLists              syntax error
#define glNewList               syntax error
#define glEndList               syntax error

/*** EOF hw_gl.h ***/
