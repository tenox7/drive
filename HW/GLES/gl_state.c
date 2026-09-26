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
 * Derived from code originally Copyright (c) 2023 Ross Cunniff
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Define this or none of our gl* calls will work: */
#define HWGL_LIGHT_IMP 1

#include "hw.h"
#include "hw_internal.h"
#include "hw_gl.h"
#include "hw_vecmath.h"

#define VALIDATE_PROG_INFO() if (!(__hwDisp && glctx && ppi)) return;

static void setError(hwProgInfo *ppi, hwInt32 num, int line)
{
    if (!ppi->lastError) {
        ppi->lastError = num;
    }
}

#define INVALID_ENUM_RETURN     \
    do { setError(ppi, HWGL_INVALID_ENUM, __LINE__); return; } while(0)
#define INVALID_VALUE_RETURN    \
    do { setError(ppi, HWGL_INVALID_VALUE, __LINE__); return; } while(0)
#define STACK_OVERFLOW_RETURN   \
    do { setError(ppi, HWGL_STACK_OVERFLOW, __LINE__); return; } while(0)
#define STACK_UNDERFLOW_RETURN  \
    do { setError(ppi, HWGL_STACK_UNDERFLOW, __LINE__); return; } while(0)

static int getEnaBit(hwInt32 pname)
{
    switch (pname) {
    case HWGL_LIGHT0 :          return ENA_LIGHT0;
    case HWGL_LIGHT1 :          return ENA_LIGHT1;
    case HWGL_LIGHT2 :          return ENA_LIGHT2;
    case HWGL_LIGHT3 :          return ENA_LIGHT3;
    case HWGL_LIGHT4 :          return ENA_LIGHT4;
    case HWGL_LIGHT5 :          return ENA_LIGHT5;
    case HWGL_LIGHT6 :          return ENA_LIGHT6;
    case HWGL_LIGHT7 :          return ENA_LIGHT7;
    case HWGL_LIGHTING :        return ENA_LIGHTING;
    case HWGL_TEXTURE0 :        return ENA_TEX0;
    case HWGL_TEXTURE1 :        return ENA_TEX1;
    case HWGL_TEXTURE2 :        return ENA_TEX2;
    case HWGL_TEXTURE3 :        return ENA_TEX3;
    case HWGL_TEXTURE4 :        return ENA_TEX4;
    case HWGL_TEXTURE5 :        return ENA_TEX5;
    case HWGL_TEXTURE6 :        return ENA_TEX6;
    case HWGL_TEXTURE7 :        return ENA_TEX7;
    case HWGL_FOG :             return ENA_FOG;
    case HWGL_COLOR_SUM :       return ENA_COLOR_SUM;
    case HWGL_NORMALIZE :       return ENA_NORMALIZE;
    case HWGL_COLOR_MATERIAL :  return ENA_COLOR_MATERIAL;
    default :                   return -1;
    }
}

static int getLightNum(hwInt32 light)
{
    switch (light) {
    case HWGL_LIGHT0 :  return 0;
    case HWGL_LIGHT1 :  return 1;
    case HWGL_LIGHT2 :  return 2;
    case HWGL_LIGHT3 :  return 3;
    case HWGL_LIGHT4 :  return 4;
    case HWGL_LIGHT5 :  return 5;
    case HWGL_LIGHT6 :  return 6;
    case HWGL_LIGHT7 :  return 7;
    default :           return -1;
    }
}

static int getAttribNum( hwProgInfo *ppi, hwInt32 ena )
{
    switch (ena) {
    case HWGL_VERTEX_ARRAY :    return HWGL_VA_VERTEX;
    case HWGL_COLOR_ARRAY :     return HWGL_VA_COLOR;
    case HWGL_NORMAL_ARRAY :    return HWGL_VA_NORMAL;
    case HWGL_TANGENT_ARRAY :   return HWGL_VA_TANGENT;

    case HWGL_TEXTURE_COORD_ARRAY :
        return HWGL_VA_TEXTURE + ppi->clientActiveTexture - HWGL_TEXTURE0;

    default :
        return -1;
    }
}

void hwGlEnable( hwInt32 pname )
{
    USE_PROG_INFO;
    hwInt32 bit;

    VALIDATE_PROG_INFO();

    if (pname == HWGL_TEXTURE_2D) {
        bit = getEnaBit(ppi->activeTexture);
    }
    else {
        bit = getEnaBit(pname);
    }

    if (bit < 0) {
        glEnable(pname);
        return;
    }

    if (IS_ENABLED(bit)) return;

    DO_ENABLE(bit);
    ppi->progDirty = 1;
}

void hwGlDisable( hwInt32 pname )
{
    USE_PROG_INFO;
    hwInt32 bit;

    VALIDATE_PROG_INFO();

    if (pname == HWGL_TEXTURE_2D) {
        bit = getEnaBit(ppi->activeTexture);
    }
    else {
        bit = getEnaBit(pname);
    }

    if (bit < 0) {
        glDisable(pname);
        return;
    }

    if (!IS_ENABLED(bit)) return;

    DO_DISABLE(bit);
    ppi->progDirty = 1;
}

void hwGlActiveTexture( hwInt32 tex )
{
    USE_PROG_INFO;

    VALIDATE_PROG_INFO();

    switch (tex) {
    case HWGL_TEXTURE0 :
    case HWGL_TEXTURE1 :
    case HWGL_TEXTURE2 :
    case HWGL_TEXTURE3 :
    case HWGL_TEXTURE4 :
    case HWGL_TEXTURE5 :
    case HWGL_TEXTURE6 :
    case HWGL_TEXTURE7 :
        ppi->activeTexture = tex;
        // Note: we have to pass through the glActiveTexture here since
        // OpenGL implements other things based on this
        glActiveTexture( tex );
        break;
    default :
        INVALID_ENUM_RETURN;
    }
}

/* Transform a 4d vector by a 4x4 matrix */
static void xform4x4(hwFloat dst[4], const hwFloat src[4], const hwFloat mx[16])
{
    dst[0] = src[0]*mx[ 0] + src[1]*mx[ 4] + src[2]*mx[ 8] + src[3]*mx[12];
    dst[1] = src[0]*mx[ 1] + src[1]*mx[ 5] + src[2]*mx[ 9] + src[3]*mx[13];
    dst[2] = src[0]*mx[ 2] + src[1]*mx[ 6] + src[2]*mx[10] + src[3]*mx[14];
    dst[3] = src[0]*mx[ 3] + src[1]*mx[ 7] + src[2]*mx[11] + src[3]*mx[15];
}

/* Transform a 3d vector by a 4x4 matrix */
static void xform3x4(hwFloat dst[3], const hwFloat src[3], const hwFloat mx[16])
{
    dst[0] = src[0]*mx[ 0] + src[1]*mx[ 4] + src[2]*mx[ 8];
    dst[1] = src[0]*mx[ 1] + src[1]*mx[ 5] + src[2]*mx[ 9];
    dst[2] = src[0]*mx[ 2] + src[1]*mx[ 6] + src[2]*mx[10];
}

/* Transform a 3d vector by a 3x3 matrix */
static void xform3x3(hwFloat dst[3], const hwFloat src[3], const hwFloat mx[9])
{
    dst[0] = src[0]*mx[0] + src[1]*mx[3] + src[2]*mx[6];
    dst[1] = src[0]*mx[1] + src[1]*mx[4] + src[2]*mx[7];
    dst[2] = src[0]*mx[2] + src[1]*mx[5] + src[2]*mx[8];
}

/* Calculate the inverse transpose of the upper left 3x3 of a 4x4 matrix */
static int invXpose3x3( hwFloat dst[9], hwFloat mat[16])
{
    /* The inverse of a 3x3 matrix:
     *
     *      [ a11 a12 a13 ]
     *  A = [ a21 a22 a23 ]
     *      [ a31 a32 a33 ]
     *
     * is defined as:
     *
     *  [  |a22 a23|  |a13 a12|  |a12 a13|  ]
     *  [  |a32 a33|  |a33 a32|  |a22 a23|  ]
     *  [                                   ]    1
     *  [  |a23 a21|  |a11 a13|  |a13 a11|  ] * ---
     *  [  |a33 a31|  |a31 a33|  |a23 a21|  ]   |A|
     *  [                                   ]
     *  [  |a21 a22|  |a12 a11|  |a11 a12|  ]
     *  [  |a31 a32|  |a32 a31|  |a21 a22|  ]
     *
     * where the 2x2 determinant of a matrix is defined as:
     *
     *  |a b|  = (a*d) - (b*c)
     *  |c d|
     *
     * and the 3x3 determinant is defined as:
     *
     *  |a11 a12 a13|        a11*(a22*a33 - a23*a32)
     *  |a21 a22 a23|  =   - a12*(a21*a33 - a23*a31)
     *  |a31 a32 a33|      + a13*(a21*a32 - a22*a31)
     *
     * The transpose is done by indexing the dest matrix.
     */
#define DET2(a,b,c,d) (mat[a]*mat[d] - mat[b]*mat[c])
    hwFloat det;
    const int a11 = 0, a12 = 1, a13 = 2;
    const int a21 = 4, a22 = 5, a23 = 6;
    const int a31 = 8, a32 = 9, a33 = 10;

    /* Determinant */
    det  =   mat[a11]*(mat[a22]*mat[a33] - mat[a23]*mat[a32]);
    det += - mat[a12]*(mat[a21]*mat[a33] - mat[a23]*mat[a31]);
    det +=   mat[a13]*(mat[a21]*mat[a32] - mat[a22]*mat[a31]);

    if ((det > -0.001) && (det < 0.001)) return 0;

    /* Invert and transpose */
    det = 1.f / det;
    dst[0] = det * DET2(a22, a23, a32, a33);
    dst[3] = det * DET2(a13, a12, a33, a32);
    dst[6] = det * DET2(a12, a13, a22, a23);

    dst[1] = det * DET2(a23, a21, a33, a31);
    dst[4] = det * DET2(a11, a13, a31, a33);
    dst[7] = det * DET2(a13, a11, a23, a21);

    dst[2] = det * DET2(a21, a22, a31, a32);
    dst[5] = det * DET2(a12, a11, a32, a31);
    dst[8] = det * DET2(a11, a12, a21, a22);

    return 1;
#undef DET2
}

void hwGlLightfv( hwInt32 light, hwInt32 pname, const hwFloat *param )
{
    USE_PROG_INFO;
    hwLightInfo *li;
    hwFloat pos[4], dir[3], wRecip;
    int lightNum;

    VALIDATE_PROG_INFO();

    lightNum = getLightNum(light);
    if (lightNum < 0) INVALID_ENUM_RETURN;
    li = ppi->lights + lightNum;

    switch (pname) {
    case HWGL_POSITION :
        xform4x4(pos, param, ppi->mvMat);
        if (pos[3] == 0.f) {
            // Directional light - normalize direction
            VNORM(pos, pos);
            if (IS_ENABLED(ENA_LIGHT0_POS+lightNum)) {
                DO_DISABLE(ENA_LIGHT0_POS+lightNum);
                ppi->progDirty = 1;
            }
        }
        else {
            // Positional light - homogenize
            wRecip = 1.f / pos[3];
            VMULC(pos, pos, wRecip);
            if (!IS_ENABLED(ENA_LIGHT0_POS+lightNum)) {
                DO_ENABLE(ENA_LIGHT0_POS+lightNum);
                ppi->progDirty = 1;
            }
        }
        if (V4_EQV(li->pos, pos)) return;
        VCOPY4(li->pos,pos);
        break;
    case HWGL_SPOT_DIRECTION :
        // The OpenGL spec says we just tranform the position by the
        // upper 3x3 matrix of the current MV matrix
        xform3x4(dir, param, ppi->mvMat);
        if (V3_EQV(dir, param)) return;
        VCOPY(li->spotDir, dir);
        break;
    case HWGL_SPOT_CUTOFF :
        if (li->spotCutoff != param[0]) return;
        li->spotCutoff = param[0];
        if (param[0] < 180.0) {
            if (!IS_ENABLED(ENA_LIGHT0_SPOT+lightNum)) {
                DO_ENABLE(ENA_LIGHT0_SPOT+lightNum);
                ppi->progDirty = 1;
            }
        }
        else {
            if (IS_ENABLED(ENA_LIGHT0_SPOT+lightNum)) {
                DO_DISABLE(ENA_LIGHT0_SPOT+lightNum);
                ppi->progDirty = 1;
            }
        }
        break;
    case HWGL_SPOT_EXPONENT :
        if (li->spotExp != param[0]) return;
        li->spotExp = param[0];
        break;
    case HWGL_LINEAR_ATTENUATION :
        if (li->linearAtten != param[0]) return;
        li->linearAtten = param[0];
        if (param[0] != 0.0) {
            if (!IS_ENABLED(ENA_LIGHT0_ATT+lightNum)) {
                DO_ENABLE(ENA_LIGHT0_ATT+lightNum);
                ppi->progDirty = 1;
            }
        }
        else {
            if (IS_ENABLED(ENA_LIGHT0_ATT+lightNum)) {
                DO_DISABLE(ENA_LIGHT0_ATT+lightNum);
                ppi->progDirty = 1;
            }
        }
        break;
    case HWGL_AMBIENT :
        if (V4_EQV(li->ambient, param)) return;
        VCOPY4(li->ambient, param);
        break;
    case HWGL_DIFFUSE :
        if (V4_EQV(li->diffuse, param)) return;
        VCOPY4(li->diffuse, param);
        break;
    case HWGL_SPECULAR :
        if (V4_EQV(li->specular, param)) return;
        VCOPY4(li->specular, param);
        break;
    default :
        return;
    }

    DO_DIRTY(DIRTY_LIGHT0 + lightNum);
}

void hwGlLightf( hwInt32 light, hwInt32 pname, hwFloat param )
{
    hwGlLightfv( light, pname, &param );
}

void hwGlFogfv( hwInt32 pname, const hwFloat *param )
{
    USE_PROG_INFO;
    hwFogInfo *fi;

    VALIDATE_PROG_INFO();

    fi = &ppi->fog;

    switch (pname) {
    case HWGL_FOG_COLOR :
        VCOPY4(fi->color, param);
        DO_DIRTY(DIRTY_FOG);
        break;
    case HWGL_FOG_START :
        fi->fogStart = param[0];
        DO_DIRTY(DIRTY_FOG);
        break;
    case HWGL_FOG_END :
        fi->fogEnd = param[0];
        DO_DIRTY(DIRTY_FOG);
        break;
    case HWGL_FOG_DENSITY :
        fi->density = param[0];
        DO_DIRTY(DIRTY_FOG);
        break;
    case HWGL_FOG_MODE :
        if (fi->mode == (hwInt32)param[0]) return;
        switch ((hwInt32)param[0]) {
        case HWGL_LINEAR : 
            DO_ENABLE(ENA_FOG_LINEAR);
            DO_DISABLE(ENA_FOG_EXP);
            DO_DISABLE(ENA_FOG_EXP2);
            break;
        case HWGL_EXP :
            DO_DISABLE(ENA_FOG_LINEAR);
            DO_ENABLE(ENA_FOG_EXP);
            DO_DISABLE(ENA_FOG_EXP2);
            break;
        case HWGL_EXP2 :
            DO_DISABLE(ENA_FOG_LINEAR);
            DO_DISABLE(ENA_FOG_EXP);
            DO_ENABLE(ENA_FOG_EXP2);
            break;
        default :
            INVALID_ENUM_RETURN;
        }
        ppi->progDirty = 1;
        break;
    default :
        INVALID_ENUM_RETURN;
    }
}

void hwGlFogf( hwInt32 pname, hwFloat param )
{
    hwGlFogfv( pname, &param );
}

void hwGlFogi( hwInt32 pname, hwInt32 param )
{
    hwFloat fParam;

    fParam = (hwFloat) param;
    hwGlFogfv(pname, &fParam);
}

void hwGlMaterialfv( hwInt32 face, hwInt32 pname, const hwFloat *param )
{
    USE_PROG_INFO;

    VALIDATE_PROG_INFO();

    if (face != HWGL_FRONT_AND_BACK) INVALID_ENUM_RETURN;

    switch (pname) {
    case HWGL_SPECULAR :
        VCOPY4(ppi->material.specular, param);
        break;
    case HWGL_EMISSION :
        VCOPY4(ppi->material.emission, param);
        break;
    case HWGL_SHININESS :
        ppi->material.specExp = param[0];
        break;
    default :
        INVALID_ENUM_RETURN;
    }
    DO_DIRTY(DIRTY_MATERIAL);
}

void hwGlMateriali( hwInt32 face, hwInt32 pname, hwInt32 param )
{
    hwFloat fParam;

    fParam = param;
    hwGlMaterialfv( face, pname, &fParam );
}

void hwGlColorMaterial( hwInt32 face, hwInt32 mode )
{
    USE_PROG_INFO;

    VALIDATE_PROG_INFO();

    /* We only support GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE */
    if (face != HWGL_FRONT_AND_BACK) INVALID_ENUM_RETURN;
    if (mode != HWGL_AMBIENT_AND_DIFFUSE) INVALID_ENUM_RETURN;

    ppi->colorMaterial = mode;
}

void hwGlLightModelfv( hwInt32 pname, const hwFloat *param )
{
    USE_PROG_INFO;
    hwInt32 iParam;

    VALIDATE_PROG_INFO();

    switch (pname) {
    case HWGL_LIGHT_MODEL_AMBIENT :
        if (V4_EQV(ppi->globalAmbient, param)) return;
        VCOPY4(ppi->globalAmbient, param);
        break;
    case HWGL_LIGHT_MODEL_COLOR_CONTROL :
        iParam = (hwInt32)param[0];
        switch (iParam) {
        case HWGL_SEPARATE_SPECULAR_COLOR :
            // Always enabled?
        default :
            INVALID_ENUM_RETURN;
        }
        ppi->lmColorControl = iParam;
        break;
    default :
        INVALID_ENUM_RETURN;
    }

    DO_DIRTY(DIRTY_LIGHT_MODEL);
}

void hwGlLightModeli( hwInt32 pname, hwInt32 param )
{
    hwFloat fParam;

    fParam = param;
    hwGlLightModelfv(pname, &fParam);
}

void hwGlTexEnvi( hwInt32 pname, hwInt32 param )
{
    USE_PROG_INFO;
    int tex;

    VALIDATE_PROG_INFO();

    if ((pname != HWGL_TEXTURE_ENV_MODE)) {
        INVALID_ENUM_RETURN;
    }

    switch (param) {
    case HWGL_MODULATE :
    case HWGL_RELIEF_MAP :
    case HWGL_NORMAL_MAP :
    case HWGL_GLOSS_MAP :
        break;
    default :
        INVALID_ENUM_RETURN;
    }

    tex = ppi->activeTexture - HWGL_TEXTURE0;
    if (ppi->texEnv[tex] == param) {
        return;
    }

    ppi->texEnv[tex] = param;
    ppi->progDirty = 1;
}

void hwGlTexGeni( hwInt32 pname, hwInt32 param )
{
    USE_PROG_INFO;
    int tex;

    VALIDATE_PROG_INFO();

    if ((pname != HWGL_TEXTURE_GEN_MODE)) {
        INVALID_ENUM_RETURN;
    }

    tex = ppi->activeTexture - HWGL_TEXTURE0;

    switch (param) {
    case HWGL_REFLECTION_MAP :
        if (IS_ENABLED(ENA_TEXGEN0_REFLECT+tex)) break;
        DO_ENABLE(ENA_TEXGEN0_REFLECT+tex);
        DO_DISABLE(ENA_TEXGEN0_STAGE_0+tex);
        ppi->progDirty = 1;
        break;

    case HWGL_STAGE_0 :
        if (IS_ENABLED(ENA_TEXGEN0_STAGE_0+tex)) break;
        DO_ENABLE(ENA_TEXGEN0_STAGE_0+tex);
        DO_DISABLE(ENA_TEXGEN0_REFLECT+tex);
        ppi->progDirty = 1;
        break;

    case HWGL_NONE :
        if ( !( IS_ENABLED(ENA_TEXGEN0_STAGE_0+tex)
             || IS_ENABLED(ENA_TEXGEN0_REFLECT+tex)))
        {
            break;
        }
        DO_DISABLE(ENA_TEXGEN0_STAGE_0+tex);
        DO_DISABLE(ENA_TEXGEN0_REFLECT+tex);
        ppi->progDirty = 1;
        break;

    default :
        INVALID_ENUM_RETURN;
    }

}

void hwGlMatrixMode( hwInt32 pname )
{
    USE_PROG_INFO;

    VALIDATE_PROG_INFO();

    switch (pname) {
    case HWGL_PROJECTION :
    case HWGL_MODELVIEW :
        ppi->matrixMode = pname;
        break;
    default :
        INVALID_ENUM_RETURN;
    }
}

void hwGlPushMatrix( void )
{
    USE_PROG_INFO;
    hwFloat *mat;

    VALIDATE_PROG_INFO();

    switch (ppi->matrixMode) {
    case HWGL_PROJECTION :
        if (ppi->prMat >= ppi->prTOS) STACK_OVERFLOW_RETURN;
        ppi->prMat += 16;
        mat = ppi->prMat;
        break;
    case HWGL_MODELVIEW :
        if (ppi->mvMat >= ppi->mvTOS) STACK_OVERFLOW_RETURN;
        ppi->mvMat += 16;
        mat = ppi->mvMat;
        break;
    default :
        return;
    }
    memcpy(mat, mat-16, 16*sizeof(hwFloat));
}

void hwGlPopMatrix( void )
{
    USE_PROG_INFO;
    hwFloat *mat;

    VALIDATE_PROG_INFO();

    switch (ppi->matrixMode) {
    case HWGL_PROJECTION :
        if (ppi->prMat <= ppi->prStack) STACK_UNDERFLOW_RETURN;
        ppi->prMat -= 16;
        mat = ppi->prMat;
        DO_DIRTY(DIRTY_PROJECTION);
        break;
    case HWGL_MODELVIEW :
        if (ppi->mvMat <= ppi->mvStack) STACK_UNDERFLOW_RETURN;
        ppi->mvMat -= 16;
        mat = ppi->mvMat;
        DO_DIRTY(DIRTY_MODELVIEW);
        break;
    default :
        return;
    }
}

void hwGlLoadMatrixf( const hwFloat *mat )
{
    USE_PROG_INFO;
    hwFloat *dst;

    VALIDATE_PROG_INFO();

    switch (ppi->matrixMode) {
    case HWGL_PROJECTION :
        dst = ppi->prMat;
        DO_DIRTY(DIRTY_PROJECTION);
        break;
    case HWGL_MODELVIEW :
        dst = ppi->mvMat;
        DO_DIRTY(DIRTY_MODELVIEW);
        break;
    default :
        return;
    }
    memcpy(dst, mat, 4*4*sizeof(hwFloat));
}

static void mkIdentity(hwFloat *mat)
{
    mat[ 0] = 1.f; mat[ 1] = 0.f; mat[ 2] = 0.f; mat[ 3] = 0.f;
    mat[ 4] = 0.f; mat[ 5] = 1.f; mat[ 6] = 0.f; mat[ 7] = 0.f;
    mat[ 8] = 0.f; mat[ 9] = 0.f; mat[10] = 1.f; mat[11] = 0.f;
    mat[12] = 0.f; mat[13] = 0.f; mat[14] = 0.f; mat[15] = 1.f;
}

static void mkIdentity3x3(hwFloat *mat)
{
    mat[0] = 1.f; mat[1] = 0.f; mat[2] = 0.f;
    mat[3] = 0.f; mat[4] = 1.f; mat[5] = 0.f;
    mat[6] = 0.f; mat[7] = 0.f; mat[8] = 1.f;
}

void hwGlLoadIdentity( void )
{
    USE_PROG_INFO;
    hwFloat *dst;

    VALIDATE_PROG_INFO();

    switch (ppi->matrixMode) {
    case HWGL_PROJECTION :
        dst = ppi->prMat;
        DO_DIRTY(DIRTY_PROJECTION);
        break;
    case HWGL_MODELVIEW :
        dst = ppi->mvMat;
        DO_DIRTY(DIRTY_MODELVIEW);
        break;
    default :
        return;
    }

    mkIdentity(dst);
}

static void matMult(hwFloat *dst, const hwFloat *lhs, const hwFloat *rhs)
{
    hwInt32 i, j, k;
    hwFloat t;

    for( i = 0; i < 4; i++ ) {
        for( j = 0; j < 4; j++ ) {
            t = 0.f;
            for( k = 0; k < 4; k++ ) {
                t += lhs[4*i + k] * rhs[4*k + j];
            }
            dst[4*i + j] = t;
        }
    }
}

void hwGlMultMatrixf( const hwFloat *mat )
{
    USE_PROG_INFO;
    hwFloat *tos; // top-of-stack
    hwFloat dst[16];

    VALIDATE_PROG_INFO();

    switch (ppi->matrixMode) {
    case HWGL_PROJECTION :
        tos = ppi->prMat;
        DO_DIRTY(DIRTY_PROJECTION);
        break;
    case HWGL_MODELVIEW :
        tos = ppi->mvMat;
        DO_DIRTY(DIRTY_MODELVIEW);
        break;
    default :
        return;
    }

    matMult(dst, mat, tos);
    memcpy(tos, dst, 4*4*sizeof(hwFloat));
}

void hwGlTranslatef( hwFloat x, hwFloat y, hwFloat z )
{
    hwFloat mat[16];

    mat[ 0] = 1.f; mat[ 1] = 0.f; mat[ 2] = 0.f; mat[ 3] = 0.f;
    mat[ 4] = 0.f; mat[ 5] = 1.f; mat[ 6] = 0.f; mat[ 7] = 0.f;
    mat[ 8] = 0.f; mat[ 9] = 0.f; mat[10] = 1.f; mat[11] = 0.f;
    mat[12] =   x; mat[13] =   y; mat[14] =   z; mat[15] = 1.f;

    hwGlMultMatrixf( mat );
}

void hwGlScalef( hwFloat x, hwFloat y, hwFloat z )
{
    hwFloat mat[16];

    mat[ 0] =   x; mat[ 1] = 0.f; mat[ 2] = 0.f; mat[ 3] = 0.f;
    mat[ 4] = 0.f; mat[ 5] =   y; mat[ 6] = 0.f; mat[ 7] = 0.f;
    mat[ 8] = 0.f; mat[ 9] = 0.f; mat[10] =   z; mat[11] = 0.f;
    mat[12] = 0.f; mat[13] = 0.f; mat[14] = 0.f; mat[15] = 1.f;

    hwGlMultMatrixf( mat );
}

void hwGlFrustum( hwFloat left, hwFloat right,
                  hwFloat bottom, hwFloat top,
                  hwFloat zNear, hwFloat zFar )
{
    USE_PROG_INFO;
    hwFloat mat[16];
    hwFloat denomX, denomY, denomZ;
    hwFloat cX, cY;
    hwFloat cA, cB, cC, cD;

    VALIDATE_PROG_INFO();

    if ((zNear <= 0.f) || (zFar <= 0.f)) INVALID_VALUE_RETURN;

    denomX = right - left;
    denomY = top - bottom;
    denomZ = zFar - zNear;
    if ((denomX == 0.f) || (denomY == 0.f) || (denomZ == 0.f)) {
        INVALID_VALUE_RETURN;
    }
    denomX = 1.f / denomX;
    denomY = 1.f / denomY;
    denomZ = 1.f / denomZ;

    cX = (2.f * zNear) * denomX;
    cY = (2.f * zNear) * denomY;
    cA = (right + left) * denomX;
    cB = (top + bottom) * denomY;
    cC = -(zFar + zNear) * denomZ;
    cD = - (2.f * zFar * zNear) * denomZ;

    // Note indexing - C is row-major but matrix shown column-major
    // to match GL spec
    mat[ 0] =   cX; mat[ 4]  = 0.f; mat[ 8] =   cA; mat[12] =  0.f;
    mat[ 1] =  0.f; mat[ 5]  =  cY; mat[ 9] =   cB; mat[13] =  0.f;
    mat[ 2] =  0.f; mat[ 6]  = 0.f; mat[10] =   cC; mat[14] =   cD;
    mat[ 3] =  0.f; mat[ 7]  = 0.f; mat[11] = -1.f; mat[15] =  0.f;

    hwGlMultMatrixf( mat );
}

void hwGlOrtho( hwFloat left, hwFloat right,
                hwFloat bottom, hwFloat top,
                hwFloat zNear, hwFloat zFar )
{
    USE_PROG_INFO;
    hwFloat mat[16];
    hwFloat dX, dY, dZ;
    hwFloat tx, ty, tz;

    VALIDATE_PROG_INFO();

    dX = right - left;
    dY = top - bottom;
    dZ = zFar - zNear;
    if ((dX == 0.f) || (dY == 0.f) || (dZ == 0.f)) {
        INVALID_VALUE_RETURN;
    }
    dX = 1.f / dX;
    dY = 1.f / dY;
    dZ = 1.f / dZ;

    tx = -(right + left) * dX;
    ty = -(top + bottom) * dY;
    tz = -(zFar + zNear) * dZ;

    // Note indexing - C is row-major but matrix shown column-major
    // to match GL spec
    mat[ 0] = 2.f*dX; mat[ 4] =    0.f; mat[ 8] =    0.f; mat[12] =  tx;
    mat[ 1] =    0.f; mat[ 5] = 2.f*dY; mat[ 9] =    0.f; mat[13] =  ty;
    mat[ 2] =    0.f; mat[ 6] =    0.f; mat[10] = 2.f*dZ; mat[14] =  tz;
    mat[ 3] =    0.f; mat[ 7] =    0.f; mat[11] =    0.f; mat[15] = 1.f;

    hwGlMultMatrixf( mat );
}

void hwGlGetFloatv( hwInt32 pname, hwFloat *result )
{
    USE_PROG_INFO;
    hwFloat *tos;

    VALIDATE_PROG_INFO();

    switch (pname) {
    case HWGL_MODELVIEW_MATRIX :
        tos = ppi->mvMat;
        break;
    case HWGL_PROJECTION_MATRIX :
        tos = ppi->prMat;
        break;
    default :
        INVALID_ENUM_RETURN;
    }
    memcpy(result, tos, 16*sizeof(hwFloat));
}

hwInt32 hwGlGetError( void )
{
    USE_PROG_INFO;
    hwInt32 result;

    if (!ppi) {
        return 0;
    }

    result = ppi->lastError;
    ppi->lastError = 0;
    return result;
}

void hwGlInitProgState( hwProgInfo *ppi )
{
    hwLightInfo *li;
    hwFogInfo *fi;
    hwMaterialInfo *mi;
    int i;

    memset(ppi->enables, 0, ((ENA_COUNT+31)/32)*sizeof(hwUint32));
    for (i = 0; i < MAX_LIGHTS; i++) {
        li = ppi->lights + i;
        VASSIGN4(li->pos,     0.f,  0.f,  1.f,  0.f);
        VASSIGN(li->spotDir,  0.f,  0.f, -1.f);
        li->spotCutoff =      180.f;
        li->spotExp =         0.f;
        li->linearAtten =     0.f;
        VASSIGN4(li->ambient, 0.f,  0.f,  0.f,  1.f);
        if (i == 0) {
            VASSIGN4(li->diffuse,  1.f, 1.f, 1.f, 1.f);
            VASSIGN4(li->specular, 1.f, 1.f, 1.f, 1.f);
        }
        else {
            VASSIGN4(li->diffuse,  0.f, 0.f, 0.f, 0.f);
            VASSIGN4(li->specular, 0.f, 0.f, 0.f, 0.f);
        }
    }

    mi = &ppi->material;
    VASSIGN4(mi->specular, 0.f, 0.f, 0.f, 1.f);
    VASSIGN4(mi->emission, 0.f, 0.f, 0.f, 1.f);
    mi->specExp = 0.f;

    fi = &ppi->fog;
    fi->mode = HWGL_EXP;
    fi->density = 1.f;
    fi->fogStart = 0.f;
    fi->fogEnd = 1.f;
    DO_ENABLE_BITS(ppi->enables, ENA_FOG_EXP);
    VASSIGN4(fi->color, 0.f, 0.f, 0.f, 0.f);

    VASSIGN4(ppi->globalAmbient, 0.2f, 0.2f, 0.2f, 1.f);

    ppi->matrixMode = HWGL_MODELVIEW;
    ppi->activeTexture = HWGL_TEXTURE0;
    for (i = 0; i < MAX_TEX; i++) {
        ppi->texEnv[i] = HWGL_MODULATE;
    }
    ppi->prMat = ppi->prStack;
    ppi->mvMat = ppi->mvStack;
    ppi->prTOS = ppi->prMat + (PROJ_STACK_DEPTH-1)*16;
    ppi->mvTOS = ppi->mvMat + (MODELVIEW_STACK_DEPTH-1)*16;
    mkIdentity(ppi->prMat);
    mkIdentity(ppi->mvMat);
    mkIdentity(ppi->mvpMatrix);
    mkIdentity3x3(ppi->mvInvXpose);

    ppi->colorMaterial = HWGL_AMBIENT_AND_DIFFUSE;
    ppi->lmColorControl = HWGL_SEPARATE_SPECULAR_COLOR;

    ppi->lastError = 0;

    ppi->progDirty = 1;
    ppi->constDirty = ALL_DIRTY_BITS;

    ppi->clientActiveTexture = HWGL_TEXTURE0;
    ppi->program = 0;

    for (i = 0; i < HWGL_PROG_HASH_SIZE; i++) {
        ppi->progHash[i] = NULL;
    }

    (void)__hwGlInitProgSource(&ppi->vtxProg);
    (void)__hwGlInitProgSource(&ppi->fragProg);
}

static void fltUniform(hwInt32 prog, const char name[],
                       int n, const hwFloat v[])
{
    int idx;

    idx = glGetUniformLocation(prog, name);
    if (idx < 0) {
//printf("Uniform %s not found\n", name);
        return;
    }

    switch (n) {
    case  1 : glUniform1fv(idx, 1, v); break;
    case  2 : glUniform2fv(idx, 1, v); break;
    case  3 : glUniform3fv(idx, 1, v); break;
    case  4 : glUniform4fv(idx, 1, v); break;
    case  9 : glUniformMatrix3fv(idx, 1, GL_FALSE, v); break;
    case 16 : glUniformMatrix4fv(idx, 1, GL_FALSE, v); break;
    default : break;
    }
}

static void fltUniformFmt(hwInt32 prog, const char name[], int i,
                          int n, const hwFloat v[])
{
    char s[128];

#ifdef WIN32
    /* Cross your fingers and hope it does not go past 128 chars... */
    sprintf(s, name, i);
#else
    snprintf(s, 128, name, i);
#endif
    fltUniform(prog, s, n, v);
}

static void intUniform(hwInt32 prog, const char name[], int v)
{
    int idx;

    idx = glGetUniformLocation(prog, name);
    if (idx < 0) {
//printf("Uniform %s not found\n", name);
        return;
    }

    glUniform1i(idx, v);
}

static void intUniformFmt(hwInt32 prog, const char name[], int i, int v)
{
    char s[128];

#ifdef WIN32
    /* Cross your fingers and hope it does not go past 128 chars... */
    sprintf(s, name, i);
#else
    snprintf(s, 128, name, i);
#endif
    intUniform(prog, s, v);
}

static void diagnoseShader(hwInt32 shader, char *name)
{
    glDisplay *gldisp = HW_GET_CURR_DISP();
    GLint status;
    GLint logLength;
    GLint shdLength;
    GLint n;
    char *buff;
    int doDump;

    doDump = gldisp->glob.dumpProgs;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if ((status == GL_TRUE) && !doDump) return;

#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HB_HW", "%s: compile %s",
                        name,
                        (status == GL_TRUE) ? "succeeded" : "FAILED");
#else
    printf("%s: compile %s\n", name,
                (status == GL_TRUE) ? "succeeded" : "FAILED");
#endif

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    logLength++;

    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &shdLength);
    shdLength++;

    n = logLength; if (shdLength > n) n = shdLength;
    buff = malloc(n);
    if (!buff) return;

    glGetShaderSource(shader, shdLength, &shdLength, buff);
#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HB_HW", "Source: %s", buff);
#else
    printf("Source: %s\n", buff);
#endif

    glGetShaderInfoLog(shader, logLength, &logLength, buff);
#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HB_HW", "Log: %s", buff);
#else
    printf("Log: %s\n", buff);
#endif

    free(buff);
}

static void diagnoseProgram(hwInt32 program)
{
    GLint status;
    GLint logLength;
    char *buff;

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) return;

#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HB_HW", "Link FAILED");
#else
    printf("Link FAILED\n");
#endif

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    logLength++;
    buff = malloc(logLength);

    if (!buff) return;
    glGetProgramInfoLog(program, logLength, &logLength, buff);
#ifdef ANDROID_NDK
    __android_log_print(ANDROID_LOG_INFO, "HB_HW", "Log: %s", buff);
#else
    printf("Log: %s\n", buff);
#endif
    free(buff);
}

/* Make canonical / "cooked" enables */
void __hwGlCookEnables(hwProgInfo *ppi)
{
    hwInt32 i, bit, numTex;
    hwInt32 bumpFound;
    hwUint32 *cooked = ppi->cookedEnables;

    for (i = 0; i < NUM_ENABLE_WORDS; i++) {
        cooked[i] = ppi->enables[i];
    }

    if (!IS_ENABLED_BITS(cooked, ENA_LIGHTING)) {
        /* No lighting, no lights */
        for (i = 0; i < MAX_LIGHTS; i++) {
            DO_DISABLE_BITS(cooked, ENA_LIGHT0+i);
            DO_DISABLE_BITS(cooked, ENA_LIGHT0_POS+i);
            DO_DISABLE_BITS(cooked, ENA_LIGHT0_SPOT+i);
            DO_DISABLE_BITS(cooked, ENA_LIGHT0_ATT+i);
        }
        /* ... or other lighting-related features */
        DO_DISABLE_BITS(cooked, ENA_NORMALIZE);
        DO_DISABLE_BITS(cooked, ENA_COLOR_SUM);
        DO_DISABLE_BITS(cooked, ENA_COLOR_MATERIAL);
    }
    if (IS_ENABLED_BITS(cooked, ENA_LIGHTING)) {
        /* HACK! Uncomment these lines to test code before we get
         * bump-mapping working */
        // DO_ENABLE_BITS(cooked, ENA_LIGHT_PER_PIXEL);
        // DO_DISABLE_BITS(cooked, ENA_COLOR_SUM);
    }

    for (i = 0; i < MAX_LIGHTS; i++) {
        if (!IS_ENABLED_BITS(cooked, ENA_LIGHT0_POS+i)) {
            /* No positional light, no attenution */
            DO_DISABLE_BITS(cooked, ENA_LIGHT0_ATT+i);
        }
    }

    if (!IS_ENABLED_BITS(cooked, ENA_FOG)) {
        /* No fog, no fog type */
        DO_DISABLE_BITS(cooked, ENA_FOG_LINEAR);
        DO_DISABLE_BITS(cooked, ENA_FOG_EXP);
        DO_DISABLE_BITS(cooked, ENA_FOG_EXP2);
    }

    numTex = 0;
    bumpFound = 0;

    for (i = 0; i < MAX_TEX; i++) {
        if (IS_ENABLED_BITS(cooked, ENA_TEX0+i)) {
            /* Check for special handling */
            switch (ppi->texEnv[i]) {
            case HWGL_RELIEF_MAP :
            case HWGL_NORMAL_MAP :
                if (!IS_ENABLED_BITS(cooked, ENA_LIGHTING)) {
                    // This texture is only useful when lighting is enabled
                    // and we have a per-vertex tangent vector
                    DO_DISABLE_BITS(cooked, ENA_TEX0+i);
                    break;
                }

                // Note that normal maps (RGBX) and relief maps (RGBA)
                // are mutually exclusive for now.  And we only
                // allow one instance of them.
                if (bumpFound) break;
                bumpFound = 1;

                bit = (ppi->texEnv[i] == HWGL_NORMAL_MAP)
                    ? (ENA_TEX0_NORMAL+i) : (ENA_TEX0_RELIEF+i);

                DO_ENABLE_BITS(cooked, bit);
                // Normal/relief maps force light-per-pixel
                // and disables traditional color sum
                DO_ENABLE_BITS(cooked, ENA_LIGHT_PER_PIXEL);
                DO_DISABLE_BITS(cooked, ENA_COLOR_SUM);
                numTex++;
                break;

            case HWGL_GLOSS_MAP :
                if (!IS_ENABLED_BITS(cooked, ENA_LIGHTING)) {
                    // This texture is only useful when lighting is enabled
                    DO_DISABLE_BITS(cooked, ENA_TEX0+i);
                    break;
                }

                DO_ENABLE_BITS(cooked, ENA_TEX0_GLOSS+i);
                numTex++;
                break;

            default :
                numTex++;
                break;
            }
        }
    }

    if (!numTex) {
        /* No texture, color sum is irrelevant */
        DO_DISABLE_BITS(cooked, ENA_COLOR_SUM);
    }
}

/* This function assumes the enables have been cooked */
hwInt32 __hwGlHashEnables(hwProgInfo *ppi)
{
    hwInt32 i, n;
    hwUint32 result, t;
    unsigned char *cooked = (unsigned char *)ppi->cookedEnables;

    n = NUM_ENABLE_WORDS * sizeof(hwUint32);

    /* Hash algorithm from Principles of Compiler Design, Aho et al */
    result = 0;
    for (i = 0; i < n; i++) {
        result = (result << 4) + cooked[i];
        t = result & 0xF0000000U;
        if (t) {
            result ^= (t >> 24);
            result ^= t;
        }
    }

    return (hwInt32)(result % HWGL_PROG_HASH_SIZE);
}

hwProgHash __hwGlFindProg(hwProgInfo *ppi)
{
    hwInt32 h;
    int i, found;
    hwProgHash ph;

    h = __hwGlHashEnables(ppi);
    for (ph = ppi->progHash[h]; ph; ph = ph->next) {
        found = 1;
        for (i = 0; i < NUM_ENABLE_WORDS; i++) {
            if (ph->cookedEnables[i] != ppi->cookedEnables[i]) {
                found = 0;
                break;
            }
        }
        if (found) return ph;
    }

    return NULL;
}

hwProgHash __hwGlSaveProg(hwProgInfo *ppi,
                          hwInt32 vtxShader, hwInt32 fragShader,
                          hwInt32 program)
{
    hwProgHash ph;
    hwInt32 i, h;

    ph = malloc(sizeof(struct __hwProgHash));
    if (!ph) return NULL;

    for (i = 0; i < NUM_ENABLE_WORDS; i++) {
        ph->cookedEnables[i] = ppi->cookedEnables[i];
    }

    ph->vtxShader = vtxShader;
    ph->fragShader = fragShader;
    ph->program = program;

    h = __hwGlHashEnables(ppi);
    ph->next = ppi->progHash[h];
    ppi->progHash[h] = ph;

    return ph;
}

static const char
    *attribNames[MAX_VTX_ATTRIB] = {
        "aPosition",    // 0
        "aColor",       // 1
        "aNormal",      // 2
        "aTangent",     // 3
        "aTexCoord0",   // 4
        "aTexCoord1",   // 5
        "aTexCoord2",   // 6
        "aTexCoord3",   // 7
        "aTexCoord4",   // 8
        "aTexCoord5",   // 9
        "aTexCoord6",   // 10
        "aTexCoord7",   // 11
    };

hwProgHash __hwGlCreateProgram(hwProgInfo *ppi)
{
    hwInt32 vtxShader, fragShader, program;
    hwProgHash ph;
    int i;

    /* Create/compile the vertex shader */
    __hwGlCreateVtxProgram(ppi);
    vtxShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtxShader, ppi->vtxProg.numLines,
                        (void *)ppi->vtxProg.lines, NULL);
    glCompileShader(vtxShader);
    diagnoseShader(vtxShader, "VTX");

    /* Create/compile the fragment shader */
    __hwGlCreateFragProgram(ppi);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, ppi->fragProg.numLines,
                        (void *)ppi->fragProg.lines, NULL);
    glCompileShader(fragShader);
    diagnoseShader(fragShader, "FRG");

    /* Create the program and attach the shaders */
    program = glCreateProgram();
    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);

    /* Bind vertex attribs to the program */
    for (i = 0; i < MAX_VTX_ATTRIB; i++) {
        glBindAttribLocation(program, i, attribNames[i]);
    }

    /* Link the program */
    glLinkProgram(program);
    diagnoseProgram(program);

    /* Save the program */
    return __hwGlSaveProg(ppi, vtxShader, fragShader, program);
}

#define CHK(a)          IS_COOKED(ENA_##a)
#define CHK_I(a, i)     IS_COOKED(ENA_##a + i)

void hwGlBeginRendering( void )
{
    USE_PROG_INFO;
    int i;
    hwInt32 prog;
    hwProgHash ph;
    hwLightInfo *li;
    hwMaterialInfo *mi;
    hwFloat tmp;
    hwFogInfo *fi;

    VALIDATE_PROG_INFO();

    if (ppi->progDirty) {
        /* We need to handle the program before the uniforms */
        __hwGlCookEnables(ppi);

        ph = __hwGlFindProg(ppi);
        if (!ph) {
            ph = __hwGlCreateProgram(ppi);
        }

        glUseProgram(ph->program);

        ppi->program = ph->program;
        ppi->progDirty = 0;

        /* HACK!  When we change the program we need to re-sync uniforms! */
        ppi->constDirty = ALL_DIRTY_BITS;
    }

    if (!ppi->constDirty) return;

    prog = ppi->program;

    if ((ppi->constDirty & ANY_LIGHT_DIRTY) && CHK(LIGHTING)) {
        for (i = 0; i < MAX_LIGHTS; i++) {
            if (!CHK_I(LIGHT0,i)) continue;
            if (!IS_DIRTY(DIRTY_LIGHT0 + i)) continue;

            li = ppi->lights + i;
            fltUniformFmt(prog, "uLightPos_%d",    i, 4, li->pos);
            if (CHK_I(LIGHT0_SPOT,i)) {
                fltUniformFmt(prog, "uSpotDir_%d",     i, 3, li->spotDir);
                fltUniformFmt(prog, "uSpotCutoff_%d",  i, 1, &li->spotCutoff);
                fltUniformFmt(prog, "uSpotExp_%d",     i, 1, &li->spotExp);
            }
            if (CHK_I(LIGHT0_ATT,i)) {
                fltUniformFmt(prog, "uLinearAtten_%d", i, 1, &li->linearAtten);
            }
            fltUniformFmt(prog, "uAmbient_%d",     i, 4, li->ambient);
            fltUniformFmt(prog, "uDiffuse_%d",     i, 4, li->diffuse);
            fltUniformFmt(prog, "uSpecular_%d",    i, 4, li->specular);
        }
    }
    for (i = 0; i < MAX_TEX; i++) {
        if (CHK_I(TEX0,i)) {
            intUniformFmt(prog, "uTexture%d", i, i);
        }
    }

    if (IS_DIRTY(DIRTY_FOG) && CHK(FOG)) {
        fi = &ppi->fog;
        if (CHK(FOG_EXP) || CHK(FOG_EXP2)) {
            fltUniform(prog, "uFogDensity", 1, &fi->density);
        }
        if (CHK(FOG_LINEAR)) {
            tmp = fi->fogEnd - fi->fogStart;
            if (tmp != 0.f) tmp = 1.0 / tmp;
            fltUniform(prog, "uFogDenom",   1, &tmp);
            fltUniform(prog, "uFogEnd",     1, &fi->fogEnd);
        }
        fltUniform(prog, "uFogColor",   4, fi->color);
    }

    if (IS_DIRTY(DIRTY_MATERIAL) && CHK(LIGHTING)) {
        mi = &ppi->material;
        fltUniform(prog, "uMatSpecular",  4, mi->specular);
        fltUniform(prog, "uMatEmission",  4, mi->emission);
        fltUniform(prog, "uMatShininess", 1, &mi->specExp);
    }

    if (IS_DIRTY(DIRTY_LIGHT_MODEL) && CHK(LIGHTING)) {
        fltUniform(prog, "uGlobalAmbient", 4, ppi->globalAmbient);
    }

    if (IS_DIRTY(DIRTY_MODELVIEW) || IS_DIRTY(DIRTY_PROJECTION)) {
        matMult(ppi->mvpMatrix, ppi->mvMat, ppi->prMat);
        fltUniform(prog, "uMvpMatrix", 16, ppi->mvpMatrix);
    }

    if (IS_DIRTY(DIRTY_MODELVIEW) && CHK(LIGHTING)) {
        (void)invXpose3x3(ppi->mvInvXpose, ppi->mvMat);
        fltUniform(prog, "uMvInvXpose", 9, ppi->mvInvXpose);
    }

    if (IS_DIRTY(DIRTY_MODELVIEW) &&
        (CHK(LIGHTING) || CHK(FOG)))
    {
        fltUniform(prog, "uMvMatrix", 16, ppi->mvMat);
    }

    if (IS_DIRTY(DIRTY_COLOR) && !CHK(VERTEX_COLOR)) {
        fltUniform(prog, "uColor", 4, ppi->color);
    }

    ppi->constDirty = 0;
}

void hwGlEnableClientState( hwInt32 ena )
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, ena);
    if (bit < 0) INVALID_ENUM_RETURN;

    if (ppi->vtxAttribEnables & (1 << bit)) return;

    glEnableVertexAttribArray(bit);

    /* HACK!  For some reason, GLFW on Linux does not allow
     * generic glVertexAttrib4f to link up with the vertex
     * shader.  So, pass color in a constant.  To do this
     * correctly, we have to generate a different program
     * when the constant is used.
     */
    if (bit == HWGL_VA_COLOR) {
        DO_ENABLE(ENA_VERTEX_COLOR);
        ppi->progDirty = 1;
    }

    ppi->vtxAttribEnables |= (1 << bit);
}

void hwGlDisableClientState( hwInt32 ena )
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, ena);
    if (bit < 0) INVALID_ENUM_RETURN;

    if (!(ppi->vtxAttribEnables & (1 << bit))) return;

    glDisableVertexAttribArray(bit);

    /* HACK!  For some reason, GLFW on Linux does not allow
     * generic glVertexAttrib4f to link up with the vertex
     * shader.  So, pass color in a constant.  To do this
     * correctly, we have to generate a different program
     * when the constant is used.
     */
    if (bit == HWGL_VA_COLOR) {
        DO_DISABLE(ENA_VERTEX_COLOR);
        ppi->progDirty = 1;
    }

    ppi->vtxAttribEnables &= ~(1 << bit);
}

void hwGlClientActiveTexture( hwInt32 tex )
{
    USE_PROG_INFO;
    int bit;

    switch (tex) {
    case HWGL_TEXTURE0 :
    case HWGL_TEXTURE1 :
    case HWGL_TEXTURE2 :
    case HWGL_TEXTURE3 :
    case HWGL_TEXTURE4 :
    case HWGL_TEXTURE5 :
    case HWGL_TEXTURE6 :
    case HWGL_TEXTURE7 :
        ppi->clientActiveTexture = tex;
        break;
    default :
        INVALID_ENUM_RETURN;
    }
}

void hwGlVertexPointer(hwInt32 size, hwInt32 typ,
                       hwInt32 stride, const void *ptr)
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, HWGL_VERTEX_ARRAY);
    glVertexAttribPointer(bit, size, typ, GL_FALSE, stride, ptr);
}

void hwGlColorPointer(hwInt32 size, hwInt32 typ,
                      hwInt32 stride, const void *ptr)
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, HWGL_COLOR_ARRAY);
    glVertexAttribPointer(bit, size, typ, GL_TRUE, stride, ptr);
}

void hwGlNormalPointer(hwInt32 typ, hwInt32 stride, const void *ptr)
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, HWGL_NORMAL_ARRAY);
    glVertexAttribPointer(bit, 3, typ, GL_TRUE, stride, ptr);
}

void hwGlTangentPointer(hwInt32 typ, hwInt32 stride, const void *ptr)
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, HWGL_TANGENT_ARRAY);
    glVertexAttribPointer(bit, 4, typ, GL_TRUE, stride, ptr);
}

void hwGlTexCoordPointer(hwInt32 size, hwInt32 typ,
                         hwInt32 stride, const void *ptr)
{
    USE_PROG_INFO;
    int bit;

    bit = getAttribNum(ppi, HWGL_TEXTURE_COORD_ARRAY);
    glVertexAttribPointer(bit, size, typ, GL_FALSE, stride, ptr);
}

void hwGlColor4f(hwFloat r, hwFloat g, hwFloat b, hwFloat a)
{
    USE_PROG_INFO;

    /* HACK!  For some reason, GLFW on Linux does not allow
     * generic glVertexAttrib4f to link up with the vertex
     * shader.  So, pass color in a constant.  To do this
     * correctly, we have to generate a different program
     * when the constant is used.
     */
    ppi->color[0] = r;
    ppi->color[1] = g;
    ppi->color[2] = b;
    ppi->color[3] = a;
    DO_DIRTY(DIRTY_COLOR);
}

void hwGlColor4ub(hwInt32 r, hwInt32 g, hwInt32 b, hwInt32 a)
{
    USE_PROG_INFO;

    /* HACK!  For some reason, GLFW on Linux does not allow
     * generic glVertexAttrib4f to link up with the vertex
     * shader.  So, pass color in a constant.  To do this
     * correctly, we have to generate a different program
     * when the constant is used.
     */
    ppi->color[0] = r*(1.f/255.f);
    ppi->color[1] = g*(1.f/255.f);
    ppi->color[2] = b*(1.f/255.f);
    ppi->color[3] = a*(1.f/255.f);
    DO_DIRTY(DIRTY_COLOR);
}

void hwGlShadeModel( hwInt32 pname )
{
    USE_PROG_INFO;

    if (pname != HWGL_SMOOTH) INVALID_ENUM_RETURN;
}

void hwGlPointSize( hwFloat size )
{
    // TBD
}
