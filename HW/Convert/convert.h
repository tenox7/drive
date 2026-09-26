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

/* Header files needed by most users of this header */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HW_NULL
#include "hw.h"

/* Handy macro to convert degrees to radians */
#ifndef M_PI
#       define  M_PI    3.141592653589
#endif

#define deg     * (M_PI/180.0)
#define rad     * (180.0/M_PI)

/* Graphic types */
typedef enum {
    G_TEXTURE,          /* Data is name of texture */
    G_MESH,             /* Graphic is a quad mesh */
    G_POLYGON,          /* Graphic is a polygon */
    G_POLYLINE,         /* Graphic is a polyline */
    G_POLYMARKER,       /* 'm' is marker type */
    G_SPHERE,           /* Data is [r,lat0,lat1,lon0,lon1] */
    G_CONE,             /* Data is [r0,r1,length,lon0,lon1] */
    G_RING,             /* Data is [inner,outer,lon0,lon1] */
    G_TORUS,            /* Data is [inner,outer,lat0,lat1,lon0,lon1] */
    G_BOX,              /* Data is [x0,y0,z0,x1,y1,z1] */
    G_SURFREV,          /* Data is X,Y pairs, (GraphN of them) */
    G_GROUP,            /* Data is [id0,id1,id2,...,idn-1] */
    G_STRING,           /* Data is "text string" */
    G_TRIMESH           /* Data is n verts followed by m*3 indexes */
} GraphicType;

/* Marker types */
#define MRK_DOT         0x01    /* . */
#define MRK_PLUS        0x02    /* + */
#define MRK_STAR        0x03    /* * */
#define MRK_CIRCLE      0x04    /* o */
#define MRK_X           0x05    /* x */

#define MRK_TYPE        0x0F    /* Mask to find marker type */
#define MRK_FLAGS       0xF0    /* Mask to find marker flags */
#define MRK_RGB         0x10    /* Flag for RGB per marker */

/* Graphic flags */
#define GF_NORMALS      0x0001  /* Mesh/polygon/strip has normals per vertex */
#define GF_RGB          0x0002  /* Mesh/polygon/strip has RGB per vertex */
#define GF_UV           0x0004  /* Texture U/V per vertex */
#define GF_BACKFACE     0x0008  /* This is backfacing... */
#define GF_TWOSIDED     0x0010  /* Don't do backface culling */
#define GF_UNCOLORED    0x0020  /* Use base color instead of Graphic color */
#define GF_TEXTURED     0x0040  /* 'color' is actually texture index */
#define GF_BRIGHT       0x0080  /* Use object color only (usually for lights) */
#define GF_INVISIBLE    0x0100  /* This is invisible */
#define GF_INTANGIBLE   0x0200  /* Don't collide with this... */
#define GF_WIREFRAME    0x0400  /* Draw this graphic using wireframe */
#define GF_TEXGENSPHERE 0x0800  /* Generate U,V as if spherical about x,y,z */
#define GF_TEXGENPLANE  0x1000  /* Generate U,V as if planar */
#define GF_TEXGENCYL    0x2000  /* Generate U,V as if planar */

/* Object properties, for parsing */
typedef enum {
    PR_COLOR,           /* Color of object */
    PR_TRANSPARENCY,    /* Transparency of object */
    PR_SCALE,           /* Scaling factors */
    PR_ROTPOS,          /* Where it's rotation is now */
    PR_POSITION,        /* 1 to 3 floats */
    PR_GRAPHTYPE,       /* Type of the graphic */
    PR_WHICHGRAPH,      /* 0 = hi, 1 = mid, 2 = lo */
    PR_GRAPHN,          /* Number of points, or number of rows */
    PR_GRAPHM,          /* Number of cols (or other...) */
    PR_NORMALS,         /* Normal-per-vertex? */
    PR_RGB,             /* RGB-per-vertex? */
    PR_UV,              /* UV-per-vertex? */
    PR_SPECULAR,        /* Is it specular? */
    PR_CONVEX,          /* Obj is convex? */
    PR_BACKFACE,        /* Obj is backface? */
    PR_TWOSIDED,        /* Obj is two sided? */
    PR_UNCOLORED,       /* Obj is uncolored? */
    PR_TEXTURED,        /* Obj has a texture? */
    PR_BRIGHT,          /* Obj is a light? */
    PR_DATA,            /* Points, etc... */
    PR_TEXTURE,         /* Object pointing to texture map */
    PR_VISIBLE,         /* Is it visible? */
    PR_COLLIDES,        /* Does it collide? */
    PR_ANIMATE,         /* Animation bits */
    PR_WIREFRAME,       /* Wireframe graphic */
    PR_TEXGENSPHERE,    /* Auto-generate U,V? */
    PR_TEXGENPLANE,     /* Auto-generate U,V? */
    PR_TEXGENCYL,       /* Auto-generate U,V? */
    PR_SPEC_COLOR,      /* Specular color */
    PR_SHININESS,       /* How shiny is it? */
    PR_VOLUME           /* CSG Volumetric description of the object */
} ObjMethod;

/* Type structure */
#define BASE_MASK       0x0F000000
#define BT_VOID         0x00000000
#define BT_INT          0x01000000
#define BT_FLOAT        0x02000000
#define BT_DOUBLE       0x03000000
#define BT_STRING       0x04000000
#define BT_OBJECT       0x05000000

#define COMPOUND_MASK   0xF0000000
#define CT_SCALAR       0x00000000
#define CT_ARRAY        0x10000000

#define COUNT_MASK      0x00FFFFFF

/* Structure of an object */
struct Object {
    char
        *Name;          /* Name of the object */
    char *Volume;       /* String containing the CSG volume description */
    struct Graphic
        *Graphic,       /* Graphic of the object */
        *GraphTail;     /* Tail of this list */
    struct Object
        *Next;          /* Next object in linked list */
    hwObject
        converted;      /* What was it converted to? */
};

struct Graphic {
    GraphicType
        Type,           /* One of G_*, above */
        CookedType;     /* Cooked version of the type */
    int
        n, m,           /* Size (n by m for mesh, just n for polygon) */
        segment,        /* display list segment */
        doneBeenCooked, /* Has the data been cooked yet? */
        Flags,          /* Some of GF_*, above, modified by cooking */
        SaveFlags,      /* Original flags as parsed in the file */
        AnimBits,       /* Animation state bits */
        TextureID,      /* ID of texture map to use */
        Selected;       /* Has this Graphic been picked? */
    float
        Scale[3],       /* Scaling factors */
        Rotate[3],      /* Rotation factors */
        Pos[3],         /* Positioning factors */
        Color[3],       /* Color, if not GF_RGB or GF_UNCOLORED */
        Transparency,   /* Transparency, from 0 to 1 */
        SpecColor[3],   /* Specular color */
        Shininess,      /* Shininess, from 0 to 1 */
        *Data,          /* Data for mesh or polygon */
        *Cooked;        /* Cooked version of data */
    struct Object
        *Texture;       /* Texture map graphic... */
    struct Graphic
        *Next;          /* Next graphic in a list of graphics */
};

/* From parse.c */
extern int
    StrEQ( const char *, const char * );
extern int
    ParseFile( char * );
extern struct Object
    *FindObject( char * );

extern struct Object
    *Objects,           /* List of all objects */
    *ObjTail,           /* Tail of that list */
    *Textures,          /* List of Textures */
    *TextureTail;       /* Tail of that list */
