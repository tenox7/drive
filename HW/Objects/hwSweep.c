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
#include "hw.h"
#include "hw_internal.h"
#include "hw_vecmath.h"

#ifndef M_PI
#       define  M_PI    3.141592653589
#endif

#if defined(WIN32) && !defined(CYGWIN) && !defined(MINGW)
#       define  ISNAN(x)        _isnan((double)x)
#else
#       define  ISNAN(x)        isnan(x)
#endif

/* Internal definitions of sweep-related flags */
#define HW_SHAPE        1
#define HW_PATH         2
#define HW_HAS_SIZES    3
#define HW_HAS_UP       4
#define HW_GRAPHN       5
#define HW_OPT_FLAGS    6
#define HW_NORMALS      7
#define HW_LIFETIME     8
#define HW_PATH_COUNT   9
#define HW_HAS_CAPS     10
#define HW_SURFACE      11
#define HW_RESOLUTION   12
#define HW_TANGENT      13
#define HW_DIRTY        14

/* These are for internal use only */
#define HW_BBOX         20
#define HW_POS_VALS     21

static hwObject create( hwObject );
static void addref( hwObject );
static void destroy( hwObject );
static void modify( hwObject, const char *, hwInt32 type, const void * );
static hwInt32 inquire( hwObject, const char *, void ** );
static void draw( hwObject );

static const char
    *propList[] = {
        /* Specific to sweep */
        hwStrShape,
        hwStrPath,
        hwStrHasSizes,
        hwStrHasUp,
        hwStrGraphN,
        hwStrOptFlags,
        hwStrHasNormals,
        hwStrHasTangent,
        hwStrHasCaps,
        hwStrSurface,
        hwStrResolution,
        hwStrLifetime,

        /* From the surface */
        STR_SURF

        /* From the orientation */
        STR_ORIENT

        /* End of list */
        0
    };
static struct _hwObjectStruct
    hwSweepStruct = {
        0,              /* Parent - NULL */
        "hwSweep",
        propList,
        create,
        addref,
        destroy,
        modify,
        inquire,
        draw
    };
hwObject
    hwSweep = &hwSweepStruct;

typedef struct {
    struct _hwObjectStruct
        hdr;                    /* Common stuff */
    hwInt32
        refCount,
        dirty,                  /* Is this sweep cooked? */
        graphN,                 /* Tesselation parameter */
        cookedN,
        hasSizes,               /* Do we have sizes in the path? */
        hasUpVectors,           /* Do we have up vectors in the path? */
        shapeCount,             /* Size of the shape array */
        pathCount,              /* Size of the path */
        pathWPV,                /* Width of the path */
        dl,                     /* Display list */
        optFlags,               /* Optimization level (0 default) */
        cookedFlags,            /* Cooked by texture pipeline */
        dataFlags,              /* Data format flags */
        numCooked,              /* Size of bezier segment arrays */
        hasCaps;                /* Should we cap this sweep? */
    hwOrientType
        orient;                 /* Orientation matrix */
    hwObject
        surfObjs[3];            /* The 3 surface objects */
    hwSurfaceType
        surfaces[3];            /* The 3 surface parameter structures */
    hwFloat
        lifetime[2],            /* Where to sweep, start & end */
        resolution,             /* Resolution mangifier */
        *shape,                 /* The shape to sweep */
        *path,                  /* The path along which to sweep */
        *upVecs,                /* The up vectors */
        *sizes,                 /* The sizes */
        *cookedPath,            /* Cooked into Bezier segments */
        *cookedUp,              /* Cooked into Bezier segments */
        *cookedSizes,           /* Cooked into Bezier segments */
        BBox[6],                /* Bounding box */
        *data,                  /* The cooked sweep data */
        *cap1_data,             /* data for polygon end cap front */
        *cap2_data,             /* data for polygon end cap back */
        *temp_data,             /* Temporary for cooked sweep data */
        *temp_cap1_data,        /* Temporary for polygon end cap front */
        *temp_cap2_data;        /* Temporary for polygon end cap back */
} Sweep;

/* The hash table for sweep strings */
static void *sweepTab = 0;

static hwObject create( hwObject proto )
{
    Sweep
        *result;
    void
        *t;

    LOG_ENTRY
    if( proto != hwSweep )      return 0;

    HW_HASH_SETUP(sweepTab, 16)
        t = sweepTab;
        HW_INSERT( hwStrShape,      HW_SHAPE,      t );
        HW_INSERT( hwStrPath,       HW_PATH,       t );
        HW_INSERT( hwStrPathCount,  HW_PATH_COUNT, t );
        HW_INSERT( hwStrHasSizes,   HW_HAS_SIZES,  t );
        HW_INSERT( hwStrHasUp,      HW_HAS_UP,     t );
        HW_INSERT( hwStrGraphN,     HW_GRAPHN,     t );
        HW_INSERT( hwStrOptFlags,   HW_OPT_FLAGS,  t );
        HW_INSERT( hwStrHasNormals, HW_NORMALS,    t );
        HW_INSERT( hwStrHasCaps,    HW_HAS_CAPS,   t );
        HW_INSERT( hwStrBounds,     HW_BBOX,       t );
        HW_INSERT( hwStrPosParams,  HW_POS_VALS,   t );
        HW_INSERT( hwStrLifetime,   HW_LIFETIME,   t );
        HW_INSERT( hwStrSurface,    HW_SURFACE,    t );
        HW_INSERT( hwStrResolution, HW_RESOLUTION, t );
        HW_INSERT( hwStrHasTangent, HW_TANGENT,    t );
        HW_INSERT( hwStrDirty,      HW_DIRTY,      t );
    HW_HASH_CLEANUP

    result = malloc( sizeof(Sweep) );
    if( !result ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return 0;
    }
    result->hdr = *proto;
    result->hdr.parent = hwSweep;
    result->refCount = 1;

    result->dirty = 1;
    result->graphN = 16;
    result->hasSizes = 0;
    result->hasCaps = 0;
    result->hasUpVectors = 0;
    result->shapeCount = 0;
    result->pathCount = 0;
    result->pathWPV = 3;
    result->dl = -1;
    result->optFlags = hwDefaultOptFlags;
    result->cookedFlags = 0;
    result->dataFlags = HW_DATA_NORMALS;
    result->resolution = 1.f;
    result->lifetime[0] = 0.f;
    result->lifetime[1] = 0.f;
    result->shape = 0;
    result->path = 0;
    result->upVecs = 0;
    result->sizes = 0;
    result->data = 0;
    result->cap1_data = 0;
    result->cap2_data = 0;
    result->cookedPath = 0;
    result->cookedUp = 0;
    result->cookedSizes = 0;
    result->numCooked = 0;

    result->surfObjs[0] = result->surfObjs[1] = result->surfObjs[2] = 0;
    hwDefaultSurf( &result->surfaces[0] );
    hwDefaultSurf( &result->surfaces[1] );
    hwDefaultSurf( &result->surfaces[2] );
    hwDefaultOrient( &result->orient );

    result->hdr.name = 0;
    LOG_EXIT
    return (hwObject)result;
}

static void addref( hwObject obj )
{
    Sweep
        *sweep = (Sweep *)obj;

    LOG_ENTRY
    sweep->refCount++;
    LOG_EXIT
}

static void destroy( hwObject obj )
{
    int
        i;
    Sweep
        *sweep = (Sweep *)obj;

    LOG_ENTRY
    if( --sweep->refCount > 0 ) return;

    for( i = 0; i < 3; i++ ) {
        if( sweep->surfObjs[i] )
            sweep->surfObjs[i]->destroy( sweep->surfObjs[i] );
    }

    if( sweep->dl >= 0 ) {
        __hwIntDestroyList( sweep->dl );
    }
    if( sweep->data )   free( sweep->data );
    if( sweep->cap1_data )      free( sweep->cap1_data );
    if( sweep->cap2_data )      free( sweep->cap2_data );
    free( sweep );
    LOG_EXIT
}

static void modify
(
    hwObject obj, const char *prop,
    hwInt32 type, const void *val
)
{
    Sweep
        *sweep = (Sweep *)obj;
    int
        dirty = 1, i, n, WPV;
    hwObject 
        tmp;
    const hwFloat
        *fval;

    LOG_ENTRY
    switch( hwLookup( prop, sweepTab ) ) {
    case HW_RESOLUTION :
        if( type != HW_TYPE_1F )        goto BadType;
        sweep->resolution = *(hwFloat *)val;
        break;
    case HW_GRAPHN :
        if( type != HW_TYPE_1I )        goto BadType;
        sweep->graphN = *(hwInt32 *)val;
        break;
    case HW_OPT_FLAGS :
        if( type != HW_TYPE_1I )        goto BadType;
        sweep->optFlags = *(hwInt32 *)val;
        break;
    case HW_NORMALS :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   sweep->dataFlags |= HW_DATA_NORMALS;
        else                    sweep->dataFlags &= ~HW_DATA_NORMALS;
        break;
    case HW_TANGENT :
        if( type != HW_TYPE_1B )        goto BadType;
        if( *(hwInt32 *)val )   sweep->dataFlags |= HW_DATA_TANGENT;
        else                    sweep->dataFlags &= ~HW_DATA_TANGENT;
        break;
    case HW_SURFACE:
        if(type != HW_MAKE_TYPE(HW_TYPE_OBJECT,3 ))  goto BadType;

        for( i = 0; i < 3; i++ ) {
            if( sweep->surfObjs[i] ) {
                sweep->surfObjs[i]->destroy( sweep->surfObjs[i] );
            }
        }
        (void)memcpy( sweep->surfObjs, val, 3*sizeof(hwObject) );
        for( i = 0; i < 3; i++ ) {
            tmp = sweep->surfObjs[i];
            tmp->addref( tmp );
        }
        break;
    case HW_SHAPE :
        if( HW_GET_BASE( type ) != HW_TYPE_FLOAT )      goto BadType;
        n = HW_GET_COUNT( type );
        if( n & 1 )                                     goto BadType;
        sweep->shape = realloc( sweep->shape, n * sizeof(hwFloat) );
        if( !sweep->shape )                             goto NoMem;
        (void)memcpy( sweep->shape, val, n * sizeof(hwFloat) );
        sweep->shapeCount = n / 2;
        break;
    case HW_LIFETIME :
        if( type != HW_TYPE_2F )        goto BadType;
        sweep->lifetime[0] = ((hwFloat *)val)[0];
        sweep->lifetime[1] = ((hwFloat *)val)[1];
        break;
    case HW_PATH :
        if( HW_GET_BASE( type ) != HW_TYPE_FLOAT ) goto BadType;
        n = HW_GET_COUNT( type );
        WPV = sweep->pathWPV;
        if( n % WPV ) goto BadType;
        n = n / WPV;
        sweep->path = realloc( sweep->path, 3*n*sizeof(hwFloat) );
        if( !sweep->path )                              goto NoMem;
        if( sweep->hasSizes ) {
            sweep->sizes = realloc( sweep->sizes, 3*n*sizeof(hwFloat) );
            if( !sweep->sizes ) goto NoMem;
        }
        if( sweep->hasUpVectors ) {
            sweep->upVecs = realloc( sweep->upVecs, 3*n*sizeof(hwFloat) );
            if( !sweep->upVecs ) goto NoMem;
        }
        fval = val;
        for( i = 0; i < n; i++ ) {
            (void)memcpy( sweep->path+3*i, fval, 3*sizeof(hwFloat) );
            fval += WPV;
        }
        if( sweep->hasSizes ) {
            fval = 3 + (hwFloat *)val;
            for( i = 0; i < n; i++ ) {
                (void)memcpy( sweep->sizes+3*i, fval, 2*sizeof(hwFloat) );
                sweep->sizes[3*i+2] = 0.0f;
                fval += WPV;
            }
        }
        if( sweep->hasUpVectors ) {
            if( sweep->hasSizes )       fval = 5 + (hwFloat *)val;
            else                        fval = 3 + (hwFloat *)val;
            for( i = 0; i < n; i++ ) {
                (void)memcpy( sweep->upVecs+3*i, fval, 3*sizeof(hwFloat) );
                VADD( (sweep->upVecs+3*i), (sweep->upVecs+3*i),
                                        (sweep->path+3*i) );
                fval += WPV;
            }
        }
        sweep->lifetime[1] = (hwFloat)(n-1);
        sweep->pathCount = n;
        break;
    case HW_HAS_SIZES :
        if( type != HW_TYPE_1B )        goto BadType;
        sweep->hasSizes = *(hwInt32 *)val;
        break;
    case HW_HAS_CAPS :
        if( type != HW_TYPE_1B )        goto BadType;
        sweep->hasCaps = *(hwInt32 *)val;
        break;
    case HW_HAS_UP :
        if( type != HW_TYPE_1B )        goto BadType;
        sweep->hasUpVectors = *(hwInt32 *)val;
        break;
    case HW_DIRTY :
        for( i = 0; i < 3; i++ ) {
            if( sweep->surfObjs[i] ) {
                HW_MODIFY_1I(sweep->surfObjs[i], hwStrDirty, 1);
            }
        }
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            for(i=0; i < 3; i++ ) {
                if( !sweep->surfObjs[i] ) {
                    sweep->surfObjs[i] = hwSurface->create( hwSurface );
                    if( !sweep->surfObjs[i] )   return;
                }
                tmp = sweep->surfObjs[i];
                tmp->modify(tmp, prop, type, val);
            }

            /* dirty = __hwIntSurfModify( &sweep->surf, n, type, val ); */
        }
        if( n < 0 ) {
            n = hwLookup( prop, __hwIntOrientTab );
            if( n >= 0 ) {
                __hwIntOrientModify( &sweep->orient, n, type, val );
            }
        }
        if( n < 0 ) {
            __hwIntSetError( HW_ERROR_BAD_PROP );
        }
        break;
    }

    /* Reset path WPV */
    sweep->pathWPV = 3;
    if( sweep->hasSizes )       sweep->pathWPV += 2;
    if( sweep->hasUpVectors )   sweep->pathWPV += 3;

    if( dirty ) sweep->dirty = 1;
    LOG_EXIT
    return;

BadType :
    __hwIntSetError( HW_ERROR_BAD_TYPE );
    return;
NoMem :
    __hwIntSetError( HW_ERROR_NO_MEMORY );
    return;
}


static void GetRot( float XYZ[3], float S, float C, float Mat[3][3] )
{
    float T,
        X, Y, Z;

    T = 1.0 - C;
    X = XYZ[0]; Y = XYZ[1]; Z = XYZ[2];
    Mat[0][0] = T*X*X+C;   Mat[1][0] = T*X*Y+S*Z; Mat[2][0] = T*X*Z-S*Y;
    Mat[0][1] = T*X*Y-S*Z; Mat[1][1] = T*Y*Y+C;   Mat[2][1] = T*Y*Z+S*X;
    Mat[0][2] = T*X*Z+S*Y; Mat[1][2] = T*Y*Z-S*X; Mat[2][2] = T*Z*Z+C;
}

static void MatMult3x3( float Res[3][3], float A[3][3], float B[3][3] )
{
    int
        i, j, k;
    float
        t;

    for( i = 0; i < 3; i++ ) {
        for( j = 0; j < 3; j++ ) {
            t = 0;
            for( k = 0; k < 3; k++ ) {
                t += A[i][k]*B[k][j];
            }
            Res[i][j] = t;
        }
    }
}

#define EPSILON 0.001

static float VecCrossRes( float R[3], const float A[3], const float B[3] )
{
    float
        d, x, y, z;

    x = A[1]*B[2] - B[1]*A[2];
    y = A[2]*B[0] - B[2]*A[0];
    z = A[0]*B[1] - B[0]*A[1];
    d = sqrt( x*x + y*y + z*z );
    if( d < EPSILON ) {
        R[0] = A[0]; R[1] = A[1]; R[2] = A[2];
        return 0.0;
    }
    else {
        R[0] = x/d; R[1] = y/d; R[2] = z/d;
        return d;
    }
}

static int UpdateMatrix
(
    float *PUp, float *PDir, float *PRight,
    float PMat[3][3]
)
{
    static const float
        Front[3] = {0,1,0};
    float
        Right[3], Basis[3],
        Mat[3][3],
        Mat1[3][3],
        Len, Ang;

    /* First, get the rotation of the direction vector */
    Len = VecCrossRes( Basis, PDir, Front );
    if( Len == 0.0 ) {
        Len = VDOT( PDir, Front );
        if( Len < 0.0 ) {
            Ang = M_PI;
        }
        else {
            Ang = 0.0;
        }
        VCOPY( Basis, PUp );
    }
    else {
        Ang = PDir[1];
        if( ISNAN( Ang ) )              Ang = 0.0;
        else if( Ang > 1.0 )    Ang = 1.0;
        else if( Ang < -1.0 )   Ang = -1.0;
        Ang = acos( Ang );
    }
    GetRot( Basis, sin(Ang), cos(Ang), Mat );

    /* Now, get the rotation of the right vector */
    Right[0] = 1; Right[1] = 0; Right[2] = 0;
    VXFORM3( Right, Right, Mat );
    Ang = VDOT( PRight, Right );
    if( ISNAN( Ang ) )          Ang = 0.0;
    else if( Ang > 1.0 )        Ang = 1.0;
    else if( Ang < -1.0 )       Ang = -1.0;
    if( VecCrossRes( Basis, PRight, Right ) == 0.0 ) {
        if( Ang < 0.0 ) Ang = M_PI;
        else            Ang = 0.0;
        VCOPY( Basis, PUp );
    }
    else {
        Ang = acos( Ang );
    }
    GetRot( Basis, sin(Ang), cos(Ang), Mat1 );
    MatMult3x3( PMat, Mat, Mat1 );
    return 0;
}

static void TexturePolygon( int NumPts, int WPV, float *Pgon, int numTex )
{
    float
        *ptr,
        *src,
        *dst,
        dist,
        dx, dy, dz,
        ds, dt,
        x1, y1, z1,
        x2, y2, z2,
        minx, miny, minz,
        maxx, maxy, maxz;
    int
        s_offset,
        t_offset,
        s_indx, t_indx,
        s_min, s_max,
        t_min, t_max,
        i, j;

    /* For this task, we want to map the entire texture (0.0 to 1.0) in s and t
    ** to the polygon face .  The way the sweep is defined, we have no idea
    ** what kind of orientation the polygon might be in, so we need to determine
    ** which two of X,Y and Z we want to use to map s and t onto.
    */

    minx = miny = minz = 1000000000.0; /* This should be be enough */
    maxx = maxy = maxz =-1000000000.0;

    for(i=0; i < NumPts; i ++ ) {
        ptr = Pgon + i*WPV;
        if( ptr[0] < minx ) minx = ptr[0];
        else if (ptr[0] > maxx) maxx = ptr[0];

        if( ptr[1] < miny ) miny = ptr[1];
        else if (ptr[1] > maxy) maxy = ptr[1];

        if( ptr[2] < minz ) minz = ptr[2];
        else if (ptr[2] > maxz) maxz = ptr[2];
    }

    dx = maxx - minx;
    dy = maxy - miny;
    dz = maxz - minz;

    /* Find the smallest and toss it */
    if( dx > dy) {
        if( dy > dz) {
           ds = dx;    s_indx = 0; s_min = minx, s_max = maxx;
           dt = dy;    t_indx = 1; t_min = miny, t_max = maxy;
        }
        else {
            ds = dx;    s_indx = 0; s_min = minx, s_max = maxx;
            dt = dz;    t_indx = 2; t_min = minz, t_max = maxz;
        }
    }
    else /* dy > dx */ {
        if( dx > dz ) {
            ds = dx;    s_indx = 0; s_min = minx, s_max = maxx;
            dt = dy;    t_indx = 1; t_min = miny, t_max = maxy;
        }
        else {
            ds = dy;    s_indx = 1; s_min = minz, s_max = maxz;
            dt = dz;    t_indx = 2; t_min = miny, t_max = maxy;
        }
    }

    /* Figure out where to start putting the texture coordinates */
    s_offset = WPV - 2*numTex;
    t_offset = WPV - 2*numTex + 1;

    /* Now, go through the array, comparing the value of the X, Y, or Z
    ** coordinates, seeing how far they are from the min, assign s and t
    ** based on the percentage distance
    */

    for(i=0; i < NumPts; i++ ) {
        src = Pgon + i*WPV;
        for( j = 0; j < numTex; j++ ) {
            src[2*j+s_offset] = (src[s_indx] - s_min) / ds;
            src[2*j+t_offset] = (src[t_indx] - t_min) / dt;
        }
    }
}


static void TextureMesh( int NumRows, int NumCols, int WPV, float *Mesh,
        int numTex )
{
    float
        *ptr,
        *src,
        *dst,
        dist,
        i,j,k,
        x1, y1, z1,
        x2, y2, z2,
        *sArray,
        *tArray,
        s_repeat,
        t_repeat,
        CurWidth = 0.0,
        CurLength = 0.0,
        TotWidth = 0.0,
        TotLength = 0.0;
    int
        row, 
        col,
        tex;

    /* First, traverse the left edge of the mesh, looking for the total dist
    ** of the perimeter.  Use this length to determine how far to strech the
    ** texture coord before replicating it.  The points of the 'shape' define
    ** the number of rows in the mesh.  The points along the 'path' define the
    ** number of columns, so we can just traverse the first column to get this
    ** information.
    */

    ptr = Mesh;
    for(col=0; col < NumCols-1; col++ ) {
        x1 = *ptr; y1 = *(ptr + 1); z1 = *(ptr +2); 
        ptr +=  WPV;   /* Skip to next vertex*/
        x2 = *ptr; y2 = *(ptr + 1); z2 = *(ptr +2); 

        dist = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
        TotLength += dist;
    }

    /* Now, allocate an array to store s values.  There will be NumRows of them.
    ** They will remain constant across the shape of the curve.
    */
    sArray = malloc( NumRows * sizeof(float));
    ptr = Mesh;
    sArray[0] = 0.0;

    /* Figure out how far across the mesh is */
    for(row=0; row < NumRows-1; row++ ) {
        x1 = *ptr; y1 = *(ptr + 1); z1 = *(ptr +2); 
        ptr +=  NumCols * WPV;   /* Skip to next row */
        x2 = *ptr; y2 = *(ptr + 1); z2 = *(ptr +2); 

        dist = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
        TotWidth += dist;
    }

    /*Hack!! s_repeat, t_repeat need to be defined */
    s_repeat = 1;
    t_repeat = TotLength / TotWidth;

    for(row=1; row < NumRows; row++ ) {
        ptr = Mesh + (row * NumCols * WPV);
        x1 = *ptr; y1 = *(ptr + 1); z1 = *(ptr +2); 
        ptr -=  NumCols * WPV;   /* Skip to next row */
        x2 = *ptr; y2 = *(ptr + 1); z2 = *(ptr +2); 

        dist = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
        CurWidth += dist;
        sArray[row] = CurWidth / TotWidth * s_repeat;
        /*printf("sArray[%d] = %f\n",row,sArray[row]); */
    }

    /* Allocate an array for t values and fill it out */
    tArray = malloc(NumCols * sizeof(float));
    tArray[0] = 0.0;
    for(col=1; col < NumCols; col++ ) {
        ptr = Mesh+(col * WPV);
        x1 = *ptr; y1 = *(ptr + 1); z1 = *(ptr +2); 
        ptr -=  WPV;   /* Skip to previous vertex*/
        x2 = *ptr; y2 = *(ptr + 1); z2 = *(ptr +2); 

        dist = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
        CurLength += dist;
        tArray[col] = CurLength / TotLength * t_repeat;
        /*printf("tArray[%d] = %f\n",col,tArray[col]); */
    }

    dst = Mesh+(WPV-2*numTex);  /* Point to the first texture coord spot */

    for(row=0; row < NumRows; row++ ) {
        for(col=0; col < NumCols; col++ ) {
            for( tex = 0; tex < numTex; tex++ ) {
                dst[2*tex+0] = sArray[row];
                dst[2*tex+1] = tArray[col];
            }
            dst += WPV;
        }
    }
    free(sArray);
    free(tArray);
}

static void cook( hwDisplay disp, Sweep *sweep )
{
    hwInt32
        dataFlags,
        i, j, jj, n, m, vn,
        t, maxPath;
    hwFloat
        fj,
        dx, dy, dz,
        px, py, pz,
        tx, ty, tz,
        mat[3][3],
        dirVec[3], rightVec[3], upVec[3],
        txyz[3],
        fw, fh, foo;
    hwInt32
        capSize1,
        capSize2,
        dataSize;
    hwFloat
        *cap1_data,
        *cap2_data,
        *data;
    void
        *v;
    hwObject
        obj;
    hwFloat
        *tmp;
    struct __hwDisplayInternal
        *intDisp;

    LOG_ENTRY

    if( !sweep->path )  return;

    /* Extract scratch pointers */
    intDisp = (struct __hwDisplayInternal *)disp;
    dataSize = intDisp->scratchDataSize;
    capSize1 = intDisp->scratchDataSize2;
    capSize2 = intDisp->scratchDataSize3;
    data = intDisp->scratchData;
    cap1_data = intDisp->scratchData2;
    cap2_data = intDisp->scratchData3;

    if( sweep->dl >= 0 ) {
        __hwIntDestroyList( sweep->dl );
    }
    sweep->dl = -1;
    if( sweep->data )   free( sweep->data );
    if( sweep->cap1_data )      free( sweep->cap1_data );
    if( sweep->cap2_data )      free( sweep->cap2_data );
    sweep->data = 0;
    sweep->cap1_data = 0;
    sweep->cap2_data = 0;

    /* Create + smooth parameters */
    n = sweep->pathCount;
    sweep->cookedPath = realloc( sweep->cookedPath, 9*n*sizeof(hwFloat) );
    if( !sweep->cookedPath ) goto ERROR;
    if( sweep->upVecs ) {
        sweep->cookedUp = realloc( sweep->cookedUp, 9*n*sizeof(hwFloat) );
        if( !sweep->cookedUp ) goto ERROR;
    }
    if( sweep->sizes ) {
        sweep->cookedSizes = realloc( sweep->cookedSizes, 9*n*sizeof(hwFloat) );
        if( !sweep->cookedSizes ) goto ERROR;
    }
    tmp = sweep->path;
    if(         (tmp[3*n-3] == tmp[0])
        &&      (tmp[3*n-2] == tmp[1])
        &&      (tmp[3*n-1] == tmp[2]) )
    {
        maxPath = n-1;
        sweep->numCooked = __hwIntCalcClosed( sweep->path,
                                n-1, sweep->cookedPath );
        if( sweep->upVecs ) {
            (void)__hwIntCalcClosed( sweep->upVecs,
                                n-1, sweep->cookedUp );
        }
        if( sweep->sizes ) {
            (void)__hwIntCalcClosed( sweep->sizes,
                                n-1, sweep->cookedSizes );
        }
    }
    else {
        maxPath = n-1;
        sweep->numCooked = __hwIntCalcOpen( sweep->path,
                                n, sweep->cookedPath );
        if( sweep->upVecs ) {
            (void)__hwIntCalcOpen( sweep->upVecs,
                                n, sweep->cookedUp );
        }
        if( sweep->sizes ) {
            (void)__hwIntCalcOpen( sweep->sizes,
                                n, sweep->cookedSizes );
        }
    }

    /* Make sure our surfaces are up to date before we try to use them */
    for( i = 0; i < 3; i++ ) {
        obj = sweep->surfObjs[i];
        if(!obj) {
            obj = sweep->surfObjs[i] = hwSurface->create( hwSurface );
        }
        if( obj->inquire( obj, hwStrSurface, &v ) ) {
            sweep->surfaces[i] = *(hwSurfaceType *)v;
            n = sweep->surfaces[i].numTextures;
            for( j = 0; j < n; j++ ) {
                sweep->surfaces[i].textures[j]->addref(
                                        sweep->surfaces[i].textures[j] );
            }
        }
    }

    /* Allocate data... */
    dataFlags = sweep->dataFlags;

    dataFlags |= hwTextureFlags( &sweep->surfaces[0] );

    n = sweep->shapeCount;      m = sweep->graphN;

    m = (m * sweep->resolution + 0.5f); if( m < 4 ) m = 4;
    if( sweep->graphN & 1 ) m |= 1;

    sweep->cookedN = m;

    vn = hwCalcWPV( dataFlags );

    if( n*m*vn > dataSize ) {
        dataSize = n*m*vn;
        data = realloc( data, dataSize*sizeof(hwFloat) );
    }
    if( !data ) goto ERROR;

    if( sweep->hasCaps) {
        if( n*vn > capSize1 ) {
            capSize1 = n*vn;
            cap1_data = realloc( cap1_data, capSize1 * sizeof(hwFloat) );
        }
        if( !cap1_data ) goto ERROR;
        if( n*vn > capSize2 ) {
            capSize2 = n*vn;
            cap2_data = realloc( cap2_data, capSize2 * sizeof(hwFloat) );
        }
        if( !cap2_data ) goto ERROR;
    }

    /* Tesselate sweep */
    for( j = 0; j < m; j++ ) {
        fj = sweep->lifetime[0]
                + (j / (hwFloat)(m-1))*(sweep->lifetime[1]-sweep->lifetime[0]);

        jj = (int)fj;
        fj = fj - (float)jj;

        if( jj >= maxPath ) {
            jj = maxPath - 1;
            fj = 1.0f;
        }

        /* Find the position and the orientation at this point */
        __hwIntBezier( sweep->cookedPath+9*jj, fj, &px, &py, &pz,
                            &dirVec[0], &dirVec[1], &dirVec[2] );

        /* Smooth out the radius at this point */
        if( sweep->sizes ) {
            __hwIntBezier( sweep->cookedSizes+9*jj, fj, &fw, &fh, &foo,
                            (hwFloat *)0, (hwFloat *)0, (hwFloat *)0 );
        }
        else {
            fw = fh = 1.0f;
        }

        /* Smooth out the pitch at this point */
        if( sweep->upVecs ) {
            __hwIntBezier( sweep->cookedUp+9*jj, fj,
                        &upVec[0], &upVec[1], &upVec[2],
                            (hwFloat *)0, (hwFloat *)0, (hwFloat *)0 );
            upVec[0] -= px; upVec[1] -= py; upVec[2] -= pz;
            VNORM( upVec, upVec );
        }
        else {
            upVec[0] = 0.f; upVec[1] = 0.f; upVec[2] = 1.f;
        }

        VCROSS( rightVec, dirVec, upVec );
        VNORM( rightVec, rightVec );

        VCROSS( upVec, dirVec, rightVec );
        VNORM( upVec, upVec );

        (void)UpdateMatrix( upVec, dirVec, rightVec, mat );

        for( i = 0; i < n; i++ ) {
            /* This is the entry in the mesh we're doing */
            t = vn*(i*m + j);

            /* Rotate the vector thusly */
            txyz[0] = sweep->shape[2*i  ] * fw;
            txyz[1] = 0.0;
            txyz[2] = sweep->shape[2*i+1] * fh;

            VXFORM3( txyz, txyz, mat );

            data[t  ] = px + txyz[0];
            data[t+1] = py + txyz[1];
            data[t+2] = pz + txyz[2];
        }
    }

    /* If we have caps, we need to take the endpoints of the mesh and
    ** group them all together in a polygon (one for each of the two
    ** ends of the mesh.  The shape of the sweep defines the number of
    ** rows in the mesh, so just pick off the first (or last) point of
    ** each row for the front (or back) cap.  Easy. 
    ** n = number of points in the shape (row in mesh)
    ** m = number of columns in each row 
    */
    if( sweep->hasCaps) {
        for( i = 0; i < n; i++ ) {
            t = vn*(i*m);
            cap1_data[i*vn + 0] = data[t];
            cap1_data[i*vn + 1] = data[t + 1];
            cap1_data[i*vn + 2] = data[t + 2];
            t = vn*((i+1)*m) - vn;
            cap2_data[i*vn + 0] = data[t];
            cap2_data[i*vn + 1] = data[t + 1];
            cap2_data[i*vn + 2] = data[t + 2];

            if( dataFlags &  HW_DATA_NORMALS) {
                /*  HACK!!! */
                cap1_data[i*vn + 3] = 0.0;
                cap1_data[i*vn + 4] = 0.0;
                cap1_data[i*vn + 5] = 1.0;

                cap2_data[i*vn + 3] = 0.0;
                cap2_data[i*vn + 4] = 0.0;
                cap2_data[i*vn + 5] = 1.0;
            }
            /* TBD: 3D texture */
            for( j = 0; j < HW_MAX_TEXTURES; j++ ) {
                if( dataFlags & (HW_DATA_ST<<(3*j)) ) {
                    cap1_data[i*vn + 6 + 2*j] = 0.0;
                    cap1_data[i*vn + 7 + 2*j] = 0.0;

                    cap2_data[i*vn + 6 + 2*j] = 0.0;
                    cap2_data[i*vn + 7 + 2*j] = 0.0;
                }
            }
        }
    }

    if( dataFlags & HW_DATA_NORMALS ) {
        __hwIntSmoothMesh( data, n, m, dataFlags );
    }

    if( dataFlags & HW_DATA_TEXCOORD ) {
        TextureMesh( n, m, vn, data, sweep->surfaces[0].numTextures );
        if( sweep->hasCaps) {
            TexturePolygon( n, vn, cap1_data, sweep->surfaces[0].numTextures );
            TexturePolygon( n, vn, cap2_data, sweep->surfaces[0].numTextures );
        }
    }

    /* Transform data */
    __hwIntTransformData( &sweep->orient, data, n*m, dataFlags );
    if( sweep->hasCaps) {
        __hwIntTransformData( &sweep->orient, cap1_data, n, dataFlags );
        __hwIntTransformData( &sweep->orient, cap2_data, n, dataFlags );
    }

    /* Update the bounding box */
    sweep->BBox[0] = sweep->BBox[1] = sweep->BBox[2] = HW_MAX_FLOAT;
    sweep->BBox[3] = sweep->BBox[4] = sweep->BBox[5] = -HW_MAX_FLOAT;
    __hwIntUpdateBounds( sweep->BBox, data, n*m, vn );

    /* Do special texture mapping tricks: UV->RGB if not texturing,
     * etc.
     */
    sweep->cookedFlags = __hwIntTexturePipeline(
                                    &sweep->surfaces[0],
                                    &data, &dataSize, &vn,
                                    n*m, dataFlags );

    if( sweep->hasCaps)   {
        __hwIntTexturePipeline( &sweep->surfaces[1],
                &cap1_data, &capSize1, &vn,
                n, dataFlags );
        __hwIntTexturePipeline( &sweep->surfaces[2],
                &cap2_data, &capSize2, &vn,
                n, dataFlags );
    }

    sweep->temp_data = data;
    sweep->temp_cap1_data = cap1_data;
    sweep->temp_cap2_data = cap2_data;

ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = dataSize;
    intDisp->scratchDataSize2 = capSize1;
    intDisp->scratchDataSize3 = capSize2;
    intDisp->scratchData = data;
    intDisp->scratchData2 = cap1_data;
    intDisp->scratchData3 = cap2_data;

    LOG_EXIT
}

static hwInt32 inquire( hwObject obj, const char *prop, void **val )
{
    Sweep
        *sweep = (Sweep *)obj;
    hwFloat
        *posVals,
        *fval,
        fj;
    int
        jj, n, vn;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    LOG_ENTRY

    intDisp = (struct __hwDisplayInternal *)__hwDisp;

    switch( hwLookup( prop, sweepTab ) ) {
    case HW_RESOLUTION :
        *val = &sweep->resolution;
        return HW_TYPE_1F;
    case HW_GRAPHN :
        *val = &sweep->graphN;
        return HW_TYPE_1I;
    case HW_OPT_FLAGS :
        *val = &sweep->optFlags;
        return HW_TYPE_1I;
    case HW_PATH_COUNT :
        *val = &sweep->pathCount;
        return HW_TYPE_1I;
    case HW_NORMALS :
        intDisp->scratchInt[0] = (sweep->dataFlags & HW_DATA_NORMALS) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_TANGENT :
        intDisp->scratchInt[0] = (sweep->dataFlags & HW_DATA_TANGENT) ? 1 : 0;
        *val = intDisp->scratchInt;
        return HW_TYPE_1B;
    case HW_BBOX :
        if( sweep->dirty )      cook( __hwDisp, sweep );
        *val = sweep->BBox;
        return HW_MAKE_TYPE(HW_TYPE_FLOAT,6);
    case HW_POS_VALS :
        /* How you would use this:
         *
         * hwFloat pos, *ptr;
         *
         * pos = 3.7; // or whatever
         * ptr = &pos;
         * sweep->inquire( sweep, hwStrPosVals, &ptr )
         *
         * Now, ptr[0-2] has the position,
         *      ptr[3-5] has the direction,
         *      ptr[6-8] has the up vector
         */
        if( sweep->dirty )      cook( __hwDisp, sweep );
        fval = *(hwFloat **)val;
        posVals = intDisp->scratchFloat;
        if( fval ) {
            fj = *fval; jj = (int)fj; fj = fj - jj;

            /* Find the position and the orientation at this point */
            __hwIntBezier( sweep->cookedPath+9*jj, fj,
                                &posVals[0], &posVals[1], &posVals[2],
                                &posVals[3], &posVals[4], &posVals[5] );

            if( sweep->hasUpVectors ) {
                __hwIntBezier( sweep->cookedUp+9*jj, fj,
                                &posVals[6], &posVals[7], &posVals[8],
                                (hwFloat *)0, (hwFloat *)0, (hwFloat *)0 );
                VSUB( (posVals+6), (posVals+6), posVals );
                VNORM( (posVals+6), (posVals+6) );
            }
            else {
                posVals[6] = 0.0;
                posVals[7] = 0.0;
                posVals[8] = 1.0;
            }
            *val = posVals;
            return HW_MAKE_TYPE(HW_TYPE_FLOAT,9);
        }
        break;
    default :
        n = hwLookup( prop, __hwIntSurfTab );
        if( n >= 0 ) {
            return __hwIntSurfInquire( &sweep->surfaces[0], n, val );
        }
        n = hwLookup( prop, __hwIntOrientTab );
        if( n >= 0 ) {
            return __hwIntOrientInquire( &sweep->orient, n, val );
        }
        break;
    }
    LOG_EXIT
    __hwIntSetError( HW_ERROR_BAD_PROP );
    return 0;
}


static void draw( hwObject obj )
{
    Sweep
        *sweep = (Sweep *)obj;
    hwInt32
        n, m, vn;
    HW_USE_CURR_DISP;

    LOG_ENTRY
    if( !sweep->path )  return;

    if( sweep->dirty ) {
        cook( __hwDisp, sweep );
    }

    /* If offscreen, don't render */
    if( !__hwDisp->boundsVisible( __hwDisp,
                    &sweep->surfaces[0], sweep->BBox,
                    2*sweep->cookedN*sweep->shapeCount ) )
    {
        return;
    }

    if( sweep->dl >= 0 ) {
        __hwDisp->callList( __hwDisp, sweep->dl );
    }
    else if( sweep->data ) {
        hwSurfAttrs( &sweep->surfaces[0] );
        __hwDisp->drawMesh( __hwDisp,
                        sweep->data, sweep->cookedFlags,
                        sweep->shapeCount, sweep->cookedN );
        if( sweep->hasCaps) {
            hwSurfAttrs( &sweep->surfaces[1] );

            __hwDisp->drawPolygon( __hwDisp,
                        sweep->cap1_data, sweep->cookedFlags,
                        sweep->shapeCount );

            hwSurfAttrs( &sweep->surfaces[2] );

            __hwDisp->drawPolygon( __hwDisp,
                        sweep->cap2_data, sweep->cookedFlags,
                        sweep->shapeCount );
        }
    }
    else {
        hwSurfAttrs( &sweep->surfaces[0] );
        if( sweep->hasCaps ) {
            hwSurfAttrs( &sweep->surfaces[1] );
            hwSurfAttrs( &sweep->surfaces[2] );
        }

        if( sweep->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            sweep->dl = __hwDisp->openList( __hwDisp );
            if( sweep->dl < 0 ) {
                sweep->optFlags &= ~(HW_OPT_USE_DL|HW_OPT_DL_ATTRS);
                sweep->optFlags |= HW_OPT_CACHE_DATA;
            }
        }

        hwSurfAttrs( &sweep->surfaces[0] );
        __hwDisp->drawMesh( __hwDisp,
                        sweep->temp_data, sweep->cookedFlags,
                        sweep->shapeCount, sweep->cookedN );
        if( sweep->hasCaps) {
            hwSurfAttrs( &sweep->surfaces[1] );

            __hwDisp->drawPolygon( __hwDisp,
                        sweep->temp_cap1_data, sweep->cookedFlags,
                        sweep->shapeCount );

            hwSurfAttrs( &sweep->surfaces[2] );

            __hwDisp->drawPolygon( __hwDisp,
                        sweep->temp_cap2_data, sweep->cookedFlags,
                        sweep->shapeCount );
        }

        if( sweep->optFlags & (HW_OPT_USE_DL|HW_OPT_DL_ATTRS) ) {
            __hwDisp->closeList( __hwDisp );
            __hwDisp->callList( __hwDisp, sweep->dl );
        }
        else if( sweep->optFlags & HW_OPT_CACHE_DATA ) {
            n = sweep->cookedN;
            m = sweep->shapeCount;
            vn = hwCalcWPV( sweep->cookedFlags );

            sweep->data = malloc( n*m*vn*sizeof(hwFloat) );
            if( sweep->hasCaps ) {
                sweep->cap1_data = malloc( m*vn*sizeof(hwFloat) );
                sweep->cap2_data = malloc( m*vn*sizeof(hwFloat) );
            }
            if( sweep->data &&
                    (!sweep->hasCaps
                        || (sweep->cap1_data && sweep->cap2_data)) )
            {
                (void)memcpy( sweep->data, sweep->temp_data,
                        n*m*vn*sizeof(hwFloat) );
                if( sweep->hasCaps ) {
                    (void)memcpy( sweep->cap1_data, sweep->temp_cap1_data,
                                m*vn*sizeof(hwFloat) );
                    (void)memcpy( sweep->cap2_data, sweep->temp_cap2_data,
                                m*vn*sizeof(hwFloat) );
                }
            }
            else {
                sweep->optFlags &= ~HW_OPT_CACHE_DATA;
            }

            if( sweep->optFlags & HW_OPT_SAVED_DATA ) {
                sweep->dirty = 0;
            }
        }
    }
    LOG_EXIT
}

/*** EOF hwSweep.c ***/
