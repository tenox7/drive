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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "hw.h"

/* shapes.c - utilities to convert high-level shape descriptions
 * into polygons and meshes
 */

void __hwIntMakeSphere
(
    hwInt32 Rows, hwInt32 Cols, /* Tesselation parameters */
    hwInt32 Flags,              /* What kind of data to generate */
    hwFloat Radius,             /* How big to make it */
    hwFloat *Verts,             /* Where to put generated data */
    hwFloat Lat0, hwFloat Lat1, /* Latitude range */
    hwFloat Lon0, hwFloat Lon1, /* Longitude range */
    hwFloat ObjColor[3]         /* Color, if needed */
)
{
    hwInt32
        i, j, tm, tx, tn, WPV;
    hwVertexOffsets
        Offs;
    double
        X, Y, Z,
        FracI, FracJ,
        LatD, LonD,
        phi, theta;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    LatD = Lat1 - Lat0;
    LonD = Lon1 - Lon0;
    for( i = 0; i < Rows; i++ ) {
        FracI = i / (double)(Rows-1);
        phi = Lat0 + LatD * FracI;

        for( j = 0; j < Cols; j++ ) {
            FracJ = j / (double)(Cols-1);
            theta = Lon0 + LonD * (1.0 - FracJ);

            /* Calculate current coordinate */
            X = cos(phi)*cos(theta);
            Y = cos(phi)*sin(theta);
            Z = sin(phi);

            /* Store away XYZ */
            Verts[0] = X * Radius;
            Verts[1] = Y * Radius;
            Verts[2] = Z * Radius;

            /* Store away default color */
            if( Offs.rgb ) {
                Verts[Offs.rgb  ] = ObjColor[0];
                Verts[Offs.rgb+1] = ObjColor[1];
                Verts[Offs.rgb+2] = ObjColor[2];
            }
            if( Offs.alpha ) {
                Verts[Offs.alpha] = 1.0;
            }

            /* Store away normals */
            if( Offs.normal ) {
                Verts[Offs.normal  ] = X;
                Verts[Offs.normal+1] = Y;
                Verts[Offs.normal+2] = Z;
            }
            if( Offs.tangent ) {
                Verts[Offs.tangent  ] = -sin(theta);
                Verts[Offs.tangent+1] = cos(theta);
                Verts[Offs.tangent+2] = 0.f;
                Verts[Offs.tangent+3] = 1.f;
            }

            /* Store away texture map coords */
            for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
                if( !(tx = Offs.texCoord[tm]) ) continue;;
                tn = Offs.texSize[tm];

                Verts[tx ] = 1.0 - FracJ;
                Verts[tx+1] = FracI;
                if( tn > 2 ) Verts[tx+2] = 0.0;
                if( tn > 3 ) Verts[tx+3] = 1.0;
            }

            Verts += WPV;
        }
    }
}


void __hwIntMakeBox
(
    hwInt32 Flags,
    hwFloat *Corners,
    hwFloat *Dst,
    hwFloat ObjColor[3]
)
{
    hwInt32
        i, j, tx, tn, tm, WPV;
    hwVertexOffsets
        Offs;
    /*
     * Flattened box.  All faces are drawn in clockwise order in
     * this picture.  The positive primary axes are drawn in the
     * appropriate corner.  All faces have vertex 0 as the lower-
     * left corner, and S=0,T=0 at lower left and S=1,T=1 at upper
     * right.  The corners are labelled with three numbers that
     * correspond to the XYZ index in the Corners input array.
     * Generally, 0=-X, 1=-Y, 2=-Z, 3=+X, 4=+Y, 5=+Z
     *
     *               045           345
     *                +-------------+
     *                |             |
     *                |             |
     *                |             |
     *                | Z   TOP     |
     *                | ^           |
     *                | |           |
     * 045         042| +-->X       |342         345           045
     *  +-------------+-------------+-------------+-------------+
     *  |             |             |             |             |
     *  |             |             |             |             |
     *  |             |             |             |             |
     *  |     LFT   Y | Y   FRT     | Y   RGT     |     BAK   Y |
     *  |           ^ | ^           | ^           |           ^ |
     *  |           | | |           | |           |           | |
     *  |       Z<--+ | +-->X       | +-->Z       |       X<--+ |
     *  +-------------+-------------+-------------+-------------+
     * 015         012| +-->X       |312         315           015
     *                | |           |
     *                | v           |
     *                | Z   BOT     |
     *                |             |
     *                |             |
     *                |             |
     *                +-------------+
     *               015           315
     */
    static const int
        faceOrder[] = {
            0,1,2,    0,4,2,    3,4,2,    3,1,2,        /* Front */
            3,1,5,    3,4,5,    0,4,5,    0,1,5,        /* Back */
            0,1,5,    0,4,5,    0,4,2,    0,1,2,        /* Left */
            3,1,2,    3,4,2,    3,4,5,    3,1,5,        /* Right */
            0,4,2,    0,4,5,    3,4,5,    3,4,2,        /* Top */
            0,1,5,    0,1,2,    3,1,2,    3,1,5,        /* Bot */
        };
    static const hwFloat
        faceNormals[6][3] = {
            {  0.0,  0.0, -1.0},        /* Front */
            {  0.0,  0.0,  1.0},        /* Back */
            { -1.0,  0.0,  0.0 },       /* Left */
            {  1.0,  0.0,  0.0 },       /* Right */
            {  0.0,  1.0,  0.0 },       /* Top */
            {  0.0, -1.0,  0.0 },       /* Bot */
        },
        faceTangents[6][4] = {
            {  1.0,  0.0,  0.0,  1.0 },       /* Front */
            { -1.0,  0.0,  0.0,  1.0 },       /* Back */
            {  0.0,  0.0, -1.0,  1.0 },       /* Left */
            {  0.0,  0.0,  1.0,  1.0 },       /* Right*/
            {  1.0,  0.0,  0.0,  1.0 },       /* Top */
            {  1.0,  0.0,  0.0,  1.0 },       /* Bot */
        },
        faceST[4][2] = {
            { 0.0, 0.0 },               /* Lower left */
            { 0.0, 1.0 },               /* Upper left */
            { 1.0, 1.0 },               /* Upper right */
            { 1.0, 0.0 },               /* Lower right */
        };

    WPV = hwGetVertexOffsets( Flags, &Offs );

    /* Vertices */
    for( i = 0; i < 6; i++ ) {
        for( j = 0; j < 4; j++) {
            Dst[(4*i+j)*WPV+0] = Corners[faceOrder[(4*i+j)*3+0]];
            Dst[(4*i+j)*WPV+1] = Corners[faceOrder[(4*i+j)*3+1]];
            Dst[(4*i+j)*WPV+2] = Corners[faceOrder[(4*i+j)*3+2]];
        }
    }

    if( (tx = Offs.rgb) ) {
        for( i = 0; i < 6; i++ ) {
            for( j = 0; j < 4; j++) {
                Dst[(4*i+j)*WPV+tx  ] = ObjColor[0];
                Dst[(4*i+j)*WPV+tx+1] = ObjColor[1];
                Dst[(4*i+j)*WPV+tx+2] = ObjColor[2];
            }
        }
    }
    if( (tx = Offs.alpha) ) {
        for( i = 0; i < 6; i++ ) {
            for( j = 0; j < 4; j++) {
                Dst[(4*i+j)*WPV+tx  ] = 1.0;
            }
        }
    }

    if( (tx = Offs.normal) ) {
        for( i = 0; i < 6; i++ ) {
            for( j = 0; j < 4; j++) {
                Dst[(4*i+j)*WPV+tx  ] = faceNormals[i][0];
                Dst[(4*i+j)*WPV+tx+1] = faceNormals[i][1];
                Dst[(4*i+j)*WPV+tx+2] = faceNormals[i][2];
            }
        }
    }
    if( (tx = Offs.tangent) ) {
        for( i = 0; i < 6; i++ ) {
            for( j = 0; j < 4; j++) {
                Dst[(4*i+j)*WPV+tx  ] = faceTangents[i][0];
                Dst[(4*i+j)*WPV+tx+1] = faceTangents[i][1];
                Dst[(4*i+j)*WPV+tx+2] = faceTangents[i][2];
                Dst[(4*i+j)*WPV+tx+3] = faceTangents[i][3];
            }
        }
    }
    for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
        if( !(tx = Offs.texCoord[tm]) ) continue;
        tn = Offs.texSize[tm];

        for( i = 0; i < 6; i++ ) {
            for( j = 0; j < 4; j++ ) {
                Dst[(4*i+j)*WPV+tx  ] = faceST[j][0];
                Dst[(4*i+j)*WPV+tx+1] = faceST[j][1];
                if( tn > 2 ) Dst[(4*i+j)*WPV+tx+2] = 0.0;
                if( tn > 3 ) Dst[(4*i+j)*WPV+tx+3] = 1.0;
            }
        }
    }
}


void __hwIntMakeRing
(
    hwInt32 Rows, hwInt32 Cols,         /* Tesselation parameters */
    hwInt32 Flags,                      /* What data to generate */
    hwFloat Inner, hwFloat Outer,       /* Inner+outer radii of ring */
    hwFloat *Verts,                     /* Where to put the data */
    hwFloat Lon0, hwFloat Lon1,         /* Which part of the ring to do */
    hwFloat ObjColor[3]                 /* The color to make it */
)
{
    hwInt32
        i, j, tm, tx, tn, WPV;
    hwVertexOffsets
        Offs;
    double
        LonD, FracI, FracJ, Rad,
        IRad,
        sx, cx,
        theta;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    LonD = Lon1 - Lon0;
    IRad = Outer;
    if( IRad != 0.0 ) IRad = 0.5 / IRad;
    Outer -= Inner;
    for( i = 0; i < Rows; i++ ) {
        FracI = i / (double)(Rows-1);
        Rad = Inner + FracI * Outer;
        for( j = 0; j < Cols; j++ ) {
            FracJ = j / (double)(Cols-1);
            theta = Lon0 + LonD * (1.0 - FracJ);
            sx = sin(theta);
            cx = cos(theta);

            Verts[0] = Rad*cx;
            Verts[1] = Rad*sx;
            Verts[2] = 0.0;

            if( Offs.rgb ) {
                Verts[Offs.rgb  ] = ObjColor[0];
                Verts[Offs.rgb+1] = ObjColor[1];
                Verts[Offs.rgb+2] = ObjColor[2];
            }
            if( Offs.alpha ) {
                Verts[Offs.alpha] = 1.0;
            }

            if( Offs.normal ) {
                Verts[Offs.normal  ] = 0.0;
                Verts[Offs.normal+1] = 0.0;
                Verts[Offs.normal+2] = -1.0;
            }
            if( Offs.tangent ) {
                Verts[Offs.tangent  ] = 1.0;
                Verts[Offs.tangent+1] = 0.0;
                Verts[Offs.tangent+2] = 0.0;
                Verts[Offs.tangent+2] = 1.0;
            }

            for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
                if( !(tx = Offs.texCoord[tm]) ) continue;
                tn = Offs.texSize[tm];

                Verts[tx  ] = 0.5 + Verts[0] * IRad;
                Verts[tx+1] = 0.5 + Verts[1] * IRad;
                if( tn > 2 ) Verts[tx+2] = 0.0;
                if( tn > 3 ) Verts[tx+3] = 1.0;
            }

            Verts += WPV;
        }
    }
}

void __hwIntMakeThinRing
(
    hwInt32 Rows,               /* Tesselation parameter */
    hwInt32 Flags,              /* What data to generate */
    hwFloat Rad,                /* Radius of ring */
    hwFloat *Verts,             /* Where to put data */
    hwFloat ObjColor[3]         /* What color, if any */
)
{
    hwInt32
        i, tm, tx, tn, offs, WPV;
    hwVertexOffsets
        Offs;
    double
        FracI,
        sx, cx,
        theta;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    for( i = 0; i < Rows; i++ ) {
        FracI = i / (double)(Rows-1);
        theta = 3.141592653589*2.0*(1.0 - FracI);
        sx = sin(theta);
        cx = cos(theta);

        Verts[0] = Rad*cx;
        Verts[1] = Rad*sx;
        Verts[2] = 0.0;

        if( Offs.rgb ) {
            Verts[Offs.rgb  ] = ObjColor[0];
            Verts[Offs.rgb+1] = ObjColor[1];
            Verts[Offs.rgb+2] = ObjColor[2];
        }

        if( Offs.alpha ) {
            Verts[Offs.alpha] = 1.0;
        }

        if( Offs.normal ) {
            Verts[Offs.normal  ] = 0.0;
            Verts[Offs.normal+1] = 0.0;
            Verts[Offs.normal+2] = -1.0;
        }
        if( Offs.tangent ) {
            Verts[Offs.tangent  ] = 1.0;
            Verts[Offs.tangent+1] = 0.0;
            Verts[Offs.tangent+2] = 0.0;
            Verts[Offs.tangent+3] = 1.0;
        }

        for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
            if( !(tx = Offs.texCoord[tm]) ) continue;
            tn = Offs.texSize[tm];

            Verts[tx  ] = (1.0 + cx) * 0.5;
            Verts[tx+1] = (1.0 + sx) * 0.5;
            if( tn > 2 ) Verts[tx+2] = 0.0;
            if( tn > 3 ) Verts[tx+3] = 1.0;
        }

        Verts += WPV;
    }
}


void __hwIntMakeTorus
(
    hwInt32 Rows, hwInt32 Cols,         /* Tesselation parameters */
    hwInt32 Flags,                      /* What data to generate */
    hwFloat Inner, hwFloat Outer,       /* Inner and outer radii */
    hwFloat *Verts,                     /* Where to store the data */
    hwFloat Lat0, hwFloat Lat1,         /* Latitude parameters */
    hwFloat Lon0, hwFloat Lon1,         /* Longitude parameters */
    hwFloat ObjColor[3]                 /* What color, if any */
)
{
    hwInt32
        i, j, tm, tx, tn, WPV;
    hwVertexOffsets
        Offs;
    double
        LatD, LonD,
        FracI, FracJ,
        tr, ar,
        sx, cx,
        sy, cy,
        phi, theta;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    LatD = Lat1 - Lat0;
    LonD = Lon1 - Lon0;

    tr = (Outer - Inner) / 2.0;
    ar = (Outer + Inner) / 2.0;

    for( i = 0; i < Rows; i++ ) {
        FracI = i / (double)(Rows-1);
        phi = Lat0 + LatD * FracI;
        sy = sin(phi);  cy = cos(phi);

        for( j = 0; j < Cols; j++ ) {
            FracJ = j / (double)(Cols-1);
            theta = Lon0 + LonD * FracJ;
            sx = sin(theta);    cx = cos(theta);

            /* Calculate position */
            Verts[0] = cy * (ar + tr*cx);
            Verts[1] = sy * (ar + tr*cx);
            Verts[2] = tr*sx;

            if( Offs.rgb ) {
                Verts[Offs.rgb  ] = ObjColor[0];
                Verts[Offs.rgb+1] = ObjColor[1];
                Verts[Offs.rgb+2] = ObjColor[2];
            }
            if( Offs.alpha ) {
                Verts[Offs.alpha] = 1.0;
            }

            if( Offs.normal ) {
                Verts[Offs.normal  ] = cx*cy;
                Verts[Offs.normal+1] = cx*sy;
                Verts[Offs.normal+2] = sx;
            }
            if( Offs.tangent ) {
                Verts[Offs.tangent  ] = -sy;
                Verts[Offs.tangent+1] = cy;
                Verts[Offs.tangent+2] = 0.f;
                Verts[Offs.tangent+3] = 1.f;
            }

            for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
                if( !(tx = Offs.texCoord[tm]) ) continue;
                tn = Offs.texSize[tm];

                Verts[tx  ] = FracI;
                Verts[tx+1] = FracJ;
                if( tn > 2 ) Verts[tx+2] = 0.0;
                if( tn > 3 ) Verts[tx+3] = 1.0;
            }

            Verts += WPV;
        }
    }
}


void __hwIntMakeCone
(
    hwInt32 Rows, hwInt32 Cols,                 /* Tesselation parameters */
    hwInt32 Flags,                              /* Data to generate */
    hwFloat Inner, hwFloat Outer, hwFloat Length,/* Describes the cone */
    hwFloat *Verts,                             /* The destination mesh data */
    hwFloat Lon0, hwFloat Lon1,                 /* Angles to start */
    hwFloat ObjColor[3]                         /* What color, if any */
)
{
    hwInt32
        i, j, tm, tx, tn, offs, WPV;
    hwVertexOffsets
        Offs;
    double
        LonD, FracI, FracJ, Rad, Dist,
        BinormR, BinormH,
        NormR, NormH,
        sx, cx,
        theta;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    /* Cross-section of a cone:
     *
     * +Z
     *  ^
     *  |         |<-(O-I)-->|
     *  |<------Outer------->|
     *  +---------+----------+
     *  | ^       .         /
     *  | |       .        /
     *  | L       .       /
     *  | e       .      /
     *  | n       .     /
     *  | g       .    /
     *  | t       .   /
     *  | h       .  /
     *  | |       . /
     *  | v       ./
     *  +---------+------->+X
     *  |<-Inner->|
     *
     * So, the binormal of the point on the +X axis is:
     *
     *    B = | (Outer-Inner), 0, Length |
     *
     * And, the normal of that point is:
     *
     *    N = { B.z, 0, -B.x }
     *
     * And, the tangent at that point is:
     *
     *    T = { 0, 1, 0 }
     */

    BinormR = Outer - Inner;
    BinormH = Length;

    Dist = sqrt( BinormR*BinormR + BinormH*BinormH );
    if( Dist > 0.0 )    Dist = 1.0 / Dist;
    BinormR *= Dist; BinormH *= Dist;

    NormR = BinormH;
    NormH = -BinormR;

    Outer -= Inner;
    LonD = Lon1 - Lon0;

    for( i = 0; i < Rows; i++ ) {
        FracI = i / (double)(Rows-1);
        Rad = Inner + FracI*Outer;
        Dist = FracI*Length;
        for( j = 0; j < Cols; j++ ) {
            FracJ = j / (double)(Cols-1);
            theta = Lon0 + LonD * (1.0 - FracJ);
            sx = sin(theta);
            cx = cos(theta);

            Verts[0] = Rad*cx;
            Verts[1] = Rad*sx;
            Verts[2] = Dist;

            if( Offs.rgb ) {
                Verts[Offs.rgb  ] = ObjColor[0];
                Verts[Offs.rgb+1] = ObjColor[1];
                Verts[Offs.rgb+2] = ObjColor[2];
            }
            if( Offs.alpha ) {
                Verts[Offs.alpha] = 1.0;
            }

            if( Offs.normal ) {
                Verts[Offs.normal  ] = NormR*cx;
                Verts[Offs.normal+1] = NormR*sx;
                Verts[Offs.normal+2] = NormH;
            }
            if( Offs.tangent ) {
                Verts[Offs.tangent  ] = sx;
                Verts[Offs.tangent+1] = -cx;
                Verts[Offs.tangent+2] = 0.0;
                Verts[Offs.tangent+2] = 1.0;
            }

            for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
                if( !(tx = Offs.texCoord[tm]) ) continue;
                tn = Offs.texSize[tm];

                Verts[tx  ] = 1.0 - FracJ;
                Verts[tx+1] = FracI;
                if( tn > 2 ) Verts[tx+2] = 0.0;
                if( tn > 3 ) Verts[tx+3] = 1.0;
            }

            Verts += WPV;
        }
    }
}


void __hwIntMakeSurfRev
(
    hwInt32 n, hwInt32 m,       /* Tesselation parameters */
    hwInt32 Flags,              /* Data to generate */
    hwFloat Ang0,               /* Starting angle */
    hwFloat Ang1,               /* Ending angle */
    hwFloat *Points,            /* Source points to revolve */
    hwFloat *Mesh,              /* Where to store data */
    hwFloat ObjColor[3]         /* What color, if any */
)
{
    hwInt32
        i, j, vn, tm, tx, tn;
    hwVertexOffsets
        Offs;
    hwFloat
        MinZ, MaxZ,
        FracI, FracJ,
        CosA, SinA,
        Rad, Ang;

    vn = hwGetVertexOffsets( Flags, &Offs );

    /* Find Z ranges for texturing */
    MinZ = MaxZ = Points[1];
    for( i = 1; i < m; i++ ) {
        if( Points[2*i+1] < MinZ )      MinZ = Points[2*i+1];
        if( Points[2*i+1] > MaxZ )      MaxZ = Points[2*i+1];
    }
    MaxZ -= MinZ; if( MaxZ > 0.0 ) MaxZ = 1.0 / MaxZ;

    for( i = 0; i < n; i++ ) {
        FracI = i / (float)(n-1);
        Ang = FracI * Ang1;
        CosA = cos( Ang ); SinA = sin( Ang );
        for( j = 0; j < m; j++ ) {
            FracJ = (Points[2*j+1] - MinZ) * MaxZ;
            Rad = Points[2*j];

            Mesh[0] = Rad * CosA;
            Mesh[1] = Rad * SinA;
            Mesh[2] = Points[2*j+1];

            if( Offs.rgb ) {
                Mesh[Offs.rgb  ] = ObjColor[0];
                Mesh[Offs.rgb+1] = ObjColor[1];
                Mesh[Offs.rgb+2] = ObjColor[2];
            }
            if( Offs.alpha ) {
                Mesh[Offs.alpha] = 1.0;
            }

            for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
                if( !(tx = Offs.texCoord[tm]) ) continue;
                tn = Offs.texSize[tm];

                Mesh[tx  ] = FracI;
                Mesh[tx+1] = FracJ;
                if( tn > 2 ) Mesh[tx+2] = 0.0;
                if( tn > 3 ) Mesh[tx+3] = 1.0;
            }

            Mesh += vn;
        }
    }
}

void __hwIntMakeTerrain
(
    hwInt32 Flags,              /* Data to generate */
    hwFloat *Data,              /* Source vertices to interpolate */
    hwInt32 gn, hwInt32 gm,     /* Size of destination mesh */
    hwFloat *NewData            /* Where to put interpolated mesh */
)
{
    hwInt32
        i, j, n, m, vn,
        ii, jj, nn, mm;
    hwFloat
         *Strip, *Verts, DX1, DY1, DX2, DY2,
        PT0[11], PT1[11], PU0[11], PU1[11],
        P00[11], P10[11], P01[11], P11[11];

    vn = hwCalcWPV( Flags );

    /* Skip to beginning of altitude mesh */
    (void)memcpy( P00, Data, vn*sizeof(hwFloat) );
    (void)memcpy( P01, Data+vn, vn*sizeof(hwFloat) );
    (void)memcpy( P10, Data+2*vn, vn*sizeof(hwFloat) );
    (void)memcpy( P11, Data+3*vn, vn*sizeof(hwFloat) );

    Verts = Data + 4*vn;
    Strip = NewData;

    for( i = 0; i < gn; i++ ) {
        /* Initialize interpolation along top row */
        DY1 = i / (hwFloat)(gn-1);
        DY2 = 1.0 - DY1;
        for( ii = 0; ii < vn; ii++ ) {
            PT0[ii] = DY2*P00[ii] + DY1*P10[ii];
            PT1[ii] = DY2*P01[ii] + DY1*P11[ii];
        }
        /* Initialize interpolation along bottom row */
        DY1 = (i+1) / (hwFloat)(gn-1);
        DY2 = 1.0 - DY1;
        for( ii = 0; ii < vn; ii++ ) {
            PU0[ii] = DY2*P00[ii] + DY1*P10[ii];
            PU1[ii] = DY2*P01[ii] + DY1*P11[ii];
        }
        /* Initialize each point */
        for( j = 0; j < gm; j++ ) {
            DX1 = j / (hwFloat)(gm-1);
            DX2 = 1.0 - DX1;
            for( jj = 0; jj < vn; jj++ ) {
                Strip[j*vn + jj] = DX2*PT0[jj] + DX1*PT1[jj];
            }
            Strip[j*vn+ 2 ] += Verts[gm*i+j];
        }
        Strip += gm * vn;
    }
}

/*** EOF hwShapes.c ***/
