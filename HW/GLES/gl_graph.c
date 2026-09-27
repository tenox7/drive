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
#include "hw_gl.h"
#include "hw_vecmath.h"

#if defined(ANDROID_NDK)
#include <android/log.h>
#endif

#if defined(WIN32) && !defined(CYGWIN) && !defined(MINGW)
#  define ISNAN(x) _isnan((double)x)
#else
#  define ISNAN(x) isnan(x)
#endif

void __hwGlPosLight
(
    hwDisplay disp,
    hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
    hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
)
{
    USE_GL_CTX(disp);
    hwFloat
        ambZero[4] = { 0.0, 0.0, 0.0, 1.0 };
    hwInt32
        NumLights, attr, n, LightNum;
    hwFloat
        newColor[4],
        newPos[4];

    LOG_ENTRY
    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    glctx->numLights++;
    NumLights = glctx->numLights;

    switch( NumLights ) {
    case 1 :  LightNum = HWGL_LIGHT0;           break;
    case 2 :  LightNum = HWGL_LIGHT1;           break;
    case 3 :  LightNum = HWGL_LIGHT2;           break;
    case 4 :  LightNum = HWGL_LIGHT3;           break;
    case 5 :  LightNum = HWGL_LIGHT4;           break;
    case 6 :  LightNum = HWGL_LIGHT5;           break;
    case 7 :  LightNum = HWGL_LIGHT6;           break;
    case 8 :  LightNum = HWGL_LIGHT7;           break;
    default : return;         /* Max 8 lights */
    }

    VCOPY( newPos, Pos );
    newPos[3] = 1.0;                /* Positional */
    hwGlLightfv( LightNum, HWGL_POSITION, newPos );
    hwGlLightfv( LightNum, HWGL_SPOT_DIRECTION, Dir );

    if( Cutoff < 0.0 )              Cutoff = 180.0;
    hwGlLightf( LightNum, HWGL_SPOT_CUTOFF, Cutoff );

    if( LightExp < 0.0 )            LightExp = 0.0;
    hwGlLightf( LightNum, HWGL_SPOT_EXPONENT, LightExp );

    if( Atten < 0.0 )               Atten = 0.0;
    hwGlLightf( LightNum, HWGL_LINEAR_ATTENUATION, Atten );

#if 0
    // Global ambient is the only Hoverware ambient
    hwGlLightfv( LightNum, HWGL_AMBIENT, glctx->ambColor );
#endif
    hwGlLightfv( LightNum, HWGL_AMBIENT, ambZero );

    VCOPY( newColor, Color );               newColor[3] = 1.0;
    hwGlLightfv( LightNum, HWGL_DIFFUSE, newColor );

    VASSIGN( newColor, 1.0, 1.0, 1.0 );
    /*VASSIGN( newColor, 0.6, 0.6, 0.6 );*//*TBD:???*/
    hwGlLightfv( LightNum, HWGL_SPECULAR, newColor );

    if( glctx->lightOn ) {
        hwGlEnable( LightNum );
    }

    /* "Lock in" the current camera */
    glctx->camSet = 1;
    LOG_EXIT
}

void __hwGlDirLight( hwDisplay disp, hwFloat Color[3], hwFloat Dir[3] )
{
    USE_GL_CTX(disp);
    hwInt32
        NumLights, LightNum;
    hwFloat
        newColor[4],
        newDir[4];

    LOG_ENTRY
    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    glctx->numLights++;
    NumLights = glctx->numLights;

    switch( NumLights ) {
    case 1 :  LightNum = HWGL_LIGHT0;           break;
    case 2 :  LightNum = HWGL_LIGHT1;           break;
    case 3 :  LightNum = HWGL_LIGHT2;           break;
    case 4 :  LightNum = HWGL_LIGHT3;           break;
    case 5 :  LightNum = HWGL_LIGHT4;           break;
    case 6 :  LightNum = HWGL_LIGHT5;           break;
    case 7 :  LightNum = HWGL_LIGHT6;           break;
    case 8 :  LightNum = HWGL_LIGHT7;           break;
    default : return;         /* Max 8 lights */
    }

    VCOPY( newDir, Dir );
    VNORM( newDir, newDir );
    VMULC(newDir, newDir, -1.0);
    newDir[3] = 0.0;                /* Directional */
    hwGlLightfv( LightNum, HWGL_POSITION, newDir );

    VCOPY( newColor, Color );               newColor[3] = 1.0;
    hwGlLightfv( LightNum, HWGL_DIFFUSE, newColor );

    VASSIGN( newColor, 1.0, 1.0, 1.0 );
    /*VASSIGN( newColor, 0.6, 0.6, 0.6 );*//*TBD:???*/
    hwGlLightfv( LightNum, HWGL_SPECULAR, newColor );

    /* Defaults for other light parms */
    hwGlLightf( LightNum, HWGL_SPOT_CUTOFF, 180.0 );
    hwGlLightf( LightNum, HWGL_SPOT_EXPONENT, 0.0 );
    hwGlLightf( LightNum, HWGL_LINEAR_ATTENUATION, 0.0 );

    if( glctx->lightOn ) {
        hwGlEnable( LightNum );
    }

    /* "Lock in" the current camera */
    glctx->camSet = 1;
    LOG_EXIT
}

void __hwGlBackground( hwDisplay disp, hwFloat Color[3] )
{
    LOG_ENTRY
    glClearColor( Color[0], Color[1], Color[2], 1.0 );
    LOG_EXIT
}

void __hwGlLighting( hwDisplay disp, hwInt32 OnOff )
{
    USE_GL_CTX(disp);
    LOG_ENTRY
    if( !glctx )             return;
    glctx->lightOn = OnOff;
    if( OnOff && (glctx->wireframeState != WFS_WIREFRAME) ) {
        hwGlEnable( HWGL_LIGHTING );
    }
    else {
        hwGlDisable( HWGL_LIGHTING );
    }
    LOG_EXIT
}

void __hwGlFog( hwDisplay disp, hwInt32 OnOff, hwFloat Color[3] )
{
    USE_GL_CTX(disp);
    hwFloat Color4[4];

    LOG_ENTRY
    if( !glctx ) return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    if( OnOff ) {
        memcpy(Color4, Color, 3 * sizeof(float));
        hwGlEnable(HWGL_FOG);
        hwGlFogfv(HWGL_FOG_COLOR, Color4);
        glctx->enableBits |= ENABLE_FOG;
    }
    else {
        hwGlDisable(HWGL_FOG);
        glctx->enableBits &= ~ENABLE_FOG;
    }
    LOG_EXIT
}

void __hwGlFogParams( hwDisplay disp,
                      hwInt32 t,             /* Type LINEAR, EXP, or EXP2 */
                      hwFloat p[2],          /* Front and back Planes */
                      hwFloat d )           /* density */
{
    USE_GL_CTX(disp);
    if( !glctx ) return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    switch( t ) {
    case HW_FOG_LINEAR:
        hwGlFogf( HWGL_FOG_START, p[0]);
        hwGlFogf( HWGL_FOG_END,   p[1]);
        hwGlFogi( HWGL_FOG_MODE,  HWGL_LINEAR);
        break;
    case HW_FOG_EXP:
        hwGlFogf( HWGL_FOG_DENSITY, d);
        hwGlFogi( HWGL_FOG_MODE, HWGL_EXP);
        break;
    case HW_FOG_EXP2:
        hwGlFogf( HWGL_FOG_DENSITY, d);
        hwGlFogi( HWGL_FOG_MODE, HWGL_EXP2);
        break;
    }
}


void __hwGlAmbient( hwDisplay disp, hwFloat Factor, hwFloat Color[3] )
{
    USE_GL_CTX(disp);
    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    glctx->ambFact = Factor;
    glctx->ambColor[0] = Color[0] * Factor;
    glctx->ambColor[1] = Color[1] * Factor;
    glctx->ambColor[2] = Color[2] * Factor;
    glctx->ambColor[3] = 1.0;
    glctx->ambFactSave = -1.0;
    hwGlLightModelfv( HWGL_LIGHT_MODEL_AMBIENT,
                      glctx->ambColor );
}



void __hwGlDestroyTexture( hwDisplay disp, hwInt32 tid )
{
    /* TBD */
}

/*
 * Really ugly code to associate a tex ID with texture parameters.
 * Don't get me started on all the ways this is stoopid.  We
 * keep an array of glTexture structures and just use the ID
 * to index it.  And we just increment until we run out of space
 * as we allocate IDs.  Didn't I say not to get me started?
 */

static glTexture
        **texArray;
static hwInt32
        texArraySize,
        texArrayAlloc;

static hwInt32 allocTexId(void)
{
    if( texArraySize >= texArrayAlloc ) {
        texArrayAlloc = texArrayAlloc ? (2*texArrayAlloc) : 32;
        texArray = realloc(texArray, texArrayAlloc * sizeof(glTexture *));
        if( !texArray ) {
            return -1;
        }
    }
    texArray[texArraySize] = 0;
    return 1+texArraySize++;
}

static void saveTexParms(hwInt32 texId, glTexture *tex)
{
    texId--;
    if( texId >= texArraySize ) return;
    if( !texArray[texId] ) {
        texArray[texId] = malloc(sizeof(glTexture));
        if( !texArray[texId] ) return;
    }
    memcpy(texArray[texId], tex, sizeof(glTexture));
}

static void delTexId(hwInt32 texId)
{
    texId--;
    if( texArray[texId] ) {
        /* TBD: Delete the texture name? */
        free( texArray[texId] );
        texArray[texId] = 0;
    }
}

static glTexture *findTexId(glDrawable *glctx, hwInt32 texId)
{
    texId--;
    if( texId >= texArraySize ) return 0;
    return texArray[texId];
}

void __hwGlEstablishTexture( hwDisplay disp, glTexture *tm )
{
    USE_GL_CTX(disp);
    hwInt32
        i, j, w, h, fmt, comp,
        result;
    GLuint
        texName;

    if( !glctx )             return;

#if defined(ANDROID_DEBUG)
__android_log_print(ANDROID_LOG_INFO, "HoverWare", "EstablishTexture");
#endif

    result = tm->textureID;
    if( result < 0 ) {
        result = allocTexId();
    }
    if( result < 0 )                return;         /* TBD */

    if( tm->texName == -1 ) {
        glGenTextures( 1, &texName );
        tm->texName = (hwInt32)texName;
    }
    else {
        texName = (GLuint)tm->texName;
    }

    glBindTexture( GL_TEXTURE_2D, texName );

    w = tm->img->width; h = tm->img->height;

    switch( tm->internalFormat ) {
    case HW_TM_ALPHA :
        fmt = GL_ALPHA;
        comp = GL_ALPHA;
        break;
/*
    case HW_TM_INTENSITY :
        fmt = GL_RED;
        comp = GL_INTENSITY;
        break;
*/
    default :
        comp = tm->img->components;
        switch( comp ) {
        case 1 : fmt = GL_LUMINANCE;       break;
        case 2 : fmt = GL_LUMINANCE_ALPHA; break;
        case 3 : fmt = GL_RGB;             break;
        case 4 : fmt = GL_RGBA;            break;
        }
    }

#if defined(ANDROID_DEBUG)
__android_log_print(ANDROID_LOG_INFO, "HoverWare", "EstablishTexture %d x %d @ %d", w, h, comp);
#endif

    hwGluBuild2DMipmaps( GL_TEXTURE_2D, fmt,
                       w, h, fmt, GL_UNSIGNED_BYTE, tm->img->data );

    if( tm->bound ) {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    }
    else {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    }

    if( tm->filt ) {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                            GL_LINEAR_MIPMAP_LINEAR );
    }
    else {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    }
    tm->textureID = result;
    saveTexParms(result, tm);
}

static void callTexId(glDrawable *glctx, hwInt32 id)
{
    glTexture *tm;
    glDisplay *gldisp;

    gldisp = glctx->disp;

    tm = findTexId(glctx, id);
    if( !tm ) return;

#if defined(ANDROID_DEBUG)
__android_log_print(ANDROID_LOG_INFO, "HoverWare", "callTexId(%d)", id);
#endif

    glBindTexture( GL_TEXTURE_2D, tm->texName );

    switch( tm->coordMode ) {
    case HW_TM_ENVMAP :
        hwGlTexGeni( HWGL_TEXTURE_GEN_MODE, HWGL_REFLECTION_MAP );
        break;
    case HW_TM_STAGE_0 :
        hwGlTexGeni( HWGL_TEXTURE_GEN_MODE, HWGL_STAGE_0 );
        break;
    default :
        hwGlTexGeni( HWGL_TEXTURE_GEN_MODE, HWGL_NONE );
        break;
    }

    switch( tm->appl ) {
    case HW_TM_BUMP :
        hwGlTexEnvi( HWGL_TEXTURE_ENV_MODE, HWGL_NORMAL_MAP );
        break;
    case HW_TM_RELIEF :
        hwGlTexEnvi( HWGL_TEXTURE_ENV_MODE, HWGL_RELIEF_MAP );
        break;
    case HW_TM_MODULATE :
        hwGlTexEnvi( HWGL_TEXTURE_ENV_MODE, HWGL_MODULATE );
        break;
    case HW_TM_GLOSS :
        hwGlTexEnvi( HWGL_TEXTURE_ENV_MODE, HWGL_GLOSS_MAP);
        break;
    }

    hwGlLightModeli( HWGL_LIGHT_MODEL_COLOR_CONTROL,
                                       HWGL_SEPARATE_SPECULAR_COLOR );
    hwGlEnable( HWGL_COLOR_SUM );
}

hwInt32 __hwGlCreateTexture
(
    hwDisplay disp,
    hwImageStruct *img, hwInt32 filt, hwInt32 appl, hwInt32 bound,
    hwInt32 coordMode, hwOrientType *orient, hwInt32 internalFormat
)
{
    USE_GL_CTX(disp);
    glTexture
        tm;

#if defined(ANDROID_DEBUG)
    __android_log_print(ANDROID_LOG_INFO, "HoverWare", "__hwGlCreateTexture -- img: %x ",img);
#endif
    if( !glctx )             return -1;
    if( !img ) return -1;

#if 0
    tm = malloc( sizeof(glTexture) );
    if( !tm )               return 0;
#endif

    tm.img = img;
    tm.filt = filt;
    tm.appl = appl;
    tm.bound = bound;
    tm.coordMode = coordMode;
    tm.orient = *orient;
    tm.internalFormat = internalFormat;
    tm.textureID = -1;
    tm.texName = -1;
    __hwGlEstablishTexture( disp, &tm );

    return tm.textureID;
}

void __hwGlCurrentTexture( hwDisplay disp, hwInt32 tid, hwInt32 texNum )
{
    USE_GL_CTX(disp);
    if( !glctx )             return;
    if( texNum >= glctx->maxTexNum ) return;

    /* Set currently active texture */
    if( texNum != glctx->tmActive ) {
        hwGlActiveTexture( HWGL_TEXTURE0 + texNum );
        glctx->tmActive = texNum;
    }

    if( tid != -1 ) {
        if( glctx->tmId[texNum] == -1 ) {
            if( !(glctx->renderMode & HW_RENDER_EDGE_MODE) ) {
                hwGlEnable( HWGL_TEXTURE_2D );
            }
        }
        if( tid != glctx->tmId[texNum] ) {
            callTexId(glctx, tid);
            glctx->tmId[texNum] = tid;
        }
    }
    else {
        if( glctx->tmId[texNum] != -1 ) {
            hwGlDisable( HWGL_TEXTURE_2D );
            glctx->tmId[texNum] = -1;
        }
    }
}

void __hwGlDrawBBox( hwDisplay disp, hwFloat Points[24] )
{
    USE_GL_CTX(disp);
    GLushort line0[] = {4,0,1,5,7};
    GLushort line1[] = {0,2,3,1};
    GLushort line2[] = {5,4,6,2};
    GLushort line3[] = {3,7,6};

    __hwgl_SetWireframeState(glctx);

    hwGlVertexPointer( 3, HWGL_FLOAT, 0, Points);
    hwGlBeginRendering();
    glDrawElements( GL_LINE_STRIP, 5, GL_UNSIGNED_SHORT, line0 );
    glDrawElements( GL_LINE_STRIP, 4, GL_UNSIGNED_SHORT, line1 );
    glDrawElements( GL_LINE_STRIP, 4, GL_UNSIGNED_SHORT, line2 );
    glDrawElements( GL_LINE_STRIP, 3, GL_UNSIGNED_SHORT, line3 );
}

int __hwGlAnimVisible( hwDisplay disp, hwInt32 v )
{
    USE_GL_CTX(disp);
    unsigned long
        srcMask, dstMask = 0;

    if( !glctx )             return 0;

    if( glctx->animMask ) {
        /* Check for Hoverball-style animation - all of the bits
         * in the animMask MUST be present in the surface mask
         */
        srcMask = (unsigned long) glctx->animMask;
        dstMask = (unsigned long) v;
        if( (srcMask & dstMask) != srcMask ) {
            return 0;
        }
    }
    else {
        /* Check for DRIVE-style animation:
         *        "Graphical primitives will not be drawn whenever the current
         *         name set includes any of the names in the invisibility
         *         filter's inclusion set but doesn't include any of the names
         *         in the invisibility filter's exclusion set."
         */
        dstMask = (unsigned long) v;
        if( glctx->invisibleInclude & dstMask ) {
            if( !(glctx->invisibleExclude & dstMask) ) {
                return 0;
            }
        }
    }
    return 1;
}

int __hwGlBoundsVisible
(
    hwDisplay disp, hwSurfaceType *surf, hwFloat BBox[6], hwInt32 complexity
)
{
    USE_GL_CTX(disp);
    glDisplay
        *gldisp = (glDisplay *)disp;
    hwFloat
        Det, *Tmp, Points[24], OPoints[24];
    hwInt32
        i, j, n, Inside;

    if( !glctx )             return 0;

    if( glctx->openList ) {
        __hwGlAppendVisibility( glctx, surf->visibility );
        return 1;
    }

    if( surf->flags & HW_SURF_INVISIBLE )           return 0;

    if( !__hwGlAnimVisible( disp, surf->visibility ) ) return 0;

    /* Don't even check if not very complex */
    if( complexity < 25 )           return 1;

    if( !BBox ) return 1;
    if( !glctx->planesValid )                return 1;

    Tmp = Points;
    *Tmp++ = BBox[0]; *Tmp++ = BBox[1]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[1]; *Tmp++ = BBox[5];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[4]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[4]; *Tmp++ = BBox[5];

    *Tmp++ = BBox[3]; *Tmp++ = BBox[1]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[1]; *Tmp++ = BBox[5];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[4]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[4]; *Tmp   = BBox[5];

    if( gldisp->glob.showBoxes ) {
        (void)memcpy( OPoints, Points, sizeof(Points) );
    }

    n = glctx->stackDepth - 1;
    if( n >= 0 ) {
        for( i = 0; i < 8; i++ ) {
            hwTransform( glctx->rawMatStack[n], Points+3*i );
        }
    }
    n = glctx->planesValid;
    for( i = 0; i < n; i++ ) {
        Inside = 0; Tmp = Points;
        for( j = 0; j < 8; j++ ) {
            Det = VDOT( Tmp, glctx->WC_Planes[i] );
            Det += glctx->WC_Planes[i][3];
            if( Det > 0. ) { Inside = 1; break; }
            Tmp += 3;
        }
        if( !Inside ) {
            if( gldisp->glob.showBoxes ) {
                hwGlColor4f( 1.0, 0.0, 1.0, 1.0 );
                __hwGlDrawBBox( disp, OPoints );
            }
            return 0;
        }
    }

    if( gldisp->glob.showBoxes ) {
        hwGlColor4f( 1.0, 1.0, 1.0, 1.0 );
        __hwGlDrawBBox( disp, OPoints );
    }

    return 1;
}

hwFloat __hwGlBoundsSize( hwDisplay disp, hwFloat BBox[6] )
{
    USE_GL_CTX(disp);
    hwFloat
        Points[6],
        len, dist, ang,
        dx, dy, dz,
        px, py, pz;
    int
        i, n;

    if( !glctx )             return 0.0;

    (void)memcpy( Points, BBox, 6*sizeof(float) );

    /* Get the point in VDCs */
    n = glctx->stackDepth - 1;
    if( n >= 0 ) {
        hwTransform( glctx->rawMatStack[n], Points );
        hwTransform( glctx->rawMatStack[n], Points+3 );
    }

    /* We now have a world-coordinate diagonal.  Its half-length
     * is the radius of a sphere which we will use for our nefarious
     * angle-calculating purposes.  The sphere's center is the center
     * of the bounding box.
     */

    px = (Points[0] + Points[3]) * 0.5;
    py = (Points[1] + Points[4]) * 0.5;
    pz = (Points[2] + Points[5]) * 0.5;
    dx = px - glctx->camPos[0];
    dy = py - glctx->camPos[1];
    dz = pz - glctx->camPos[2];
    dist = sqrt( dx*dx + dy*dy + dz*dz );

    dx = (Points[0] - Points[3]);
    dy = (Points[1] - Points[4]);
    dz = (Points[2] - Points[5]);
    len = sqrt( dx*dx + dy*dy + dz*dz ) * 0.5;

    if( (dist <= 0.0) || ISNAN(dist) || (len >= dist) ) {
        /* Oops - inside the object.  Return maximum. */
        return 99.999;
    }

    ang = asin( len / dist );
    ang = ang / glctx->camField;
    if( ang > 0.99999 ) ang = 0.99999;
    return ang * 100.0;
}

void __hwGlCamera( hwDisplay disp, hwCamStruct *cam )
{
    USE_GL_CTX(disp);
    glDisplay
        *gldisp = (glDisplay *)disp;
    hwFloat
        *ptr,
        (*mat)[4],
        normDir[3],
        pos[3], dir[3], up[3], right[3],
        tmpMat[4][4],
        WC_Corners[4][3],
        dist, dist1;
    hwInt32
        i, n;
    CameraArg
        glcam;

    if( !glctx )             return;
    if( glctx->camSet )              return;

    n = glctx->stackDepth - 1;
    if( n >= 0 ) {
        mat = (void *)&glctx->matStack[n][0][0];
    }

    VCOPY( pos, cam->pos );
    if( n >= 0 ) {
        /* VXFORM4, since it's position and not a vector */
        VXFORM4( pos, pos, mat );
    }

    VCOPY( up, cam->up );
    if( n >= 0 ) {
        /* VXFORM3, since it's a vector */
        VXFORM3( up, up, mat );
    }

    glcam.field_of_view = cam->field;
#if 1
    dist = glctx->VDC_YMax / glctx->VDC_XMax;
    if( dist < 1.0 ) {
        /* Starbase field of view is always in Y.  We want the
         * field of view to be the maximum in X and Y.  SO,
         * we do this little kludge.
         */
        glcam.field_of_view *= dist;
    }
#endif
    glctx->camField = glcam.field_of_view*3.141592653589/180.0;

    VCOPY( dir, cam->dir );
    if( n >= 0 ) {
        /* VXFORM3, since it's a vector */
        VXFORM3( dir, dir, mat );
    }
    VNORM( normDir, dir );

    VCROSS( right, up, dir );
    VNORM( right, right );

    VCROSS( up, dir, right );
    VNORM( up, up );

    dist = (cam->planes[0] + cam->planes[1]) * 0.5f;
    dist1 = 100.0f * cam->planes[0];
    if( dist > dist1 ) dist = dist1;

    glctx->camPos[0] = glcam.camx = pos[0];
    glctx->camPos[1] = glcam.camy = pos[1];
    glctx->camPos[2] = glcam.camz = pos[2];

    glcam.upx = up[0];
    glcam.upy = up[1];
    glcam.upz = up[2];

    glcam.refx = glcam.camx + dist * dir[0];
    glcam.refy = glcam.camy + dist * dir[1];
    glcam.refz = glcam.camz + dist * dir[2];

    glcam.front = cam->planes[0] - dist;
    glcam.back = cam->planes[1] - dist;

    glcam.projection = cam->perspective;
    glcam.skewX = cam->skew[0];
    glcam.skewY = cam->skew[1];
    glcam.jitterX = cam->jitter[0];
    glcam.jitterY = cam->jitter[1];
    glcam.mirror = cam->mirror;

    /* Make sure to be at the *bottom* of the matrix stack */
    hwGlMatrixMode( HWGL_MODELVIEW );
    for( i = 0; i <= n; i++ ) {
        hwGlPopMatrix();
    }

    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        __glCamera( &glcam, &glctx->sel,
                    glctx->winW, glctx->winH );
    }
    else {
        __glCamera( &glcam, 0,
                    glctx->winW, glctx->winH );
    }

    hwGlGetFloatv( HWGL_MODELVIEW_MATRIX,
                   &glctx->camMat[0][0] );
    hwGlGetFloatv( HWGL_PROJECTION_MATRIX,
                   &glctx->projMat[0][0] );

    /* Get corners of view volume, in WCs.  The viewport need not sit at
     * the window origin (DRIVE puts its dash below the 3D view).
     */
    {
        GLint viewport[4];
        hwFloat modelMatrix[16];
        hwFloat projMatrix[16];
        hwFloat xyz[3];
        hwFloat x0, y0, x1, y1;

        hwGlGetFloatv( HWGL_MODELVIEW_MATRIX, modelMatrix );
        hwGlGetFloatv( HWGL_PROJECTION_MATRIX, projMatrix );

        glGetIntegerv( GL_VIEWPORT, viewport );
        x0 = viewport[0]; x1 = x0 + viewport[2];
        y0 = viewport[1]; y1 = y0 + viewport[3];

        hwGluUnProject( x0, y0, 0.f,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[0][0] = xyz[0];
        WC_Corners[0][1] = xyz[1];
        WC_Corners[0][2] = xyz[2];

        hwGluUnProject( x1, y0, 0.f,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[1][0] = xyz[0];
        WC_Corners[1][1] = xyz[1];
        WC_Corners[1][2] = xyz[2];

        hwGluUnProject( x1, y1, 0.f,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[2][0] = xyz[0];
        WC_Corners[2][1] = xyz[1];
        WC_Corners[2][2] = xyz[2];

        hwGluUnProject( x0, y1, 0.f,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[3][0] = xyz[0];
        WC_Corners[3][1] = xyz[1];
        WC_Corners[3][2] = xyz[2];
    }

    /* Convert corners to vectors from camera to corners */
    for( i = 0; i < 4; i++ ) {
        WC_Corners[i][0] -= pos[0];
        WC_Corners[i][1] -= pos[1];
        WC_Corners[i][2] -= pos[2];
        VNORM( WC_Corners[i], WC_Corners[i] );
    }

    /* Establish plane equations */
    glctx->WC_Planes[0][0] = normDir[0];
    glctx->WC_Planes[0][1] = normDir[1];
    glctx->WC_Planes[0][2] = normDir[2];

    VCROSS(glctx->WC_Planes[1],WC_Corners[0],WC_Corners[1] );
    VCROSS(glctx->WC_Planes[2],WC_Corners[1],WC_Corners[2] );
    VCROSS(glctx->WC_Planes[3],WC_Corners[2],WC_Corners[3] );
    VCROSS(glctx->WC_Planes[4],WC_Corners[3],WC_Corners[0] );

    VNORM(glctx->WC_Planes[1],
                    glctx->WC_Planes[1]);
    VNORM(glctx->WC_Planes[2],
                    glctx->WC_Planes[2]);
    VNORM(glctx->WC_Planes[3],
                    glctx->WC_Planes[3]);
    VNORM(glctx->WC_Planes[4],
                    glctx->WC_Planes[4]);

    /* Make sure position of camera is accounted for in plane equations */
    for( i = 0; i < 5; i++ ) {
        ptr = glctx->WC_Planes[i];
        ptr[3] = -VDOT(ptr,pos);
    }

    if( cam->mirror )                   glctx->planesValid = 0;
    else if( !gldisp->glob.doPrune )    glctx->planesValid = 0;
    else if( gldisp->glob.pruneEye )    glctx->planesValid = 1;
    else if( cam->perspective )         glctx->planesValid = 5;
    else                                glctx->planesValid = 1;

    /* Restore the matrix stack */
    hwGlMatrixMode( HWGL_MODELVIEW );
    for( i = 0; i <= n; i++ ) {
        hwGlPushMatrix();
        hwMatMult(              tmpMat,
                                        glctx->matStack[i],
                                        glctx->camMat );
        (void)memcpy( &glctx->matStack[i][0][0], &tmpMat[0][0],
                                        16*sizeof(float) );
        hwGlLoadMatrixf( &glctx->matStack[i][0][0] );
    }

    glctx->camSet = 1;
}

/*#pragma OPT_LEVEL 0*/
void __hwGlSurfAttrs( hwDisplay disp, hwSurfaceType *surf )
{
    USE_GL_CTX(disp);
    hwInt32
        i, j, n, doBlend = 0;
    float
        value[4];
    GLenum
        blendSrc = GL_SRC_ALPHA,
        blendDst = GL_ONE_MINUS_SRC_ALPHA;

    if( !glctx )             return;

    // Always do this, even if we're compiling a list, because we
    // need to know wireframe when building the list.
    glctx->primFlags = surf->flags;

    if( glctx->openList ) {
        // We also need to know transparency since we will stick it into RGB
        // arrays
        glctx->transp = surf->transp;
        __hwGlAppendSurf( glctx, surf );
        return;
    }

    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    /* If unlit, refect 100% of the ambient color (which happens
     * to be white), else reflect the ambient dimly...
     */
    if( surf->flags & HW_SURF_EMISSIVE ) {
        if( glctx->ambFactSave != 1000.0 ) {
            value[0] = value[1] = value[2] = 1.0;
            value[3] = 1.0;
            hwGlLightModelfv( HWGL_LIGHT_MODEL_AMBIENT, value );
            glctx->ambFactSave = 1000.0;
        }
    }
    else {
        if(glctx->ambFactSave != glctx->ambFact) {
            hwGlLightModelfv( HWGL_LIGHT_MODEL_AMBIENT,
                                    glctx->ambColor );
            glctx->ambFactSave = glctx->ambFact;
        }
    }

    if(    (surf->specColor[0] != glctx->specColor[0])
        || (surf->specColor[1] != glctx->specColor[1])
        || (surf->specColor[2] != glctx->specColor[2])
        || (surf->shininess != glctx->shininess) )
    {
        if( surf->shininess > 0.0 ) {
            value[0] = surf->specColor[0];
            value[1] = surf->specColor[1];
            value[2] = surf->specColor[2];
            value[3] = 1.0;
        }
        else {
            value[0] = 0.0;
            value[1] = 0.0;
            value[2] = 0.0;
            value[3] = 1.0;
        }
        hwGlMaterialfv( HWGL_FRONT_AND_BACK, HWGL_SPECULAR, value );
        glctx->specColor[0] = surf->specColor[0];
        glctx->specColor[1] = surf->specColor[1];
        glctx->specColor[2] = surf->specColor[2];
    }


    if( surf->shininess != glctx->shininess ) {
        n = (int)(surf->shininess * 256.0);
        if( n > 127 )           n = 127;
        if( n < 1 )             n = 1;
        hwGlMateriali( HWGL_FRONT_AND_BACK, HWGL_SHININESS, n );
        glctx->shininess = surf->shininess;
    }

    if( surf->primSize != glctx->primSize ) {
        glctx->primSize = surf->primSize;
        glLineWidth( surf->primSize );
        hwGlPointSize( surf->primSize );
        if( surf->primSize <= 1.0 ) {
            hwGlEnable( HWGL_LINE_SMOOTH );
            hwGlEnable( HWGL_POINT_SMOOTH );
        }
        else {
            hwGlDisable( HWGL_LINE_SMOOTH );
            hwGlDisable( HWGL_POINT_SMOOTH );
        }
    }

    if( surf->numTextures && !(surf->flags & HW_SURF_TEX_SAMP) ) {
        for( n = 0; n < surf->numTextures; n++ ) {
            __hwGlCurrentTexture( disp, surf->textureIDs[n], n );
        }
        for( ; n < HW_MAX_TEXTURES; n++ ) {
            __hwGlCurrentTexture( disp, -1, n );
        }
    }
    else if( glctx->tmActive != -1 ) {
        for( n = 0; n < HW_MAX_TEXTURES; n++ ) {
            hwGlActiveTexture( HWGL_TEXTURE0 + n );
            hwGlDisable( HWGL_TEXTURE_2D );
            glctx->tmId[n] = -1;
        }
        glctx->tmActive = -1;
    }

    /* Need to get a color */
    if( !(surf->flags & HW_SURF_UNCOLORED) ) {
        if(    (surf->color[0] != glctx->color[0])
            || (surf->color[1] != glctx->color[1])
            || (surf->color[2] != glctx->color[2]) )
        {
            value[0] = surf->color[0];
            value[1] = surf->color[1];
            value[2] = surf->color[2];
            value[3] = 1.0 - glctx->transp;
#if defined(ANDROID_DEBUG)
            __android_log_print(ANDROID_LOG_INFO, "HoverWare", "__hwGlSurfAttrs: setting color to %f, %f, %f, %f",
                value[0], value[1], value[2], value[3]);
#endif
            hwGlColor4f( value[0], value[1], value[2], value[3] );
            glctx->color[0] = surf->color[0];
            glctx->color[1] = surf->color[1];
            glctx->color[2] = surf->color[2];
            doBlend = (glctx->transp != 0.0);
        }
    }

    if( surf->transp != glctx->transp ) {
        value[0] = glctx->color[0];
        value[1] = glctx->color[1];
        value[2] = glctx->color[2];
        value[3] = 1.f - surf->transp;
        hwGlColor4f( value[0], value[1], value[2], value[3] );
        glctx->transp = surf->transp;
        doBlend = (glctx->transp != 0.0);
    }

    if( surf->flags & HW_SURF_BLEND ) {
        if( surf->flags & HW_SURF_BLEND_PREMULT ) {
            blendSrc = GL_ONE;
        }
        doBlend = 1;
    }
    else if( surf->flags & HW_SURF_WIREFRAME ) {
        if( surf->primSize <= 1.0 ) {
            doBlend = 1;
        }
    }

    if( doBlend ) {
        if( glctx->blend != 1 ) {
            if( glctx->blendSrc != blendSrc ) {
                glBlendFunc(blendSrc, blendDst);
                glctx->blendSrc = blendSrc;
            }
            hwGlEnable(GL_BLEND);
            glctx->blend = 1;
        }
    }
    else {
        if( glctx->blend != 0 ) {
            hwGlDisable(GL_BLEND);
            glctx->blend = 0;
        }
    }
}

static void __hwGlTextAttrs( hwDisplay disp, hwSurfaceType *surf )
{
    int
            n;

    __hwGlCurrentTexture( disp, surf->textureIDs[0], 0 );

    for( n = 1; n < HW_MAX_TEXTURES; n++ ) {
        __hwGlCurrentTexture( disp, -1, n );
    }
    hwGlColor4f( surf->color[0], surf->color[1], surf->color[2],
                1.0 - surf->transp );
}

/*#pragma OPT_LEVEL 2*/

/* Calculate normal at P0, given previous point P1 and next point P2 */
static void CalcNormal( hwFloat *P0, hwFloat *P1, hwFloat *P2, hwFloat *Normal )
{
    hwFloat
        vecA[3],
        vecB[3];

    VSUB( vecA, P1, P0 );
    VSUB( vecB, P2, P0 );
    VCROSS( Normal, vecA, vecB );
    VNORM( Normal, Normal );
}


void __hwgl_SetDirectRenderingParams( glDrawable *glctx )
{
    int i;

    if( glctx->tmActive != -1 ) {
        for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
            hwGlActiveTexture( HWGL_TEXTURE0 + i );
            hwGlDisable( HWGL_TEXTURE_2D );
        }
        glctx->tmActive = -1;
    }
    hwGlDisable( HWGL_LIGHTING );
    hwGlDisable( GL_CULL_FACE );
}

void __hwgl_SetWireframeState( glDrawable *glctx )
{
    int
        pt = 0;

    if( glctx->wireframeState != WFS_WIREFRAME ) {
        glctx->wireframeState = WFS_WIREFRAME;

        __hwgl_SetDirectRenderingParams(glctx);

        if((glctx->renderMode & HW_RENDER_MASK)
                == HW_RENDER_SELECT )
        {
            if( glctx->sel.flags & HW_SELECT_VERTICES ) {
                pt = 1;
            }
        }
#if !defined(ANDROID_NDK)
        if( pt )    glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
        else        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
#endif
    }
}

void __hwgl_SetSolidState( glDrawable *glctx )
{
    int
        i, cull, wf, pt;

    if( glctx->wireframeState != WFS_SOLID ) {
        glctx->wireframeState = WFS_SOLID;

        if( (glctx->renderMode & HW_RENDER_MASK)
                == HW_RENDER_SELECT )
        {
            cull = glctx->sel.flags & HW_SELECT_CULL_FACE;
            wf = glctx->sel.flags & HW_SELECT_EDGES;
            pt = glctx->sel.flags & HW_SELECT_VERTICES;
        }
        else {
            cull = glctx->renderMode & HW_RENDER_CULL_FACE;
            wf = glctx->renderMode & HW_RENDER_EDGE_MODE;
            pt = 0;
        }

        if( glctx->lightOn ) {
            hwGlEnable( HWGL_LIGHTING );
        }
        if( glctx->shadeFlat != 0 ) {
            glctx->shadeFlat = 0;
            hwGlShadeModel( HWGL_SMOOTH );
        }
        hwGlEnable(HWGL_NORMALIZE);

        if( pt || wf ) {
            /* Disable texturing */
            if( glctx->tmActive != -1 ) {
                for( i = 0 ; i < HW_MAX_TEXTURES; i++ ) {
                    hwGlActiveTexture( HWGL_TEXTURE0 + i );
                    hwGlDisable( HWGL_TEXTURE_2D );
                }
                glctx->tmActive = -1;
            }
        }
        else {
                /* Re-enable texturing, if applicable */
            if( glctx->tmActive == -1 ) {
                for( i = 0 ; i < HW_MAX_TEXTURES; i++ ) {
                    if( glctx->tmId[i] != -1 ) {
                        glctx->tmActive = i;
                        hwGlActiveTexture( HWGL_TEXTURE0 + i );
                        hwGlEnable( HWGL_TEXTURE_2D );
                    }
                }
            }
        }

#if !defined(ANDROID_NDK)
    if( pt )            glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
    else if( wf )       glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    else                glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
#endif

        if( cull ) {
            hwGlEnable( GL_CULL_FACE );
        }
        else {
            hwGlDisable( GL_CULL_FACE );
        }
    }
}

void __hwGlBeginSelect( glDrawable *glctx )
{
#if !defined(ANDROID_NDK)
    if((glctx->renderMode & HW_RENDER_MASK)!=HW_RENDER_SELECT) {
        return;
    }
    glSelectBuffer( SEL_SIZE, glctx->selMem );
    glInitNames();
    glPushName( 1 );
    (void)glRenderMode( GL_SELECT );
#endif
}

void __hwGlEndSelect( glDrawable *glctx )
{
#if !defined(ANDROID_NDK)
    int
        i, hits, n;
    GLuint
        *ptr,
        z1, z2;
    hwFloat
        depth;
    hwSelectionType
        *sel;

    if((glctx->renderMode & HW_RENDER_MASK)!=HW_RENDER_SELECT) {
        return;
    }
    hits = glRenderMode( GL_RENDER );
    ptr = glctx->selMem;
    sel = &glctx->sel;
    for( i = 0; i < hits; i++ ) {
        n = *ptr++;             /* # of names on stack - only 1! */
        z1 = *ptr++;
        z2 = *ptr++;
        depth = (hwFloat)z2;
        if( depth < sel->dist ) {
            sel->dist = depth;
            sel->obj = glctx->currObj;
            sel->picked = 1;
        }
        ptr += n;
    }
#endif
}

static void WireMesh
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m
)
{
    USE_GL_CTX(disp);
    hwInt32
        r, c, x;
    hwInt16
        *colIdx;

    if( !glctx )             return;

    x = 4*n*m;
    colIdx = __hwGlGrowIdxPool( glctx, x );

    if( glctx->openList ) {
        __hwGlAppendWireframe(glctx);
        __hwGlAppendVertices( glctx, data, dataFlags, n*m );
    }
    else {
        __hwgl_SetWireframeState(glctx);
        __hwGlSetDataPointers( glctx, data, dataFlags, n*m );

        __hwGlBeginSelect(glctx);
    }

    /* First, advance through each row */
    x = 0;
    for( r = 0; r < n; r++ ) {
        for (c = 1; c < m; c++) {
            colIdx[x++] = r*m + c - 1;
            colIdx[x++] = r*m + c;
        }
    }
    /* Next, advance through each Column, forming 1 polyline per column */
    for( c = 0; c < m; c++ ) {
        for( r = 1; r < n; r++ ) {
            colIdx[x++] = r*m + c - m;
            colIdx[x++] = r*m + c;
        }
    }

    if( glctx->openList ) {
        __hwGlAppendDrawElements( glctx, GL_LINES, x, colIdx );
    }
    else {
        hwGlBeginRendering();
        glDrawElements( GL_LINES, x, GL_UNSIGNED_SHORT, colIdx );
        __hwGlEndSelect(glctx);
    }
}

void __hwGlSetOffsPointers(glDrawable *glctx, float *vptr,
                           hwVertexOffsets *offs, int nv, int wpv,
                           int optimized)
{
    int BPV;
    int i, tm, ofx, tn;
    hwFloat *rgba;
    hwFloat alpha;

    BPV = wpv * sizeof(float);  // Bytes per vertex;

    if( !offs->w ) {
        hwGlVertexPointer(3, HWGL_FLOAT, BPV, vptr);
    }
    else {
        hwGlVertexPointer(4, HWGL_FLOAT, BPV, vptr);
    }

    if( (ofx = offs->rgb) ) {
        if( offs->alpha ) {
            hwGlColorPointer(4, HWGL_FLOAT, BPV, vptr+ofx);
        }
        else {
            /* OGL ES does not support 3-component vertex color arrays */
            /* So, allocate a vertex array with an alpha channel. */
            rgba = __hwGlGrowAuxPool( glctx, nv*4 );
            alpha = 1.0;// - glctx->transp;
            for( i = 0; i < nv; i++ ) {
                rgba[4*i  ] = vptr[wpv*i + ofx    ];
                rgba[4*i+1] = vptr[wpv*i + ofx + 1];
                rgba[4*i+2] = vptr[wpv*i + ofx + 2];
                rgba[4*i+3] = alpha;
            }
            hwGlColorPointer(4, HWGL_FLOAT, 0, rgba);
            vptr += 3;
        }
        hwGlEnableClientState(HWGL_COLOR_ARRAY);
    }
    else {
        hwGlDisableClientState(HWGL_COLOR_ARRAY);
    }

    if( (ofx = offs->normal) ) {
        hwGlNormalPointer(HWGL_FLOAT, BPV, vptr + ofx);
        hwGlEnableClientState(HWGL_NORMAL_ARRAY);
    }
    else {
        hwGlDisableClientState(HWGL_NORMAL_ARRAY);
    }

    if( (ofx = offs->tangent) ) {
        hwGlTangentPointer(HWGL_FLOAT, BPV, vptr + ofx);
        hwGlEnableClientState(HWGL_TANGENT_ARRAY);
    }
    else {
        hwGlDisableClientState(HWGL_TANGENT_ARRAY);
    }

    for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
        hwGlClientActiveTexture( HWGL_TEXTURE0 + tm );

        if( (ofx = offs->texCoord[tm]) ) {
            tn = offs->texSize[tm];
            hwGlEnableClientState(HWGL_TEXTURE_COORD_ARRAY);
            hwGlTexCoordPointer(tn, HWGL_FLOAT, BPV, vptr+ofx);
        }
        else {
            hwGlDisableClientState(HWGL_TEXTURE_COORD_ARRAY);
        }
    }
}

void __hwGlSetDataPointers(glDrawable *glctx, float *vptr, int Flags, int nv)
{
    hwVertexOffsets offs;
    hwInt32 wpv;

    wpv = hwGetVertexOffsets( Flags, &offs );
    __hwGlSetOffsPointers(glctx, vptr, &offs, nv, wpv, 0);
}


static void AppendOnePoly
(
    glDrawable *glctx,
    hwFloat *Poly, hwInt32 dataFlags, hwInt32 NP, hwInt32 Rev, hwInt32 flipNml,
    hwInt32 *vtxSize, hwInt32 *idxSize, hwInt32 *dstFlags
);

static void FacetMesh
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m
)
{
    USE_GL_CTX(disp);
    hwFloat
        Poly[4*HW_DATA_MAX_WPV], *Curr, *Next, *Point;
    hwInt32
        vtxSize, idxSize, dstFlags,
        sf, i, j, vn;

    /* We need to generate facet normals, but I'm lazy.
     * Use the AppendOnePoly routine.
     */

    if( glctx->openList ) {
        __hwGlAppendSolid(glctx);
    }
    else {
        __hwgl_SetSolidState(glctx);
        __hwGlBeginSelect(glctx);
    }

    sf = glctx->primFlags;
    vn = hwCalcWPV( dataFlags );
    Curr = data;  Next = data + m*vn;

    vtxSize = idxSize = 0;

    for (i = 1; i < n; i++) {
        for (j = 1; j < m; j++) {
            memcpy(Poly+0*vn, Next, vn*sizeof(hwFloat));
            memcpy(Poly+1*vn, Next+vn, vn*sizeof(hwFloat));
            memcpy(Poly+2*vn, Curr+vn, vn*sizeof(hwFloat));
            memcpy(Poly+3*vn, Curr, vn*sizeof(hwFloat));
            if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
                AppendOnePoly( glctx, Poly, dataFlags, 4, 1,
                    (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0,
                    &vtxSize, &idxSize, &dstFlags );
            }
            if( !(sf & HW_SURF_BACKFACE) ) {
                AppendOnePoly( glctx, Poly, dataFlags, 4, 0,
                    (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0,
                    &vtxSize, &idxSize, &dstFlags );
            }
            if( vtxSize > 32700 ) {
                if( glctx->openList ) {
                    __hwGlAppendVertices( glctx, glctx->vtxPool,
                                          dstFlags, vtxSize );
                    __hwGlAppendDrawElements( glctx, GL_TRIANGLES, idxSize,
                                              glctx->idxPool );
                }
                else {
                    __hwGlSetDataPointers( glctx, glctx->vtxPool,
                                           dstFlags, vtxSize );
                    hwGlBeginRendering();
                    glDrawElements( GL_TRIANGLES, idxSize, GL_UNSIGNED_SHORT,
                                    glctx->idxPool );
                }
                vtxSize = idxSize = 0;
            }
            Curr += vn; Next += vn;
        }
        Curr += vn; Next += vn;
    }

    if( idxSize ) {
        if( glctx->openList ) {
            __hwGlAppendVertices( glctx, glctx->vtxPool, dstFlags, vtxSize );
            __hwGlAppendDrawElements( glctx, GL_TRIANGLES,
                                      idxSize, glctx->idxPool );
        }
        else {
            __hwGlSetDataPointers( glctx, glctx->vtxPool, dstFlags, vtxSize );
            hwGlBeginRendering();
            glDrawElements( GL_TRIANGLES, idxSize, GL_UNSIGNED_SHORT,
                            glctx->idxPool );
        }
    }

    if( !glctx->openList ) {
        __hwGlEndSelect(glctx);
    }
}

void __hwGlDrawMesh
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m
)
{
    USE_GL_CTX(disp);
    hwInt16
        *index;
    hwInt32
        idxSize,
        currIdx,
        nmlOffs,
        numVerts,
        i, j, k, vn, nb, sf;
    hwFloat
        *tmp, *src, *dst;

    if( !glctx ) return;

    //__android_log_print(ANDROID_LOG_INFO, "HoverWare", "__hwGlDrawMesh");
    sf = glctx->primFlags;
    if( sf & HW_SURF_WIREFRAME ) {
        WireMesh( disp, data, dataFlags, n, m );
        return;
    }

    if( !(dataFlags & HW_DATA_NORMALS) ) {
        FacetMesh( disp, data, dataFlags, n, m );
        return;
    }

    vn = hwCalcWPV( dataFlags );

    // Make a mess 'o triangles
    numVerts = n * m;
    idxSize = (m-1) * (n-1) * 2 * 3;
    if( sf & HW_SURF_TWOSIDED ) {
        idxSize *= 2;   // Need backfacing ones, too
    }
    index = __hwGlGrowIdxPool( glctx, idxSize );

    if( sf & (HW_SURF_TWOSIDED | HW_SURF_FLIP_NORMALS) ) {
        /* Need to know offset of the normal */
        nmlOffs = 3;
        if( dataFlags & HW_DATA_RGB ) {
            nmlOffs += 3;
            if( dataFlags & HW_DATA_ALPHA ) {
                nmlOffs += 1;
            }
        }

        if( sf & HW_SURF_TWOSIDED ) {
            /* Need a backface with normals flipped. */
            tmp = __hwGlGrowVtxPool( glctx, 2*m*n*vn );
            memcpy( tmp, data, m*n*vn*sizeof(hwFloat) );
            dst = tmp + m*n*vn;
            numVerts *= 2;
        }
        else if( sf & HW_SURF_FLIP_NORMALS ) {
            /* Need a frontface with normals flipped. */
            tmp = __hwGlGrowVtxPool( glctx, m*n*vn );
            dst = tmp;
        }
        src = data;
        for( i = 0; i < n; i++ ) {
            for( j = 0; j < m; j++ ) {
                for( k = 0; k < nmlOffs; k++ )  *dst++ = *src++;
                for( ; k < (nmlOffs+3); k++ )   *dst++ = -*src++;
                for( ; k < vn; k++ )            *dst++ = *src++;
            }
        }
        data = tmp;
    }

    currIdx = 0;
    if( !(sf & HW_SURF_BACKFACE) ) {
        nmlOffs = 0;
        if( sf & HW_SURF_TWOSIDED ) {
            /* We're drawing the frontface - flip if requested */
            if( sf & HW_SURF_FLIP_NORMALS ) nmlOffs = n*m;
        }

        for( i = 0; i < (n-1); i++ ) {
            for( j = 0; j < (m-1); j++ ) {
                index[currIdx++] = nmlOffs + (i+1)*m + j;   // 1 0
                index[currIdx++] = nmlOffs + (i+1)*m + j+1; // 1 1
                index[currIdx++] = nmlOffs + i*m + j;       // 0 0
                index[currIdx++] = nmlOffs + (i+1)*m + j+1; // 1 1
                index[currIdx++] = nmlOffs + i*m + j+1;     // 0 1
                index[currIdx++] = nmlOffs + i*m + j;       // 0 0
            }
        }
    }
    if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
        nmlOffs = 0;
        if( sf & HW_SURF_TWOSIDED ) {
            /* We're drawing the backtface - flip if *not* requested */
            if( !(sf & HW_SURF_FLIP_NORMALS) ) nmlOffs = n*m;
        }

        for( i = 0; i < (n-1); i++ ) {
            for( j = 0; j < (m-1); j++ ) {
                index[currIdx++] = nmlOffs + i*m + j;       // 0 0
                index[currIdx++] = nmlOffs + i*m + j+1;     // 0 1
                index[currIdx++] = nmlOffs + (i+1)*m + j+1; // 1 1
                index[currIdx++] = nmlOffs + i*m + j;       // 0 0
                index[currIdx++] = nmlOffs + (i+1)*m + j+1; // 1 1
                index[currIdx++] = nmlOffs + (i+1)*m + j;   // 1 0
            }
        }
    }

    if( glctx->openList ) {
        __hwGlAppendSolid(glctx);
        __hwGlAppendVertices( glctx, data, dataFlags, numVerts );
        __hwGlAppendDrawElements( glctx, GL_TRIANGLES, currIdx, index );
    }
    else {
        __hwgl_SetSolidState(glctx);
        __hwGlSetDataPointers(glctx, data, dataFlags, numVerts);
        __hwGlBeginSelect(glctx);
        hwGlBeginRendering();
        glDrawElements(GL_TRIANGLES, currIdx, GL_UNSIGNED_SHORT, index);
        __hwGlEndSelect(glctx);
    }
}

static void WirePolygon
(
    glDrawable *glctx,
    hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts
)
{
    hwInt16
        *idx, *iptr;
    int
        i;

    idx = __hwGlGrowIdxPool( glctx, 2*numVerts );
    iptr = idx;
    for( i = 0; i < numVerts; i++ ) {
        *iptr++ = i;
        *iptr++ = (i+1) % numVerts;
    }

#ifndef ANDROID_NDK
    /* Some Android phones *cough*Qualcomm*cough* are really bad at
     * line loops
     */
    if( glctx->openList ) {
        __hwGlAppendWireframe(glctx);
        __hwGlAppendVertices(glctx, polyData, dataFlags, numVerts);
        __hwGlAppendDrawElements( glctx, GL_LINES, 2*numVerts, idx );
    }
    else {
        __hwgl_SetWireframeState(glctx);
        __hwGlSetDataPointers(glctx, polyData, dataFlags, numVerts);
        __hwGlBeginSelect(glctx);
        hwGlBeginRendering();
        glDrawElements( GL_LINES, 2*numVerts, GL_UNSIGNED_SHORT, idx );
        __hwGlEndSelect(glctx);
    }
#endif
}

static void AppendOnePoly
(
    glDrawable *glctx,
    hwFloat *Poly, hwInt32 dataFlags, hwInt32 NP, hwInt32 Rev, hwInt32 flipNml,
    hwInt32 *vtxSize, hwInt32 *idxSize, hwInt32 *pDstFlags
)
{
    int
        i, tm, tn, WPV, dstWPV, ofx, size, dstFlags;
    hwVertexOffsets
        Offs;
    hwFloat
        Normal[3];
    hwFloat
        *ptr,
        *tmpPoly;
    hwInt16
        *iptr,
        *tmpIdx;

    WPV = hwGetVertexOffsets( dataFlags, &Offs );

    dstFlags = dataFlags;
    dstWPV = WPV;

    if( dataFlags & HW_DATA_RGB ) {
        if( !(dataFlags & HW_DATA_ALPHA) ) {
            /* We'll have to add alpha later.  Might as well
             * add it now.
             */
            dstFlags |= HW_DATA_ALPHA;
            dstWPV += 1;
        }
    }

    if( !(dataFlags & HW_DATA_NORMALS) ) {
        // Always add normals if not present */
        dstFlags |= HW_DATA_NORMALS;
        dstWPV += 3;
    }

    *pDstFlags = dstFlags;

    /* Make sure we have enough data to hold the whole polygon */
    size = (NP + *vtxSize) * dstWPV;
    tmpPoly = __hwGlGrowVtxPool( glctx, size ) + (dstWPV * *vtxSize);

    /* Also, need indices */
    size = ((NP - 2) * 3) + *idxSize;
    tmpIdx = __hwGlGrowIdxPool( glctx, size ) + *idxSize;

    /* Calculate the normal, facing the side the emitted winding shows */
    if( !(dataFlags & HW_DATA_NORMALS) ) {
        CalcNormal( Poly, Poly+WPV, Poly+2*WPV, Normal );
        if( Rev ) VMULC( Normal, Normal, -1.0 );
    }

    ptr = tmpPoly;
    iptr = tmpIdx;

    for( i = 0; i < NP; i++ ) {
        *ptr++ = Poly[0];
        *ptr++ = Poly[1];
        *ptr++ = Poly[2];

        if( (ofx = Offs.rgb) ) {
            *ptr++ = Poly[ofx  ];
            *ptr++ = Poly[ofx+1];
            *ptr++ = Poly[ofx+2];
            // Always add alpha
            if( Offs.alpha ) *ptr++ = Poly[ofx+3];
            else             *ptr++ = 1.0;
        }

        if( (ofx = Offs.normal) ) {
            if( flipNml ) {
                *ptr++ = -Poly[ofx  ];
                *ptr++ = -Poly[ofx+1];
                *ptr++ = -Poly[ofx+2];
            }
            else {
                *ptr++ = Poly[ofx  ];
                *ptr++ = Poly[ofx+1];
                *ptr++ = Poly[ofx+2];
            }
        }
        else {
            // Always add a normal
            *ptr++ = Normal[0];
            *ptr++ = Normal[1];
            *ptr++ = Normal[2];
        }

        if( (ofx = Offs.tangent) ) {
            *ptr++ = Poly[ofx  ];
            *ptr++ = Poly[ofx+1];
            *ptr++ = Poly[ofx+2];
            *ptr++ = Poly[ofx+3];
        }

        for( tm = 0; tm < HW_MAX_TEXTURES; tm++ ) {
            if( !(ofx = Offs.texCoord[tm]) ) continue;
            tn = Offs.texSize[tm];

            *ptr++ = Poly[ofx  ];
            *ptr++ = Poly[ofx+1];
            if( tn > 2 ) *ptr++ = Poly[ofx+2];
            if( tn > 3 ) *ptr++ = Poly[ofx+3];
        }

        Poly += WPV;
    }

    /* Make index list */
    size = *vtxSize;
    if( Rev ) {
        for( i = 2; i < NP; i++ ) {
            *iptr++ = size + 0; *iptr++ = size + i; *iptr++ = size + i-1;
        }
    }
    else {
        for( i = 2; i < NP; i++ ) {
            *iptr++ = size + 0; *iptr++ = size + i-1; *iptr++ = size + i;
        }
    }

    /* Increment sizes */
    *vtxSize += NP;
    *idxSize += (NP - 2) * 3;
}

void __hwGlDrawPolygon
(
    hwDisplay disp,
    hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts
)
{
    USE_GL_CTX(disp);
    hwInt32
        vtxSize, idxSize, dstFlags,
        sf;

    if( !glctx )             return;
    sf = glctx->primFlags;

    if( sf & HW_SURF_WIREFRAME ) {
        WirePolygon( glctx, polyData, dataFlags, numVerts );
        return;
    }

    if( glctx->openList ) {
        __hwGlAppendSolid(glctx);
    }
    else {
        __hwgl_SetSolidState(glctx);
        __hwGlBeginSelect(glctx);
    }

    vtxSize = idxSize = 0;

    if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
        AppendOnePoly( glctx, polyData, dataFlags, numVerts, 1,
                     (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0,
                     &vtxSize, &idxSize, &dstFlags );
    }
    if( !(sf & HW_SURF_BACKFACE) ) {
        AppendOnePoly( glctx, polyData, dataFlags, numVerts, 0,
                                    (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0,
                                    &vtxSize, &idxSize, &dstFlags );
    }

    if( glctx->openList ) {
        __hwGlAppendVertices( glctx, glctx->vtxPool, dstFlags, vtxSize );
        __hwGlAppendDrawElements( glctx, GL_TRIANGLES,
                                  idxSize, glctx->idxPool );
    }
    else {
        __hwGlSetDataPointers( glctx, glctx->vtxPool, dstFlags, vtxSize );
        hwGlBeginRendering();
        glDrawElements( GL_TRIANGLES, idxSize, GL_UNSIGNED_SHORT,
                        glctx->idxPool );
        __hwGlEndSelect(glctx);
    }
}

void __hwGlDrawPolyline( hwDisplay disp, hwFloat *data, hwInt32 fl, hwInt32 n )
{
    USE_GL_CTX(disp);
    hwInt32
        i, wpv;
    hwVertexOffsets
        offs;
    hwInt16
        *idx, *iptr;

    if( !glctx )             return;

    idx = __hwGlGrowIdxPool( glctx, 2*n );
    iptr = idx;

    wpv = hwGetVertexOffsets( fl, &offs );

    if( fl & HW_DATA_MD_FLAGS ) {
        for( i = 1; i < n; i++ ) {
            if( data[wpv*(i+1)-1] != 0.0 ) {
                /* Lineto */
                *iptr++ = i - 1;
                *iptr++ = i;
            }
        }
    }
    else {
        for( i = 1; i < n; i++ ) {
            *iptr++ = i - 1;
            *iptr++ = i;
        }
    }

    if( glctx->openList ) {
        __hwGlAppendWireframe(glctx);
        __hwGlAppendVertices(glctx, data, fl, n);
        __hwGlAppendDrawElements( glctx, GL_LINES, iptr - idx, idx );
    }
    else {
        __hwgl_SetWireframeState(glctx);
        __hwGlSetDataPointers(glctx, data, fl, n);
        __hwGlBeginSelect(glctx);
        hwGlBeginRendering();
        glDrawElements( GL_LINES, iptr - idx, GL_UNSIGNED_SHORT, idx );
        __hwGlEndSelect(glctx);
    }
}

void __hwGlDrawQuads
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n
)
{
    USE_GL_CTX(disp);
    hwInt32
        i, vn;
    hwInt32
        vtxSize, idxSize, dstFlags,
        sf;

    if( !glctx )             return;
    sf = glctx->primFlags;

    vn = hwCalcWPV( dataFlags );

    if( sf & HW_SURF_WIREFRAME ) {
        sf &= ~HW_SURF_TWOSIDED;
    }

    if( glctx->openList ) {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwGlAppendWireframe(glctx);
        }
        else {
            __hwGlAppendSolid(glctx);
        }
    }
    else {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwgl_SetWireframeState(glctx);
        }
        else {
            __hwgl_SetSolidState(glctx);
        }
        __hwGlBeginSelect(glctx);
    }

    vtxSize = idxSize = 0;

    for( i = 0; i < n; i++ ) {
        if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
            AppendOnePoly( glctx, data, dataFlags, 4, 1,
                        (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0,
                        &vtxSize, &idxSize, &dstFlags );
        }
        if( !(sf & HW_SURF_BACKFACE) ) {
            AppendOnePoly( glctx, data, dataFlags, 4, 0,
                               (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0,
                               &vtxSize, &idxSize, &dstFlags );
        }
        if( vtxSize > 32700 ) {
            if( glctx->openList ) {
                __hwGlAppendVertices( glctx, glctx->vtxPool,
                                      dstFlags, vtxSize );
                __hwGlAppendDrawElements( glctx, GL_TRIANGLES, idxSize,
                                          glctx->idxPool );
            }
            else {
                __hwGlSetDataPointers( glctx, glctx->vtxPool,
                                       dstFlags, vtxSize );
                hwGlBeginRendering();
                glDrawElements( GL_TRIANGLES, idxSize, GL_UNSIGNED_SHORT,
                                glctx->idxPool );
            }
            vtxSize = idxSize = 0;
        }
        data += 4*vn;
    }

    if( idxSize ) {
        if( glctx->openList ) {
            __hwGlAppendVertices( glctx, glctx->vtxPool, dstFlags, vtxSize );
            __hwGlAppendDrawElements( glctx, GL_TRIANGLES,
                                      idxSize, glctx->idxPool );
        }
        else {
            __hwGlSetDataPointers( glctx, glctx->vtxPool, dstFlags, vtxSize );
            hwGlBeginRendering();
            glDrawElements( GL_TRIANGLES, idxSize, GL_UNSIGNED_SHORT,
                            glctx->idxPool );
        }
    }
    if( !glctx->openList ) {
        __hwGlEndSelect(glctx);
    }
}

static void __hwGlTextQuads
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n
)
{
    USE_GL_CTX(disp);
    hwInt32
        i;
    hwInt16
        *idx, *iptr;

    idx = __hwGlGrowIdxPool( glctx, 6*n );    /* 6 vts/quad, n quads */
    iptr = idx;
    for( i = 0; i < n; i++ ) {
        *iptr++ = 4*i;
        *iptr++ = 4*i+1;
        *iptr++ = 4*i+2;
        *iptr++ = 4*i;
        *iptr++ = 4*i+2;
        *iptr++ = 4*i+3;
    }

    __hwGlSetDataPointers(glctx, data, dataFlags, 4*n);
    hwGlBeginRendering();
    glDrawElements( GL_TRIANGLES, iptr-idx, GL_UNSIGNED_SHORT, idx );
}

void __hwGlIndexedTris
(
    hwDisplay disp,
    hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
    hwInt32 *indexList, hwInt32 numTris
)
{
    USE_GL_CTX(disp);
    hwInt32
        i, numi, sf;
    hwInt16
        *shortList;

    if( !glctx )             return;
    sf = glctx->primFlags;

    if( sf & HW_SURF_WIREFRAME ) {
        sf &= ~HW_SURF_TWOSIDED;
    }

    numi = numTris*3;
    if( sf & HW_SURF_TWOSIDED ) {
        numi *= 2;
    }

    shortList = __hwGlGrowIdxPool( glctx, numi );

    // Create 16-bit version of index list
    numi = 0;
    if( !(sf & HW_SURF_BACKFACE) ) {
        if( indexList ) {
            for( i = 0; i < numTris; i++) {
                shortList[numi++] = indexList[3*i  ];
                shortList[numi++] = indexList[3*i+1];
                shortList[numi++] = indexList[3*i+2];
            }
        }
        else {
            for( i = 0; i < numTris; i++) {
                shortList[numi++] = 3*i;
                shortList[numi++] = 3*i+1;
                shortList[numi++] = 3*i+2;
            }
        }
    }
    if( sf & (HW_SURF_TWOSIDED | HW_SURF_BACKFACE) ) {
        if( indexList ) {
            for( i = 0; i < numTris; i++) {
                shortList[numi++] = indexList[3*i+2];
                shortList[numi++] = indexList[3*i+1];
                shortList[numi++] = indexList[3*i  ];
            }
        }
        else {
            for( i = 0; i < numTris; i++) {
                shortList[numi++] = 3*i+2;
                shortList[numi++] = 3*i+1;
                shortList[numi++] = 3*i  ;
            }
        }
    }

    // TBD: Insert facet normals, etc.
    if( glctx->openList ) {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwGlAppendWireframe(glctx);
        }
        else {
            __hwGlAppendSolid(glctx);
        }
        __hwGlAppendVertices(glctx, verts, dataFlags, numVerts);
        __hwGlAppendDrawElements(glctx, GL_TRIANGLES, numi, shortList);
    }
    else {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwgl_SetWireframeState(glctx);
        }
        else {
            __hwgl_SetSolidState(glctx);
        }
        __hwGlBeginSelect(glctx);

        __hwGlSetDataPointers(glctx, verts, dataFlags, numVerts);
        hwGlBeginRendering();
        glDrawElements(GL_TRIANGLES, numi, GL_UNSIGNED_SHORT, shortList);

        __hwGlEndSelect(glctx);
    }
}

void __hwGlDrawStrip
(
    hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n
)
{
    USE_GL_CTX(disp);
    hwInt32
        i, ni, sf;
    hwInt16
        *idx;

    if( !glctx )             return;
    sf = glctx->primFlags;

    if( sf & HW_SURF_WIREFRAME ) {
        sf &= ~HW_SURF_TWOSIDED;
    }

    ni = (n - 2) * 3;   /* Actually count triangles */
    if( sf & HW_SURF_TWOSIDED ) {
        ni *= 2;
    }

    idx = __hwGlGrowIdxPool( glctx, ni );

    if( glctx->openList ) {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwGlAppendWireframe(glctx);
        }
        else {
            __hwGlAppendSolid(glctx);
        }
        __hwGlAppendVertices(glctx, data, flags, n);
    }
    else {
        if( sf & HW_SURF_WIREFRAME ) {
            __hwgl_SetWireframeState(glctx);
        }
        else {
            __hwgl_SetSolidState(glctx);
        }
        __hwGlBeginSelect(glctx);
        __hwGlSetDataPointers(glctx, data, flags, n);
    }

    ni = 0;
    if( !(sf & HW_SURF_BACKFACE) ) {
        for( i = 2; i < n; i++ ) {
            if( i & 1 ) {
                idx[ni++] = i-1;
                idx[ni++] = i-2;
                idx[ni++] = i;
            }
            else {
                idx[ni++] = i-2;
                idx[ni++] = i-1;
                idx[ni++] = i;
            }
        }
    }
    if( sf & (HW_SURF_TWOSIDED | HW_SURF_BACKFACE) ) {
        for( i = 2; i < n; i++ ) {
            if( i & 1 ) {
                idx[ni++] = i-2;
                idx[ni++] = i-1;
                idx[ni++] = i;
            }
            else {
                idx[ni++] = i-1;
                idx[ni++] = i-2;
                idx[ni++] = i;
            }
        }
    }

    if( glctx->openList ) {
        __hwGlAppendDrawElements(glctx, GL_TRIANGLES, ni, idx);
    }
    else {
        hwGlBeginRendering();
        glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_SHORT, idx);
    }

    if( !glctx->openList ) {
        __hwGlEndSelect(glctx);
    }
}


void __hwGlDrawMarkers
(
    hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n
)
{
    USE_GL_CTX(disp);
    int
        i;
    hwInt16
        *idx;

    if( !glctx )             return;

    idx = __hwGlGrowIdxPool( glctx, n );
    for( i = 0; i < n; i++ ) {
        idx[i] = i;
    }

    if( glctx->openList ) {
        __hwGlAppendVertices( glctx, data, flags, n );
        __hwGlAppendWireframe(glctx);
        __hwGlAppendDrawElements( glctx, GL_POINTS, n, idx );
    }
    else {
        __hwGlSetDataPointers( glctx, data, flags, n );
        __hwgl_SetWireframeState(glctx);
        __hwGlBeginSelect(glctx);
        hwGlBeginRendering();
        glDrawElements( GL_POINTS, n, GL_UNSIGNED_SHORT, idx );
        __hwGlEndSelect(glctx);
    }
}

void __hwGlSetDrawBuffer( hwDisplay disp, hwInt32 buff )
{
#if !defined(ANDROID_NDK)
    switch( buff ) {
    case HW_DRAW_BACK_LEFT :
        glDrawBuffer( GL_BACK_LEFT );
        glReadBuffer( GL_BACK_LEFT );
        break;
    case HW_DRAW_BACK_RIGHT :
        glDrawBuffer( GL_BACK_RIGHT );
        glReadBuffer( GL_BACK_RIGHT );
        break;
    case HW_DRAW_FRONT_LEFT :
        glDrawBuffer( GL_FRONT_LEFT );
        glReadBuffer( GL_FRONT_LEFT );
        break;
    case HW_DRAW_FRONT_RIGHT :
        glDrawBuffer( GL_FRONT_RIGHT );
        glReadBuffer( GL_FRONT_RIGHT );
        break;
    }
#endif

}

glGuiElement __hwGlAllocGuiElem( glDrawable *glctx )
{
    glGuiElement result;
    int i;

    if (!glctx->guiFree) {
        /* Manage the free list */
        glctx->guiFree
                        = malloc(GUI_ELEMENT_FREE_SIZE
                                         * sizeof(struct __glGuiElement));
        if (!glctx->guiFree) {
            return NULL;
        }
        for (i = 1; i < GUI_ELEMENT_FREE_SIZE; i++) {
            glctx->guiFree[i-1].next
                                        = glctx->guiFree + i;
        }
        glctx->guiFree[i-1].next = NULL;
    }

    /* Pull an entry from the free list */
    result = glctx->guiFree;
    glctx->guiFree = result->next;
    result->next = NULL;

    /* Go ahead and add it to the rendering list */
    if (glctx->activeGuiList) {
        if (glctx->activeGuiList->tail) {
            glctx->activeGuiList->tail->next = result;
        } else {
            glctx->activeGuiList->head = result;
        }
        glctx->activeGuiList->tail = result;
    } else {
        if (glctx->guiTail) {
            glctx->guiTail->next = result;
        } else {
            glctx->guiList = result;
        }
        glctx->guiTail = result;
    }

    return result;
}

void __hwGlGuiText
(
        hwDisplay disp,
        TexFont *txf,
        hwInt32 color, hwInt32 halign, hwInt32 valign, hwInt32 height,
        hwInt32 x, hwInt32 y, unsigned char *text
)
{
    USE_GL_CTX(disp);
    glGuiElement elem;
    int width, max_ascent, max_descent;
    int i, n, len;
    hwFloat fx, fy, fwidth, scale;

    if( !glctx ) return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    /* Calculate position of text */
    len = strlen((const char *)text);
    txfGetStringMetrics(txf, text, len, &width, &max_ascent, &max_descent);

    scale = height / (float)(max_ascent + max_descent);
    fx = x;
    fy = y;
    fwidth = width * scale;

    switch( halign ) {
    case HW_TEXT_ALIGN_CENTER :
        fx -= fwidth * 0.5;
        break;
    case HW_TEXT_ALIGN_RIGHT :
        fx -= fwidth;
        break;
    case HW_TEXT_ALIGN_LEFT :
    default :
        break;
    }

    switch( valign ) {
    case HW_TEXT_ALIGN_TOP :
        fy += max_ascent * scale;
        break;
    case HW_TEXT_ALIGN_CENTER :
        fy += max_ascent * scale;
        fy -= (max_ascent + max_descent) * scale * 0.5;
        break;
    case HW_TEXT_ALIGN_BOTTOM :
    default :
        break;
    }

    for (i = 0; i < len; i += MAX_GUI_STRING_SIZE) {
        elem = __hwGlAllocGuiElem(glctx);
        if (!elem) return;

        n = len - i;
        if (n > MAX_GUI_STRING_SIZE) {
            n = MAX_GUI_STRING_SIZE;
        }
        txfGetStringMetrics(txf, text+i, n, &width, &max_ascent, &max_descent);

        elem->typeFlags = GUI_TEXT;
        elem->color = color;
        elem->u.guiText.txf = txf;
        elem->u.guiText.pos[0] = (int)fx;
        elem->u.guiText.pos[1] = (int)fy;
        elem->u.guiText.size = height;
        (void)strncpy((char *)elem->u.guiText.string, (char *)(text+i), n);
        elem->u.guiText.string[n] = 0;

        fx += width * scale;
    }
}


void __hwGlGuiRaster
(
    hwDisplay disp,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 color,
    hwInt32 texId
)
{
    USE_GL_CTX(disp);
    glGuiElement elem;

    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    elem = __hwGlAllocGuiElem(glctx);
    if (!elem) return;
    elem->typeFlags = GUI_TEXTURE;
    elem->color = color;
    elem->u.guiRect.pointA[0] = x;
    elem->u.guiRect.pointA[1] = y;
    elem->u.guiRect.pointB[0] = x+w;
    elem->u.guiRect.pointB[1] = y+h;
    elem->u.guiRect.texture = texId;
}

void __hwGlGuiRectangle
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 radius
)
{
    USE_GL_CTX(disp);
    glGuiElement elem;

    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    elem = __hwGlAllocGuiElem(glctx);
    if (!elem) return;
    elem->typeFlags = GUI_RECT | flags;
    elem->color = color;
    elem->u.guiRect.pointA[0] = x;
    elem->u.guiRect.pointA[1] = y;
    elem->u.guiRect.pointB[0] = x+w;
    elem->u.guiRect.pointB[1] = y+h;
    elem->u.guiRect.texture = 0;
}

void __hwGlGuiPolyline
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    USE_GL_CTX(disp);
    int i;
    glGuiElement elem;

    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    for (i = 1; i < numPts; i++) {
        elem = __hwGlAllocGuiElem(glctx);
        if (!elem) return;
        elem->typeFlags = GUI_LINE | flags;
        elem->color = color;
        elem->u.guiRect.pointA[0] = pts[2*i-2];
        elem->u.guiRect.pointA[1] = pts[2*i-1];
        elem->u.guiRect.pointB[0] = pts[2*i  ];
        elem->u.guiRect.pointB[1] = pts[2*i+1];
        elem->u.guiRect.texture = 0;
    }
}

void __hwGlGuiLines
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    USE_GL_CTX(disp);
    int i;
    glGuiElement elem;

    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    numPts &= ~1;

    for (i = 0; i < numPts; i += 2) {
        elem = __hwGlAllocGuiElem(glctx);
        if (!elem) return;
        elem->typeFlags = GUI_LINE | flags;
        elem->color = color;
        elem->u.guiRect.pointA[0] = pts[2*i  ];
        elem->u.guiRect.pointA[1] = pts[2*i+1];
        elem->u.guiRect.pointB[0] = pts[2*i+2];
        elem->u.guiRect.pointB[1] = pts[2*i+3];
        elem->u.guiRect.texture = 0;
    }
}

void __hwGlGuiPolygon
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    USE_GL_CTX(disp);
    int i;
    glGuiElement elem;

    if( !glctx )             return;
    if((glctx->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    for (i = 2; i < numPts; i++) {
        elem = __hwGlAllocGuiElem(glctx);
        if (!elem) return;
        elem->typeFlags = GUI_TRIANGLE | flags;
        elem->color = color;
        elem->u.guiTriangle.pointA[0] = pts[0];
        elem->u.guiTriangle.pointA[1] = pts[1];
        elem->u.guiTriangle.pointB[0] = pts[2*i-2];
        elem->u.guiTriangle.pointB[1] = pts[2*i-1];
        elem->u.guiTriangle.pointC[0] = pts[2*i  ];
        elem->u.guiTriangle.pointC[1] = pts[2*i+1];
    }
}

static void beginGui( hwDisplay disp )
{
    USE_GL_CTX(disp);
    int i;

    disp->drawQuads = __hwGlTextQuads;
    disp->surfAttrs = __hwGlTextAttrs;

    /* Go to a defined state */
    __hwgl_SetSolidState(glctx);

    hwGlMatrixMode(HWGL_MODELVIEW);
    hwGlPushMatrix();
    hwGlLoadIdentity();

    hwGlMatrixMode(HWGL_PROJECTION);
    hwGlPushMatrix();
    hwGlLoadIdentity();

    hwGlOrtho(0.f, (float)glctx->winW,
              0.f, (float)glctx->winH,
              0.f, 1.f);

    hwGlDisable( HWGL_FOG );
    hwGlDisable( GL_DEPTH_TEST );
    hwGlDisable( HWGL_LIGHTING );
    hwGlDisable( GL_CULL_FACE );
    hwGlEnable( GL_BLEND );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if !defined(ANDROID_NDK)
    hwGlDisable( GL_POLYGON_STIPPLE );
#endif

    for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
        hwGlActiveTexture(HWGL_TEXTURE0 + i);
        hwGlDisable(HWGL_TEXTURE_2D);
        glctx->tmId[i] = -1;
    }

    glctx->lastElemType = -1;
}

static void endGui( hwDisplay disp )
{
    USE_GL_CTX(disp);

    hwGlMatrixMode(HWGL_PROJECTION);
    hwGlPopMatrix();
    hwGlMatrixMode(HWGL_MODELVIEW);
    hwGlPopMatrix();

    disp->drawQuads = __hwGlDrawQuads;
    disp->surfAttrs = __hwGlSurfAttrs;

    if( glctx->renderMode & HW_RENDER_CULL_FACE ) {
        hwGlEnable( GL_CULL_FACE );
    }
    if( glctx->renderMode & HW_RENDER_Z_TEST ) {
        hwGlEnable( GL_DEPTH_TEST );
    }
    if( glctx->enableBits & ENABLE_FOG ) {
        hwGlEnable( HWGL_FOG );
    }
    glctx->blend = 0;
    glctx->blendSrc = 0;
    hwGlDisable(GL_BLEND);
    glctx->transp = 0.;
    glctx->color[0] = 1.;
    glctx->color[1] = 1.;
    glctx->color[2] = 1.;
    hwGlColor4f(1., 1., 1., 1.);
    glctx->tmActive = -1;
    __hwgl_SetWireframeState(glctx);
    __hwgl_SetSolidState(glctx);
}

static void drawGuiElements( hwDisplay disp, glGuiElement elems )
{
    USE_GL_CTX(disp);
    glGuiElement elem;
    hwFloat rgb[4];
    hwFloat rect[4*5];  /* XYZST */
    hwInt32 mode = 0;
    int i, h;
    glGuiList list;

    h = glctx->winH;
    for (elem = elems; elem; elem = elem->next) {
        if (elem->typeFlags != glctx->lastElemType) {
            switch( elem->typeFlags & GUI_TYPE_MASK ) {
            case GUI_TEXT :
                /* Text must have texture 0 enabled... */
                glctx->tmActive = 0;
                hwGlActiveTexture(HWGL_TEXTURE0);
                hwGlEnable(HWGL_TEXTURE_2D);
                /* And blending */
                hwGlEnable(GL_BLEND);
                break;
            case GUI_LINE :
                /* Lines must have texture 0 disabled */
                glctx->tmActive = 0;
                hwGlActiveTexture(HWGL_TEXTURE0);
                hwGlDisable(HWGL_TEXTURE_2D);
                if (elem->typeFlags & HW_GUI_SMOOTH) {
                    hwGlEnable(HWGL_LINE_SMOOTH);
                    hwGlEnable(GL_BLEND);
                } else if (elem->typeFlags & HW_GUI_BLEND) {
                    hwGlDisable(HWGL_LINE_SMOOTH);
                    hwGlEnable(GL_BLEND);
                } else {
                    hwGlDisable(HWGL_LINE_SMOOTH);
                    hwGlDisable(GL_BLEND);
                }
                /* We're drawing lines... */
                mode = GL_LINES;
                __hwGlSetDataPointers( glctx, rect, 0, 4 );
                break;
            case GUI_TRIANGLE :
                /* Triangles must have texture 0 disabled */
                glctx->tmActive = 0;
                hwGlActiveTexture(HWGL_TEXTURE0);
                hwGlDisable(HWGL_TEXTURE_2D);
#ifndef ANDROID_NDK
                if (elem->typeFlags & HW_GUI_SMOOTH) {
                    hwGlEnable(GL_POLYGON_SMOOTH);
                    hwGlEnable(GL_BLEND);
                } else if (elem->typeFlags & HW_GUI_BLEND) {
                    hwGlDisable(GL_POLYGON_SMOOTH);
                    hwGlEnable(GL_BLEND);
                } else {
                    hwGlDisable(GL_POLYGON_SMOOTH);
                    hwGlDisable(GL_BLEND);
                }
#endif
                /* We're drawing triangles... */
                mode = GL_TRIANGLES;
                __hwGlSetDataPointers( glctx, rect, 0, 4 );
                break;
            case GUI_RECT :
                /* Rects must have texture 0 disabled... */
                glctx->tmActive = 0;
                hwGlActiveTexture(HWGL_TEXTURE0);
                hwGlDisable(HWGL_TEXTURE_2D);
                if (elem->typeFlags & HW_GUI_BLEND) {
                    hwGlEnable(GL_BLEND);
                } else {
                    hwGlDisable(GL_BLEND);
                }
                /* We're drawing quads... */
                mode = GL_TRIANGLE_FAN;
                __hwGlSetDataPointers( glctx, rect, 0, 4 );
                break;
            case GUI_TEXTURE :
                /* Texture rects must have texture 0 enabled... */
                glctx->tmActive = 0;
                hwGlActiveTexture(HWGL_TEXTURE0);
                hwGlEnable(HWGL_TEXTURE_2D);
                /* And blending enabled */
                hwGlEnable(GL_BLEND);
                __hwGlSetDataPointers( glctx, rect, HW_DATA_ST, 4 );
                break;
            }
            glctx->lastElemType = elem->typeFlags;
        }

        hwGlColor4ub( (elem->color >> 16) & 0xFF,
                    (elem->color >>  8) & 0xFF,
                    (elem->color      ) & 0xFF,
                    (elem->color >> 24) & 0xFF );

        switch (elem->typeFlags & GUI_TYPE_MASK) {
        case GUI_LIST :
            /* World's stupidest hash function */
            i = elem->u.guiList.id % GUI_HT_SIZE;
            for (list = glctx->guiListHash[i];
                     list; list = list->nextHash ) {
                if (list->id == elem->u.guiList.id) {
                    break;
                }
            }
            if (!list) {
                break;
            }

            drawGuiElements(disp, list->head);
            break;

        case GUI_TEXT :
            rgb[0] = ((elem->color >> 16) & 0xFF) * (1.f / 255.f);
            rgb[1] = ((elem->color >>  8) & 0xFF) * (1.f / 255.f);
            rgb[2] = ((elem->color      ) & 0xFF) * (1.f / 255.f);
            rgb[3] = ((elem->color >> 24) & 0xFF) * (1.f / 255.f);
            txfSetColor(rgb);
            txfSetScale((hwFloat)elem->u.guiText.size);
            txfSetPosition((hwFloat)elem->u.guiText.pos[0],
                                       (hwFloat)(h - elem->u.guiText.pos[1]));
            txfBeginFontRendering(disp,
                                  elem->u.guiText.txf, TXF_COORD_MODE_NDC);
            txfRenderString(disp, elem->u.guiText.txf,
                                  elem->u.guiText.string,
                                  strlen((char *)elem->u.guiText.string));
            txfEndFontRendering(disp);
            break;

        case GUI_RECT :
            rect[3*0  ] = elem->u.guiRect.pointA[0];
            rect[3*0+1] = h - elem->u.guiRect.pointA[1];
            rect[3*0+2] = 0;
            rect[3*1  ] = elem->u.guiRect.pointB[0];
            rect[3*1+1] = h - elem->u.guiRect.pointA[1];
            rect[3*1+2] = 0;
            rect[3*2  ] = elem->u.guiRect.pointB[0];
            rect[3*2+1] = h - elem->u.guiRect.pointB[1];
            rect[3*2+2] = 0;
            rect[3*3  ] = elem->u.guiRect.pointA[0];
            rect[3*3+1] = h - elem->u.guiRect.pointB[1];
            rect[3*3+2] = 0;
            hwGlBeginRendering();
            glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
            break;

        case GUI_LINE :
            rect[3*0  ] = elem->u.guiRect.pointA[0];
            rect[3*0+1] = h - elem->u.guiRect.pointA[1];
            rect[3*0+2] = 0;
            rect[3*1  ] = elem->u.guiRect.pointB[0];
            rect[3*1+1] = h - elem->u.guiRect.pointB[1];
            rect[3*1+2] = 0;
            hwGlBeginRendering();
            glDrawArrays( GL_LINES, 0, 2 );
            break;

        case GUI_TRIANGLE :
            rect[3*0  ] = elem->u.guiTriangle.pointA[0];
            rect[3*0+1] = h - elem->u.guiTriangle.pointA[1];
            rect[3*0+2] = 0;
            rect[3*1  ] = elem->u.guiTriangle.pointB[0];
            rect[3*1+1] = h - elem->u.guiTriangle.pointB[1];
            rect[3*1+2] = 0;
            rect[3*2  ] = elem->u.guiTriangle.pointC[0];
            rect[3*2+1] = h - elem->u.guiTriangle.pointC[1];
            rect[3*2+2] = 0;
            hwGlBeginRendering();
            glDrawArrays( GL_TRIANGLES, 0, 3 );
            break;

        case GUI_TEXTURE :
            __hwGlCurrentTexture( disp, elem->u.guiRect.texture, 0 );
            for (i = 1; i < HW_MAX_TEXTURES; i++) {
                __hwGlCurrentTexture( disp, -1, i );
            }
            rect[5*0  ] = elem->u.guiRect.pointA[0];
            rect[5*0+1] = h - elem->u.guiRect.pointA[1];
            rect[5*0+2] = 0;
            rect[5*0+3] = 0.;
            rect[5*0+4] = 0.;
            rect[5*1  ] = elem->u.guiRect.pointB[0];
            rect[5*1+1] = h - elem->u.guiRect.pointA[1];
            rect[5*1+2] = 0;
            rect[5*1+3] = 1.;
            rect[5*1+4] = 0.;
            rect[5*2  ] = elem->u.guiRect.pointB[0];
            rect[5*2+1] = h - elem->u.guiRect.pointB[1];
            rect[5*2+2] = 0;
            rect[5*2+3] = 1.;
            rect[5*2+4] = 1.;
            rect[5*3  ] = elem->u.guiRect.pointA[0];
            rect[5*3+1] = h - elem->u.guiRect.pointB[1];
            rect[5*3+2] = 0;
            rect[5*3+3] = 0.;
            rect[5*3+4] = 1.;
            hwGlBeginRendering();
            glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
            break;
        }
    }
}

void __hwGlDrawElements( hwDisplay disp )
{
    USE_GL_CTX(disp);
    if (!glctx) return;

    beginGui(disp);

    drawGuiElements(disp, glctx->guiList);

    endGui(disp);
}

hwInt32 __hwGlOpenGuiList( hwDisplay disp )
{
    USE_GL_CTX(disp);
    glGuiList newList;

    if (!glctx) return 0;

    if (glctx->activeGuiList) {
        return 0;
    }

    if (glctx->freeGuiList) {
        newList = glctx->freeGuiList;
        glctx->freeGuiList = newList->nextHash;
    } else {
        newList = malloc(sizeof(struct __glGuiList));
        if (!newList) {
            return 0;
        }
        newList->id = ++glctx->numGuiLists;
    }

    newList->prevHash = newList->nextHash = NULL;
    newList->head = newList->tail = NULL;

    glctx->activeGuiList = newList;

    return newList->id;
}

void __hwGlCloseGuiList( hwDisplay disp )
{
    USE_GL_CTX(disp);
    int h;
    glGuiList newList;

    if (!glctx) return;

    if (!glctx->activeGuiList) {
        return;
    }

    newList = glctx->activeGuiList;

    h = newList->id % GUI_HT_SIZE; /* World's stupidest hash function */
    newList->nextHash = glctx->guiListHash[h];
    if (glctx->guiListHash[h]) {
        glctx->guiListHash[h]->prevHash = newList;
    }
    glctx->guiListHash[h] = newList;

    glctx->activeGuiList = NULL;
}

void __hwGlCallGuiList( hwDisplay disp, hwInt32 id )
{
    USE_GL_CTX(disp);
    glGuiElement elem;

    if (!glctx) return;

    elem = __hwGlAllocGuiElem(glctx);
    if (!elem) return;

    elem->typeFlags = GUI_LIST;
    elem->color = 0;
    elem->u.guiList.id = id;
}

void __hwGlDestroyGuiList( hwDisplay disp, hwInt32 id )
{
    USE_GL_CTX(disp);
    int h;
    glGuiList list;

    if (!glctx) return;

    /* TBD: Some day it would not be hard to have nested GUI DLs */
    if (glctx->activeGuiList) {
        return;
    }

    h = id % GUI_HT_SIZE; /* World's stupidest hash function */
    for (list = glctx->guiListHash[h];
         list; list = list->nextHash ) {
        if (list->id == id) {
            break;
        }
    }

    if (!list) {
        return;
    }

    if (list->prevHash) {
        list->prevHash->nextHash = list->nextHash;
    } else {
        glctx->guiListHash[h] = list->nextHash;
    }

    if (list->nextHash) {
        list->nextHash->prevHash = list->prevHash;
    }

    if (list->tail) {
        list->tail->next = glctx->guiFree;
        glctx->guiFree = list->head;
    }

    list->nextHash = glctx->freeGuiList;
    glctx->freeGuiList = list;
}
