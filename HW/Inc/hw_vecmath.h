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

/* Macros for doing vector math.  Works with any arithmetic type. */
/* WARNING: Very few of these are side-effect-proof! */

/* Dst[] = Src[] */
#define VCOPY(Dst,Src) do { \
        Dst[0] = Src[0]; \
        Dst[1] = Src[1]; \
        Dst[2] = Src[2]; \
    } while( 0 )

#define VCOPY4(Dst,Src) do { \
        Dst[0] = Src[0]; \
        Dst[1] = Src[1]; \
        Dst[2] = Src[2]; \
        Dst[3] = Src[3]; \
    } while( 0 )

#define VASSIGN(Dst,a,b,c) do { \
        Dst[0] = (a); \
        Dst[1] = (b); \
        Dst[2] = (c); \
    } while( 0 )

#define VASSIGN4(Dst,a,b,c,d) do { \
        Dst[0] = (a); \
        Dst[1] = (b); \
        Dst[2] = (c); \
        Dst[3] = (d); \
    } while( 0 )

/* Dst[] = Src1[] + Src2[] */
#define VADD(Dst,Src1,Src2) do { \
        (Dst)[0] = (Src1)[0] + (Src2)[0]; \
        (Dst)[1] = (Src1)[1] + (Src2)[1]; \
        (Dst)[2] = (Src1)[2] + (Src2)[2]; \
    } while( 0 )

/* Dst[] = Src1[] * Src2[] */
#define VMUL(Dst,Src1,Src2) do { \
        (Dst)[0] = (Src1)[0] * (Src2)[0]; \
        (Dst)[1] = (Src1)[1] * (Src2)[1]; \
        (Dst)[2] = (Src1)[2] * (Src2)[2]; \
    } while( 0 )

/* Dst[] = Src1[] - Src[] */
#define VSUB(Dst,Src1,Src2) do { \
        (Dst)[0] = (Src1)[0] - (Src2)[0]; \
        (Dst)[1] = (Src1)[1] - (Src2)[1]; \
        (Dst)[2] = (Src1)[2] - (Src2)[2]; \
    } while( 0 )

/* Dst[] = Src[] * Const; */
#define VMULC(Dst,Src,Const) do { \
        (Dst)[0] = (Src)[0] * (Const); \
        (Dst)[1] = (Src)[1] * (Const); \
        (Dst)[2] = (Src)[2] * (Const); \
    } while( 0 )

/* Length-squared of Vec[] */
#define VLEN2(Vec)      ((Vec)[0]*(Vec)[0]+(Vec)[1]*(Vec)[1]+(Vec)[2]*(Vec)[2])

/* Length of Vec[] */
#define VLEN(Vec)       sqrt(VLEN2(Vec))

/* Dst = normalized(Src) */
#define VNORM(Dst,Src) do { \
        double __L__ = VLEN(Src); \
        if( __L__ != 0.0 ) __L__ = 1.0 / __L__; \
        VMULC(Dst,Src,__L__); \
    } while( 0 )

/* Dot product: Src1 * Src2 */
#define VDOT(S1,S2)     ((S1)[0]*(S2)[0]+(S1)[1]*(S2)[1]+(S1)[2]*(S2)[2])

/* Cross product: Src1 X Src2 */
#define VCROSS(Dst,S1,S2) do { \
        (Dst)[0] = (S1)[1]*(S2)[2] - (S2)[1]*(S1)[2]; \
        (Dst)[1] = (S1)[2]*(S2)[0] - (S2)[2]*(S1)[0]; \
        (Dst)[2] = (S1)[0]*(S2)[1] - (S2)[0]*(S1)[1]; \
    } while( 0 )

/* Dst = 3x3 transform by Mat of Src */
#define VXFORM3(Dst,Src,Mat) do { \
        double __A__, __B__, __C__; \
        __A__ = (Src)[0]; __B__ = (Src)[1]; __C__ = (Src)[2]; \
        (Dst)[0] = __A__*(Mat)[0][0] + __B__*(Mat)[1][0] + __C__*(Mat)[2][0]; \
        (Dst)[1] = __A__*(Mat)[0][1] + __B__*(Mat)[1][1] + __C__*(Mat)[2][1]; \
        (Dst)[2] = __A__*(Mat)[0][2] + __B__*(Mat)[1][2] + __C__*(Mat)[2][2]; \
    } while( 0 )

/* Dst = 4x3 transform by Mat of Src */
#define VXFORM4(Dst,Src,Mat) do { \
        double __A, __B, __C; \
        __A = (Src)[0]; __B = (Src)[1]; __C = (Src)[2]; \
        (Dst)[0]=__A*(Mat)[0][0]+__B*(Mat)[1][0]+__C*(Mat)[2][0]+(Mat)[3][0]; \
        (Dst)[1]=__A*(Mat)[0][1]+__B*(Mat)[1][1]+__C*(Mat)[2][1]+(Mat)[3][1]; \
        (Dst)[2]=__A*(Mat)[0][2]+__B*(Mat)[1][2]+__C*(Mat)[2][2]+(Mat)[3][2]; \
    } while( 0 )

/* Determinant of 3x3 matrix */
#define DET3(mat)       \
     ((mat)[0][0]*((mat)[1][1]*(mat)[2][2] - (mat)[1][2]*(mat)[2][1]) \
    - (mat)[0][1]*((mat)[1][0]*(mat)[2][2] - (mat)[1][2]*(mat)[2][0]) \
    + (mat)[0][2]*((mat)[1][0]*(mat)[2][1] - (mat)[1][1]*(mat)[2][0]))

#define V3_EQV(a,b)     (       ((a)[0] == (b)[0]) \
                        &&      ((a)[1] == (b)[1]) \
                        &&      ((a)[2] == (b)[2])      )

#define V4_EQV(a,b)     (       ((a)[0] == (b)[0]) \
                        &&      ((a)[1] == (b)[1]) \
                        &&      ((a)[2] == (b)[2]) \
                        &&      ((a)[3] == (b)[3])      )
