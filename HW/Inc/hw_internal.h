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

/* Internal datatypes/functions for HoverWare implementation */
#define HW_MAX_FLOAT    1.0e38
#define HW_OPT_SAVED_DATA       (HW_OPT_CACHE_DATA \
                                | HW_OPT_USE_DL \
                                | HW_OPT_DL_ATTRS)

/* Selection type, used by selection code */
typedef struct {
    hwInt32
        flags;          /* In: Picking info */
    hwFloat
        point[2],       /* In: XY position for selection */
        aperture,       /* In: pick aperture */
        matrix[4][4],   /* In: WC->DC matrix */
        dist;           /* Out: Depth of pick */
    hwObject
        obj;            /* Out: Which object was active at selection */
    hwInt32
        picked,         /* Did a pick happen? */
        vert;           /* Out: vertex offset if HW_SELECT_VERTICES */
} hwSelectionType;

/* Possible strings for vertex inquiry */
#define STR_VERTEX      hwStrHasRGB, hwStrHasAlpha, hwStrHasNormals, \
                        hwStrHasTangent, \
                        hwStrHasST0, hwStrHasST1, hwStrHasST2, hwStrHasST3, \
                        hwStrHasST4, hwStrHasST5, hwStrHasST6, hwStrHasST7, \
                        hwStrHasR0, hwStrHasR1, hwStrHasR2, hwStrHasR3, \
                        hwStrHasR4, hwStrHasR5, hwStrHasR6, hwStrHasR7, \
                        hwStrHasQ0, hwStrHasQ1, hwStrHasQ2, hwStrHasQ3, \
                        hwStrHasQ4, hwStrHasQ5, hwStrHasQ6, hwStrHasQ7,

/* Possible strings for orient inquiry */
#define STR_ORIENT      hwStrScale, hwStrRotate, hwStrPos,


/* Possible strings for surface inquiry */
#define STR_SURF        hwStrColor, hwStrTransparency, hwStrShininess, \
                        hwStrBackface, hwStrTwoSided, hwStrBright, \
                        hwStrInvisible, hwStrWireframe, hwStrVisibility, \
                        hwStrTexture, hwStrSpecColor, hwStrPrunePrim, \
                        hwStrBlend, hwStrFlipNormals, hwStrUncolored, \
                        hwStrSampVert, hwStrPrimSize, hwStrPreMult,

/* Vertex utilities */
extern void *__hwIntVtxTab;
extern int __hwIntInitVtx( void );
extern int __hwIntVtxModify( hwInt32 *, hwInt32 prop,
                                        hwInt32 type, const void *val );
extern hwInt32 __hwIntVtxInquire( hwInt32 *, hwInt32 prop, void **val );

/* Surface and orientation utilities */
extern void *__hwIntSurfTab;
extern int __hwIntInitSurf( void );
extern int __hwIntSurfModify( hwSurfaceType *, int propno,
                                        hwInt32 type, const void *val );
extern hwInt32 __hwIntSurfInquire( hwSurfaceType *, int propno, void **val );

extern void *__hwIntOrientTab;
extern int __hwIntInitOrient( void );
extern void __hwIntOrientModify( hwOrientType *, int propno,
                                        hwInt32 type, const void *val );
extern hwInt32 __hwIntOrientInquire( hwOrientType *, int propno, void **val );

/* Tesselation functions */
extern void __hwIntMakeSphere
(
    hwInt32 Rows, hwInt32 Cols,         /* Tesselation parameters */
    hwInt32 Flags,                      /* What kind of data to generate */
    hwFloat Radius,             /* How big to make it */
    hwFloat *Verts,             /* Where to put generated data */
    hwFloat Lat0, hwFloat Lat1, /* Latitude range */
    hwFloat Lon0, hwFloat Lon1, /* Longitude range */
    hwFloat ObjColor[3]         /* Color, if needed */
);
extern void __hwIntMakeBox
(
    hwInt32 Flags, hwFloat *Corners, hwFloat *Dst,
    hwFloat ObjColor[3]
);
extern void __hwIntMakeRing
(
    hwInt32 Rows, hwInt32 Cols,                 /* Tesselation parameters */
    hwInt32 Flags,                              /* What data to generate */
    hwFloat Inner, hwFloat Outer,               /* Inner+outer radii of ring */
    hwFloat *Verts,                     /* Where to put the data */
    hwFloat Lon0, hwFloat Lon1,         /* Which part of the ring to do */
    hwFloat ObjColor[3]                 /* The color to make it */
);
extern void __hwIntMakeThinRing
(
    hwInt32 Rows,                       /* Tesselation parameter */
    hwInt32 Flags,                      /* What data to generate */
    hwFloat Rad,                        /* Radius of ring */
    hwFloat *Verts,             /* Where to put data */
    hwFloat ObjColor[3]         /* What color, if any */
);
extern void __hwIntMakeTorus
(
    hwInt32 Rows, hwInt32 Cols,                 /* Tesselation parameters */
    hwInt32 Flags,                              /* What data to generate */
    hwFloat Inner, hwFloat Outer,               /* Inner and outer radii */
    hwFloat *Verts,                     /* Where to store the data */
    hwFloat Lat0, hwFloat Lat1,         /* Latitude parameters */
    hwFloat Lon0, hwFloat Lon1,         /* Longitude parameters */
    hwFloat ObjColor[3]                 /* What color, if any */
);
extern void __hwIntMakeCone
(
    hwInt32 Rows, hwInt32 Cols,                         /* Tesselation parameters */
    hwInt32 Flags,                                      /* Data to generate */
    hwFloat Inner, hwFloat Outer, hwFloat Length,       /* Describes the cone */
    hwFloat *Verts,                             /* The destination mesh data */
    hwFloat Lon0, hwFloat Lon1,                 /* Angles to start */
    hwFloat ObjColor[3]                         /* What color, if any */
);
extern void __hwIntMakeSurfRev
(
    hwInt32 n, hwInt32 m,               /* Tesselation parameters */
    hwInt32 Flags,                      /* Data to generate */
    hwFloat Ang0,                       /* Ending angle */
    hwFloat Ang1,                       /* Ending angle */
    hwFloat *Points,            /* Source points to revolve */
    hwFloat *Mesh,              /* Where to store data */
    hwFloat ObjColor[3]         /* What color, if any */
);
extern void __hwIntMakeTerrain
(
    hwInt32 Flags,                      /* Data to generate */
    hwFloat *Data,              /* Source vertices to interpolate */
    hwInt32 gn, hwInt32 gm,             /* Size of destination mesh */
    hwFloat *NewData            /* Where to put interpolated mesh */
);
extern hwInt32 __hwIntCalcOpen
(
    hwFloat *Joints,
    hwInt32 Count,
    hwFloat *BPoints
);
extern hwInt32 __hwIntCalcClosed
(
    hwFloat *Joints,
    hwInt32 Count,
    hwFloat *BPoints
);

/* Data utility functions */
extern hwInt32 __hwIntGuiColor( hwInt32 type, const void *val );

extern void __hwIntTextureData
(
    hwImageStruct **img, hwInt32 numTex,
    hwInt32 Replace, hwInt32 Clamp,
    hwFloat *Data, hwInt32 n, hwInt32 Flags
);

extern void __hwIntRemoveUV( hwInt32 vn, hwInt32 n, hwFloat *Data,
                hwInt32 numCoord );
extern hwFloat *__hwIntAddUV(
    hwInt32 vn, hwInt32 n, hwFloat *Data, hwInt32 *dataSize
);
extern hwFloat *__hwIntAddRGB(
    hwInt32 vn, hwInt32 n, const hwFloat Color[3],
    hwFloat *Data, hwInt32 *dataSize,
    hwInt32 alphaOffset
);
void __hwIntSphereGen(
    const hwOrientType *orient,
    hwFloat *Data, hwInt32 n, hwInt32 Flags, hwInt32 which
);
void __hwIntCylGen(
    const hwOrientType *orient,
    hwFloat *Data, hwInt32 n, hwInt32 Flags, hwInt32 which
);
void __hwIntPlaneGen(
    const hwOrientType *orient,
    hwFloat *Data, hwInt32 n, hwInt32 Flags, hwInt32 which
);
void __hwIntOrientData(
    const hwOrientType *orient,
    hwFloat *Data, hwInt32 n, hwInt32 Flags, hwInt32 which
);
extern hwInt32 __hwIntTexturePipeline
(
    hwSurfaceType *surf,
    hwFloat **data,
    int *dataSize,
    hwInt32 *vn,
    hwInt32 numVerts,
    hwInt32 dataFlags
);
extern void __hwIntSmoothMesh
(
    hwFloat *Data, hwInt32 n, hwInt32 m, hwInt32 Flags
);
extern void __hwIntMatXformData
(
    hwFloat Mat[4][4], hwFloat *Data, hwInt32 n, hwInt32 Flags
);
extern void __hwIntTransformData
(
    const hwOrientType *orient, hwFloat *Data, hwInt32 n, hwInt32 Flags
);
extern void __hwIntMeshInterp
(
    hwFloat *data, hwInt32 n, hwInt32 m,
    hwFloat **result, hwInt32 newN, hwInt32 newM,
    hwInt32 flags
);
extern void __hwIntUpdateBounds(
    hwFloat Bounds[6], const hwFloat *Data, hwInt32 n, hwInt32 vn
);

extern void __hwIntBezier
(
    hwFloat *BPoints,
    hwFloat U,
    hwFloat *RX, hwFloat *RY, hwFloat *RZ,
    hwFloat *TX, hwFloat *TY, hwFloat *TZ
);

/* Error functions */
extern void __hwIntSetError( hwInt32 err );

/* Image manipulation functions */
extern hwImageStruct *__hwIntReadPPM( char *Name );
extern hwImageStruct *__hwIntReadJPG( char *Name );
extern hwImageStruct *__hwIntReadTIF( char *Name );
extern hwImageStruct *__hwIntReadPNG( char *Name );
extern void __hwIntModifyAlpha( hwImageStruct *, unsigned char [] );
extern hwImageStruct *__hwIntCreateImage( void );
extern void __hwIntDestroyImage( hwImageStruct *img );

extern double __hwIntStartTime;
extern double __hwIntElapsedTime;

extern void __hwIntTextOrient
(
    hwDisplay disp,
    hwFloat Upx, hwFloat Upy, hwFloat Upz,
    hwFloat Dirx, hwFloat Diry, hwFloat Dirz
);

extern void __hwInitTextState( hwTextState *ptr );
extern void __hwIntText3d( hwDisplay disp, hwFloat, hwFloat, hwFloat, char * );

extern void __hwIntTextHeight( hwDisplay disp, hwFloat Height );

extern void __hwIntTextExtrude( hwDisplay disp,
                hwFloat ex0, hwFloat ex1, hwFloat ex2 );

extern void __hwIntTextAlign( hwDisplay disp,
        hwInt32 Horiz, hwInt32 Vert, hwInt32 Depth );

extern void __hwIntTextAttrs(
        hwDisplay disp,
        hwSurfaceType *, hwSurfaceType *, hwSurfaceType *);

extern void __hwIntVecToMat(
        const hwFloat UpVec[3],
        const hwFloat DirVec[3],
        hwFloat Result[4][4]
    );

/* Picking functions */
extern hwInt32 __hwIntPickMarkers(
                hwFloat *, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickStrip(
                hwFloat *, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickMesh(
                hwFloat *, hwInt32, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickPolygon(
                hwFloat *, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickPolyline(
                hwFloat *, hwInt32, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickQuads(
                hwFloat *, hwInt32, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickIndexedTris(
                hwFloat *, hwInt32, hwInt32,
                hwInt32 *, hwInt32, hwSelectionType * );
extern hwInt32 __hwIntPickPolyhedron(
                hwFloat *, hwInt32, hwInt32,
                hwInt32 *, hwInt32, hwSelectionType * );

/* Global state management functions */
extern void __hwInternalInitDisplay( hwDisplay disp, hwDisplay shareDisp );
extern void __hwInternalMakeCurrent( hwDisplay disp );
extern void __hwIntDestroyList( hwInt32 dl );
extern void __hwIntDestroyGuiList( hwInt32 dl );

/* GUI support routines */
void __hwIntGetParentBounds( hwDisplay disp, hwObject parent,
                             hwInt32 parentBounds[] );
void __hwIntPositionWidget( hwInt32 bounds[],
                            hwInt32 parentBounds[],
                            hwInt32 posX, hwInt32 posY,
                            hwInt32 width, hwInt32 height,
                            hwInt32 align,
                            hwFloat aspectRatio );
extern void __hwIntDrawButton( hwDisplay disp,
                        char *label, hwInt32 size, TexFont *txf,
                        hwInt32 fg, hwInt32 bg, hwInt32 bord, hwInt32 tm,
                        hwInt32 bbox[], hwInt32 parentBbox[],
                        hwInt32 align,
                        hwInt32 checkBox );

#define HW_USE_CURR_DISP hwDisplay __hwDisp = HW_GET_CURR_DISP()

/* Hoverware malloc / free / realloc routines */
extern void *hwMalloc( size_t size, const char *file, int line );
extern void hwFree( void *ptr, const char *file, int line );
extern void *hwRealloc( void *ptr, size_t size, const char *file, int line );

#define malloc(size) hwMalloc(size, __FILE__, __LINE__)
#define free(ptr) hwFree(ptr, __FILE__, __LINE__)
#define realloc(ptr, size) hwRealloc(ptr, size, __FILE__, __LINE__)

/* Locking hash create */
extern int __hwAllocTabLock(void **tab, int tabSize);

#include "hw_mutex.h"

/* Name space for objects */
typedef struct __hwNameSpace {
    HW_DECLARE_MUTEX
        (mutex);
    struct __hwDisplayInternal
        *shared;
    void
        *objectHash;
    hwObject
        *objectTable;
    int
        numObject, allocObject;
} *hwNameSpace;

/* Place to keep private data - such as scratch arrays */
typedef struct __hwDisplayInternal {
    /* The user-visible base display stuff */
    struct __hwDisplayStruct vtab;

    /* The name space for this hwDisplay */
    hwNameSpace nameSpace;

    /* The last error */
    hwInt32 hwErrNo;

    /* Random number stuff */
    unsigned short rand48_seed[3];
    unsigned short rand48_mult[3];
    unsigned short rand48_add;

    /* Scratch data areas for various uses */
    hwFloat *scratchData, *scratchData2, *scratchData3;
    hwInt32 scratchDataSize, scratchDataSize2, scratchDataSize3;
    hwInt32 scratchInt[3];
    hwFloat scratchFloat[9];
    hwSurfaceType scratchSurf;
} *hwDisplayInternal;
