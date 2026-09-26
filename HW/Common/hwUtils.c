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
#include <string.h>
#include <math.h>
#include "hw.h"
#include "hw_internal.h"
#include "hw_vecmath.h"


#ifndef M_PI
#   define  M_PI    3.141592653589
#endif

#define deg     * (M_PI / 180.0)

/* Define hwGetVertexOffsets */
#include "hwVtxFlags.c"

/* Define hwCalcWPV */
#define HW_CALC_WPV 1
#include "hwVtxFlags.c"
#undef HW_CALC_WPV

static void mkSurfaceNormal
(
    hwUint8 dst[3],
    hwFloat h_00_m1, hwFloat h_00_p1,
    hwFloat h_m1_00, hwFloat h_p1_00
)
{
    float vec_00_p1[3];
    float vec_00_m1[3];
    float vec_p1_00[3];
    float vec_m1_00[3];
    float cp[3];
    float sum[3];

    /* Local coordinate system:
     *
     *             * h_00_p1
     *             | (0,+1)
     *             |
     * h_m1_00 *---*----* h_p1_00
     * (-1,0)      |      (+1,0)
     *             |
     *             * h_00_m1
     *               (0,-1)
     */

    VASSIGN(vec_00_p1,  0.f,  1.f, h_00_p1); VNORM(vec_00_p1, vec_00_p1);
    VASSIGN(vec_00_m1,  0.f, -1.f, h_00_m1); VNORM(vec_00_m1, vec_00_m1);
    VASSIGN(vec_p1_00,  1.f,  0.f, h_p1_00); VNORM(vec_p1_00, vec_p1_00);
    VASSIGN(vec_m1_00, -1.f,  0.f, h_m1_00); VNORM(vec_m1_00, vec_m1_00);

    VASSIGN(sum, 0.f, 0.f, 0.f);
    VCROSS(cp, vec_00_p1, vec_m1_00); VNORM(cp, cp); VADD(sum,sum,cp);
    VCROSS(cp, vec_m1_00, vec_00_m1); VNORM(cp, cp); VADD(sum,sum,cp);
    VCROSS(cp, vec_00_m1, vec_p1_00); VNORM(cp, cp); VADD(sum,sum,cp);
    VCROSS(cp, vec_p1_00, vec_00_p1); VNORM(cp, cp); VADD(sum,sum,cp);
    VNORM(sum, sum);

    dst[0] = (int)((sum[0] + 1.f) * 127.5f + 0.5f);
    dst[1] = (int)((sum[1] + 1.f) * 127.5f + 0.5f);
    dst[2] = (int)((sum[2] + 1.f) * 127.5f + 0.5f);
}

void hwMakeNormalMap8
(
    hwUint8 *dst,        /* Where to put the data - 3 bytes per texel */
    hwUint8 *src,       /* Source data - 1 byte per texel */
    hwInt32 w, hwInt32 h, /* Image size */
    hwInt32 clampW, hwInt32 clampH, /* How to clamp the borders */
    hwFloat bumpScale   /* Scaling ratio */
)
{
    int i, j, i_add_1, j_add_1, i_sub_1, j_sub_1;
    hwFloat h_00_00, h_00_p1, h_p1_00, h_00_m1, h_m1_00;

    bumpScale *= 1.f / 255.f;

    for (i = 0; i < h; i++) {
        if (clampH == HW_TM_CLAMP) {
            i_add_1 = i + 1; if (i_add_1 >= h) i_add_1 = (h - 1);
            i_sub_1 = i - 1; if (i_sub_1 < 0)  i_sub_1 = 0;
        }
        else {
            i_add_1 = (i + 1) % h;
            i_sub_1 = (i + h - 1) % h;
        }

        for (j = 0; j < w; j++) {
            if (clampW == HW_TM_CLAMP) {
                j_add_1 = j + 1; if (j_add_1 >= w) j_add_1 = (w - 1);
                j_sub_1 = j - 1; if (j_sub_1 < 0)  j_sub_1 = 0;
            }
            else {
                j_add_1 = (j + 1) % w;
                j_sub_1 = (j + w - 1) % w;
            }

            // Fetch 5 bumps to compute 4 cross products
            h_00_00 = src[i*w + j]       * bumpScale;
            h_00_p1 = src[i_add_1*w + j] * bumpScale - h_00_00;
            h_00_m1 = src[i_sub_1*w + j] * bumpScale - h_00_00;
            h_p1_00 = src[i*w + j_add_1] * bumpScale - h_00_00;
            h_m1_00 = src[i*w + j_sub_1] * bumpScale - h_00_00;

            // Make the bump into a normal
            mkSurfaceNormal( dst, h_00_m1, h_00_p1, h_m1_00, h_p1_00 );

            // Store this height in the alpha component,
            // useful for parallax mapping
            dst[3] = src[i*w + j];

            // Increment to next pixel
            dst += 4;
        }
    }
}

void hwMakeNormalMap16
(
    hwUint8 *dst,        /* Where to put the data - 3 bytes per texel */
    hwUint16 *src,       /* Source data - 1 short per texel */
    hwInt32 w, hwInt32 h, /* Image size */
    hwInt32 clampW, hwInt32 clampH, /* How to clamp the borders */
    hwFloat bumpScale   /* Scaling ratio */
)
{
    int i, j, i_add_1, j_add_1, i_sub_1, j_sub_1;
    hwFloat h_00_00, h_00_p1, h_p1_00, h_00_m1, h_m1_00;

    bumpScale *= 1.f / 65535.f;

    for (i = 0; i < h; i++) {
        if (clampH == HW_TM_CLAMP) {
            i_add_1 = i + 1; if (i_add_1 >= h) i_add_1 = (h - 1);
            i_sub_1 = i - 1; if (i_sub_1 < 0)  i_sub_1 = 0;
        }
        else {
            i_add_1 = (i + 1) % h;
            i_sub_1 = (i + h - 1) % h;
        }

        for (j = 0; j < w; j++) {
            if (clampW == HW_TM_CLAMP) {
                j_add_1 = j + 1; if (j_add_1 >= w) j_add_1 = (w - 1);
                j_sub_1 = j - 1; if (j_sub_1 < 0)  j_sub_1 = 0;
            }
            else {
                j_add_1 = (j + 1) % w;
                j_sub_1 = (j + w - 1) % w;
            }

            // Fetch 5 bumps to compute 4 cross products
            h_00_00 = src[i*w + j]       * bumpScale;
            h_00_p1 = src[i_add_1*w + j] * bumpScale - h_00_00;
            h_00_m1 = src[i_sub_1*w + j] * bumpScale - h_00_00;
            h_p1_00 = src[i*w + j_add_1] * bumpScale - h_00_00;
            h_m1_00 = src[i*w + j_sub_1] * bumpScale - h_00_00;

            // Make the bump into a normal
            mkSurfaceNormal( dst, h_00_m1, h_00_p1, h_m1_00, h_p1_00 );

            // Store this height in the alpha component,
            // useful for parallax mapping
            dst[3] = src[i*w + j] >> 8;

            // Increment to next pixel
            dst += 4;
        }
    }
}
hwInt32 hwTextureFlags( const hwSurfaceType *surf )
{
    hwInt32
        i, n, st, result,
        type;
    hwObject
        tex;
    void
        *val;

    n = surf->numTextures;
    result = 0;
    st = HW_DATA_ST;
    for( i = 0; i < n; i++, st <<= 3 ) {
        tex = surf->textures[i];
        type = tex->inquire( tex, hwStrCoordMode, &val );
        if( type != HW_TYPE_1I ) continue;

        switch( *(hwInt32 *)val ) {
        case HW_TM_ENVMAP :
        case HW_TM_STAGE_0 :
            /* Don't need tex coords for this */
            break;
        default :
            /* TBD: Somehow figure out if we're doing 3D/4D textures */
            result |= st;
            break;
        }
    }
    return result;
}

void
    *__hwIntVtxTab;

int __hwIntInitVtx( void )
{
    void
        *t;

    HW_HASH_SETUP(__hwIntVtxTab, 48)
        t = __hwIntVtxTab;
        HW_INSERT( hwStrHasW,        HW_DATA_W,        t );
        HW_INSERT( hwStrHasRGB,      HW_DATA_RGB,      t );
        HW_INSERT( hwStrHasAlpha,    HW_DATA_ALPHA,    t );
        HW_INSERT( hwStrHasNormals,  HW_DATA_NORMALS,  t );
        HW_INSERT( hwStrHasTangent,  HW_DATA_TANGENT,  t );
        /* We don't insert HW_DATA_MD_FLAGS, because only polyline uses it */
        HW_INSERT( hwStrHasST0,      HW_DATA_ST,       t );
        HW_INSERT( hwStrHasST1,      HW_DATA_ST1,      t );
        HW_INSERT( hwStrHasST2,      HW_DATA_ST2,      t );
        HW_INSERT( hwStrHasST3,      HW_DATA_ST3,      t );
        HW_INSERT( hwStrHasST4,      HW_DATA_ST4,      t );
        HW_INSERT( hwStrHasST5,      HW_DATA_ST5,      t );
        HW_INSERT( hwStrHasST6,      HW_DATA_ST6,      t );
        HW_INSERT( hwStrHasST7,      HW_DATA_ST7,      t );
        HW_INSERT( hwStrHasR0,       HW_DATA_R,        t );
        HW_INSERT( hwStrHasR1,       HW_DATA_R1,       t );
        HW_INSERT( hwStrHasR2,       HW_DATA_R2,       t );
        HW_INSERT( hwStrHasR3,       HW_DATA_R3,       t );
        HW_INSERT( hwStrHasR4,       HW_DATA_R4,       t );
        HW_INSERT( hwStrHasR5,       HW_DATA_R5,       t );
        HW_INSERT( hwStrHasR6,       HW_DATA_R6,       t );
        HW_INSERT( hwStrHasR7,       HW_DATA_R7,       t );
        HW_INSERT( hwStrHasQ0,       HW_DATA_Q,        t );
        HW_INSERT( hwStrHasQ1,       HW_DATA_Q1,       t );
        HW_INSERT( hwStrHasQ2,       HW_DATA_Q2,       t );
        HW_INSERT( hwStrHasQ3,       HW_DATA_Q3,       t );
        HW_INSERT( hwStrHasQ4,       HW_DATA_Q4,       t );
        HW_INSERT( hwStrHasQ5,       HW_DATA_Q5,       t );
        HW_INSERT( hwStrHasQ6,       HW_DATA_Q6,       t );
        HW_INSERT( hwStrHasQ7,       HW_DATA_Q7,       t );
        HW_INSERT( hwStrHasUV,       HW_DATA_ST,       t );
        /* TBD: Do we really want hwStrFlags? */
    HW_HASH_CLEANUP
    return 1;
}

int __hwIntVtxModify( hwInt32 *flags, hwInt32 prop, hwInt32 type,
                      const void *val )
{
    if( (type & ~HW_TYPE_CLEAN) != HW_TYPE_1B ) {
        __hwIntSetError( HW_ERROR_BAD_TYPE );
        return 0;
    }
    if( *(hwInt32 *)val ) {
        *flags |= prop;
    }
    else {
        *flags &= ~prop;
    }
    return 1;
}

hwInt32 __hwIntVtxInquire( hwInt32 *flags, hwInt32 prop, void **val )
{
    struct __hwDisplayInternal
        *intDisp;
    HW_USE_CURR_DISP;

    intDisp = (struct __hwDisplayInternal *)__hwDisp;
    *val = intDisp->scratchInt;
    if( *flags & prop ) {
        intDisp->scratchInt[0] = 1;
        return HW_TYPE_1B;
    }
    else {
        intDisp->scratchInt[0] = 0;
        return HW_TYPE_1B | HW_TYPE_CLEAN;
    }
}

void hwIdentity( hwFloat Res[4][4] )
{
    hwInt32
        i, j;

    for( i = 0; i < 4; i++ ) {
        for( j = 0; j < 4; j++ ) {
            Res[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

void hwScale( hwFloat Res[4][4], hwFloat sx, hwFloat sy, hwFloat sz )
{
    hwIdentity( Res );
    Res[0][0] = sx;
    Res[1][1] = sy;
    Res[2][2] = sz;
}

void hwRotateX( hwFloat Res[4][4], hwFloat Angle )
{
    hwFloat
        ca, sa;

    ca = cos( Angle deg );
    sa = sin( Angle deg );
    hwIdentity( Res );
    Res[1][1] = ca;
    Res[1][2] = -sa;
    Res[2][1] = sa;
    Res[2][2] = ca;
}


void hwRotateY( hwFloat Res[4][4], hwFloat Angle )
{
    hwFloat
        ca, sa;

    ca = cos( Angle deg );
    sa = sin( Angle deg );
    hwIdentity( Res );
    Res[0][0] = ca;
    Res[0][2] = sa;
    Res[2][0] = -sa;
    Res[2][2] = ca;
}


void hwRotateZ( hwFloat Res[4][4], hwFloat Angle )
{
    hwFloat
        ca, sa;

    ca = cos( Angle deg );
    sa = sin( Angle deg );
    hwIdentity( Res );
    Res[0][0] = ca;
    Res[0][1] = -sa;
    Res[1][0] = sa;
    Res[1][1] = ca;
}

void hwRotateAxis( hwFloat Mat[4][4], hwFloat Angle, const hwFloat XYZ[3] )
{
    hwFloat
        S, C, T,
        X, Y, Z;

    if(         (Angle == 0.0) ||
                ((XYZ[0] == 0.0) && (XYZ[1] == 0.0) && (XYZ[2] == 0.0)) )
    {
        Mat[0][0] = 1.0; Mat[1][0] = 0.0; Mat[2][0] = 0.0;
        Mat[0][1] = 0.0; Mat[1][1] = 1.0; Mat[2][1] = 0.0;
        Mat[0][2] = 0.0; Mat[1][2] = 0.0; Mat[2][2] = 1.0;
    }
    else {
        S = sin(Angle); C = cos(Angle); T = 1.0 - C;
        X = XYZ[0]; Y = XYZ[1]; Z = XYZ[2];
        Mat[0][0] = T*X*X+C;   Mat[1][0] = T*X*Y+S*Z; Mat[2][0] = T*X*Z-S*Y;
        Mat[0][1] = T*X*Y-S*Z; Mat[1][1] = T*Y*Y+C;   Mat[2][1] = T*Y*Z+S*X;
        Mat[0][2] = T*X*Z+S*Y; Mat[1][2] = T*Y*Z-S*X; Mat[2][2] = T*Z*Z+C;
    }

    /* Identity right and bottom... */
    Mat[0][3] = Mat[1][3] = Mat[2][3] = 0.0;
    Mat[3][0] = Mat[3][1] = Mat[3][2] = 0.0;
    Mat[3][3] = 1.0;
}

void hwMatMult( hwFloat Res[4][4], hwFloat A[4][4], hwFloat B[4][4] )
{
    hwInt32
        i, j, k;
    hwFloat
        t;

    for( i = 0; i < 4; i++ ) {
        for( j = 0; j < 4; j++ ) {
            t = 0;
            for( k = 0; k < 4; k++ ) {
                t += A[i][k]*B[k][j];
            }
            Res[i][j] = t;
        }
    }
}

void hwMakeMatrix( const hwOrientType *orient, hwFloat Mat[4][4] )
{
    hwFloat
        MatA[4][4],
        MatB[4][4],
        MatC[4][4];

    hwScale( MatA, orient->scale[0], orient->scale[1], orient->scale[2] );

    hwRotateX( MatB, orient->rotate[0] );
    hwMatMult( MatC, MatA, MatB );

    hwRotateY( MatA, orient->rotate[1] );
    hwMatMult( MatB, MatC, MatA );

    hwRotateZ( MatA, orient->rotate[2] );
    hwMatMult( Mat, MatB, MatA );

    Mat[3][0] = orient->pos[0];
    Mat[3][1] = orient->pos[1];
    Mat[3][2] = orient->pos[2];
}

void hwTransform4d( hwFloat *dst, const hwFloat *src, hwFloat mat[4][4] )
{
    hwFloat
        x, y, z, w;

    x = src[0]*mat[0][0]+src[1]*mat[1][0]+src[2]*mat[2][0]+src[3]*mat[3][0];
    y = src[0]*mat[0][1]+src[1]*mat[1][1]+src[2]*mat[2][1]+src[3]*mat[3][1];
    z = src[0]*mat[0][2]+src[1]*mat[1][2]+src[2]*mat[2][2]+src[3]*mat[3][2];
    w = src[0]*mat[0][3]+src[1]*mat[1][3]+src[2]*mat[2][3]+src[3]*mat[3][3];
    dst[0] = x; dst[1] = y; dst[2] = z; dst[3] = w;
}

void hwTransformPersp( hwFloat *dst, const hwFloat *src, hwFloat mat[4][4] )
{
    hwFloat
        x, y, z, w;

    x = src[0]*mat[0][0] + src[1]*mat[1][0] + src[2]*mat[2][0] + mat[3][0];
    y = src[0]*mat[0][1] + src[1]*mat[1][1] + src[2]*mat[2][1] + mat[3][1];
    z = src[0]*mat[0][2] + src[1]*mat[1][2] + src[2]*mat[2][2] + mat[3][2];
    w = src[0]*mat[0][3] + src[1]*mat[1][3] + src[2]*mat[2][3] + mat[3][3];
    if( w != 0.f ) w = 1.f / w;
    dst[0] = x * w; dst[1] = y * w; dst[2] = z * w;
}

void hwTransform( hwFloat Mat[4][4], hwFloat Vect[3] )
{
    hwFloat
        a, b, c;

    a = Vect[0]; b = Vect[1]; c = Vect[2];
    Vect[0] = a*Mat[0][0] + b*Mat[1][0] + c*Mat[2][0] + Mat[3][0];
    Vect[1] = a*Mat[0][1] + b*Mat[1][1] + c*Mat[2][1] + Mat[3][1];
    Vect[2] = a*Mat[0][2] + b*Mat[1][2] + c*Mat[2][2] + Mat[3][2];
}


void __hwIntTextureData
(
    hwImageStruct **texList, hwInt32 numTex,
    hwInt32 Replace, hwInt32 Clamp,
    hwFloat *Data, hwInt32 n, hwInt32 Flags
)
{
    hwInt32
        i, j, k, tm, aa, x, y, xx, yy, tmo, tmn,
        WPV;
    hwVertexOffsets
        Offs;
    hwFloat
        tr, tg, tb, ta, denom,
        *ptr;
    unsigned char
        *Img;
    hwInt32
        W, H, Comp;

    if( !n )    return;
    if( !(Flags & HW_DATA_RGB) )        return;
    if( !(Flags & HW_DATA_TEXCOORD) )   return;

    WPV = hwGetVertexOffsets( Flags, &Offs );

    for( tm = 0; tm < numTex; tm++ ) {
        if( !(tmo = Offs.texCoord[tm]) ) continue;
        tmn = Offs.texSize[tm];

        Img = texList[tm]->data;
        W = texList[tm]->width;
        H = texList[tm]->height;
        Comp = texList[tm]->components;

        /* How many pixels to contribute... */
        aa = sqrt( (W*H) / (double)n ) * 0.5 + 0.5;
        if( aa < 0 ) aa = 0;
        if( aa > 2 ) aa = 2;
        denom = 1.0 / (double)(4*aa*aa + 4*aa + 1);

        if( !Img || !W || !H )  continue;

        ptr = Data;
        for( i = 0; i < n; i++ ) {
            x = ptr[tmo+0] * W + 0.5;
            y = ptr[tmo+1] * H + 0.5;
            tr = tg = tb = ta = 0.0;
            if( (Comp == 1) || (Comp == 3) ) {
                ta = 1.0 / denom;
            }
            for( j = -aa; j <= aa; j++ ) {
                yy = y + j;
                if( Clamp ) {
                    if( yy < 0 )        yy = 0;
                    if( yy >= H )       yy = H-1;
                }
                else {
                    if( yy < 0 )        yy = (H-1) - (-yy) % H;
                    else                yy = yy % H;
                }
                for( k = -aa; k <= aa; k++ ) {
                    xx = x + k;
                    if( Clamp ) {
                        if( xx < 0 )    xx = 0;
                        if( xx >= W )   xx = W-1;
                    }
                    else {
                        if( xx < 0 )    xx = (W-1) - (-xx) % W;
                        else            xx = xx % W;
                    }
                    if( Comp < 3 ) {
                        /* Monochrome image */
                        tr += Img[Comp*(W*yy+xx)+0] * (1.0 / 255.0);
                        tg += Img[Comp*(W*yy+xx)+0] * (1.0 / 255.0);
                        tb += Img[Comp*(W*yy+xx)+0] * (1.0 / 255.0);
                        if( Comp == 2 ) {
                            ta += Img[Comp*(W*yy+xx)+1] * (1.0 / 255.0);
                        }
                    }
                    else {
                        /* Color image */
                        tr += Img[Comp*(W*yy+xx)+0] * (1.0 / 255.0);
                        tg += Img[Comp*(W*yy+xx)+1] * (1.0 / 255.0);
                        tb += Img[Comp*(W*yy+xx)+2] * (1.0 / 255.0);
                        if( Comp == 4 ) {
                            ta += Img[Comp*(W*yy+xx)+3] * (1.0 / 255.0);
                        }
                    }
                }
            }
            tr *= denom; tg *= denom;
            tb *= denom; ta *= denom;
            switch( Replace ) {
            case HW_TM_MODULATE :
                ptr[Offs.rgb+0] *= tr;
                ptr[Offs.rgb+1] *= tg;
                ptr[Offs.rgb+2] *= tb;
                if( Offs.alpha ) {
                    ptr[Offs.alpha] = ta;
                }
                break;
            case HW_TM_BUMP :
            case HW_TM_GLOSS :
                /* TBD */
                break;
            }
            ptr += WPV;
        }
    }
}

void __hwIntCalcTangentSpace
(
    hwFloat tangent[4],
    const hwFloat normal[],
    const hwFloat v1[], const hwFloat v2[], const hwFloat v3[],
    const hwFloat t1[], const hwFloat t2[], const hwFloat t3[]
)
{
    hwFloat binormal[3];
    hwFloat tNormal[3];
    hwFloat v2v1[3], v3v1[3];
    hwFloat t2t1[2], t3t1[2];

    /* Get delta vectors */
    VSUB(v2v1, v2, v1);
    VSUB(v3v1, v3, v1);
    t2t1[0] = t2[0]-t1[0]; t2t1[1] = t2[1]-t1[1];
    t3t1[0] = t3[0]-t1[0]; t3t1[1] = t3[1]-t1[1];

    VASSIGN(tangent,     t3t1[1] * v2v1[0] - t2t1[1] * v3v1[0],
                         t3t1[1] * v2v1[1] - t2t1[1] * v3v1[1],
                         t3t1[1] * v2v1[2] - t2t1[1] * v3v1[2]);
    VNORM(tangent, tangent);

    VASSIGN(binormal,   -t3t1[0] * v2v1[0] + t2t1[0] * v3v1[0],
                        -t3t1[0] * v2v1[1] + t2t1[0] * v3v1[1],
                        -t3t1[0] * v2v1[2] + t2t1[0] * v3v1[2]);
    VCROSS(tNormal, tangent, binormal);
    if (VDOT(normal, tNormal) < 0.) {
        tangent[3] = -1.;
    }
    else {
        tangent[3] = 1.;
    }
}

void __hwIntSmoothMesh
(
    hwFloat *Data, hwInt32 n, hwInt32 m, hwInt32 Flags
)
{
#define EPSILON 0.01
    hwInt32
        vn, NormalOffs,
        WrapRows, WrapCols,
        i, j;
    hwFloat
        *Normals, *ptr,
        DX1, DY1, DZ1,
        DX2, DY2, DZ2,
        DD, NX, NY, NZ;
    hwInt32
        i0, i1, j0, j1,
        ii, jj, nn, mm;

    vn = 3;
    NormalOffs = 0;
    if( Flags & HW_DATA_RGB )   vn += 3;
    if( Flags & HW_DATA_ALPHA ) vn += 1;
    if( Flags & HW_DATA_NORMALS ) {
        NormalOffs = vn;
    }

    vn = hwCalcWPV( Flags );

    if( !NormalOffs )   return;

    ptr = Normals = malloc( m * n * 3 * sizeof(hwFloat) );
    if( !Normals ) return;

    WrapRows = WrapCols = 1;
    mm = m * vn;
    for( i = 0; i < n; i++ ) {
        DX1 = Data[i*mm  ] - Data[(i+1)*mm-vn  ];
        DY1 = Data[i*mm+1] - Data[(i+1)*mm-vn+1];
        DZ1 = Data[i*mm+2] - Data[(i+1)*mm-vn+2];
        DD = DX1*DX1 + DY1*DY1 + DZ1*DZ1;
        if( DD > EPSILON ) {
            WrapCols = 0;
            break;
        }
    }
    nn = mm*(n-1);
    for( j = 0; j < m; j++ ) {
        DX1 = Data[j*vn  ] - Data[nn+j*vn  ];
        DY1 = Data[j*vn+1] - Data[nn+j*vn+1];
        DZ1 = Data[j*vn+2] - Data[nn+j*vn+2];
        DD = DX1*DX1 + DY1*DY1 + DZ1*DZ1;
        if( DD > EPSILON ) {
            WrapRows = 0;
            break;
        }
    }

    nn = n * m * vn;
    mm = m * vn;
    for( i = mm; i < nn; i += mm ) {
        ii = i - mm;
        for( j = vn; j < mm; j += vn ) {
            jj = j - vn;
            DX1 = Data[ii + jj     ] - Data[i + j     ];
            DY1 = Data[ii + jj + 1 ] - Data[i + j + 1 ];
            DZ1 = Data[ii + jj + 2 ] - Data[i + j + 2 ];
            DD = sqrt(DX1*DX1 + DY1*DY1 + DZ1*DZ1 );
            DX1 /= DD; DY1 /= DD; DZ1 /= DD;

            DX2 = Data[i + jj     ] - Data[ii + j     ];
            DY2 = Data[i + jj + 1 ] - Data[ii + j + 1 ];
            DZ2 = Data[i + jj + 2 ] - Data[ii + j + 2 ];
            DD = sqrt(DX2*DX2 + DY2*DY2 + DZ2*DZ2 );
            DX2 /= DD; DY2 /= DD; DZ2 /= DD;

            NX = DY1*DZ2 - DY2*DZ1;
            NY = DZ1*DX2 - DZ2*DX1;
            NZ = DX1*DY2 - DX2*DY1;
            DD = sqrt( NX*NX + NY*NY + NZ*NZ );
            NX /= DD; NY /= DD; NZ /= DD;

            *ptr++ = NX; *ptr++ = NY; *ptr++ = NZ;
        }
    }

    /* OK.  We've created all of the facet normals.
     * Average the facet normals at each vertex to make
     * the vertex normal.
     */

    ptr = Data;
    mm = m-1;
    nn = n-1;
    for( i = 0; i < n; i++ ) {
        if( WrapRows ) {
            i0 = (i + nn - 1) % nn;
            i1 = i % nn;
        }
        else {
            i0 = i - 1;
            i1 = i;
        }

        for( j = 0; j < m; j++ ) {
            NX = NY = NZ = 0.0;

            if( WrapCols ) {
                j0 = (j + mm - 1) % mm;
                j1 = j % mm;
            }
            else {
                j0 = j - 1;
                j1 = j;
            }

            if( (i0 >= 0) && (j0 >= 0) ) {
                NX += Normals[3*(i0*mm+j0)+0];
                NY += Normals[3*(i0*mm+j0)+1];
                NZ += Normals[3*(i0*mm+j0)+2];
            }

            if( (i0 >= 0) && (j1 < mm) ) {
                NX += Normals[3*(i0*mm+j1)+0];
                NY += Normals[3*(i0*mm+j1)+1];
                NZ += Normals[3*(i0*mm+j1)+2];
            }

            if( (i1 < nn) && (j0 >= 0) ) {
                NX += Normals[3*(i1*mm+j0)+0];
                NY += Normals[3*(i1*mm+j0)+1];
                NZ += Normals[3*(i1*mm+j0)+2];
            }

            if( (i1 < nn) && (j1 < mm) ) {
                NX += Normals[3*(i1*mm+j1)+0];
                NY += Normals[3*(i1*mm+j1)+1];
                NZ += Normals[3*(i1*mm+j1)+2];
            }

            DD = sqrt(NX*NX + NY*NY + NZ*NZ);
            if( DD > 0.0 )      DD = 1.0 / DD;

            ptr[NormalOffs+0] = NX * DD;
            ptr[NormalOffs+1] = NY * DD;
            ptr[NormalOffs+2] = NZ * DD;
            ptr += vn;
        }
    }
    free( Normals );
#undef EPSILON
}


void __hwIntOrientData
(
    const hwOrientType *orient, hwFloat *Data,
    hwInt32 n, hwInt32 Flags, hwInt32 Which
)
{
    hwFloat
        Mat[4][4],
        xyz[3];
    hwVertexOffsets
        Offs;
    hwInt32
        i, wpv,
        txo, txn;

    wpv = hwGetVertexOffsets( Flags, &Offs );

    if( !(txo = Offs.texCoord[Which]) ) return;
    txn = Offs.texSize[Which];

    hwMakeMatrix( orient, Mat );

    if( txn > 3 ) {
        for( i = 0; i < n; i++ ) {
            hwTransform4d( Data+txo, Data+txo, Mat );
            Data += wpv;
        }
    }
    else if( txn > 2 ) {
        for( i = 0; i < n; i++ ) {
            hwTransform( Mat, Data+txo );
            Data += wpv;
        }
    }
    else {
        for( i = 0; i < n; i++ ) {
            xyz[0] = Data[txo  ];
            xyz[1] = Data[txo+1];
            xyz[2] = 0.0;

            hwTransform( Mat, xyz );

            Data[txo  ] = xyz[0];
            Data[txo+1] = xyz[1];

            Data += wpv;
        }
    }
}


void __hwIntMatXformData
(
    hwFloat     Mat[4][4],
    hwFloat     *Data,
    hwInt32     n,
    hwInt32     Flags
)
{
    hwFloat
        IXpose[4][4],
        x, y, z, d;
    hwInt32
        i, ofx, wpv;
    hwVertexOffsets
        Offs;

    wpv = hwGetVertexOffsets( Flags, &Offs );

    // Construct inverse transpose if we need it
    if( Flags & (HW_DATA_NORMALS | HW_DATA_TANGENT) ) {
        hwInvertMat( Mat, IXpose );
        x = IXpose[0][1]; IXpose[0][1] = IXpose[1][0]; IXpose[1][0] = x;
        x = IXpose[0][2]; IXpose[0][2] = IXpose[2][0]; IXpose[2][0] = x;
        x = IXpose[1][2]; IXpose[1][2] = IXpose[2][1]; IXpose[2][1] = x;
        IXpose[0][3] = IXpose[1][3] = IXpose[2][3] = 0.f;
        IXpose[3][0] = IXpose[3][1] = IXpose[3][2] = 0.f;
        IXpose[3][3] = 1.f;
    }

    for( i = 0; i < n; i++ ) {
        hwTransform( Mat, Data );

        if( (ofx = Offs.normal) ) {
            hwTransform( IXpose, Data + ofx );
            x = Data[ofx+0];
            y = Data[ofx+1];
            z = Data[ofx+2];
            d = sqrt( x*x + y*y + z*z );
            if( d != 0.0 )      d = 1.0 / d;
            Data[ofx+0] = x * d;
            Data[ofx+1] = y * d;
            Data[ofx+2] = z * d;
        }
        if( (ofx = Offs.tangent) ) {
            hwTransform( IXpose, Data + ofx );
            x = Data[ofx+0];
            y = Data[ofx+1];
            z = Data[ofx+2];
            d = sqrt( x*x + y*y + z*z );
            if( d != 0.0 )      d = 1.0 / d;
            Data[ofx+0] = x * d;
            Data[ofx+1] = y * d;
            Data[ofx+2] = z * d;
        }
        Data += wpv;
    }
}

void __hwIntTransformData
(
    const hwOrientType *orient, hwFloat *Data,
    hwInt32 n, hwInt32 Flags
)
{
    hwFloat
        Mat[4][4];

    hwMakeMatrix( orient, Mat );
    __hwIntMatXformData( Mat, Data, n, Flags );
}

hwInt32 hwInvertMat( hwFloat Mat[4][4], hwFloat Dst[4][4] )
{
#define MO      4
#define EPSILON 0.01
    hwFloat
        Tmp,
        Src[MO][MO];
    hwInt32
        i, j, k;

    /* Copy matrix (since the original is destroyed) */
    (void)memcpy( Src, Mat, MO*MO*sizeof(hwFloat) );

    /* Create identity matrix */
    for( i = 0; i < MO; i++ ) {
        for( j = 0; j < MO; j++ ) {
            Dst[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }

    /* Solve each row */
    for( i = 0; i < MO; i++ ) {
        Tmp = Src[i][i];
        if( Tmp < 0 )   Tmp = -Tmp;
        if( Tmp < EPSILON ) {
            /* Oops - we need to swap some */
            for( j = i+1; j < MO; j++ ) {
                Tmp = Src[j][i];
                if( Tmp < 0 )   Tmp = -Tmp;
                if( Tmp > EPSILON ) {
                    /* Found a valid row */
                    break;
                }
            }
            if( j >= MO ) {
                /* Matrix cannot be inverted */
                return 0;
            }
            /* Swap rows i and j */
            for( k = 0; k < MO; k++ ) {
                Tmp = Src[i][k]; Src[i][k] = Src[j][k]; Src[j][k] = Tmp;
                Tmp = Dst[i][k]; Dst[i][k] = Dst[j][k]; Dst[j][k] = Tmp;
            }
        }

        /* OK, now scale to 1.0 */
        Tmp = 1.0 / Src[i][i];
        Src[i][i] = 1.0;
        for( k = i+1; k < MO; k++ ) {
            Src[i][k] *= Tmp;
        }
        for( k = 0; k < MO; k++ ) {
            Dst[i][k] *= Tmp;
        }

        /* Now, transform all the rest of the rows so this column is 0 */
        for( j = 0; j < MO; j++ ) {
            if( i == j )        continue;
            Tmp = Src[j][i];
            for( k = 0; k < MO; k++ ) {
                Src[j][k] -= Tmp * Src[i][k];
                Dst[j][k] -= Tmp * Dst[i][k];
            }
        }
    }

    return 1;
#undef  MO
#undef EPSILON
}

void __hwIntSphereGen
(
    const hwOrientType *orient, hwFloat *Data,
    hwInt32 n, hwInt32 Flags, hwInt32 Which
)
{
    hwFloat
        XYZ[3],
        Mat[4][4], IMat[4][4],
        u, v;
    hwOrientType
        rotOnly;
    hwInt32
        i, wpv, txo, txn;
    hwVertexOffsets
        Offs;
    double
        dx, dy, dz, dd;

    wpv = hwGetVertexOffsets( Flags, &Offs );

    if( !(txo = Offs.texCoord[Which]) ) return;
    txn = Offs.texSize[Which];

    /* Set pos/scale to normal */
    rotOnly.scale[0] = rotOnly.scale[1] = rotOnly.scale[2] = 1.0;
    rotOnly.pos[0] = rotOnly.pos[1] = rotOnly.pos[2] = 0.0;

    /* Copy rotation */
    rotOnly.rotate[0] = orient->rotate[0];
    rotOnly.rotate[1] = orient->rotate[1];
    rotOnly.rotate[2] = orient->rotate[2];

    /* Make the rotation matrix and its inverse */
    hwMakeMatrix( &rotOnly, Mat );
    if( !hwInvertMat( Mat, IMat ) ) {
        hwIdentity( IMat );
    }

    for( i = 0; i < n; i++ ) {
        /* Transform point into rotated space */
        XYZ[0] = Data[0] - orient->pos[0];
        XYZ[1] = Data[1] - orient->pos[1];
        XYZ[2] = Data[2] - orient->pos[2];
        hwTransform( IMat, XYZ );

        /* Normalize vector to this point */
        dx = XYZ[0]; dy = XYZ[1]; dz = XYZ[2];
        dd = sqrt( dx*dx + dy*dy + dz*dz );
        if( dd > 0.0 )  dd = 1.0 / dd;
        dx *= dd; dy *= dd; dz *= dd;

        /* Extract spherical coordinates */
        u = 0.5 + atan2( dy, dx ) * (1.0/(2.0*M_PI));
        v = 0.5*(1.0 + dz); 

        Data[txo  ] = u * orient->scale[0];
        Data[txo+1] = v * orient->scale[1];
        if( txn > 2 ) Data[txo+2] = 0.0f;
        if( txn > 3 ) Data[txo+3] = 1.0f;

        Data += wpv;
    }
}

void __hwIntCylGen
(
    const hwOrientType *orient, hwFloat *Data,
    hwInt32 n, hwInt32 Flags, hwInt32 Which
)
{
    hwFloat
        XYZ[3],
        Mat[4][4], IMat[4][4],
        u, v;
    hwOrientType
        rotOnly;
    hwInt32
        i, wpv, txo, txn;
    hwVertexOffsets
        Offs;
    double
        UScale, VScale,
        dx, dy, dz, dd;

    wpv = hwGetVertexOffsets( Flags, &Offs );

    if( !(txo = Offs.texCoord[Which]) ) return;
    txn = Offs.texSize[Which];

    /* Set pos/scale to normal */
    rotOnly.scale[0] = rotOnly.scale[1] = rotOnly.scale[2] = 1.0;
    rotOnly.pos[0] = rotOnly.pos[1] = rotOnly.pos[2] = 0.0;

    /* Copy rotation */
    rotOnly.rotate[0] = orient->rotate[0];
    rotOnly.rotate[1] = orient->rotate[1];
    rotOnly.rotate[2] = orient->rotate[2];

    /* Make the rotation matrix and its inverse */
    hwMakeMatrix( &rotOnly, Mat );
    if( !hwInvertMat( Mat, IMat ) ) {
        hwIdentity( IMat );
    }

    UScale = orient->scale[0]; if( UScale > 0.0 ) UScale = 1.0 / UScale;
    VScale = orient->scale[1]; if( VScale > 0.0 ) VScale = 1.0 / VScale;

    for( i = 0; i < n; i++ ) {
        /* Transform point into rotated space */
        XYZ[0] = Data[0] - orient->pos[0];
        XYZ[1] = Data[1] - orient->pos[1];
        XYZ[2] = Data[2] - orient->pos[2];
        hwTransform( IMat, XYZ );

        /* Normalize vector to this point */
        dx = XYZ[0]; dy = XYZ[1]; dz = XYZ[2];
        dd = sqrt( dx*dx + dy*dy );
        if( dd > 0.0 )  dd = 1.0 / dd;
        dx *= dd; dy *= dd;

        /* Extract cylindrical coordinates */
        u = 0.5 + atan2( dy, dx ) * (1.0/(2.0*M_PI));
        v = dz;

        Data[txo  ] = u * UScale;
        Data[txo+1] = v * VScale;
        if( txn > 2 ) Data[txo+2] = 0.f;
        if( txn > 3 ) Data[txo+3] = 1.f;

        Data += wpv;
    }
}

void __hwIntPlaneGen
(
    const hwOrientType *orient, hwFloat *userData,
    hwInt32 n, hwInt32 Flags, hwInt32 Which
)
{
#define EPSILON 0.01
    const hwFloat
        *Pos;           /* Position... */
    hwFloat
        *Data = userData,
        Mat[4][4],      /* The matrix */
        IMat[4][4],     /* Inverse of this matrix */
        pa, pb, pc, pd, /* The plane equation represented by above mat */
        det,            /* Determinant of ray/plane intersection */
        xyz[3],         /* Point to be transformed */
        minS, minT,     /* Minimum S,T coords */
        s0, t0, ds, dt, /* Beginning s, t coords, delta to add */
        t;              /* Parametric distance along normal */
    hwInt32
        i, wpv, txo, txn;
    hwVertexOffsets
        Offs;

    wpv = hwGetVertexOffsets( Flags, &Offs );

    if( !(txo = Offs.texCoord[Which]) ) return;
    txn = Offs.texSize[Which];

    /* Figure out plane equation */
    hwMakeMatrix( orient, Mat );
    if( !hwInvertMat( Mat, IMat ) )     return;

    Pos = orient->pos;
    pa = Mat[2][0];
    pb = Mat[2][1];
    pc = Mat[2][2];
    pd = -(Pos[0]*pa + Pos[1]*pb + Pos[2]*pc);
    det = pa*pa + pb*pb + pc*pc;
    if( det < EPSILON ) return;
    det = 1.0 / det;

    minS = minT = 1e38;
    for( i = 0; i < n; i++ ) {
        /* Get ray/plane intersection point */
        t = -(pa*Data[0]+pb*Data[1]+pc*Data[2]+pd) * det;
        xyz[0] = Data[0] + t*pa;
        xyz[1] = Data[1] + t*pb;
        xyz[2] = Data[2] + t*pc;

        /* Back transform to proper space */
        hwTransform( IMat, xyz );

        Data[txo  ] = xyz[0];
        Data[txo+1] = xyz[1];
        if( txn > 2 ) Data[txo+2] = xyz[2];
        if( txn > 3 ) Data[txo+3] = 1.f;

        if( Data[txo  ] < minS )   minS = Data[txo  ];
        if( Data[txo+1] < minT )   minT = Data[txo+1];

        Data += wpv;
    }

    /* Now fudge texture coordinates to get them to as close to [0..1]
     * as practical
     */
    s0 = fmod( minS, 1.0 );     t0 = fmod( minT, 1.0 );
    if( s0 < 0.f ) s0 += 1.f;   if( t0 < 0.f ) t0 += 1.f;

    ds = minS - s0;             dt = minT - t0;
    Data = userData;
    for( i = 0; i < n; i++ ) {
        Data[txo  ] -= ds;
        Data[txo+1] -= dt;
        Data += wpv;
    }
#undef EPSILON
}


/* Removes UV data from an array (can also be called to remove
 * other data from the end of an array)
 */
void __hwIntRemoveUV( hwInt32 vn, hwInt32 n, hwFloat *Data, hwInt32 numCoord )
{
    hwInt32
        i, vn2;

    vn2 = vn - numCoord;
    for( i = 0; i < n; i++ ) {
        (void)memcpy( Data+i*vn2, Data+i*vn, vn2*sizeof(hwFloat) );
    }
}

/* Adds RGB to an array */
hwFloat *__hwIntAddRGB
(
    hwInt32 vn, hwInt32 n, const hwFloat Color[3],
    hwFloat *Data, hwInt32 *dataSize, hwInt32 addAlpha
)
{
    hwFloat
        *Src, *Dst;
    hwInt32
        vn3, i, j;

    if( addAlpha ) {
        vn3 = vn + 4;
    }
    else {
        vn3 = vn + 3;
    }
    if( (vn3 * n) > *dataSize ) {
        Data = realloc( Data, vn3 * n * sizeof(hwFloat) );
        if( !Data )     return 0;
        *dataSize = vn3 * n;
    }

    /* Start at end of buffer */
    Src = Data + (n - 1) * vn;
    Dst = Data + (n - 1) * vn3;

    if( addAlpha ) {
        /* RGBA */
        for( i = 0; i < n; i++ ) {
            for( j = vn3-1; j >= 7; j-- ) {
                Dst[j] = Src[j-4];
            }
            Dst[6] = 1.0;
            Dst[5] = Color[2];
            Dst[4] = Color[1];
            Dst[3] = Color[0];
            for( j = 2; j >= 0; j-- ) {
                Dst[j] = Src[j];
            }
            Src -= vn;
            Dst -= vn3;
        }
    }
    else {
        /* RGB */
        for( i = 0; i < n; i++ ) {
            for( j = vn3-1; j >= 6; j-- ) {
                Dst[j] = Src[j-3];
            }
            Dst[5] = Color[2];
            Dst[4] = Color[1];
            Dst[3] = Color[0];
            for( j = 2; j >= 0; j-- ) {
                Dst[j] = Src[j];
            }
            Src -= vn;
            Dst -= vn3;
        }
    }

    return Data;
}

/* Adds UV to an array */
hwFloat *__hwIntAddUV( hwInt32 vn, hwInt32 n, hwFloat *Data, hwInt32 *dataSize )
{
    hwFloat
        *Src, *Dst;
    hwInt32
        vn2, i, j;

    vn2 = vn + 2;
    if( (vn2 * n) > *dataSize ) {
        Data = realloc( Data, vn2 * n * sizeof(hwFloat) );
        if( !Data )     return 0;
        *dataSize = vn2 * n;
    }

    /* Start at end of buffer */
    Src = Data + (n - 1) * vn;
    Dst = Data + (n - 1) * vn2;

    for( i = 0; i < n; i++ ) {
        Dst[vn2-2] = 0.0;
        Dst[vn2-1] = 0.0;
        for( j = vn - 1; j >= 0; j-- ) {
            Dst[j] = Src[j];
        }
        Src -= vn;
        Dst -= vn2;
    }
    return Data;
}

void __hwIntMeshInterp
(
    hwFloat *data, hwInt32 n, hwInt32 m,
    hwFloat **result, hwInt32 newN, hwInt32 newM,
    hwInt32 flags
)
{
    hwInt32
        i, j, ii, jj, k, WPV;
    hwFloat
        *v1, *v2, *v3, *v4,
        tmp1, tmp2,
        fi, fj,
        *newData;

    WPV = hwCalcWPV( flags );

    newData = *result;

    for( i = 0; i < newN; i++ ) {
        fi = (n - 1) * i / (hwFloat)(newN - 1);
        ii = (int)fi;   fi -= (hwFloat)ii;
        if( ii >= (n - 1) ) { ii = n - 2; fi = 1.0f; }

        for( j = 0; j < newM; j++ ) {
            fj = (m - 1) * j / (hwFloat)(newM - 1);
            jj = (int)fj;       fj -= (hwFloat)jj;
            if( jj >= (m - 1) ) { jj = m - 2; fj = 1.0f; }

            v1 = data + WPV*((ii  )*m+jj  );
            v2 = data + WPV*((ii  )*m+jj+1);
            v3 = data + WPV*((ii+1)*m+jj  );
            v4 = data + WPV*((ii+1)*m+jj+1);

            for( k = 0; k < WPV; k++ ) {
                tmp1 = v1[k] * (1.f - fi) + v3[k] * fi;
                tmp2 = v2[k] * (1.f - fi) + v4[k] * fi;
                newData[k] = tmp1 * (1.f - fj) + tmp2 * fj;
            }
            newData += WPV;
        }
    }
}

void __hwIntUpdateBounds( hwFloat Bounds[6],
                          const hwFloat *Data, hwInt32 n, hwInt32 vn )
{
    while( n-- ) {
        if( Data[0] < Bounds[0] )       Bounds[0] = Data[0];
        if( Data[1] < Bounds[1] )       Bounds[1] = Data[1];
        if( Data[2] < Bounds[2] )       Bounds[2] = Data[2];

        if( Data[0] > Bounds[3] )       Bounds[3] = Data[0];
        if( Data[1] > Bounds[4] )       Bounds[4] = Data[1];
        if( Data[2] > Bounds[5] )       Bounds[5] = Data[2];

        Data += vn;
    }
}

void __hwIntSetError( hwInt32 err )
{
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;

    intDisp->hwErrNo = err;
#if 0
    printf("HoverWare Error %d: ", err);
    switch(err) {
    case HW_ERROR_NO_MEMORY:
        printf("No Memory\n");
        break;
    case HW_ERROR_BAD_PROP:
        printf("Bad Property\n");
        break;
    case HW_ERROR_BAD_TYPE:
        printf("Bad Type\n");
        break;
    case HW_ERROR_INTERNAL:
        printf("Internal\n");
        break;
    default:
        printf("unknown\n");
        break;
    }
#endif
}

hwInt32 hwGetError( void )
{
    hwInt32
        result;
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;

    result = intDisp->hwErrNo;
    intDisp->hwErrNo = 0;

    return result;
}

hwInt32 __hwIntGuiColor( hwInt32 type, const void *val )
{
    hwInt32
        r, g, b, a;
    const hwFloat
        *fp;

    switch( type ) {
    case HW_TYPE_1I :
        return ((hwInt32 *)val)[0];
    case HW_TYPE_3F :
        fp = (hwFloat *)val;
        r = (fp[0] < 0.f) ? 0 : ((fp[0] > 1.f) ? 0xFF : (hwInt32)(255.*fp[0]));
        g = (fp[1] < 0.f) ? 0 : ((fp[1] > 1.f) ? 0xFF : (hwInt32)(255.*fp[1]));
        b = (fp[2] < 0.f) ? 0 : ((fp[2] > 1.f) ? 0xFF : (hwInt32)(255.*fp[2]));
        a = 0xFF;
        return (a << 24) | (r << 16) | (g << 8) | b;
    case HW_TYPE_4F :
        fp = (hwFloat *)val;
        r = (fp[0] < 0.f) ? 0 : ((fp[0] > 1.f) ? 0xFF : (hwInt32)(255.*fp[0]));
        g = (fp[1] < 0.f) ? 0 : ((fp[1] > 1.f) ? 0xFF : (hwInt32)(255.*fp[1]));
        b = (fp[2] < 0.f) ? 0 : ((fp[2] > 1.f) ? 0xFF : (hwInt32)(255.*fp[2]));
        a = (fp[3] < 0.f) ? 0 : ((fp[3] > 1.f) ? 0xFF : (hwInt32)(255.*fp[3]));
        return (a << 24) | (r << 16) | (g << 8) | b;
    default :
        __hwIntSetError( HW_ERROR_BAD_TYPE );
        break;
    }
    return 0;
}

void hwDefaultSurf( hwSurfaceType *surf )
{
    LOG_ENTRY
    surf->color[0] = surf->color[1] = surf->color[2] = 1.0;
    surf->transp = 0.0;
    surf->specColor[0] = surf->specColor[1] = surf->specColor[2] = 1.0;
    surf->shininess = 0.0;
    surf->primSize = 1.0;
    surf->visibility = 0xFFFFFFFF;
    surf->flags = 0;
    surf->numTextures = 0;
    LOG_EXIT
}

#if defined(WIN32) && !defined(CYGWIN) && !defined(MINGW)
/* HACK! Win32 doesn't seem to have isnan() */
#define isnan(a)        0
#endif

void __hwIntVecToMat
(
    const hwFloat UpVec[3],
    const hwFloat DirVec[3],
    hwFloat Result[4][4]
)
{
#define EPSILON         0.0001
#define EPSILON2        0.000001

    hwFloat
        Ang, Len,
        TmpUp[3],
        Right[3], Basis[3], Mat[4][4], Mat1[4][4],
        Front[3], Up[3];

    Front[0] = 1.0; Front[1] = 0.0; Front[2] = 0.0;

    /* Normalize vectors: unit, perpendicular to each other */
    VCROSS( Right, UpVec, DirVec ); VNORM( Right, Right );
    VCROSS( TmpUp, DirVec, Right ); VNORM( TmpUp, TmpUp );

    /* Get rotation of Direction vector */
    VCROSS( Basis, DirVec, Front );
    Len = VLEN2( Basis );
    if( Len < EPSILON2 ) {
        Len = VDOT( DirVec, Front );
        if( Len < 0.0 ) Ang = M_PI;
        else            Ang = 0.0;
        VCOPY( Basis, TmpUp );
    }
    else {
        VNORM( Basis, Basis );
        Ang = DirVec[0];
        if( isnan( Ang ) )              Ang = 0.;
        else if( Ang > 1.0 )    Ang = 1.0;
        else if( Ang < -1.0 )   Ang = -1.0;
        Ang = acos( Ang );
    }
    hwRotateAxis( Mat, Ang, Basis );

    /* Get rotation of Up vector */
    Up[0] = 0.0; Up[1] = 1.0; Up[2] = 0.0;
    VXFORM3( Up, Up, Mat );

    /* Dot product = cos of angle... */
    Ang = VDOT( TmpUp,  Up );
    if( isnan( Ang ) )          Ang = 0.;
    else if( Ang > 1.0 )        Ang = 1.0;
    else if( Ang < -1.0 )       Ang = -1.0;

    /* Cross product gives us basis for rotation */
    VCROSS( Basis, TmpUp, Up );
    if( VLEN2( Basis ) < EPSILON2 ) {
        /* Oops - might be parallel */
        if( Ang < 0.0 )         Ang = M_PI;
        else                    Ang = 0.0;
        VCOPY( Basis, DirVec );
    }
    else {
        VNORM( Basis, Basis );
        Ang = acos( Ang );
    }
    hwRotateAxis( Mat1, Ang, Basis );
    hwMatMult( Result, Mat, Mat1 );
#undef EPSILON
#undef EPSILON2
}

void hwSurfAttrs(hwSurfaceType *surf)
{
    int i;
    hwInt32 *ptr;
    HW_USE_CURR_DISP;

    if (!(surf->flags & HW_SURF_TEX_SAMP)) {
        for (i = 0; i < surf->numTextures; i++) {
            if (surf->textures[i]) {
                surf->textures[i]->draw(surf->textures[i]);
                (void)surf->textures[i]->inquire(surf->textures[i],
                                    hwStrTextureID, (void **)&ptr);
                surf->textureIDs[i] = *ptr;
            }
            /* Otherwise, assume that textureIDs are used */
        }
    }
    __hwDisp->surfAttrs(__hwDisp, surf);
}

/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

#include <math.h>

static void _dorand48(hwDisplayInternal, unsigned short[3]);

#define RAND48_SEED_0   (0x330e)
#define RAND48_SEED_1   (0xabcd)
#define RAND48_SEED_2   (0x1234)
#define RAND48_MULT_0   (0xe66d)
#define RAND48_MULT_1   (0xdeec)
#define RAND48_MULT_2   (0x0005)
#define RAND48_ADD      (0x000b)

static double _erand48(hwDisplayInternal intDisp, unsigned short xseed[3])
{
    _dorand48(intDisp, xseed);
    return ldexp((double) xseed[0], -48) +
           ldexp((double) xseed[1], -32) +
           ldexp((double) xseed[2], -16);
}

double hwDrand48(void)
{
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;

    return _erand48(intDisp, intDisp->rand48_seed);
}

void hwSrand48(long seed)
{
    HW_USE_CURR_DISP;
    hwDisplayInternal
        intDisp = (hwDisplayInternal)__hwDisp;

    intDisp->rand48_seed[0] = RAND48_SEED_0;
    intDisp->rand48_seed[1] = (unsigned short) seed;
    intDisp->rand48_seed[2] = (unsigned short) (seed >> 16);
    intDisp->rand48_mult[0] = RAND48_MULT_0;
    intDisp->rand48_mult[1] = RAND48_MULT_1;
    intDisp->rand48_mult[2] = RAND48_MULT_2;
    intDisp->rand48_add = RAND48_ADD;
}

static void _dorand48(hwDisplayInternal intDisp ,unsigned short xseed[3])
{
    unsigned long accu;
    unsigned short temp[2];

    accu = (unsigned long) intDisp->rand48_mult[0] * (unsigned long) xseed[0] +
           (unsigned long) intDisp->rand48_add;
    temp[0] = (unsigned short) accu;        /* lower 16 bits */
    accu >>= sizeof(unsigned short) * 8;
    accu += (unsigned long) intDisp->rand48_mult[0] * (unsigned long) xseed[1] +
            (unsigned long) intDisp->rand48_mult[1] * (unsigned long) xseed[0];
    temp[1] = (unsigned short) accu;        /* middle 16 bits */
    accu >>= sizeof(unsigned short) * 8;
    accu += intDisp->rand48_mult[0] * xseed[2] +
            intDisp->rand48_mult[1] * xseed[1] +
            intDisp->rand48_mult[2] * xseed[0];
    xseed[0] = temp[0];
    xseed[1] = temp[1];
    xseed[2] = (unsigned short) accu;
}

/* Implement HW debugging malloc routines */
#undef malloc
#undef free
#undef realloc

void *hwMalloc(size_t size, const char *file, int line)
{
    return malloc(size);
}

void hwFree(void *ptr, const char *file, int line)
{
    free(ptr);
}

void *hwRealloc(void *ptr, size_t size, const char *file, int line)
{
    return realloc(ptr, size);
}

int __hwAllocTabLock(void **tab, int n)
{
    int
        doInit = 0;

    while( !*tab ) {
        HW_GLOBAL_LOCK();
        if( !*tab ) {
            doInit = 1;
            *tab = hwCreateHash( n );
            if( !*tab ) {
                __hwIntSetError( HW_ERROR_NO_MEMORY );
                HW_GLOBAL_UNLOCK();
                return -1;
            }
        }
        else {
            HW_GLOBAL_UNLOCK();
        }
    }
    return doInit;
}

/*** EOF hwUtil.c ***/
