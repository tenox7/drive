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

#include <stdint.h>

/* Basic type definitions */
typedef char            hwInt8;
typedef unsigned char   hwUint8;
typedef short           hwInt16;
typedef unsigned short  hwUint16;
typedef int32_t         hwInt32;
typedef uint32_t        hwUint32;
typedef float           hwFloat;

/* A HoverWare display object, forward reference (see hw_display.h) */
typedef struct __hwDisplayStruct *hwDisplay;

/* List of property/type pairs that each object supports
 * for its inquire method:
 */
/* The root of all HoverWare: the hwObject */
typedef struct _hwObjectStruct *hwObject;
struct _hwObjectStruct {
    hwObject    parent;                 /* Parent class */
    const char  *name;                  /* Name, if any */
    const char  **props;                /* List of valid properties */

    hwObject    (*create)( hwObject );
    void        (*addref)( hwObject );
    void        (*destroy)( hwObject );
    void        (*modify)( hwObject, const char *, hwInt32, const void * );
    hwInt32     (*inquire)( hwObject, const char *, void ** );
    void        (*draw)( hwObject );
};

/* Type construction for modify - avoid the high-order bit for
 * portability!
 */
#define HW_TYPE_BYTE            0x01000000L
#define HW_TYPE_SHORT           0x02000000L
#define HW_TYPE_INT             0x03000000L
#define HW_TYPE_BOOL            0x04000000L
#define HW_TYPE_FLOAT           0x05000000L
#define HW_TYPE_STRING          0x06000000L
#define HW_TYPE_OBJECT          0x07000000L
#define HW_TYPE_CALLBACK        0x08000000L
#define HW_TYPE_EVENT           0x09000000L
#define HW_TYPE_IMAGE           0x0A000000L
#define HW_TYPE_FONT            0x0B000000L

/* If, on inquire, this bit is set, it means the property is unmodified
 * from default values, and need not be written out.
 */
#define HW_TYPE_CLEAN           0x80000000L

#define HW_MAKE_TYPE(base,count)        ((base)|((count)&0x00FFFFFF))
#define HW_GET_BASE(type)               ((type)&0x7F000000)
#define HW_GET_COUNT(type)              ((type)&0x00FFFFFF)

#define HW_TYPE_1I              HW_MAKE_TYPE(HW_TYPE_INT,1)
#define HW_TYPE_2I              HW_MAKE_TYPE(HW_TYPE_INT,2)
#define HW_TYPE_3I              HW_MAKE_TYPE(HW_TYPE_INT,3)
#define HW_TYPE_4I              HW_MAKE_TYPE(HW_TYPE_INT,4)
#define HW_TYPE_1B              HW_MAKE_TYPE(HW_TYPE_BOOL,1)
#define HW_TYPE_1F              HW_MAKE_TYPE(HW_TYPE_FLOAT,1)
#define HW_TYPE_2F              HW_MAKE_TYPE(HW_TYPE_FLOAT,2)
#define HW_TYPE_3F              HW_MAKE_TYPE(HW_TYPE_FLOAT,3)
#define HW_TYPE_4F              HW_MAKE_TYPE(HW_TYPE_FLOAT,4)

#define HW_TRUE         1
#define HW_FALSE        0

/* Utility macros for the most common "immediate" calls */
#define HW_MODIFY_1I( obj, prop, a ) { \
    hwInt32 __tmp[1]; __tmp[0] = (a); \
    (obj)->modify( (obj), (prop), HW_TYPE_1I, __tmp ); \
}
#define HW_MODIFY_2I( obj, prop, a, b ) { \
    hwInt32 __tmp[2]; __tmp[0] = (a); __tmp[1] = (b); \
    (obj)->modify( (obj), (prop), HW_TYPE_2I, __tmp ); \
}
#define HW_MODIFY_3I( obj, prop, a, b, c ) { \
    hwInt32 __tmp[3]; __tmp[0] = (a); __tmp[1] = (b); __tmp[2] = (c); \
    (obj)->modify( (obj), (prop), HW_TYPE_3I, __tmp ); \
}
#define HW_MODIFY_1B( obj, prop, a ) { \
    hwInt32 __tmp[1]; __tmp[0] = (a); \
    (obj)->modify( (obj), (prop), HW_TYPE_1B, __tmp ); \
}
#define HW_MODIFY_1F( obj, prop, a ) { \
    hwFloat __tmp[1]; __tmp[0] = (a); \
    (obj)->modify( (obj), (prop), HW_TYPE_1F, __tmp ); \
}
#define HW_MODIFY_2F( obj, prop, a, b ) { \
    hwFloat __tmp[2]; __tmp[0] = (a); __tmp[1] = (b); \
    (obj)->modify( (obj), (prop), HW_TYPE_2F, __tmp ); \
}
#define HW_MODIFY_3F( obj, prop, a, b, c ) { \
    hwFloat __tmp[3]; __tmp[0] = (a); __tmp[1] = (b); __tmp[2] = (c); \
    (obj)->modify( (obj), (prop), HW_TYPE_3F, __tmp ); \
}
#define HW_MODIFY_4F( obj, prop, a, b, c, d ) { \
    hwFloat __tmp[4]; __tmp[0] = (a); __tmp[1] = (b); __tmp[2] = (c); __tmp[3] = (d); \
    (obj)->modify( (obj), (prop), HW_TYPE_4F, __tmp ); \
}

/* GUI callbacks */
#define HW_CB_ACTIVATE          1       /* Widget was activated */
#define HW_CB_TRACK             2       /* Widget was changed while tracking */
#define HW_CB_DEACTIVATE        3       /* Widget was cancelled */
#define HW_CB_COMPLETE          4       /* W was deactivated inside bbox */

typedef void (*hwCallback)( hwObject, hwInt32 );

/* Errors from hwGetError */
#define HW_ERROR_NO_MEMORY      1
#define HW_ERROR_BAD_PROP       2
#define HW_ERROR_BAD_TYPE       3
#define HW_ERROR_INTERNAL       4

/* Flags for hwChooseVisual */
#define HW_VIS_DBUFF    0x00000001      /* Give me a double-buffered visual */
#define HW_VIS_DEPTH    0x00000002      /* ...with a depth buffer */
#define HW_VIS_ALPHA    0x00000004      /* ...with an alpha buffer */
#define HW_VIS_STENCIL  0x00000008      /* ...with a stencil buffer */
#define HW_VIS_TEXTURE  0x00000010      /* ...with accelerated textures */
#define HW_VIS_OVERLAY  0x00000020      /* ...an overlay visual */
#define HW_VIS_STEREO   0x00000040      /* ...quadbuffered stereo */
#define HW_VIS_AA       0x00000080      /* ... multisample AA (< 4 samples) */
#define HW_VIS_AA_MED   0x00000100      /* ... multisample AA (< 8 samples) */
#define HW_VIS_AA_HI    0x00000200      /* ... multisample AA (>= 8 samples) */
#define HW_VIS_SUCCESS  0x80000000      /* We succeeded! */

/* Values for disp->createWindow */
#define HW_WIN_INPUT            0x00000001      /* Allow window input */
#define HW_WIN_FULLSCREEN       0x00000002      /* Full screen */
#define HW_WIN_BORDERLESS       0x00000004      /* No window decorations */
#define HW_WIN_HIDE_CURSOR      0x00000008      /* Hide the cursor */
#define HW_WIN_JOYSTICK         0x00000010      /* Allow joystick input */

/* Values for disp->getInfo */
#define HW_INFO_HW              0       /* The name of the HW renderer */
#define HW_INFO_VENDOR          1       /* The name of the gfx base vendor */
#define HW_INFO_RENDERER        2       /* The name of the gfx base renderer */
#define HW_INFO_VERSION         3       /* The version of the gfx base rend. */
#define HW_INFO_EXTENSIONS      4       /* Any extensions? */
#define HW_INFO_JOYSTICK0       5       /* Joystick 0 name, if any? */
#define HW_INFO_JOYSTICK1       6       /* Joystick 1 name, if any? */
#define HW_INFO_JOYSTICK2       7       /* Joystick 2 name, if any? */
#define HW_INFO_JOYSTICK3       8       /* Joystick 3 name, if any? */

#define HW_MAX_JOYSTICKS        4

/* Texture parameters */
#define HW_TM_MODULATE  0       /* Modulate amb/diff color with texture color */
#define HW_TM_BUMP      1       /* Normal map */
#define HW_TM_RELIEF    2       /* Normal + relief */
#define HW_TM_GLOSS     3       /* Gloss map */
#define HW_TM_SHADOW    4       /* Shadow map */
#define HW_TM_SHADOW_VAR 5      /* Variance shadow map */

#define HW_TM_REPEAT    0       /* Repeat texels */
#define HW_TM_CLAMP     1       /* Clamp to border value */

#define HW_TM_EXPLICIT  0       /* Explicit texture coords */
#define HW_TM_PLANAR    1       /* Planar texture coords */
#define HW_TM_SPHERE    2       /* Spherical texture coords */
#define HW_TM_CYLINDER  3       /* Cylindrical texture coords */
#define HW_TM_ENVMAP    4       /* Environment mapping */
#define HW_TM_STAGE_0   5       /* Reuse the texture coords from stage 0 */

#define HW_TM_POINT     0       /* Point sampled */
#define HW_TM_LINEAR    1       /* Linear base */
#define HW_TM_MIP       2       /* Linear MIP */
#define HW_TM_TRILINEAR 3       /* Trilinear filtered */

#define HW_TM_COLOR     0       /* This is a lum, lum-alpha, rgb, or rgba tex */
#define HW_TM_ALPHA     1       /* This is a one-component alpha texture */
#define HW_TM_INTENSITY 2       /* This is a one-component intensity texture */
#define HW_TM_HEIGHT    3       /* This is a one-component height field */
#define HW_TM_NORMAL    4       /* This is a 3-component normal map */
#define HW_TM_NORMAL_HEIGHT 5   /* This is a 4-component (nx,ny,nz,h) map */
#define HW_TM_DEPTH     6       /* This is a 1-component depth map */
#define HW_TM_DEPTH_VAR 7       /* This is a 2-component depth+variance map */

#define HW_MAX_TEXTURES 8       /* Maximum # of textures supported */

/* Fog parameters */
#define HW_FOG_LINEAR   0
#define HW_FOG_EXP      1
#define HW_FOG_EXP2     2

/* Text parameters */
#define HW_TEXT_ALIGN_LEFT      1
#define HW_TEXT_ALIGN_CENTER    2
#define HW_TEXT_ALIGN_RIGHT     3

#define HW_TEXT_ALIGN_TOP       4
#define HW_TEXT_ALIGN_BOTTOM    5

#define HW_TEXT_ALIGN_FRONT     6
#define HW_TEXT_ALIGN_BACK      7

/* Widget alignment parameters */
#define HW_GUI_REL_RIGHT        0x00000001      /* Position is right-rel */
#define HW_GUI_REL_BOT          0x00000002      /* Position is bot-rel */
#define HW_GUI_REL_WIDTH        0x00000004      /* Width is size-rel */
#define HW_GUI_REL_HEIGHT       0x00000008      /* Height is size-rel */
#define HW_GUI_FRACTIONAL       0x00000010      /* Pos/size are rel. to win. */
#define HW_GUI_CENTER           0x00000020      /* Pos is center of widget */
#define HW_GUI_POS_MASK         0x0000003F      /* Mask of widget pos flags */
#define HW_GUI_LABEL_LEFT       0x00000040      /* Left adjust label */
#define HW_GUI_LABEL_RIGHT      0x00000080      /* Right adjust label */
#define HW_GUI_LABEL_TOP        0x00000100      /* Top-adjust label */
#define HW_GUI_LABEL_BOTTOM     0x00000200      /* Right adjust label */
#define HW_GUI_TOGGLE_RIGHT     0x00000400      /* Vs. on the left */
#define HW_GUI_FONT_FRACTIONAL  0x00000800      /* The font height is fract. */
#define HW_GUI_FONT_FRACT_W     0x00001000      /* The font height is fract. */
#define HW_GUI_DO_NOT_USE_BITS  0xFF800000      /* Don't use these bits */

/* Primitive per-vertex data parameters.  Note:  Some of the
 * flags are dependent on other flags.  Specifically, HW_DATA_ALPHA
 * is only valid if HW_DATA_RGB is present.  Likewise, HW_DATA_[RQ]*
 * is only valid if HW_DATA_ST* is present.  Data is present in
 * the vertex in the order shown here, with XYZ always present and
 * always first in the vertex.  The largest vertex format
 * supported currently is 49 floats per vertex (!).
 */                                             /* 3 - XYZ */
#define HW_DATA_W               (1 <<  0)       /* +1 = 4 */
#define HW_DATA_RGB             (1 <<  1)
#define HW_DATA_ALPHA           (1 <<  2)       /* +4 = 8 */
#define HW_DATA_NORMALS         (1 <<  3)       /* +3 = 11 */
#define HW_DATA_MD_FLAGS        (1 <<  4)       /* +1 = 12 */
#define HW_DATA_TANGENT         (1 <<  5)       /* +3 = 15 */
#define HW_DATA_ST              (1 <<  6)
#define HW_DATA_R               (1 <<  7)
#define HW_DATA_Q               (1 <<  8)       /* +4 = 19 */
#define HW_DATA_ST1             (1 <<  9)
#define HW_DATA_R1              (1 << 10)
#define HW_DATA_Q1              (1 << 11)       /* +4 = 23 */
#define HW_DATA_ST2             (1 << 12)
#define HW_DATA_R2              (1 << 13)
#define HW_DATA_Q2              (1 << 14)       /* +4 = 27 */
#define HW_DATA_ST3             (1 << 15)
#define HW_DATA_R3              (1 << 16)
#define HW_DATA_Q3              (1 << 17)       /* +4 = 31 */
#define HW_DATA_ST4             (1 << 18)
#define HW_DATA_R4              (1 << 19)
#define HW_DATA_Q4              (1 << 20)       /* +4 = 35 */
#define HW_DATA_ST5             (1 << 21)
#define HW_DATA_R5              (1 << 22)
#define HW_DATA_Q5              (1 << 23)       /* +4 = 39 */
#define HW_DATA_ST6             (1 << 24)
#define HW_DATA_R6              (1 << 25)
#define HW_DATA_Q6              (1 << 26)       /* +4 = 43 */
#define HW_DATA_ST7             (1 << 27)
#define HW_DATA_R7              (1 << 28)
#define HW_DATA_Q7              (1 << 29)       /* +4 = 47 */
/* Bits 30, 31 unused & reserved */

#define HW_DATA_MAX_WPV         47

#define HW_DATA_UV      HW_DATA_ST      /* Backwards compatibility */

/* Some convenience macros which combine some of the above flags */
#define HW_DATA_RGBA            (HW_DATA_RGB |HW_DATA_ALPHA)
#define HW_DATA_STR             (HW_DATA_ST  |HW_DATA_R )
#define HW_DATA_STRQ            (HW_DATA_STR |HW_DATA_Q )
#define HW_DATA_STR1            (HW_DATA_ST1 |HW_DATA_R1)
#define HW_DATA_STRQ1           (HW_DATA_STR1|HW_DATA_Q1)
#define HW_DATA_STR2            (HW_DATA_ST2 |HW_DATA_R2)
#define HW_DATA_STRQ2           (HW_DATA_STR2|HW_DATA_Q2)
#define HW_DATA_STR3            (HW_DATA_ST3 |HW_DATA_R3)
#define HW_DATA_STRQ3           (HW_DATA_STR3|HW_DATA_Q3)
#define HW_DATA_STR4            (HW_DATA_ST4 |HW_DATA_R4)
#define HW_DATA_STRQ4           (HW_DATA_STR4|HW_DATA_Q4)
#define HW_DATA_STR5            (HW_DATA_ST5 |HW_DATA_R5)
#define HW_DATA_STRQ5           (HW_DATA_STR5|HW_DATA_Q5)
#define HW_DATA_STR6            (HW_DATA_ST6 |HW_DATA_R6)
#define HW_DATA_STRQ6           (HW_DATA_STR6|HW_DATA_Q6)
#define HW_DATA_STR7            (HW_DATA_ST7 |HW_DATA_R7)
#define HW_DATA_STRQ7           (HW_DATA_STR7|HW_DATA_Q7)
#define HW_DATA_TEXCOORD        (HW_DATA_STRQ|HW_DATA_STRQ1|HW_DATA_STRQ2\
                                |HW_DATA_STRQ3|HW_DATA_STRQ4|HW_DATA_STRQ5\
                                |HW_DATA_STRQ6|HW_DATA_STRQ7)

/* A structure used to extract offets from vertex flags - see
 * hwGetVertexOffsets for a function which fills this in
 */
typedef struct {
    hwUint8 w;
    hwUint8 rgb;
    hwUint8 alpha;
    hwUint8 normal;
    hwUint8 tangent;
    hwUint8 texCoord[HW_MAX_TEXTURES];
    hwUint8 texSize[HW_MAX_TEXTURES];
} hwVertexOffsets;

/* Image types */
#define HW_IMG_UBYTE    0
#define HW_IMG_USHORT   1
#define HW_IMG_UINT     2
#define HW_IMG_FLOAT    3

/* Optimization levels */
#define HW_OPT_SAFE_USER_DATA   0x00000001
#define HW_OPT_CACHE_DATA       0x00000002
#define HW_OPT_USE_DL           0x00000004
#define HW_OPT_DL_ATTRS         0x00000008

#define HW_OPT_DEFAULT          HW_OPT_USE_DL

/* Spinner motion types */
#define HW_SPIN_CONSTANT        0
#define HW_SPIN_LINEAR  1
#define HW_SPIN_SMOOTH  2
#define HW_SPIN_RANDOM  3

/* Orientation values, used for many graphics */
typedef struct {
    hwFloat
        scale[3],               /* Scaling on XYZ axes */
        rotate[3],              /* Rotation about XYZ axes */
        pos[3];                 /* Position in 3-space */
} hwOrientType;

/* Surface parameters, used for many graphics */
#define HW_SURF_BACKFACE        0x00000001
#define HW_SURF_TWOSIDED        0x00000002
#define HW_SURF_UNCOLORED       0x00000004
#define HW_SURF_EMISSIVE        0x00000008
#define HW_SURF_INVISIBLE       0x00000010
#define HW_SURF_WIREFRAME       0x00000020
#define HW_SURF_PRUNE_PRIM      0x00000040
#define HW_SURF_BLEND           0x00000080
#define HW_SURF_FLIP_NORMALS    0x00000100
#define HW_SURF_TEX_SAMP        0x00000200
#define HW_SURF_BLEND_PREMULT   0x00000400 // Premultiplied alpha

/* These flags are intended for the three-pass rendering/occlusion algorithm.  They
 * are, in general, NOT intended for direct end-user playing with.  The
 * algorithm is as follows:
 *
 *      1. Set render mode to HW_RENDER_DRAW_UNOCCLUDED.  Draw ALL of the objects.
 *         They will render if and only if the HW_INT_SURF_OCCLUDED flag is NOT set
 *         (initial flag value is clear).
 *      2. Set render mode to HW_RENDER_VISIBILITY_TEST.  Draw ALL of the objects.  The
 *         flags will be set as follows:
 *              HW_INT_FORCE_REDRAW     set if object is visible and OCCLUDED
 *                                      flag is set (indicates an error in the
 *                                      predictive occlusion test)
 *              HW_INT_SURF_OCCLUDED    set only if the object is not visible
 *                                      in the view frustum or depth buffer
 *      3. Set render mode to HW_RENDER_VISIBILITY_FIXUP.  Draw ALL of the objects.
 *         They will render if and only if the HW_INT_FORCE_REDRAW flag is set.
 */
#define HW_INT_SURF_OCCLUDED    0x00000001      /* Surface occluded on check */
#define HW_INT_FORCE_REDRAW     0x00000002      /* Please redraw on 3rd pass */

typedef struct {
    hwFloat
        color[3],               /* RGB color */
        transp,                 /* Transparency */
        specColor[3],           /* Specular highlight color */
        shininess,              /* Shininess coefficient */
        primSize;               /* Line width/point size, in pixels */
    hwInt32
        visibility,             /* Visibility mask */
        flags,                  /* From HW_SURF_*, above */
        internalFlags,          /* Internal flags, for Hoverware use only */
        numTextures;            /* Size of texture array */
    hwObject
        textures[HW_MAX_TEXTURES]; /* Pointer to the textures to use */
    hwInt32
        textureIDs[HW_MAX_TEXTURES]; /* IDs of the textures */
} hwSurfaceType;

/* Image manipulation structure */
typedef struct __hwImageStruct {
    hwInt32
        width, height, depth,   /* Dimensions of image */
        components, type,       /* Type of image */
        refCount,               /* Reference count */
        tmId;                   /* Texture ID for e.g. glBindTexture */
    char
        *fileName;              /* File this was loaded from */
    void
        *data;                  /* Allocated data */
    struct __hwImageStruct
        *next;                  /* Next in hash bin */
} hwImageStruct;

/* Information needed for a camera */
typedef struct {
    hwInt32
        perspective,
        mirror;
    hwFloat
        pos[3],
        dir[3],
        up[3],
        jitter[2],
        skew[2],
        field, aspect,
        planes[2];
} hwCamStruct;

/* Information needed for text */
typedef struct {
    hwFloat
        textUp[3],              /* Up vector of the text */
        textDir[3],             /* Direction vector of the text */
        textOrient[4][4],       /* Orientation, derived from above */
        textScale,              /* Scaling factor of text */
        textScaleOrient[4][4],  /* Combination of scale and orient */
        textExtrude[3];         /* Extrude distance in MCs */
    hwInt32
        horizAlign,             /* From HW_TEXT_ALIGN_* */
        vertAlign,              /* Ditto */
        depthAlign;             /* Ditto */
    hwSurfaceType
        frontSurf,              /* Surface of the front plane */
        sideSurf,               /* Surface of the side */
        backSurf;               /* Surface of the back plane */
} hwTextState;

/* Possible render modes */
#define HW_RENDER_MASK                  0x0000000F      /* One of... */
#define         HW_RENDER_DRAW          0x00000000      /*      Draw prims */
#define         HW_RENDER_SELECT        0x00000001      /*      Select prims */
#define         HW_RENDER_VIS_TEST      0x00000002      /*      Occl. test */
#define HW_RENDER_Z_TEST                0x00000010      /* With depth test? */
#define HW_RENDER_Z_WRITE               0x00000020      /* With Z writes? */
#define HW_RENDER_XOR                   0x00000040      /* With XOR? */
#define HW_RENDER_EDGE_MODE             0x00000080      /* With edge mode? */
#define HW_RENDER_CULL_FACE             0x00000100      /* With backface cull?*/
#define HW_RENDER_MULTISAMPLE           0x00000200      /* Supersampled? */
#define HW_RENDER_QUERY                 0xffffffff      /* return current mode*/

/* Control flags for selection */
#define HW_SELECT_VERTICES      0x00000001      /* Pick only vertices */
#define HW_SELECT_EDGES         0x00000002      /* Pick only edges */
#define HW_SELECT_CHILD         0x00000004      /* Pick child (def: parent) */
#define HW_SELECT_CULL_FACE     0x00000008      /* Don't select backfaces */

#define HW_RENDER_DEFAULT       (HW_RENDER_DRAW | HW_RENDER_Z_TEST | \
                                        HW_RENDER_Z_WRITE | HW_RENDER_CULL_FACE)

/* Possible update flags */
#define HW_UPDATE_SWAP          0x00000001
#define HW_UPDATE_CLEAR         0x00000006
#define HW_UPDATE_CLEAR_COLOR   0x00000002
#define HW_UPDATE_CLEAR_DEPTH   0x00000004
#define HW_UPDATE_MATRIX        0x00000008
#define HW_UPDATE_TIME          0x00000010
#define HW_UPDATE_GUI           0x00000020
#define HW_CLEAR_GUI            0x00000040
#define HW_CHECK_EVENTS         0x00000080
#define HW_UPDATE_ALL           0xFFFFFFFF

/* Possible drawing buffers */
#define HW_DRAW_BACK_LEFT       0
#define HW_DRAW_BACK_RIGHT      1
#define HW_DRAW_FRONT_LEFT      2
#define HW_DRAW_FRONT_RIGHT     3

/* Flags for GUI lines / triangles */
#define HW_GUI_BLEND            0x00000001
#define HW_GUI_SMOOTH           0x00000002

/* Flags for hwBeginBinary() */
#define HW_FILE_WRITE_HDR       0x00000001
#define HW_FILE_READ_HDR        0x00000002

/* Prototype for stream I/O for HW binary utilities */
typedef int (*hwObjRW)( void *, int, int, void * );

/* Window input events */
#define HW_INPUT_EXPOSE         0       /* Change in window exposure */
#define HW_INPUT_CONFIG         1       /* Change in window size/position */
#define HW_INPUT_DESTROY        2       /* Window is destroyed */
#define HW_INPUT_FOCUS          3       /* Change in window focus */
#define HW_INPUT_KEYBOARD       4       /* Keyboard input */
#define HW_INPUT_POINTER        5       /* Pointer (mouse) motion */
#define HW_INPUT_BUTTON_PRESS   6       /* Pointer button press event */
#define HW_INPUT_BUTTON_RELEASE 7       /* Pointer button release event */
#define HW_INPUT_JOY_PRESS      8       /* Joystick button press event */
#define HW_INPUT_JOY_RELEASE    9       /* Joystick button release event */
#define HW_INPUT_JOY_AXIS       10      /* Joystick axis event */

/* Keyboard modifiers */
#define HW_KBD_MOD_SHIFT        0x0001
#define HW_KBD_MOD_CTRL         0x0002
#define HW_KBD_MOD_ALT          0x0004

/* Special keys */
#define HW_KEY_LEFT             0x10000
#define HW_KEY_RIGHT            0x10001
#define HW_KEY_UP               0x10002
#define HW_KEY_DOWN             0x10003
#define HW_KEY_PREV             0x10004
#define HW_KEY_NEXT             0x10005
#define HW_KEY_HOME             0x10006
#define HW_KEY_END              0x10007
#define HW_KEY_F1               0x10008
#define HW_KEY_F2               0x10009
#define HW_KEY_F3               0x1000A
#define HW_KEY_F4               0x1000B
#define HW_KEY_F5               0x1000C
#define HW_KEY_F6               0x1000D
#define HW_KEY_F7               0x1000E
#define HW_KEY_F8               0x1000F
#define HW_KEY_F9               0x10010
#define HW_KEY_F10              0x10011
#define HW_KEY_F11              0x10012
#define HW_KEY_F12              0x10013

/* Joystick / gamepad support */
/* Passed with HW_INPUT_JOY_PRESS / HW_INPUT_JOY_RELEASE */
#define HW_JOY_DPAD_UP          0
#define HW_JOY_DPAD_DOWN        1
#define HW_JOY_DPAD_LEFT        2
#define HW_JOY_DPAD_RIGHT       3
#define HW_JOY_START            4
#define HW_JOY_SELECT           5
#define HW_JOY_THUMB_LEFT       6
#define HW_JOY_THUMB_RIGHT      7
#define HW_JOY_SHOULDER_LEFT    8
#define HW_JOY_SHOULDER_RIGHT   9
#define HW_JOY_HOME             10
#define HW_JOY_A                11
#define HW_JOY_B                12
#define HW_JOY_X                13
#define HW_JOY_Y                14
#define HW_JOY_TRIGGER_LEFT     15
#define HW_JOY_TRIGGER_RIGHT    16

/* Passed with HW_INPUT_JOY_AXIS */
#define HW_AXIS_LEFT_X          0
#define HW_AXIS_LEFT_Y          1
#define HW_AXIS_RIGHT_X         2
#define HW_AXIS_RIGHT_Y         3
#define HW_AXIS_LTRIGGER        4
#define HW_AXIS_RTRIGGER        5
#define HW_AXIS_HAT_X           6
#define HW_AXIS_HAT_Y           7

typedef union {
    hwInt32 type;
    struct {
        hwInt32 type;
        hwInt32 numClipRects;
        hwInt32 *clipRectList;
    } expose;
    struct {
        hwInt32 type;
        hwInt32 posx, posy;
        hwInt32 width, height;
    } config;
    struct {
        hwInt32 type;
        hwInt32 hasFocus;
    } focus;
    struct {
        hwInt32 type;
        hwInt32 key;
        hwInt32 mods;
    } keyboard;
    struct {
        hwInt32 type;
        hwInt32 x, y, buttonState, kbdMods;
    } pointer;
    struct {
        hwInt32 type;
        hwInt32 x, y, button, kbdMods;
    } button;
    struct {
        hwInt32 type;
        hwInt32 joystickNum;
        hwInt32 value;
    } joystickButton;
    struct {
        hwInt32 type;
        hwInt32 joystickNum;
        hwInt32 axis;
        hwFloat value;
    } joystickAxis;
} hwWinEvent;

/* Callback for joystick objects */
typedef void (*hwJoystickCB)( hwObject, hwWinEvent * );

/*** EOF hw_types.h ***/
