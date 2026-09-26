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
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "hw.h"
#include "hw_internal.h"
#include "hw_vecmath.h"

#include "fontdata.h"

#define EPSILON         0.0001
#define EPSILON2        0.000001

static void PlaneGen( hwFloat *Data, int n );
static void StripGen( hwFloat *Data, int n );
static hwFloat *AddUV( int vn, int n, hwFloat *Data );
static void extrude_text_from_polyhedron( hwDisplay, int, float [4][4] );

/************************** TEXT STATE AND STATE FUNCS ***********************/
void __hwInitTextState( hwTextState *ptr )
{
    int
        i, j;

    ptr->textUp[0] = 0.; ptr->textUp[1] = 1.; ptr->textUp[2] = 0.;
    ptr->textDir[0] = 1.; ptr->textDir[1] = 0.; ptr->textDir[2] = 0.;
    for( i = 0; i < 4; i++ ) for( j = 0; j < 4; j++ ) {
        ptr->textOrient[i][j] = ptr->textScaleOrient[i][j] = (i == j) ? 1. : 0.;
    }
    ptr->textScale = 0.01;
    ptr->textExtrude[0] =  0.0;
    ptr->textExtrude[1] = 25.0;
    ptr->textExtrude[2] =  0.0;
    ptr->horizAlign = HW_TEXT_ALIGN_LEFT;
    ptr->vertAlign = HW_TEXT_ALIGN_BOTTOM;
    ptr->depthAlign = HW_TEXT_ALIGN_CENTER;
    hwDefaultSurf( &ptr->frontSurf );
    hwDefaultSurf( &ptr->sideSurf );
    hwDefaultSurf( &ptr->backSurf );
}


void __hwIntTextAlign( hwDisplay disp,
                        hwInt32 Horiz, hwInt32 Vert, hwInt32 Depth )
{
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    state->horizAlign = Horiz;
    state->vertAlign = Vert;
    state->depthAlign = Depth;
}

void __hwIntTextExtrude( hwDisplay disp,
                hwFloat Extrude0, hwFloat Extrude1, hwFloat Extrude2 )
{
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    state->textExtrude[0] = Extrude0 * 50.0;
    state->textExtrude[1] = Extrude1 * 50.0;
    state->textExtrude[2] = Extrude2 * 50.0;
}

void __hwIntTextHeight( hwDisplay disp, hwFloat Height )
{
    int
        i, j;
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    Height *= 0.01;
    state->textScale = Height;

    /* Create the scale + orientation matrix */
    for( i = 0; i < 3; i++ ) for( j = 0; j < 3; j++ ) {
        state->textScaleOrient[i][j] = state->textOrient[i][j] * Height;
    }
}

void __hwIntTextOrient
(
    hwDisplay disp,
    hwFloat Upx, hwFloat Upy, hwFloat Upz,
    hwFloat Dirx, hwFloat Diry, hwFloat Dirz
)
{
    int
        i, j;
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    state->textUp[0] = Upx; state->textUp[1] = Upy; state->textUp[2] = Upz;
    VNORM( state->textUp, state->textUp );

    state->textDir[0] = Dirx;state->textDir[1] = Diry;state->textDir[2] = Dirz;
    VNORM( state->textDir, state->textDir );

    /* Create orientation matrix */
    __hwIntVecToMat( state->textUp, state->textDir, state->textOrient );

    /* Create the scale + orientation matrix */
    for( i = 0; i < 3; i++ ) for( j = 0; j < 3; j++ ) {
        state->textScaleOrient[i][j] = state->textOrient[i][j]
                                        * state->textScale;
    }
}

void __hwIntTextAttrs
(
    hwDisplay disp,
    hwSurfaceType *front,
    hwSurfaceType *side,
    hwSurfaceType *back
)
{
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    if( front ) {
        state->frontSurf = *front;
        if( front->flags & HW_SURF_TWOSIDED ) {
            state->frontSurf.flags &= ~HW_SURF_BACKFACE;
        }
    }
    if( side ) {
        state->sideSurf = *side;
        /* Until we fix things... */
        state->sideSurf.flags &= ~HW_SURF_BACKFACE;
        state->sideSurf.flags |= HW_SURF_TWOSIDED;
    }
    if( back ) {
        state->backSurf = *back;
        if( back->flags & HW_SURF_TWOSIDED ) {
            state->backSurf.flags &= ~HW_SURF_BACKFACE;
        }
    }
}

void __hwIntText3d( hwDisplay disp,
                hwFloat px, hwFloat py, hwFloat pz, char *s )
{
    hwInt32
        i, j, n;
    hwFloat
        Mat[4][4],
        xoffs = 0.0, yoffs = 0.0,
        w, TotWidth;
    hwTextState
        *state;

    state = disp->getTextState( disp );
    if( !state )        return;

    if( state->horizAlign != HW_TEXT_ALIGN_LEFT ) {
        TotWidth = 0.0;
        for( i = 0; s[i]; i++ ) {
            n = s[i] - 33;
            if( (n >= 0) && (n <= 93) ) {
                TotWidth += __hwIntFont[n].CharWidth;
            }
            else {
                TotWidth += 0.5;        /* Assume space is 1/2 wide */
            }
        }
        TotWidth *= state->textScale;
    }

    switch( state->horizAlign ) {
    case HW_TEXT_ALIGN_LEFT :
        xoffs = 0.0;
        break;
    case HW_TEXT_ALIGN_RIGHT :
        xoffs = -TotWidth;
        break;
    case HW_TEXT_ALIGN_CENTER :
        xoffs = -TotWidth * 0.5;
        break;
    }

    switch( state->vertAlign ) {
    case HW_TEXT_ALIGN_TOP :
        yoffs = -100.0 * state->textScale;
        break;
    case HW_TEXT_ALIGN_CENTER :
        yoffs = -50.0 * state->textScale;
        break;
    case HW_TEXT_ALIGN_BOTTOM :
        yoffs = 0.0;
        break;
    }

    px += xoffs * state->textDir[0] + yoffs * state->textUp[0];
    py += xoffs * state->textDir[1] + yoffs * state->textUp[1];
    pz += xoffs * state->textDir[2] + yoffs * state->textUp[2];

    /* Initialize the matrix */
    (void)memcpy( Mat, state->textScaleOrient, 4*4*sizeof(hwFloat) );

    for( i = 0; s[i]; i++ ) {
        n = s[i] - 33;
        if( (n >= 0) && (n <= 93) ) {
            Mat[3][0] = px;
            Mat[3][1] = py;
            Mat[3][2] = pz;
            extrude_text_from_polyhedron( disp, n, Mat );
            w = __hwIntFont[n].CharWidth * state->textScale;
        }
        else {
            w = 50.0 * state->textScale;
        }
        px += w * state->textDir[0];
        py += w * state->textDir[1];
        pz += w * state->textDir[2];
    }
}

static void CalcNormal( hwFloat *prev, hwFloat *center, hwFloat *next)
{
    hwFloat
        x0, y0, /* Normalized vector to previous point */
        x1, y1, /* Normalized vector to next point */
        d,      /* Temp for normalizing */
        norm_x, norm_y; /* The normal */

    /* Get normalized vector (x0,y0) from (px,py) to (cx,cy) */
    x0 = center[0] - prev[0]; y0 = center[1] - prev[1];
    d = sqrt( x0*x0 + y0*y0 ); if( d > 0.0 ) d = 1.0 / d;
    x0 *= d; y0 *= d;

    /* Get normalized vector (x1,y1) from (cx,cy) to (nx,ny) */
    x1 = next[0] - center[0]; y1 = next[1] - center[1];
    d = sqrt( x1*x1 + y1*y1 ); if( d > 0.0 ) d = 1.0 / d;
    x1 *= d; y1 *= d;

    /* Normal of face 0 is then (-y0,x0), and face 1 is (-y1,x1)
     * Average these two and renormalize to get vertex normal
     * (Z component is always 0).  This assumes clockwise
     * contours; if counterclockwise, reverse both signs.
     */
    norm_x = -(y0 + y1); norm_y = (x0 + x1);

    d = sqrt( norm_x*norm_x + norm_y*norm_y ); if( d > 0.0 ) d = 1.0 / d;
    center[3] = norm_x * d;
    center[4] = norm_y * d;
    center[5] = 0.0;
}

static void CalculateStripNormals( hwFloat *tris, int num_verts, int wpv )
{
    int
        i;
    hwFloat
        *prev, *center, *next;

    prev = tris+(num_verts-1)*wpv;
    center = tris;
    next = center + wpv;
    CalcNormal( prev, center, next );

    prev = tris;
    center += wpv;
    next += wpv;
    for( i = 1; i < (num_verts-1); i++ ) {
        CalcNormal( prev, center, next );
        prev += wpv;
        center += wpv;
        next += wpv;
    }

    next = tris;
    CalcNormal( prev, center, next );
}

static void cookChar( int n )
{
    hwFloat
        *dst, *tris;
    hwInt32
        edgeLen, numVerts, numEdges,
        i, j, wpv = 6, wpv2 = 8;
    const hwFloat
        *vertList, *src, *startpos;
    const hwInt32
        *Edges;

    vertList = __hwIntFont[n].Verts;
    numVerts = __hwIntFont[n].NumVerts;
    numEdges = __hwIntFont[n].NumEdges;
    Edges = __hwIntFont[n].Edges;

    /* Create data for texture mapped version */
    __hwIntFont[n].TexVerts = AddUV( wpv, numVerts,
                                     (hwFloat *) __hwIntFont[n].Verts );
    if( !__hwIntFont[n].TexVerts ) {
        __hwIntSetError( HW_ERROR_NO_MEMORY );
        return;
    }

    if( numEdges > FC_CONT_MAX ) {
        /* This should really never happen */
        numEdges = FC_CONT_MAX;
    }

    PlaneGen( __hwIntFont[n].TexVerts, numVerts );

    /* Copy data for tristrip */
    for( i = 0; i < numEdges; i++ ) {
        startpos = vertList + (Edges[i*2] * wpv);
        edgeLen = Edges[i*2 + 1];

        if(             (startpos[(edgeLen-1)*wpv  ] == startpos[0])
                &&      (startpos[(edgeLen-1)*wpv+1] == startpos[1])
                &&      (startpos[(edgeLen-1)*wpv+2] == startpos[2])    )
        {
            /* Delete redundant point */
            edgeLen--;
        }

        tris = malloc( (edgeLen+1)*2 * wpv * sizeof(hwFloat) );
        if( !tris ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }

        src = startpos;
        dst = tris;
        for( j = 0; j < edgeLen; j++ ) {
            (void)memcpy( dst, src, wpv * sizeof(hwFloat) );
            src += wpv;
            dst += wpv;
        }

        CalculateStripNormals( tris, edgeLen, wpv );

        /* Make last vertex a duplicate of first vertex */
        (void)memcpy( dst, tris, wpv * sizeof(hwFloat) );
        dst += wpv;

        edgeLen++;

        /* Now, recopy all data for mesh */
        src = tris;
        for( j = 0; j < edgeLen; j++ ) {
            (void)memcpy( dst, src, wpv * sizeof(hwFloat) );
            src += wpv;
            dst += wpv;
        }

        __hwIntFont[n].StripData[i] = tris;

        /* Copy textured coordinates to TexStripData */
        __hwIntFont[n].TexStripData[i] = AddUV( wpv, edgeLen*2, tris );
        if( !__hwIntFont[n].TexStripData[i] ) {
            __hwIntSetError( HW_ERROR_NO_MEMORY );
            return;
        }

        StripGen( __hwIntFont[n].TexStripData[i], edgeLen );

        /* Save away the vertex count */
        __hwIntFont[n].StripCounts[i] = edgeLen;
    }

    __hwIntFont[n].Cooked = 1;
}

static void extrude_text_from_polyhedron
(
    hwDisplay disp, int n, float Mat[4][4]
)
{
    int
        i, j,                   /* Loop counters */
        numEdges,
        wpv = 6, wpv2 = 8,
        textureFront, textureBack, textureSide,
        facet,
        numVerts,
        numFacetSets,
        edgeLen;
    hwFloat
        *xformed;
    hwInt32
        xformedSize;
    hwFloat
        *src, *dst, *tris;
    const hwFloat
        *vertList;
    hwTextState
        *state;
    void
        *val;
    const hwInt32
        *Edges,
        *indexList;
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    /* Extract scratch data */
    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    xformed = intDisp->scratchData;
    xformedSize = intDisp->scratchDataSize;

    state = disp->getTextState( disp );
    if( !state )        goto ERROR;

    textureFront = textureBack = textureSide = 0;
    if( !(state->frontSurf.flags & (HW_SURF_INVISIBLE|HW_SURF_WIREFRAME)) ) {
        if( state->frontSurf.numTextures ) {
            if( state->frontSurf.textures[0] ) {
                textureFront = 1;
            }
            else if( state->frontSurf.textureIDs[0] > 0 ) {
                textureFront = 1;
            }
        }
    }
    if( !(state->backSurf.flags & (HW_SURF_INVISIBLE|HW_SURF_WIREFRAME)) ) {
        if( state->backSurf.numTextures ) {
            if( state->backSurf.textures[0] ) {
                textureBack = 1;
            }
            else if( state->backSurf.textureIDs[0] > 0 ) {
                textureBack = 1;
            }
        }
    }
    if( !(state->sideSurf.flags & (HW_SURF_INVISIBLE|HW_SURF_WIREFRAME)) ) {
        if( state->sideSurf.numTextures ) {
            if( state->sideSurf.textures[0] ) {
                textureSide = 1;
            }
            else if( state->sideSurf.textureIDs[0] > 0 ) {
                textureSide = 1;
            }
        }
    }

    /* All this stuff used to be passed in.  No longer. */
    vertList = __hwIntFont[n].Verts;
    numVerts = __hwIntFont[n].NumVerts;
    numFacetSets = __hwIntFont[n].NumTris;

    numEdges = __hwIntFont[n].NumEdges;
    Edges = __hwIntFont[n].Edges;
    indexList = __hwIntFont[n].Tris;

    if( numEdges > FC_CONT_MAX ) {
        /* This should really never happen */
        numEdges = FC_CONT_MAX;
    }

    /* If we haven't cooked the texture information yet, do so now */
    if( !__hwIntFont[n].Cooked ) {
        cookChar( n );
    }

    if( !__hwIntFont[n].Cooked ) {
        /* Oops - gotta go! */
        goto ERROR;
    }

    if( (numVerts*3*wpv2) > xformedSize ) {
        xformedSize = numVerts*3*wpv2;
        xformed = realloc( xformed, numVerts*3*wpv2*sizeof(hwFloat) );
        if( !xformed )  goto ERROR;
    }

    if( state->backSurf.flags & HW_SURF_WIREFRAME ) {
        (void)memcpy( xformed, vertList, numVerts*wpv*sizeof(hwFloat) );
        src = xformed;
        for( i = 0; i < numVerts; i++ ) {
            VADD( src, src, state->textExtrude );
            src += wpv;
        }
        __hwIntMatXformData( Mat, xformed, numVerts, HW_DATA_NORMALS );

        hwSurfAttrs( &state->backSurf );
        for( i = 0; i < numEdges; i++ ) {
            disp->drawPolygon( disp,
                                xformed + Edges[2*i]*wpv,
                                HW_DATA_NORMALS,
                                __hwIntFont[n].StripCounts[i]-1 );
        }
    }
    else if( !(state->backSurf.flags & HW_SURF_INVISIBLE) ) {
        if( !(state->backSurf.flags & HW_SURF_TWOSIDED) ) {
            state->backSurf.flags ^= HW_SURF_BACKFACE;
        }
        hwSurfAttrs( &state->backSurf );
        if( !(state->backSurf.flags & HW_SURF_TWOSIDED) ) {
            state->backSurf.flags ^= HW_SURF_BACKFACE;
        }

        if( textureBack ) {
            (void)memcpy( xformed, __hwIntFont[n].TexVerts,
                        numVerts*wpv2*sizeof(hwFloat) );
            src = xformed;
            for( i = 0; i < numVerts; i++ ) {
                VADD( src, src, state->textExtrude );
                src += wpv2;
            }
            __hwIntMatXformData( Mat, xformed, numVerts,
                                        HW_DATA_NORMALS | HW_DATA_ST );

            disp->drawIndexedTriangles( disp,
                        xformed,
                        numVerts, HW_DATA_NORMALS | HW_DATA_ST,
                        (hwInt32 *)indexList, numFacetSets );
        }
        else {
            (void)memcpy( xformed, vertList, numVerts*wpv*sizeof(hwFloat) );
            src = xformed;
            for( i = 0; i < numVerts; i++ ) {
                VADD( src, src, state->textExtrude );
                src += wpv;
            }
            __hwIntMatXformData( Mat, xformed, numVerts, HW_DATA_NORMALS );

            disp->drawIndexedTriangles( disp, xformed,
                        numVerts, HW_DATA_NORMALS,
                        (hwInt32 *)indexList, numFacetSets );
        }
    }

    if( state->frontSurf.flags & HW_SURF_WIREFRAME ) {
        (void)memcpy( xformed, vertList, numVerts*wpv*sizeof(hwFloat) );
        src = xformed;
        for( i = 0; i < numVerts; i++ ) {
            VSUB( src, src, state->textExtrude );
            src += wpv;
        }
        __hwIntMatXformData( Mat, xformed, numVerts, HW_DATA_NORMALS );

        hwSurfAttrs( &state->frontSurf );
        for( i = 0; i < numEdges; i++ ) {
            disp->drawPolygon( disp,
                                xformed + Edges[2*i]*wpv,
                                HW_DATA_NORMALS,
                                __hwIntFont[n].StripCounts[i]-1 );
        }
    }
    else if( !(state->frontSurf.flags & HW_SURF_INVISIBLE) ) {
        hwSurfAttrs( &state->frontSurf );

        if( textureFront ) {
            (void)memcpy( xformed, __hwIntFont[n].TexVerts,
                        numVerts*wpv2*sizeof(hwFloat) );
            src = xformed;
            for( i = 0; i < numVerts; i++ ) {
                VSUB( src, src, state->textExtrude );
                src += wpv2;
            }
            __hwIntMatXformData( Mat, xformed, numVerts,
                                        HW_DATA_NORMALS | HW_DATA_ST );

            disp->drawIndexedTriangles( disp,
                        xformed,
                        numVerts, HW_DATA_NORMALS | HW_DATA_ST,
                        (hwInt32 *)indexList, numFacetSets );
        }
        else {
            (void)memcpy( xformed, vertList,
                        numVerts*wpv*sizeof(hwFloat) );
            src = xformed;
            for( i = 0; i < numVerts; i++ ) {
                VSUB( src, src, state->textExtrude );
                src += wpv;
            }
            __hwIntMatXformData( Mat, xformed, numVerts, HW_DATA_NORMALS );

            disp->drawIndexedTriangles( disp, xformed,
                        numVerts, HW_DATA_NORMALS,
                        (hwInt32 *)indexList, numFacetSets );
        }
    }

    /* Now, traverse the edgelists, rendering triangle strips around the
    ** outside of the character
    */
    if( !(state->sideSurf.flags & HW_SURF_INVISIBLE) ) {
        hwSurfAttrs( &state->sideSurf );
        if( textureSide ) {
            for( i = 0; i < numEdges; i++ ) {
                edgeLen = __hwIntFont[n].StripCounts[i];

                (void)memcpy( xformed,
                                __hwIntFont[n].TexStripData[i],
                                edgeLen*2*wpv2*sizeof(hwFloat) );
                dst = xformed;
                for( j = 0; j < edgeLen; j++ ) {
                    VADD( dst, dst, state->textExtrude );
                    dst += wpv2;
                }
                for( j = 0; j < edgeLen; j++ ) {
                    VSUB( dst, dst, state->textExtrude );
                    dst += wpv2;
                }
                __hwIntMatXformData( Mat, xformed, 2*edgeLen,
                                HW_DATA_NORMALS | HW_DATA_ST );

                disp->drawMesh( disp,
                                xformed,
                                HW_DATA_NORMALS | HW_DATA_ST,
                                2, edgeLen );
            }
        }
        else {
            for( i = 0; i < numEdges; i++ ) {
                edgeLen = __hwIntFont[n].StripCounts[i];

                (void)memcpy( xformed,
                                __hwIntFont[n].StripData[i],
                                edgeLen*2*wpv*sizeof(hwFloat) );
                dst = xformed;
                for( j = 0; j < edgeLen; j++ ) {
                    VADD( dst, dst, state->textExtrude );
                    dst += wpv;
                }
                for( j = 0; j < edgeLen; j++ ) {
                    VSUB( dst, dst, state->textExtrude );
                    dst += wpv;
                }
                __hwIntMatXformData( Mat, xformed, 2*edgeLen,
                                HW_DATA_NORMALS );

                disp->drawMesh( disp,
                                xformed,
                                HW_DATA_NORMALS,
                                2, edgeLen );
            }
        }
    }
ERROR :
    /* Stash scratch data */
    intDisp->scratchDataSize = xformedSize;
    intDisp->scratchData = xformed;
}

static void PlaneGen( hwFloat *Data, int n )
{
    int
        i, wpv, UVOffset;

    wpv = 8;    /* XYZNxNyNzST */
    UVOffset = 6;

    for( i = 0; i < n; i++ ) {
        Data[UVOffset  ] = Data[0] * 0.01;
        Data[UVOffset+1] = Data[1] * 0.01;
        Data += wpv;
    }
}


static void StripGen( hwFloat *Data, int n )
{
    hwFloat
        u, *ptr, dist;
    int
        i, wpv, UVOffset;
    double
        dx, dy;

    wpv = 8;            /* XYZNxNyNzST */
    UVOffset = 6;

    /* Get bottom (v = 0.0) */
    ptr = Data;
    for( i = 0; i < n; i++ ) {
        if( i < 1 ) {
            u = 0.0;  /* First vertex; make it start at the beginning */
        }
        else {
            dx = ptr[0] - ptr[0-wpv];   dy = ptr[1] - ptr[1-wpv];
            dist = sqrt( dx*dx + dy*dy );
            u += dist;
        }

        ptr[UVOffset  ] = u * .02;
        ptr[UVOffset+1] = 0.0;
        ptr += wpv;
    }

    /* Get top (v = 1.0) */
    for( i = 0; i < n; i++ ) {
        if( i < 1 ) {
            u = 0.0;  /* First vertex; make it start at the beginning */
        }
        else {
            dx = ptr[0] - ptr[0-wpv];   dy = ptr[1] - ptr[1-wpv];
            dist = sqrt( dx*dx + dy*dy );
            u += dist;
        }

        ptr[UVOffset  ] = u * .02;
        ptr[UVOffset+1] = 1.0;
        ptr += wpv;
    }
}

/* Adds UV to an array */
static hwFloat *AddUV( int vn, int n, hwFloat *Data )
{
    hwFloat
        *Result;
    int
        i, vn2;

    vn2 = vn + 2;
    Result = malloc( vn2 * n * sizeof(hwFloat) );
    if( !Result )       return NULL;

    for( i = 0; i < n; i++ ) {
        (void)memcpy( Result+i*vn2, Data+i*vn, vn*sizeof(hwFloat) );
        Result[(i+1)*vn2-2] = 0.0;
        Result[(i+1)*vn2-1] = 0.0;
    }
    return Result;
}

/*** EOF polytext.c ***/
