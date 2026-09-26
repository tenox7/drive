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
#include "hw.h"
#include "hw_internal.h"
#include "hw_dx.h"
#include "hw_vecmath.h"

#ifdef WIN32
#       define  ISNAN(x)        _isnan((double)x)
#else
#       define  ISNAN(x)        isnan(x)
#endif

extern int
    __hwdxShowBoxes,
    __hwdxPruneEye,
    __hwdxDoPrune;

void __hwDxPosLight
(
    hwDisplay disp,
    hwFloat Color[3], hwFloat Pos[3], hwFloat Dir[3],
    hwFloat Cutoff, hwFloat LightExp, hwFloat Atten
)
{
    hwInt32
        NumLights, attr, n, LightNum;
    hwFloat
        newColor[4],
        newPos[4];
    D3DLIGHT9 light;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    NumLights = ++__hwdxCurrentContext->numLights;
    LightNum = NumLights-1;

    ZeroMemory( &light, sizeof(light) );

    if( Cutoff >= 180.0 ) {
        light.Type = D3DLIGHT_POINT;
    }
    else {
        light.Type = D3DLIGHT_SPOT;
    }
    light.Position.x = Pos[0];
    light.Position.y = Pos[1];
    light.Position.z = Pos[2];
    light.Direction.x = Dir[0];
    light.Direction.y = Dir[1];
    light.Direction.z = Dir[2];
    light.Ambient.r = 0;//__hwdxCurrentContext->ambColor[0];
    light.Ambient.g = 0;//__hwdxCurrentContext->ambColor[1];
    light.Ambient.b = 0;//__hwdxCurrentContext->ambColor[2];
    light.Ambient.a = 1.0;
    light.Diffuse.r = Color[0];
    light.Diffuse.g = Color[1];
    light.Diffuse.b = Color[2];
    light.Diffuse.a = 1.0;
    light.Specular.r = 1.0;
    light.Specular.g = 1.0;
    light.Specular.b = 1.0;
    light.Specular.a = 1.0;
    light.Attenuation0 = 0.0;
    light.Attenuation0 = 0.0;
    light.Attenuation0 = 1.0;
    light.Theta = 3.14159265358979;
    light.Phi = 3.14159265358979;

    if( Cutoff < 0.0 )  Cutoff = 180.0;
    //glLightf( LightNum, GL_SPOT_CUTOFF, Cutoff );

    if( LightExp < 0.0 )        LightExp = 0.0;
    //glLightf( LightNum, GL_SPOT_EXPONENT, LightExp );

    if( Cutoff < 0.0 )  Cutoff = 180.0;
    //glLightf( LightNum, GL_SPOT_CUTOFF, Cutoff );

    if( LightExp < 0.0 )        LightExp = 0.0;
    //glLightf( LightNum, GL_SPOT_EXPONENT, LightExp );

    if( Atten < 0.0 )   Atten = 0.0;
    //glLightf( LightNum, GL_LINEAR_ATTENUATION, Atten );

#if 0
    glLightfv( LightNum, GL_AMBIENT, __hwdxCurrentContext->ambColor );
#endif
    {
       float ambient[4] = {
          0.0,
          0.0,
          0.0,
          1.0
       };
    //glLightfv( LightNum, GL_AMBIENT, ambient );
    }

    VCOPY( newColor, Color );   newColor[3] = 1.0;
    //glLightfv( LightNum, GL_DIFFUSE, newColor );

    VASSIGN( newColor, 1.0, 1.0, 1.0 );
    ///*VASSIGN( newColor, 0.6, 0.6, 0.6 );*//*TBD:???*/
    //glLightfv( LightNum, GL_SPECULAR, newColor );
    IDirect3DDevice9_LightEnable( __hwdxCurrentContext->d3dDevice,
                                        LightNum, TRUE );
    IDirect3DDevice9_SetLight( __hwdxCurrentContext->d3dDevice,
                                        LightNum, &light );

    /* "Lock in" the current camera */
    __hwdxCurrentContext->camSet = 1;
}

void __hwDxDirLight( hwDisplay disp, hwFloat Color[3], hwFloat Dir[3] )
{
    hwInt32
        NumLights, attr, n, LightNum;
    hwFloat
        newColor[4],
        newPos[4];
    D3DLIGHT9 light;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    NumLights = ++__hwdxCurrentContext->numLights;
    LightNum = NumLights-1;

    ZeroMemory( &light, sizeof(light) );

    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Direction.x = Dir[0];
    light.Direction.y = Dir[1];
    light.Direction.z = Dir[2];
    light.Ambient.r = 0;//__hwdxCurrentContext->ambColor[0];
    light.Ambient.g = 0;//__hwdxCurrentContext->ambColor[1];
    light.Ambient.b = 0;//__hwdxCurrentContext->ambColor[2];
    light.Ambient.a = 1.0;
    light.Diffuse.r = Color[0];
    light.Diffuse.g = Color[1];
    light.Diffuse.b = Color[2];
    light.Diffuse.a = 1.0;
    light.Specular.r = 1.0;
    light.Specular.g = 1.0;
    light.Specular.b = 1.0;
    light.Specular.a = 1.0;
    light.Attenuation0 = 0.0;
    light.Attenuation0 = 0.0;
    light.Attenuation0 = 1.0;
    light.Theta = 3.14159265358979;
    light.Phi = 3.14159265358979;
    IDirect3DDevice9_LightEnable( __hwdxCurrentContext->d3dDevice,
                                        LightNum, TRUE );
    IDirect3DDevice9_SetLight( __hwdxCurrentContext->d3dDevice,
                                        LightNum, &light );

    /* "Lock in" the current camera */
    __hwdxCurrentContext->camSet = 1;
}

static DWORD DXcolor(float r, float g, float b, float a)
{
    int rr, gg, bb, aa;

    rr = r * 255.f; if(rr<0)rr=0;if(rr>255)rr=255;
    gg = g * 255.f; if(gg<0)gg=0;if(gg>255)gg=255;
    bb = b * 255.f; if(bb<0)bb=0;if(bb>255)bb=255;
    aa = a * 255.f; if(aa<0)aa=0;if(aa>255)aa=255;
    return D3DCOLOR_ARGB(aa,rr,gg,bb);
}

void __hwDxBackground( hwDisplay disp, hwFloat Color[3] )
{
    if (!__hwdxCurrentContext) return;

    __hwdxCurrentContext->clearColor = DXcolor(Color[0], Color[1],
                                               Color[2], 1.0);
}

void __hwDxLighting( hwDisplay disp, hwInt32 OnOff )
{
    if( !__hwdxCurrentContext ) return;
    __hwdxCurrentContext->lightOn = OnOff;
    if( OnOff && !__hwdxCurrentContext->wireframeState ) {
        IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_LIGHTING, TRUE );
    }
    else {
        IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_LIGHTING, FALSE );
    }
}

void __hwDxFog( hwDisplay disp, hwInt32 OnOff, hwFloat Color[3] )
{
    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    if( OnOff ) {
        IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGCOLOR,
                                        DXcolor(Color[0], Color[1],
                                                Color[2], 1.0));
        IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGENABLE, TRUE );
    }
    else {
        IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGENABLE, FALSE );
    }
}

void __hwDxFogParams( hwDisplay disp, 
                      hwInt32 t,        /* Type LINEAR, EXP, or EXP2 */
                      hwFloat p[2],     /* Front and back Planes */
                      hwFloat d )       /* density */
{
    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    switch( t ) {
        case HW_FOG_LINEAR:
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGSTART,
                                        ((DWORD *)p)[0] );
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGEND,
                                        ((DWORD *)p)[1] );
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGTABLEMODE,
                                        D3DFOG_LINEAR );
            break;
        case HW_FOG_EXP:
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGDENSITY,
                                        *(DWORD *)&d );
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGTABLEMODE,
                                        D3DFOG_EXP );
            break;
        case HW_FOG_EXP2:
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGDENSITY,
                                        *(DWORD *)&d );
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                        D3DRS_FOGTABLEMODE,
                                        D3DFOG_EXP2 );
            break;
    }
}


void __hwDxAmbient( hwDisplay disp, hwFloat Factor, hwFloat Color[3] )
{
    hwFloat *amb;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    __hwdxCurrentContext->ambFact = Factor;
    __hwdxCurrentContext->ambColor[0] = Color[0] * Factor;
    __hwdxCurrentContext->ambColor[1] = Color[1] * Factor;
    __hwdxCurrentContext->ambColor[2] = Color[2] * Factor;
    __hwdxCurrentContext->ambColor[3] = 1.0;
    __hwdxCurrentContext->ambFactSave = -1.0;
    amb = __hwdxCurrentContext->ambColor;
    IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                     D3DRS_AMBIENT,
                                     DXcolor(amb[0], amb[1], amb[2], 1.0) );
}



void __hwDxDestroyTexture( hwDisplay disp, hwInt32 tid )
{
    /* TBD */
}

void __hwDxEstablishTexture( hwDisplay disp, dxTexture *tm )
{
#if 0
    hwInt32
        i, j, w, h, fmt,
        result, texName;

    if( !__hwdxCurrentContext ) return;

    /* This GenLists is outside of the ifdef since we ALWAYS
     * use a display list for our textures; we do this because
     * we establish state which is not part of the texture
     * object.
     */
    result = glGenLists( 1 );
    if( !result )       return; /* TBD */


    if( tm->img->tmId == -1 ) {
        glGenTextures( 1, (GLuint *)&texName );
        glBindTexture( GL_TEXTURE_2D, texName );
        tm->img->tmId = texName;

        w = tm->img->width; h = tm->img->height;

        switch( tm->img->components ) {
        case 1 :        fmt = GL_LUMINANCE;             break;
        case 2 :        fmt = GL_LUMINANCE_ALPHA;       break;
        case 3 :        fmt = GL_RGB;                   break;
        case 4 :        fmt = GL_RGBA;                  break;
        }

        gluBuild2DMipmaps( GL_TEXTURE_2D, tm->img->components,
                        w, h, fmt, GL_UNSIGNED_BYTE, tm->img->data );

    }
    else {
        texName = tm->img->tmId;
    }

    glNewList( result, GL_COMPILE );
    glBindTexture( GL_TEXTURE_2D, texName );

    if(tm->coordMode == HW_TM_ENVMAP) {
        glTexGeni(GL_S, GL_SPHERE_MAP, 0);
        glTexGeni(GL_T, GL_SPHERE_MAP, 0);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
    }
    else {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    if( tm->bound ) {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
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

    switch( tm->appl ) {
    case HW_TM_MODULATE :
        glTexEnvi( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        break;
    default :
        /* TBD */
        break;
    }

#ifdef GL_TEXTURE_PRE_SPECULAR_HP
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_LIGHTING_MODE_HP,
                        GL_TEXTURE_PRE_SPECULAR_HP );
#endif
    glEndList();

    tm->textureID = result;
#endif
}

hwInt32 __hwDxCreateTexture
(
    hwDisplay disp,
    hwImageStruct *img, hwInt32 filt, hwInt32 appl, hwInt32 bound,
    hwInt32 coordMode, hwOrientType *orient, hwInt32 internalFormat
)
{
#if 0
    glTexture
        tm;

    if( !__hwdxCurrentContext ) return -1;
    if( !img ) return -1;

#if 0
    tm = malloc( sizeof(glTexture) );
    if( !tm )   return 0;
#endif

    tm.img = img;
    tm.filt = filt;
    tm.appl = appl;
    tm.bound = bound;
    tm.coordMode = coordMode;
    tm.orient = *orient;
    __hwDxEstablishTexture( disp, &tm );

    return tm.textureID;
#else
    return -1;
#endif
}

void __hwDxCurrentTexture( hwDisplay disp, hwInt32 tid, hwInt32 texNum )
{
#if 0
    if( !__hwdxCurrentContext ) return;
    if( texNum >= __hwdxCurrentContext->maxTexNum ) return;

    /* Set currently active texture */
    if( texNum != __hwdxCurrentContext->tmActive ) {
        glActiveTextureARB( GL_TEXTURE0_ARB + texNum );
        __hwdxCurrentContext->tmActive = texNum;
    }

    if( tid != -1 ) {
        if( __hwdxCurrentContext->tmId[texNum] == -1 ) {
            if( !(__hwdxCurrentContext->renderMode & HW_RENDER_EDGE_MODE) ) {
                glEnable( GL_TEXTURE_2D );
            }
        }
        if( tid != __hwdxCurrentContext->tmId[texNum] ) {
            glCallList( tid );
            __hwdxCurrentContext->tmId[texNum] = tid;
        }
    }
    else {
        if( __hwdxCurrentContext->tmId[texNum] != -1 ) {
            glDisable( GL_TEXTURE_2D );
            __hwdxCurrentContext->tmId[texNum] = -1;
        }
    }
#endif
}

void __hwDxDrawBBox( hwDisplay disp, hwFloat Points[24] )
{
#if 0
#define POLYVERT(n,m)   glVertex3fv( Points + 3*m );

    if( __hwdxCurrentContext->lightOn ) {
        glDisable( GL_LIGHTING );
    }

    glBegin( GL_LINE_STRIP );
    POLYVERT(0,4) POLYVERT(1,0) POLYVERT(2,1) POLYVERT(3,5) POLYVERT(4,7)
    glEnd();

    glBegin( GL_LINE_STRIP );
    POLYVERT(0,0) POLYVERT(1,2) POLYVERT(2,3) POLYVERT(3,1)
    glEnd();

    glBegin( GL_LINE_STRIP );
    POLYVERT(0,5) POLYVERT(1,4) POLYVERT(2,6) POLYVERT(3,2)
    glEnd();

    glBegin( GL_LINE_STRIP );
    POLYVERT(0,3) POLYVERT(1,7) POLYVERT(2,6)
    glEnd();

    if( __hwdxCurrentContext->lightOn ) {
        glEnable( GL_LIGHTING );
    }
#undef POLYVERT
#endif
}

int __hwDxBoundsVisible
(
    hwDisplay disp, hwSurfaceType *surf, hwFloat BBox[6], hwInt32 complexity
)
{
    hwFloat
        Det, *Tmp, Points[24], OPoints[24];
    hwInt32
        i, j, n, Inside;
    unsigned long
        srcMask, dstMask = 0;

    if( !__hwdxCurrentContext ) return 0;
    if( surf->flags & HW_SURF_INVISIBLE )       return 0;

    if( __hwdxCurrentContext->animMask ) {
        /* Check for Hoverball-style animation - all of the bits
         * in the animMask MUST be present in the surface mask
         */
        srcMask = (unsigned long)__hwdxCurrentContext->animMask;
        dstMask = (unsigned long)surf->visibility;
        if( (srcMask & dstMask) != srcMask ) {
            return 0;
        }
    }
    else {
        /* Check for DRIVE-style animation:
         *      "Graphical primitives will not be drawn whenever the current
         *      name set includes any of the names in the invisibility
         *      filter's inclusion set but doesn't include any of the names
         *      in the invisibility filter's exclusion set."
         */
        dstMask = surf->visibility;
        if( __hwdxCurrentContext->invisibleInclude & dstMask ) {
            if( !(__hwdxCurrentContext->invisibleExclude & dstMask) ) {
                return 0;
            }
        }
    }

#if 0
    /* Don't even check if not very complex */
    if( complexity < 25 )       return 1;

    if( !BBox )                                 return 1;
    if( !__hwdxCurrentContext->planesValid )    return 1;

    Tmp = Points;
    *Tmp++ = BBox[0]; *Tmp++ = BBox[1]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[1]; *Tmp++ = BBox[5];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[4]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[0]; *Tmp++ = BBox[4]; *Tmp++ = BBox[5];

    *Tmp++ = BBox[3]; *Tmp++ = BBox[1]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[1]; *Tmp++ = BBox[5];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[4]; *Tmp++ = BBox[2];
    *Tmp++ = BBox[3]; *Tmp++ = BBox[4]; *Tmp   = BBox[5];

    if( __hwdxShowBoxes ) {
        (void)memcpy( OPoints, Points, sizeof(Points) );
    }

    n = __hwdxCurrentContext->stackDepth - 1;
    if( n >= 0 ) {
        for( i = 0; i < 8; i++ ) {
            hwTransform( __hwdxCurrentContext->rawMatStack[n], Points+3*i );
        }
    }

    n = __hwdxCurrentContext->planesValid;
    for( i = 0; i < n; i++ ) {
        Inside = 0; Tmp = Points;
        for( j = 0; j < 8; j++ ) {
            Det = VDOT( Tmp, __hwdxCurrentContext->WC_Planes[i] );
            Det += __hwdxCurrentContext->WC_Planes[i][3];
            if( Det > 0 ) { Inside = 1; break; }
            Tmp += 3;
        }
        if( !Inside ) {
            if( __hwdxShowBoxes ) {
                glColor3f( 1.0, 0.0, 1.0 );
                __hwDxDrawBBox( disp, OPoints );
            }
            return 0;
        }
    }

    if( __hwdxShowBoxes ) {
        glColor3f( 1.0, 1.0, 1.0 );
        __hwDxDrawBBox( disp, OPoints );
    }

#endif
    return 1;
}

hwFloat __hwDxBoundsSize( hwDisplay disp, hwFloat BBox[6] )
{
#if 0
    hwFloat
        Points[6],
        len, dist, ang,
        dx, dy, dz,
        px, py, pz;
    int
        i, n;

    if( !__hwdxCurrentContext ) return 0.0;

    (void)memcpy( Points, BBox, 6*sizeof(float) );

    /* Get the point in VDCs */
    n = __hwdxCurrentContext->stackDepth - 1;
    if( n >= 0 ) {
        hwTransform( __hwdxCurrentContext->rawMatStack[n],
                                    Points );
        hwTransform( __hwdxCurrentContext->rawMatStack[n],
                                    Points+3 );
    }

    /* We now have a world-coordinate diagonal.  Its half-length
     * is the radius of a sphere which we will use for our nefarious
     * angle-calculating purposes.  The sphere's center is the center
     * of the bounding box.
     */

    px = (Points[0] + Points[3]) * 0.5;
    py = (Points[1] + Points[4]) * 0.5;
    pz = (Points[2] + Points[5]) * 0.5;
    dx = px - __hwdxCurrentContext->camPos[0];
    dy = py - __hwdxCurrentContext->camPos[1];
    dz = pz - __hwdxCurrentContext->camPos[2];
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
    ang = ang / __hwdxCurrentContext->camField;
    if( ang > 0.99999 ) ang = 0.99999;
    return ang * 100.0;
#else
    return 100.0f;
#endif
}

void __hwDxCamera( hwDisplay disp, hwCamStruct *cam )
{
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
        dxcam;

    if( !__hwdxCurrentContext ) return;
    if( __hwdxCurrentContext->camSet )  return;

    n = __hwdxCurrentContext->stackDepth - 1;
    if( n >= 0 ) {
        mat = (void *)&__hwdxCurrentContext->matStack[n][0][0];
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

    dxcam.field_of_view = cam->field;
#if 1
    dist = __hwdxCurrentContext->VDC_YMax / __hwdxCurrentContext->VDC_XMax;
    if( dist < 1.0 ) {
        /* Starbase field of view is always in Y.  We want the
         * field of view to be the maximum in X and Y.  SO,
         * we do this little kludge.
         */
        dxcam.field_of_view *= dist;
    }
#endif
    __hwdxCurrentContext->camField = dxcam.field_of_view*3.141592653589/180.0;

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

    __hwdxCurrentContext->camPos[0] = dxcam.camx = pos[0];
    __hwdxCurrentContext->camPos[1] = dxcam.camy = pos[1];
    __hwdxCurrentContext->camPos[2] = dxcam.camz = pos[2];

    dxcam.upx = up[0];
    dxcam.upy = up[1];
    dxcam.upz = up[2];

    dxcam.refx = dxcam.camx + dist * dir[0];
    dxcam.refy = dxcam.camy + dist * dir[1];
    dxcam.refz = dxcam.camz + dist * dir[2];

    dxcam.front = cam->planes[0] - dist;
    dxcam.back = cam->planes[1] - dist;

    dxcam.projection = cam->perspective;
    dxcam.skewX = cam->skew[0];
    dxcam.skewY = cam->skew[1];
    dxcam.jitterX = cam->jitter[0];
    dxcam.jitterY = cam->jitter[1];

#if 0
    /* Make sure to be at the *bottom* of the matrix stack */
    glMatrixMode( GL_MODELVIEW );
    for( i = 0; i <= n; i++ ) {
        glPopMatrix();
    }
#endif

    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        __dxCamera( &dxcam, &__hwdxCurrentContext->sel,
                __hwdxCurrentContext->winW, __hwdxCurrentContext->winH );
    }
    else {
        __dxCamera( &dxcam, 0,
                __hwdxCurrentContext->winW, __hwdxCurrentContext->winH );
    }

#if 0
    glGetFloatv( GL_MODELVIEW_MATRIX, &__hwdxCurrentContext->camMat[0][0] );

    /* Get corners of view volume, in WCs */
    {
        GLdouble modelMatrix[16];
        GLdouble projMatrix[16];
        GLint viewport[4];
        double xyz[3];

        glGetDoublev( GL_MODELVIEW_MATRIX, modelMatrix );
        glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );
        glGetIntegerv( GL_VIEWPORT, viewport );

        (void)gluUnProject( (double)0, (double)0, (double)0,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[0][0] = xyz[0];
        WC_Corners[0][1] = xyz[1];
        WC_Corners[0][2] = xyz[2];

        (void)gluUnProject( (double)(__hwdxCurrentContext->VDC_XMax-1),
                        (double)0, (double)0,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[1][0] = xyz[0];
        WC_Corners[1][1] = xyz[1];
        WC_Corners[1][2] = xyz[2];

        (void)gluUnProject( (double)(__hwdxCurrentContext->VDC_XMax-1),
                        (double)(__hwdxCurrentContext->VDC_YMax-1),
                        (double)0,
                        modelMatrix, projMatrix, viewport,
                        xyz+0, xyz+1, xyz+2 );
        WC_Corners[2][0] = xyz[0];
        WC_Corners[2][1] = xyz[1];
        WC_Corners[2][2] = xyz[2];

        (void)gluUnProject( (double)0,
                        (double)(__hwdxCurrentContext->VDC_YMax-1),
                        (double)0,
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
    __hwdxCurrentContext->WC_Planes[0][0] = normDir[0];
    __hwdxCurrentContext->WC_Planes[0][1] = normDir[1];
    __hwdxCurrentContext->WC_Planes[0][2] = normDir[2];

    VCROSS(__hwdxCurrentContext->WC_Planes[1],WC_Corners[0],WC_Corners[1] );
    VCROSS(__hwdxCurrentContext->WC_Planes[2],WC_Corners[1],WC_Corners[2] );
    VCROSS(__hwdxCurrentContext->WC_Planes[3],WC_Corners[2],WC_Corners[3] );
    VCROSS(__hwdxCurrentContext->WC_Planes[4],WC_Corners[3],WC_Corners[0] );

    VNORM(__hwdxCurrentContext->WC_Planes[1],
            __hwdxCurrentContext->WC_Planes[1]);
    VNORM(__hwdxCurrentContext->WC_Planes[2],
            __hwdxCurrentContext->WC_Planes[2]);
    VNORM(__hwdxCurrentContext->WC_Planes[3],
            __hwdxCurrentContext->WC_Planes[3]);
    VNORM(__hwdxCurrentContext->WC_Planes[4],
            __hwdxCurrentContext->WC_Planes[4]);

    /* Make sure position of camera is accounted for in plane equations */
    for( i = 0; i < 5; i++ ) {
        ptr = __hwdxCurrentContext->WC_Planes[i];
        ptr[3] = -VDOT(ptr,pos);
    }

    if( !__hwdxDoPrune )        __hwdxCurrentContext->planesValid = 0;
    else if( __hwdxPruneEye )   __hwdxCurrentContext->planesValid = 1;
    else if( cam->perspective ) __hwdxCurrentContext->planesValid = 5;
    else                        __hwdxCurrentContext->planesValid = 1;

    /* Restore the matrix stack */
    glMatrixMode( GL_MODELVIEW );
    for( i = 0; i <= n; i++ ) {
        glPushMatrix();
        hwMatMult(      tmpMat,
                        __hwdxCurrentContext->matStack[i],
                        __hwdxCurrentContext->camMat );
        (void)memcpy( &__hwdxCurrentContext->matStack[i][0][0], &tmpMat[0][0],
                        16*sizeof(float) );
        glLoadMatrixf( &__hwdxCurrentContext->matStack[i][0][0] );
    }
#endif

    __hwdxCurrentContext->camSet = 1;
}

void __hwDxSurfAttrs( hwDisplay disp, hwSurfaceType *surf )
{
    hwInt32
        *TM;
    hwInt32
        matChanged = 0,
        i, j, n, doBlend;
    float
        value[4];
    static unsigned int
        Trans[17][32],
        TransInit = 0;
    static unsigned char
        TransVals[17][4] = {
            { 0xF, 0xF, 0xF, 0xF },                     /* 16/16 */
            { 0xF, 0xF, 0xF, 0x7 },                     /* 15/16 */
            { 0xF, 0xD, 0xF, 0x7 },                     /* 14/16 */
            { 0xF, 0xD, 0xF, 0x5 },                     /* 13/16 */
            { 0xF, 0x5, 0xF, 0x5 },                     /* 12/16 */
            { 0xF, 0x5, 0xB, 0x5 },                     /* 11/16 */
            { 0xC, 0x5, 0xB, 0x5 },                     /* 10/16 */
            { 0xC, 0x5, 0xA, 0x5 },                     /*  9/16 */
            { 0xA, 0x5, 0xA, 0x5 },                     /*  8/16 */
            { 0xA, 0x5, 0xA, 0x1 },                     /*  7/16 */
            { 0xA, 0x4, 0xA, 0x1 },                     /*  6/16 */
            { 0xA, 0x4, 0xA, 0x0 },                     /*  5/16 */
            { 0xA, 0x0, 0xA, 0x0 },                     /*  4/16 */
            { 0xA, 0x0, 0x2, 0x0 },                     /*  3/16 */
            { 0x8, 0x0, 0x2, 0x0 },                     /*  2/16 */
            { 0x8, 0x0, 0x0, 0x0 },                     /*  1/16 */
            { 0x0, 0x0, 0x0, 0x0 },                     /*  0/16 */
        };
    D3DMATERIAL9
        mat;

    ZeroMemory(&mat, sizeof(mat));

    if( !TransInit ) {
        /* Copy the one nybble dither cells into the 32x32 patterns */
        for( i = 0; i < 17; i++ ) {
            for( j = 0; j < 4; j++ ) {
                n = TransVals[i][j];    /* 0x0000000F */
                n |= n << 4;            /* 0x000000FF */
                n |= n << 8;            /* 0x0000FFFF */
                n |= n << 16;           /* 0xFFFFFFFF */
                Trans[i][j+ 0] = n;
                Trans[i][j+ 4] = n;
                Trans[i][j+ 8] = n;
                Trans[i][j+12] = n;
                Trans[i][j+16] = n;
                Trans[i][j+20] = n;
                Trans[i][j+24] = n;
                Trans[i][j+28] = n;
            }
        }
        TransInit = 1;
    }

    if( !__hwdxCurrentContext ) return;

    __hwdxCurrentContext->primFlags = surf->flags;

    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    /* If unlit, refect 100% of the ambient color (which happens
     * to be white), else reflect the ambient dimly...
     */
    if( surf->flags & HW_SURF_EMISSIVE ) {
        if( __hwdxCurrentContext->ambFactSave != 1000.0 ) {
            matChanged = 1;
            __hwdxCurrentContext->ambFactSave = 1000.0;
        }
    }
    else {
        if(__hwdxCurrentContext->ambFactSave != __hwdxCurrentContext->ambFact) {
            matChanged = 1;
            __hwdxCurrentContext->ambFactSave = __hwdxCurrentContext->ambFact;
        }
    }

    if(            (surf->specColor[0] != __hwdxCurrentContext->specColor[0])
                || (surf->specColor[1] != __hwdxCurrentContext->specColor[1])
                || (surf->specColor[2] != __hwdxCurrentContext->specColor[2]) 
                || (surf->shininess != __hwdxCurrentContext->shininess) )
    {
        if( surf->shininess > 0.0 ) {
        }
        matChanged = 1;
        __hwdxCurrentContext->specColor[0] = surf->specColor[0];
        __hwdxCurrentContext->specColor[1] = surf->specColor[1];
        __hwdxCurrentContext->specColor[2] = surf->specColor[2];
    }

    /* Need to get a color */
    if( !(surf->flags & HW_SURF_UNCOLORED) ) {
        if(        (surf->color[0] != __hwdxCurrentContext->color[0])
                || (surf->color[1] != __hwdxCurrentContext->color[1])
                || (surf->color[2] != __hwdxCurrentContext->color[2]) )
        {
            __hwdxCurrentContext->color[0] = surf->color[0];
            __hwdxCurrentContext->color[1] = surf->color[1];
            __hwdxCurrentContext->color[2] = surf->color[2];
            matChanged = 1;
        }
    }


    if( surf->shininess != __hwdxCurrentContext->shininess ) {
        matChanged = 1;
        __hwdxCurrentContext->shininess = surf->shininess;
    }

    if( matChanged ) {
        mat.Diffuse.r = __hwdxCurrentContext->color[0];
        mat.Diffuse.g = __hwdxCurrentContext->color[1];
        mat.Diffuse.b = __hwdxCurrentContext->color[2];
        mat.Diffuse.a = 1.0;
        if( surf->flags & HW_SURF_EMISSIVE ) {
            mat.Ambient.r = 1.0;
            mat.Ambient.g = 1.0;
            mat.Ambient.b = 1.0;
            mat.Ambient.a = 1.0;
        }
        else {
            mat.Ambient.r = __hwdxCurrentContext->color[0];
            mat.Ambient.g = __hwdxCurrentContext->color[1];
            mat.Ambient.b = __hwdxCurrentContext->color[2];
            mat.Ambient.a = 1.0;
        }
        mat.Specular.r = surf->specColor[0];
        mat.Specular.g = surf->specColor[1];
        mat.Specular.b = surf->specColor[2];
        mat.Specular.a = 1.0;
        mat.Power = surf->shininess * 256.0;
        if( mat.Power > 0.0 ) {
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                                D3DRS_SPECULARENABLE, TRUE );
        }
        else {
            IDirect3DDevice9_SetRenderState( __hwdxCurrentContext->d3dDevice,
                                                D3DRS_SPECULARENABLE, FALSE );
        }
        IDirect3DDevice9_SetMaterial( __hwdxCurrentContext->d3dDevice, &mat );
    }

    if( surf->primSize != __hwdxCurrentContext->primSize ) {
        __hwdxCurrentContext->primSize = surf->primSize;
        //glLineWidth( surf->primSize );
        //glPointSize( surf->primSize );
        if( surf->primSize <= 1.0 ) {
            //glEnable( GL_LINE_SMOOTH );
            //glEnable( GL_POINT_SMOOTH );
        }
        else {
            //glDisable( GL_LINE_SMOOTH );
            //glDisable( GL_POINT_SMOOTH );
        }
    }

    n = (int)(surf->transp * 16.0 + 0.5);
    if( n < 0 ) n = 0; if( n > 16 ) n = 16;

    if( n != __hwdxCurrentContext->transp ) {
        if( n > 0 ) {
            //glPolygonStipple( (GLubyte *)&Trans[n][0] );
            //glEnable( GL_POLYGON_STIPPLE );
        }
        else {
            //glDisable( GL_POLYGON_STIPPLE );
        }
        __hwdxCurrentContext->transp = n;
    }

    if( surf->numTextures && !(surf->flags & HW_SURF_TEX_SAMP) ) {
        for( n = 0; n < surf->numTextures; n++ ) {
            surf->textures[n]->draw( surf->textures[n], disp );
            (void)surf->textures[n]->inquire( surf->textures[n],
                                                hwStrTextureID, (void **)&TM );
            __hwDxCurrentTexture( disp, *TM, n );
        }
        for( ; n < HW_MAX_TEXTURES; n++ ) {
            __hwDxCurrentTexture( disp, -1, n );
        }
    }
    else if( __hwdxCurrentContext->tmActive != -1 ) {
        for( n = 0; n < HW_MAX_TEXTURES; n++ ) {
            //glActiveTextureARB( GL_TEXTURE0_ARB + n );
            //glDisable( GL_TEXTURE_2D );
            __hwdxCurrentContext->tmId[n] = -1;
        }
        __hwdxCurrentContext->tmActive = -1;
    }

    doBlend = 0;
    if( surf->flags & HW_SURF_BLEND ) {
        doBlend = 1;
    }
    else if( surf->flags & HW_SURF_WIREFRAME ) {
        if( surf->primSize <= 1.0 ) {
            doBlend = 1;
        }
    }

    if( doBlend ) {
        if( __hwdxCurrentContext->blend != 1 ) {
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            //glEnable(GL_BLEND);
            __hwdxCurrentContext->blend = 1;
        }
    }
    else {
        if( __hwdxCurrentContext->blend != 0 ) {
            //glDisable(GL_BLEND);
            //glBlendFunc(GL_ONE, GL_ZERO);
            __hwdxCurrentContext->blend = 0;
        }
    }
}

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


static void SetWireframeState( void )
{
    int
        i, pt = 0;

    if( __hwdxCurrentContext->wireframeState == 1 )     return;
    __hwdxCurrentContext->wireframeState = 1;

    if( __hwdxCurrentContext->tmActive != -1 ) {
        for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
            //glActiveTextureARB( GL_TEXTURE0_ARB + i );
            //glDisable( GL_TEXTURE_2D );
        }
        __hwdxCurrentContext->tmActive = -1;
    }
    IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                    D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                    D3DRS_CULLMODE,
                                    D3DCULL_NONE);

    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        if( __hwdxCurrentContext->sel.flags & HW_SELECT_VERTICES ) {
            pt = 1;
        }
    }

    if( pt ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_FILLMODE, D3DFILL_POINT);
    }
    else {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    }
}

static void SetSolidState( void )
{
    int
        i, cull, wf, pt;

    if( __hwdxCurrentContext->wireframeState == 0 )     return;
    __hwdxCurrentContext->wireframeState = 0;

    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        cull = __hwdxCurrentContext->sel.flags & HW_SELECT_CULL_FACE;
        wf = __hwdxCurrentContext->sel.flags & HW_SELECT_EDGES;
        pt = __hwdxCurrentContext->sel.flags & HW_SELECT_VERTICES;
    }
    else {
        cull = __hwdxCurrentContext->renderMode & HW_RENDER_CULL_FACE;
        wf = __hwdxCurrentContext->renderMode & HW_RENDER_EDGE_MODE;
        pt = 0;
    }

    if( __hwdxCurrentContext->lightOn ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_LIGHTING, TRUE);
    }
    if( __hwdxCurrentContext->shadeFlat != 0 ) {
        __hwdxCurrentContext->shadeFlat = 0;
        IDirect3DDevice9_SetRenderState(
                                __hwdxCurrentContext->d3dDevice,
                                D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
    }

    if( pt || wf ) {
        /* Disable texturing */
        if( __hwdxCurrentContext->tmActive != -1 ) {
            for( i = 0 ; i < HW_MAX_TEXTURES; i++ ) {
                //glActiveTextureARB( GL_TEXTURE0_ARB + i );
                //glDisable( GL_TEXTURE_2D );
            }
            __hwdxCurrentContext->tmActive = -1;
        }
    }
    else {
        /* Re-enable texturing, if applicable */
        if( __hwdxCurrentContext->tmActive == -1 ) {
            for( i = 0 ; i < HW_MAX_TEXTURES; i++ ) {
                if( __hwdxCurrentContext->tmId[i] != -1 ) {
                    __hwdxCurrentContext->tmActive = i;
                    //glActiveTextureARB( GL_TEXTURE0_ARB + i );
                    //glEnable( GL_TEXTURE_2D );
                }
            }
        }
    }

    if( pt ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_FILLMODE, D3DFILL_POINT );
    }
    else if( wf ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_FILLMODE, D3DFILL_WIREFRAME );
    }
    else {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_FILLMODE, D3DFILL_SOLID );
    }

    if( cull ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_CULLMODE,
                                        D3DCULL_CCW); /* TBD: Or CW */
    }
    else {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_CULLMODE,
                                        D3DCULL_NONE);
    }
}

void __hwDxBeginSelect( void )
{
#if 0
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)!=HW_RENDER_SELECT) {
        return;
    }
    glSelectBuffer( SEL_SIZE, __hwdxCurrentContext->selMem );
    glInitNames();
    glPushName( 1 );
    (void)glRenderMode( GL_SELECT );
#endif
}

void __hwDxEndSelect( void )
{
#if 0
    int
        i, hits, n;
    GLuint
        *ptr,
        z1, z2;
    hwFloat
        depth;
    hwSelectionType
        *sel;

    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)!=HW_RENDER_SELECT) {
        return;
    }
    hits = glRenderMode( GL_RENDER );
    ptr = __hwdxCurrentContext->selMem;
    sel = &__hwdxCurrentContext->sel;
    for( i = 0; i < hits; i++ ) {
        n = *ptr++;     /* # of names on stack - only 1! */
        z1 = *ptr++;
        z2 = *ptr++;
        depth = (hwFloat)z2;
        if( depth < sel->dist ) {
            sel->dist = depth;
            sel->obj = __hwdxCurrentContext->currObj;
            sel->picked = 1;
        }
        ptr += n;
    }
#endif
}

static void addVertex(hwFloat **pvp, hwInt32 Flags, hwInt32 sf,
                      hwInt32 FlipNml, hwInt32 WPV, hwFloat **pdst)
{
    hwInt32 color;
    hwFloat *vp = *pvp;
    hwFloat *dst = *pdst;

    /* Position */
    *dst++ = *vp++; *dst++ = *vp++; *dst++ = *vp++;

    /* Stash off color (D3D has color after normal) */
    if( Flags & HW_DATA_RGB ) {
        if( Flags & HW_DATA_ALPHA ) {
            color = DXcolor(vp[0], vp[1], vp[2], vp[3]);
            vp += 4;
        }
        else {
            color = DXcolor(vp[0], vp[1], vp[2], 1.0);
            vp += 3;
        }
    }

    /* Stuff extra data in */
    if (!(sf & HW_SURF_WIREFRAME)) {
        /* First, the normal */
        if( Flags & HW_DATA_NORMALS ) {
            if( FlipNml ) {
                *dst++ = -*vp++; *dst++ = -*vp++; *dst++ = -*vp++;
            }
            else {
                *dst++ = *vp++; *dst++ = *vp++; *dst++ = *vp++;
            }
        }
        /* Now, stuff in the DWORD color */
        if( Flags & HW_DATA_RGB ) {
            *(DWORD *)dst = color;
            dst++;
        }
        /* Finally, tex coords */
        if( Flags & HW_DATA_TEXCOORD ) {
            int t, st = HW_DATA_ST, rq = HW_DATA_RQ;
            for( t = 0; t < HW_MAX_TEXTURES; t++ ) {
                if( Flags & st ) {
                    *dst++ = *vp++;
                    *dst++ = *vp++;
                    if( Flags & rq ) {
                        *dst++ = *vp++;
                        *dst++ = *vp++;
                    }
                }
                st <<= 2; rq <<= 2;
            }
        }
    }
    else if( Flags & HW_DATA_RGB ) {
        /* Don't forget to add the color */
        *(DWORD *)dst = color;
        dst++;
    }

    /* Increment caller's pointers */
    *pvp += WPV;
    *pdst = dst;
}

static void addVertexNml(hwFloat **pvp, hwFloat *Normal,
                        hwInt32 Flags, hwInt32 sf, hwInt32 WPV,
                        hwFloat **pdst)
{
    hwInt32 color;
    hwFloat *vp = *pvp;
    hwFloat *dst = *pdst;

    /* Position */
    *dst++ = *vp++; *dst++ = *vp++; *dst++ = *vp++;

    /* Stash off color (D3D has color after normal) */
    if( Flags & HW_DATA_RGB ) {
        if( Flags & HW_DATA_ALPHA ) {
            color = DXcolor(vp[0], vp[1], vp[2], vp[3]);
            vp += 4;
        }
        else {
            color = DXcolor(vp[0], vp[1], vp[2], 1.0);
            vp += 3;
        }
    }

    /* Stuff extra data in */
    if (!(sf & HW_SURF_WIREFRAME)) {
        /* First, the normal */
        *dst++ = Normal[0];
        *dst++ = Normal[1];
        *dst++ = Normal[2];

        /* Now, stuff in the DWORD color */
        if( Flags & HW_DATA_RGB ) {
            *(DWORD *)dst = color;
            dst++;
        }
        /* Finally, tex coords */
        if( Flags & HW_DATA_TEXCOORD ) {
            int t, st = HW_DATA_ST, rq = HW_DATA_RQ;
            for( t = 0; t < HW_MAX_TEXTURES; t++ ) {
                if( Flags & st ) {
                    *dst++ = *vp++;
                    *dst++ = *vp++;
                    if( Flags & rq ) {
                        *dst++ = *vp++;
                        *dst++ = *vp++;
                    }
                }
                st <<= 2; rq <<= 2;
            }
        }
    }
    else if( Flags & HW_DATA_RGB ) {
        /* Don't forget to add the color */
        *(DWORD *)dst = color;
        dst++;
    }

    /* Increment caller's pointers */
    *pvp += WPV;
    *pdst = dst;
}

static void WireMesh
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m
)
{
    hwInt32
        i, j, r, c,
        FVF, color,
        vn, v, dstSize, vsize,
        ii, jj, nn, mm;
    hwFloat
        *src;
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    if( !__hwdxCurrentContext ) return;

    vn = hwCalcWPV( dataFlags );
    SetWireframeState();

    if( m > n ) {
        dstSize = m;
    }
    else {
        dstSize = n;
    }
    /* TBD HACK!  We really need to leave lighting enabled move
     * the specular / ambient colors to zero when wireframe. Uggh.
     */
    color = DXcolor(__hwdxCurrentContext->color[0],
                    __hwdxCurrentContext->color[1],
                    __hwdxCurrentContext->color[2],
                    1.0);
    if( 1/*dataFlags & HW_DATA_RGB*/ ) {
        dstSize *= 4;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
        vsize = 4*sizeof(DWORD);
    }
    else {
        dstSize *= 3;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ;
        vsize = 3*sizeof(DWORD);
    }

    if( dstSize > dstAlloc ) {
        dst = realloc(dst, dstSize*sizeof(hwFloat));
        dstAlloc = dstSize;
    }
    if( !dst ) return;

    /* First, advance through each row, forming 1 polyline per row */
    __hwDxBeginSelect();
    IDirect3DDevice9_SetFVF( __hwdxCurrentContext->d3dDevice, FVF );

    for( r = 0; r < n; r++ ) {
        src = data + (r * m * vn); 

        if( dataFlags & HW_DATA_RGB ) {
            for( c = 0; c < m; c++ ) {
                dst[4*c  ] = src[0];
                dst[4*c+1] = src[1];
                dst[4*c+2] = src[2];
                ((DWORD *)dst)[4*c+3] = DXcolor(src[3], src[4],src[5],1.0);
                src += vn;
            }
        }
        else {
            for( c = 0; c < m; c++ ) {
                dst[4*c  ] = src[0];
                dst[4*c+1] = src[1];
                dst[4*c+2] = src[2];
                ((DWORD *)dst)[4*c+3] = color;
                src += vn;
            }
        }
                            
        IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        D3DPT_LINESTRIP, m-1, dst, vsize);
    }

    /* Next, advance through each Column, forming 1 polyline per column */
    for( c = 0; c < m; c++ ) {
        src = data + (c * vn); 

        if( dataFlags & HW_DATA_RGB ) {
            for( r = 0; r < n; r++ ) {
                dst[4*r  ] = src[0];
                dst[4*r+1] = src[1];
                dst[4*r+2] = src[2];
                ((DWORD *)dst)[4*c+3] = DXcolor(src[3], src[4],src[5],1.0);
                src += m*vn;
            }
        }
        else {
            for( r = 0; r < n; r++ ) {
                dst[4*r  ] = src[0];
                dst[4*r+1] = src[1];
                dst[4*r+2] = src[2];
                ((DWORD *)dst)[4*r+3] = color;
                src += m*vn;
            }
        }
        IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        D3DPT_LINESTRIP, n-1, dst, vsize);
    }
    __hwDxEndSelect();
}

static hwInt32 d3dFVF(hwInt32 flags, hwInt32 sf)
{
    hwInt32 result;
    int t, st = HW_DATA_ST, rq = HW_DATA_RQ;

    result = D3DFVF_XYZ;
    if (flags & HW_DATA_RGB) {
        result |= D3DFVF_DIFFUSE;
    }
    if (!(sf & HW_SURF_WIREFRAME)) {
        if (flags & HW_DATA_NORMALS) {
            result |= D3DFVF_NORMAL;
        }
        for( t = 0; t < HW_MAX_TEXTURES; t++ ) {
            if (flags & st) {
                if (flags & rq) {
                    result |= D3DFVF_TEXCOORDSIZE4(t);
                }
                else {
                    result |= D3DFVF_TEXCOORDSIZE2(t);
                }
            }
            st <<= 2; rq <<= 2;
        }
    }
    return result;
}

static hwInt32 d3dWPV(hwInt32 flags, hwInt32 sf)
{
    hwInt32 result;
    int t, st = HW_DATA_ST, rq = HW_DATA_RQ;

    result = 3; // Always have XYZ
    if (flags & HW_DATA_RGB) {
        // D3D colors are 1 DWORD
        result++;
    }
    if (!(sf & HW_SURF_WIREFRAME)) {
        if (flags & HW_DATA_NORMALS) {
            result += 3;
        }
        for( t = 0; t < HW_MAX_TEXTURES; t++ ) {
            if (flags & st) {
                if (flags & rq) {
                    result += 4;
                }
                else {
                    result += 2;
                }
            }
            st <<= 2; rq <<= 2;
        }
    }
    return result;
}

static void DrawTriStrip
(
    hwInt32 Flags,
    hwFloat *StripEven, hwFloat *StripOdd,
    int NP, int Rev, int FlipNml
)
{
    hwInt32
        i, n, sf, WPV, Skip,
        dstSize,
        d3dSize;
    hwFloat
        *vptr, *v0, *v1, *v2,
        *tptr,
        Normal[3];
    hwInt32
        doFlat = 0,
        prim;
    D3DPRIMITIVETYPE
        d3dPrim;
    DWORD
        FVF;
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    /* Calculate words-per-vertex */
    WPV = hwCalcWPV( Flags );

    sf = __hwdxCurrentContext->primFlags;
    FVF = d3dFVF(Flags, sf);

    /* Establish vertex swap stuff (for efficent mesh traversal) */
    if( StripOdd ) {
        Skip = WPV;
    }
    else {
        StripOdd = StripEven + WPV;
        Skip = 2*WPV;
    }

    if( !(Flags & HW_DATA_NORMALS) && !(sf & HW_SURF_WIREFRAME) ) {
        if( Flags & HW_DATA_RGB ) {
            d3dSize = d3dWPV( Flags | HW_DATA_NORMALS, sf );
            FVF = d3dFVF( Flags | HW_DATA_NORMALS, sf );

            dstSize = d3dSize * (NP-2) * 3 * sizeof(hwFloat);
            if( dstSize > dstAlloc ) {
                dst = realloc(dst, dstSize);
                dstAlloc = dstSize;
            }
            if (!dst) {
                return;
            }
            tptr = dst;

            /* Bust 'em up into discrete triangles */
            prim = DX_DL_TRIANGLES;
            d3dPrim = D3DPT_TRIANGLELIST;

            v0 = StripEven;     StripEven += Skip;
            v1 = StripOdd;      StripOdd += Skip;
            for( i = 2; i < NP; i++ ) {
                if( i & 1 )     { v2 = StripOdd;  StripOdd  += Skip; }
                else            { v2 = StripEven; StripEven += Skip; }

                /* Calculate the normal for this triangle */
                if( i & 1 ) {
                    CalcNormal( v1, v0, v2, Normal );
                }
                else {
                    CalcNormal( v0, v1, v2, Normal );
                }
                if( Rev ) {
                    Normal[0] = -Normal[0];
                    Normal[1] = -Normal[1];
                    Normal[2] = -Normal[2];
                }

                if( Rev ^ (i & 1) ) {
                    addVertexNml( &v1, Normal, Flags, sf, WPV, &tptr );
                    addVertexNml( &v0, Normal, Flags, sf, WPV, &tptr );
                    addVertexNml( &v2, Normal, Flags, sf, WPV, &tptr );
                }
                else {
                    addVertexNml( &v0, Normal, Flags, sf, WPV, &tptr );
                    addVertexNml( &v1, Normal, Flags, sf, WPV, &tptr );
                    addVertexNml( &v2, Normal, Flags, sf, WPV, &tptr );
                }
                v0 = v1; v1 = v2;
            }
        }
        else {
            /* Do flat shading */
            doFlat = 1;

            prim = DX_DL_TRISTRIP;
            d3dPrim = D3DPT_TRIANGLESTRIP;

            d3dSize = d3dWPV( Flags | HW_DATA_NORMALS, sf );
            FVF = d3dFVF( Flags | HW_DATA_NORMALS, sf );
            dstSize = d3dSize*(NP+1)*sizeof(hwFloat);
            if( dstSize > dstAlloc ) {
                dst = realloc(dst, dstSize);
                dstAlloc = dstSize;
            }
            if (!dst) {
                return;
            }
            tptr = dst;

            Normal[0] = 1.0; Normal[1] = 0.0; Normal[2] = 0.0;
            if( Rev ) {
                vptr = StripEven;
                addVertexNml( &vptr, Normal, Flags, sf, WPV, &tptr );
            }

            /* Send down 1st two vertices; flat shade mode has normal
             * on 3rd vert
             */
            v0 = StripEven; v1 = StripOdd;
            for( i = 0; i < 2; i++ ) {
                if( i & 1 )     { vptr = StripOdd;  StripOdd += Skip; }
                else            { vptr = StripEven; StripEven += Skip; }
                addVertexNml( &vptr, Normal, Flags, sf, WPV, &tptr );
            }

            for( i = 2; i < NP; i++ ) {
                /* Get pointer to current vertex */
                if( i & 1 )     { vptr = StripOdd;  StripOdd += Skip; }
                else            { vptr = StripEven; StripEven += Skip; }

                /* Calculate the normal for this triangle */
                if( i & 1 ) {
                    CalcNormal( v1, v0, vptr, Normal );
                }
                else {
                    CalcNormal( v0, v1, vptr, Normal );
                }
                if( Rev ) {
                    Normal[0] = -Normal[0];
                    Normal[1] = -Normal[1];
                    Normal[2] = -Normal[2];
                }

                addVertexNml( &vptr, Normal, Flags, sf, WPV, &tptr );

                /* Advance vertex pointers pointers */
                v0 = v1; v1 = vptr;
            }
        }
    }
    else {
        prim = DX_DL_TRISTRIP;
        d3dPrim = D3DPT_TRIANGLESTRIP;

        d3dSize = d3dWPV( Flags, sf );
        dstSize = d3dSize * (NP+Rev) * sizeof(hwFloat);
        if( dstSize > dstAlloc ) {
            dst = realloc(dst, dstSize);
            dstAlloc = dstSize;
        }
        if (!dst) {
            return;
        }
        tptr = dst;

        if( Rev ) {
            /* Send down 1 extra vertex to flip order of strip */
            vptr = StripEven;
            addVertex( &vptr, Flags, sf, FlipNml, WPV, &tptr );
        }

        /* Send down the vertices */
        for( i = 0; i < NP; i++ ) {
            /* Get pointer to current vertex */
            if( i & 1 ) { vptr = StripOdd;  StripOdd  += Skip; }
            else        { vptr = StripEven; StripEven += Skip; }

            addVertex( &vptr, Flags, sf, FlipNml, WPV, &tptr );
        }
    }

    if( doFlat != __hwdxCurrentContext->shadeFlat ) {
        if( doFlat ) {
            IDirect3DDevice9_SetRenderState(
                                __hwdxCurrentContext->d3dDevice,
                                D3DRS_SHADEMODE, D3DSHADE_FLAT );
        }
        else {
            IDirect3DDevice9_SetRenderState(
                                __hwdxCurrentContext->d3dDevice,
                                D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
        }
        __hwdxCurrentContext->shadeFlat = 0;
    }

    n = (tptr - dst) / d3dSize;

    if( FAILED( IDirect3DDevice9_SetFVF(
                        __hwdxCurrentContext->d3dDevice, FVF ) ) ) {
        printf("FVF fail\n");
    }
                            
    if( FAILED( IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        d3dPrim, n - 2, dst, d3dSize*sizeof(float))) ) {
        printf("FAIL!\n");
    }
}

void __hwDxDrawMesh
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n, hwInt32 m
)
{
    hwInt32
        i, j, vn, nb, sf;
    hwFloat
        *Curr, *Next, *Point;

    sf = __hwdxCurrentContext->primFlags;
    if( sf & HW_SURF_WIREFRAME ) {
        WireMesh( disp, data, dataFlags, n, m );
        return;
    }
    vn = hwCalcWPV( dataFlags );
    SetSolidState();

#if 0
    if( dataFlags & HW_DATA_RGB ) {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_AMBIENTMATERIALSOURCE,
                                        D3DMCS_COLOR1);
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_DIFFUSEMATERIALSOURCE,
                                        D3DMCS_COLOR1);
    }
    else {
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_AMBIENTMATERIALSOURCE,
                                        D3DMCS_MATERIAL );
        IDirect3DDevice9_SetRenderState(__hwdxCurrentContext->d3dDevice,
                                        D3DRS_DIFFUSEMATERIALSOURCE,
                                        D3DMCS_MATERIAL);
    }
#endif

    /* Get pointers to quad mesh data */
    Curr = data;
    Next = Curr + m * vn;

    /* Render the n-1 strips in a quad mesh... */
    __hwDxBeginSelect();
    for( i = 1; i < n; i++ ) {
        /* Now draw it */
        if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
            DrawTriStrip( dataFlags, Next, Curr, 2*m, 0,
                        (sf & (HW_SURF_FLIP_NORMALS|HW_SURF_TWOSIDED)) ? 1:0 );
        }
        if( !(sf & HW_SURF_BACKFACE) ) {
            DrawTriStrip( dataFlags, Curr, Next, 2*m, 0,
                        (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
        }
        Curr = Next;
        Next += m*vn;
    }
    __hwDxEndSelect();
}

static void WirePolygon
(
    hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts
)
{
    hwInt32
        dstSize,
        color,
        FVF, vsize,
        i, vn;
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    SetWireframeState();
    vn = hwCalcWPV( dataFlags );

    /* TBD HACK!  We really need to leave lighting enabled move
     * the specular / ambient colors to zero when wireframe. Uggh.
     */
    color = DXcolor( __hwdxCurrentContext->color[0],
                     __hwdxCurrentContext->color[1],
                     __hwdxCurrentContext->color[2],
                     1.0 );
    dstSize = numVerts+1;
    if( 1/*dataFlags & HW_DATA_RGB*/ ) {
        dstSize *= 4;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
        vsize = 4*sizeof(DWORD);
    }
    else {
        dstSize *= 3;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ;
        vsize = 3*sizeof(DWORD);
    }

    if( dstSize > dstAlloc ) {
        dst = realloc(dst, dstSize*sizeof(hwFloat));
        dstAlloc = dstSize;
    }
    if( !dst ) return;

    __hwDxBeginSelect();
    for( i = 0; i < numVerts; i++ ) {
        if( dataFlags & HW_DATA_RGB ) {
            dst[4*i  ] = polyData[i*vn+0];
            dst[4*i+1] = polyData[i*vn+1];
            dst[4*i+2] = polyData[i*vn+2];
            ((DWORD *)dst)[4*i+3] = DXcolor(polyData[i*vn+3],
                                            polyData[i*vn+4],
                                            polyData[i*vn+5],
                                            1.0);
        }
        else {
            dst[4*i  ] = polyData[i*vn+0];
            dst[4*i+1] = polyData[i*vn+1];
            dst[4*i+2] = polyData[i*vn+2];
            ((DWORD *)dst)[4*i+3] = color;
        }
    }
    /* Gotta close the loop */
    if( dataFlags & HW_DATA_RGB ) {
        dst[4*i  ] = polyData[0];
        dst[4*i+1] = polyData[1];
        dst[4*i+2] = polyData[2];
        ((DWORD *)dst)[4*i+3] = DXcolor(polyData[3],
                                        polyData[4],
                                        polyData[5],
                                        1.0);
    }
    else {
        dst[4*i  ] = polyData[0];
        dst[4*i+1] = polyData[1];
        dst[4*i+2] = polyData[2];
        ((DWORD *)dst)[4*i+3] = color;
    }
    IDirect3DDevice9_SetFVF( __hwdxCurrentContext->d3dDevice, FVF );
    IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        D3DPT_LINESTRIP, numVerts, dst, vsize);
    __hwDxEndSelect();
}

static void DrawOnePoly
(
    hwFloat *Poly, hwInt32 dataFlags, hwInt32 NP, hwInt32 Rev, hwInt32 flipNml
)
{
    hwInt32
        dstSize, FVF, dxSize, needNml,
        i, WPV, sf;
    hwFloat
        *vptr, *tptr,
        Normal[3];
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    sf = __hwdxCurrentContext->primFlags;

    /* Calculate sizes */
    WPV = hwCalcWPV( dataFlags );

    /* Calculate the normal... */
    if( !(dataFlags & HW_DATA_NORMALS) && !(sf & HW_SURF_WIREFRAME) ) {
        CalcNormal( Poly, Poly+WPV, Poly+2*WPV, Normal );
        if( flipNml ) {
            Normal[0] = -Normal[0];
            Normal[1] = -Normal[1];
            Normal[2] = -Normal[2];
        }
        FVF = d3dFVF(dataFlags|HW_DATA_NORMALS, sf);
        dxSize = d3dWPV(dataFlags|HW_DATA_NORMALS, sf) * sizeof(hwFloat);
        needNml = 1;
    }
    else {
        FVF = d3dFVF(dataFlags, sf);
        dxSize = d3dWPV(dataFlags, sf) * sizeof(hwFloat);
        needNml = 0;
    }

    /* Make sure we have enough memory */
    dstSize = dxSize * NP;
    if( dstSize > dstAlloc ) {
        dst = realloc( dst, dstSize );
        dstAlloc = dstSize;
    }
    if( !dst ) return;

    if( Rev ) {
        Poly += (NP - 1)*WPV;   /* Point to end of polygon */
        WPV = -WPV;             /* Step backward */
    }

    vptr = Poly; tptr = dst;

    if( needNml ) {
        for( i = 0; i < NP; i++ ) {
            addVertexNml( &vptr, Normal, dataFlags, sf, WPV, &tptr );
        }
    }
    else {
        for( i = 0; i < NP; i++ ) {
            addVertex( &vptr, dataFlags, sf, flipNml, WPV, &tptr );
        }
    }

    IDirect3DDevice9_SetFVF( __hwdxCurrentContext->d3dDevice, FVF );
    IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        D3DPT_TRIANGLEFAN, NP-2, dst, dxSize);
}

void __hwDxDrawPolygon
(
    hwDisplay disp,
    hwFloat *polyData, hwInt32 dataFlags, hwInt32 numVerts
)
{
    hwInt32
        sf;

    if( !__hwdxCurrentContext ) return;
    sf = __hwdxCurrentContext->primFlags;

    if( sf & HW_SURF_WIREFRAME ) {
        WirePolygon( polyData, dataFlags, numVerts );
        return;
    }

    SetSolidState();
    __hwDxBeginSelect();

    if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
        DrawOnePoly( polyData, dataFlags, numVerts, 1,
                    (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0 );
    }
    if( !(sf & HW_SURF_BACKFACE) ) {
        DrawOnePoly( polyData, dataFlags, numVerts, 0,
                    (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
    }

    __hwDxEndSelect();
}

void __hwDxDrawPolyline( hwDisplay disp, hwFloat *data, hwInt32 fl, hwInt32 n )
{
    hwInt32
        color,
        i, count, FVF, dstSize, dstVsize;
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    if( !__hwdxCurrentContext ) return;

    dstVsize = 4*sizeof(hwFloat);
    dstSize = n * dstVsize;
    FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;

    if( dstSize > dstAlloc ) {
        dst = realloc( dst, dstSize );
        dstAlloc = dstSize;
    }
    if( !dst ) return;

    color = DXcolor(__hwdxCurrentContext->color[0],
                    __hwdxCurrentContext->color[1],
                    __hwdxCurrentContext->color[2],
                    1.0);

    SetWireframeState();

    __hwDxBeginSelect();

    count = 0;

    IDirect3DDevice9_SetFVF( __hwdxCurrentContext->d3dDevice, FVF );

    if( fl & HW_DATA_MD_FLAGS ) {
        for( i = 0; i < n; i++ ) {
            if( data[3] == 0.0 ) {
                if( count > 1 ) {
                    IDirect3DDevice9_DrawPrimitiveUP(
                            __hwdxCurrentContext->d3dDevice,
                            D3DPT_LINESTRIP, count-1, dst, dstVsize);
                }
                count = 0;
            }
            dst[4*count  ] = data[0];
            dst[4*count+1] = data[1];
            dst[4*count+2] = data[2];
            ((DWORD *)dst)[4*count+3] = color;
            data += 4;
            count++;
        }
    }
    else {
        for( i = 0; i < n; i++ ) {
            dst[4*count  ] = data[0];
            dst[4*count+1] = data[1];
            dst[4*count+2] = data[2];
            ((DWORD *)dst)[4*count+3] = color;
            data += 3;
            count++;
        }
    }
    if( count > 1 ) {
        IDirect3DDevice9_DrawPrimitiveUP(
                            __hwdxCurrentContext->d3dDevice,
                            D3DPT_LINESTRIP, count-1, dst, dstVsize);
    }
    __hwDxEndSelect();
}

void __hwDxDrawQuads
(
    hwDisplay disp, hwFloat *data, hwInt32 dataFlags, hwInt32 n
)
{
    hwInt32
        i, vn;
    hwInt32
        sf;

    if( !__hwdxCurrentContext ) return;
    sf = __hwdxCurrentContext->primFlags;

    vn = hwCalcWPV( dataFlags );

    if( sf & HW_SURF_WIREFRAME ) {
        SetWireframeState();
        sf &= ~HW_SURF_TWOSIDED;
    }
    else {
        SetSolidState();
    }

    __hwDxBeginSelect();

    /* Egregiously inefficient */
    for( i = 0; i < n; i++ ) {
        if( sf & (HW_SURF_BACKFACE|HW_SURF_TWOSIDED) ) {
            DrawOnePoly( data, dataFlags, 4, 1,
                        (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0 );
        }
        if( !(sf & HW_SURF_BACKFACE) ) {
            DrawOnePoly( data, dataFlags, 4, 0,
                        (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
        }
        data += 4*vn;
    }
    __hwDxEndSelect();
}

void __hwDxIndexedTris
(
    hwDisplay disp,
    hwFloat *verts, hwInt32 numVerts, hwInt32 dataFlags,
    hwInt32 *indexList, hwInt32 numTris
)
{
    hwInt32
        i, vn, sf,
        *TPtr;
    hwFloat
        triangle[11*3]; /* XYZRGBNNNST */

    if( !__hwdxCurrentContext ) return;
    sf = __hwdxCurrentContext->primFlags;

    vn = hwCalcWPV( dataFlags );

    if( sf & HW_SURF_WIREFRAME ) {
        SetWireframeState();
        sf &= ~HW_SURF_TWOSIDED;
    }
    else {
        SetSolidState();
    }

    __hwDxBeginSelect();

    /* Egregiously inefficient */
    if( indexList ) {
        TPtr = indexList;
        for( i = 0; i < numTris; i++, TPtr += 3 ) {
            (void)memcpy( triangle     , verts + TPtr[0]*vn, vn*sizeof(float) );
            (void)memcpy( triangle+  vn, verts + TPtr[1]*vn, vn*sizeof(float) );
            (void)memcpy( triangle+2*vn, verts + TPtr[2]*vn, vn*sizeof(float) );
            if( sf & (HW_SURF_TWOSIDED | HW_SURF_BACKFACE) ) {
                DrawOnePoly( triangle, dataFlags, 3, 1,
                        (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0 );
            }
            if( !(sf & HW_SURF_BACKFACE) ) {
                DrawOnePoly( triangle, dataFlags, 3, 0,
                        (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
            }
        }
    }
    else {
        for( i = 0; i < numTris; i++ ) {
            if( sf & (HW_SURF_TWOSIDED | HW_SURF_BACKFACE) ) {
                DrawOnePoly( verts, dataFlags, 3, 1,
                        (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0 );
            }
            if( !(sf & HW_SURF_BACKFACE) ) {
                DrawOnePoly( verts, dataFlags, 3, 0,
                        (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
            }
            verts += 3*vn;
        }
    }

    __hwDxEndSelect();
}

void __hwDxDrawStrip
(
    hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n
)
{
    hwInt32
        sf;

    if( !__hwdxCurrentContext ) return;
    sf = __hwdxCurrentContext->primFlags;

    if( sf & HW_SURF_WIREFRAME ) {
        SetWireframeState();
        sf &= ~HW_SURF_TWOSIDED;
    }
    else {
        SetSolidState();
    }

    __hwDxBeginSelect();
    if( sf & (HW_SURF_TWOSIDED | HW_SURF_BACKFACE) ) {
        DrawTriStrip( flags, data, (hwFloat *)0, n, 1,
                        (sf & (HW_SURF_TWOSIDED|HW_SURF_FLIP_NORMALS)) ? 1:0 );
    }
    if( !(sf & HW_SURF_BACKFACE) ) {
        DrawTriStrip( flags, data, (hwFloat *)0, n, 0,
                        (sf & HW_SURF_FLIP_NORMALS) ? 1 : 0 );
    }
    __hwDxEndSelect();
}


void __hwDxDrawMarkers
(
    hwDisplay disp, hwFloat *data, hwInt32 flags, hwInt32 n
)
{
    hwInt32
        dstSize,
        color,
        FVF, vsize,
        i, vn;
    static hwFloat
        *dst;
    static hwInt32
        dstAlloc;

    SetWireframeState();
    vn = hwCalcWPV( flags );

    /* TBD HACK!  We really need to leave lighting enabled move
     * the specular / ambient colors to zero when wireframe. Uggh.
     */
    color = DXcolor( __hwdxCurrentContext->color[0],
                     __hwdxCurrentContext->color[1],
                     __hwdxCurrentContext->color[2],
                     1.0 );
    dstSize = n;
    if( 1/*flags & HW_DATA_RGB*/ ) {
        dstSize *= 4;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
        vsize = 4*sizeof(DWORD);
    }
    else {
        dstSize *= 3;   /* XYZ [RGB] */
        FVF = D3DFVF_XYZ;
        vsize = 3*sizeof(DWORD);
    }

    if( dstSize > dstAlloc ) {
        dst = realloc(dst, dstSize*sizeof(hwFloat));
        dstAlloc = dstSize;
    }
    if( !dst ) return;

    __hwDxBeginSelect();
    for( i = 0; i < n; i++ ) {
        if( flags & HW_DATA_RGB ) {
            dst[4*i  ] = data[i*vn+0];
            dst[4*i+1] = data[i*vn+1];
            dst[4*i+2] = data[i*vn+2];
            ((DWORD *)dst)[4*i+3] = DXcolor(data[i*vn+3],
                                            data[i*vn+4],
                                            data[i*vn+5],
                                            1.0);
        }
        else {
            dst[4*i  ] = data[i*vn+0];
            dst[4*i+1] = data[i*vn+1];
            dst[4*i+2] = data[i*vn+2];
            ((DWORD *)dst)[4*i+3] = color;
        }
    }
    IDirect3DDevice9_SetFVF( __hwdxCurrentContext->d3dDevice, FVF );
    IDirect3DDevice9_DrawPrimitiveUP(
                        __hwdxCurrentContext->d3dDevice,
                        D3DPT_POINTLIST, n, dst, vsize);
    __hwDxEndSelect();
}

void __hwDxSetDrawBuffer( hwDisplay disp, hwInt32 buff )
{
#if 0
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

void __hwDxAccumOp( hwDisplay disp, hwInt32 op, hwFloat arg )
{
#if 0
    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }
    switch( op ) {
    case HW_ACCUM_ACCUM :
        glAccum( GL_ACCUM, arg );
        break;
    case HW_ACCUM_RETURN :
        glAccum( GL_RETURN, arg );
        break;
    case HW_ACCUM_CLEAR :
        glClear( GL_ACCUM_BUFFER_BIT );
        break;
    case HW_ACCUM_LOAD :
        glAccum( GL_LOAD, arg );
        break;
    case HW_ACCUM_MULT :
        glAccum( GL_MULT, arg );
        break;
    }
#endif
}

dxGuiElement __hwDxAllocGuiElem( void )
{
    dxGuiElement result;
    int i;

    if (!__hwdxCurrentContext->guiFree) {
        /* Manage the free list */
        __hwdxCurrentContext->guiFree
                = malloc(GUI_ELEMENT_FREE_SIZE
                         * sizeof(struct __dxGuiElement));
        if (!__hwdxCurrentContext->guiFree) {
            return NULL;
        }
        for (i = 1; i < GUI_ELEMENT_FREE_SIZE; i++) {
            __hwdxCurrentContext->guiFree[i-1].next
                        = __hwdxCurrentContext->guiFree + i;
        }
        __hwdxCurrentContext->guiFree[i-1].next = NULL;
    }

    /* Pull an entry from the free list */
    result = __hwdxCurrentContext->guiFree;
    __hwdxCurrentContext->guiFree = result->next;
    result->next = NULL;

    /* Go ahead and add it to the rendering list */
    if (__hwdxCurrentContext->guiTail) {
        __hwdxCurrentContext->guiTail->next = result;
    } else {
        __hwdxCurrentContext->guiList = result;
    }
    __hwdxCurrentContext->guiTail = result;

    return result;
}

void __hwDxGuiText
(
    hwDisplay disp,
    TexFont *txf,
    hwInt32 color, hwInt32 halign, hwInt32 valign, hwInt32 height,
    hwInt32 x, hwInt32 y, char *text
)
{
    dxGuiElement elem;
    int width, max_ascent, max_descent;
    int i, n, len;
    hwFloat fx, fy, fwidth, scale;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    /* Calculate position of text */
    len = strlen(text);
    txfGetStringMetrics(txf, text, len, &width, &max_ascent, &max_descent);

    scale = height / (float)(max_ascent + max_descent);
    fx = x;
    fy = __hwdxCurrentContext->winH - y;
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
        fy -= max_ascent * scale;
        break;
    case HW_TEXT_ALIGN_CENTER :
        fy -= max_ascent * scale;
        fy += (max_ascent + max_descent) * scale * 0.5;
        break;
    case HW_TEXT_ALIGN_BOTTOM :
    default :
        break;
    }

    for (i = 0; i < len; i += MAX_GUI_STRING_SIZE) {
        elem = __hwDxAllocGuiElem();
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
        (void)strncpy(elem->u.guiText.string, text+i, n);
        elem->u.guiText.string[n] = 0;

        fx += width * scale;
    }
}


void __hwDxGuiRaster
(
    hwDisplay disp,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 color,
    hwInt32 texId 
)
{
    dxGuiElement elem;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    elem = __hwDxAllocGuiElem();
    if (!elem) return;
    elem->typeFlags = GUI_TEXTURE;
    elem->color = color;
    elem->u.guiRect.pointA[0] = x;
    elem->u.guiRect.pointA[1] = y;
    elem->u.guiRect.pointB[0] = x+w;
    elem->u.guiRect.pointB[1] = y+h;
    elem->u.guiRect.texture = texId;
}

void __hwDxGuiRectangle
(
    hwDisplay disp,
    hwInt32 color,
    hwInt32 flags,
    hwInt32 x, hwInt32 y,
    hwInt32 w, hwInt32 h,
    hwInt32 radius
)
{
    dxGuiElement elem;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    elem = __hwDxAllocGuiElem();
    if (!elem) return;
    elem->typeFlags = GUI_RECT | flags;
    elem->color = color;
    elem->u.guiRect.pointA[0] = x;
    elem->u.guiRect.pointA[1] = y;
    elem->u.guiRect.pointB[0] = x+w;
    elem->u.guiRect.pointB[1] = y+h;
    elem->u.guiRect.texture = 0;
}

void __hwDxGuiPolyline
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    int i;
    dxGuiElement elem;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    for (i = 1; i < numPts; i++) {
        elem = __hwDxAllocGuiElem();
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

void __hwDxGuiLines
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    int i;
    dxGuiElement elem;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    numPts &= ~1;

    for (i = 0; i < numPts; i += 2) {
        elem = __hwDxAllocGuiElem();
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

void __hwDxGuiPolygon
(
    hwDisplay disp,
    hwInt32 flags,
    hwInt32 color,
    hwInt32 numPts,
    hwInt32 *pts
)
{
    int i;
    dxGuiElement elem;

    if( !__hwdxCurrentContext ) return;
    if((__hwdxCurrentContext->renderMode & HW_RENDER_MASK)==HW_RENDER_SELECT) {
        return;
    }

    for (i = 2; i < numPts; i++) {
        elem = __hwDxAllocGuiElem();
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

void __hwDxDrawElements( hwDisplay disp )
{
#if 0
    dxGuiElement elem;
    hwInt32 lastElemType;
    hwFloat rgb[3];
    int i, h;

    if (!__hwdxCurrentContext) return;

    disp->drawQuads = __hwDxTextQuads;
    disp->surfAttrs = __hwDxTextAttrs;

    /* Go to a defined state */
    __hwdx_SetSolidState();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (double)__hwdxCurrentContext->winW,
            0.0, (double)__hwdxCurrentContext->winH,
            0.0, 1.0);
    h = __hwdxCurrentContext->winH;

    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_FOG );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );
    glDisable( GL_CULL_FACE );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for( i = 0; i < HW_MAX_TEXTURES; i++ ) {
        glActiveTextureHW(GL_TEXTURE0_ARB + i);
        glDisable(GL_TEXTURE_2D);
    }

    lastElemType = -1;
    for (elem = __hwdxCurrentContext->guiList; elem; elem = elem->next) {
        if (elem->typeFlags != lastElemType) {
            switch( lastElemType ) {
            case GUI_LINE :
            case GUI_RECT :
                glEnd();
                break;
            }
            switch( elem->typeFlags ) {
            case GUI_TEXT :
                /* Text must have texture 0 enabled... */
                __hwdxCurrentContext->tmActive = 0;
                glActiveTextureHW(GL_TEXTURE0_ARB);
                glEnable(GL_TEXTURE_2D);
                /* And blending */
                glEnable(GL_BLEND);
                break;
            case GUI_LINE :
                /* Lines must have texture 0 disabled */
                __hwdxCurrentContext->tmActive = 0;
                glActiveTextureHW(GL_TEXTURE0_ARB);
                glDisable(GL_TEXTURE_2D);
                /* ...and no blending (TBD?) */
                glDisable(GL_BLEND);
                /* We're drawing lines... */
                glBegin(GL_LINES);
                break;
            case GUI_RECT :
                /* Rects must have texture 0 disabled... */
                __hwdxCurrentContext->tmActive = 0;
                glActiveTextureHW(GL_TEXTURE0_ARB);
                glDisable(GL_TEXTURE_2D);
                /* ...and no blending (TBD?) */
                glDisable(GL_BLEND);
                /* We're drawing quads... */
                glBegin(GL_QUADS);
                break;
            case GUI_TEXTURE :
                /* Texture rects must have texture 0 enabled... */
                __hwdxCurrentContext->tmActive = 0;
                glActiveTextureHW(GL_TEXTURE0_ARB);
                glEnable(GL_TEXTURE_2D);
                /* And blending enabled */
                glEnable(GL_BLEND);
                break;
            }
            lastElemType = elem->typeFlags;
        }

        glColor4ub( (elem->color >> 16) & 0xFF,
                    (elem->color >>  8) & 0xFF,
                    (elem->color      ) & 0xFF,
                    (elem->color >> 24) & 0xFF );

        switch (elem->typeFlags) {
        case GUI_TEXT :
            rgb[0] = ((elem->color >> 16) & 0xFF) * (1.f / 255.f);
            rgb[1] = ((elem->color >>  8) & 0xFF) * (1.f / 255.f);
            rgb[2] = ((elem->color      ) & 0xFF) * (1.f / 255.f);
            txfSetColor(rgb);
            txfSetScale((hwFloat)elem->u.guiText.size);
            txfSetPosition((hwFloat)elem->u.guiText.pos[0],
                           (hwFloat)elem->u.guiText.pos[1]);
            txfBeginFontRendering(disp,
                                  elem->u.guiText.txf, TXF_COORD_MODE_NDC);
            txfRenderString(disp, elem->u.guiText.txf,
                                elem->u.guiText.string,
                                strlen(elem->u.guiText.string));
            txfEndFontRendering(disp);
            break;
        case GUI_RECT :
            glVertex2i( elem->u.guiRect.pointA[0],
                        h - elem->u.guiRect.pointA[1] );
            glVertex2i( elem->u.guiRect.pointB[0],
                        h - elem->u.guiRect.pointA[1] );
            glVertex2i( elem->u.guiRect.pointB[0],
                        h - elem->u.guiRect.pointB[1] );
            glVertex2i( elem->u.guiRect.pointA[0],
                        h - elem->u.guiRect.pointB[1] );
            break;
        case GUI_LINE :
            glVertex2i( elem->u.guiRect.pointA[0],
                        h - elem->u.guiRect.pointA[1] );
            glVertex2i( elem->u.guiRect.pointB[0],
                        h - elem->u.guiRect.pointB[1] );
            break;
        case GUI_TEXTURE :
            glFinish();
            __hwDxCurrentTexture( disp, elem->u.guiRect.texture, 0 );
            for (i = 1; i < HW_MAX_TEXTURES; i++) {
                __hwDxCurrentTexture( disp, -1, i );
            }
            glBegin(GL_QUADS);
            glTexCoord2f( 0.f, 0.f);
            glVertex2i( elem->u.guiRect.pointA[0],
                        h - elem->u.guiRect.pointA[1] );
            glTexCoord2f( 1.f, 0.f );
            glVertex2i( elem->u.guiRect.pointB[0],
                        h - elem->u.guiRect.pointA[1] );
            glTexCoord2f( 1.f, 1.f );
            glVertex2i( elem->u.guiRect.pointB[0],
                        h - elem->u.guiRect.pointB[1] );
            glTexCoord2f( 0.f, 1.f );
            glVertex2i( elem->u.guiRect.pointA[0],
                        h - elem->u.guiRect.pointB[1] );
            glEnd();
            break;
        }
    }

    switch( lastElemType ) {
    case GUI_LINE :
    case GUI_RECT :
        glEnd();
        break;
    }

    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    disp->drawQuads = __hwDxDrawQuads;
    disp->surfAttrs = __hwDxSurfAttrs;
#endif
}

hwInt32 __hwDxOpenGuiList( hwDisplay disp )
{
    dxGuiList newList;

    if (!__hwdxCurrentContext) return 0;

    if (__hwdxCurrentContext->activeGuiList) {
        return 0;
    }

    if (__hwdxCurrentContext->freeGuiList) {
        newList = __hwdxCurrentContext->freeGuiList;
        __hwdxCurrentContext->freeGuiList = newList->nextHash;
    } else {
        newList = malloc(sizeof(struct __dxGuiList));
        if (!newList) {
            return 0;
        }
        newList->id = ++__hwdxCurrentContext->numGuiLists;
    }

    newList->prevHash = newList->nextHash = NULL;
    newList->head = newList->tail = NULL;

    __hwdxCurrentContext->activeGuiList = newList;

    return newList->id;
}

void __hwDxCloseGuiList( hwDisplay disp )
{
    int h;
    dxGuiList newList;

    if (!__hwdxCurrentContext) return;

    if (!__hwdxCurrentContext->activeGuiList) {
        return;
    }

    newList = __hwdxCurrentContext->activeGuiList;

    h = newList->id % GUI_HT_SIZE; /* World's stupidest hash function */
    newList->nextHash = __hwdxCurrentContext->guiListHash[h];
    if (__hwdxCurrentContext->guiListHash[h]) {
        __hwdxCurrentContext->guiListHash[h]->prevHash = newList;
    }
    __hwdxCurrentContext->guiListHash[h] = newList;

    __hwdxCurrentContext->activeGuiList = NULL;
}

void __hwDxCallGuiList( hwDisplay disp, hwInt32 id )
{
    dxGuiElement elem;

    if (!__hwdxCurrentContext) return;

    elem = __hwDxAllocGuiElem();
    if (!elem) return;

    elem->typeFlags = GUI_LIST;
    elem->color = 0;
    elem->u.guiList.id = id;
}

void __hwDxDestroyGuiList( hwDisplay disp, hwInt32 id )
{
    int h;
    dxGuiList list;

    if (!__hwdxCurrentContext) return;

    /* TBD: Some day it would not be hard to have nested GUI DLs */
    if (__hwdxCurrentContext->activeGuiList) {
        return;
    }

    h = id % GUI_HT_SIZE; /* World's stupidest hash function */
    for (list = __hwdxCurrentContext->guiListHash[h];
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
        __hwdxCurrentContext->guiListHash[h] = list->nextHash;
    }

    if (list->nextHash) {
        list->nextHash->prevHash = list->prevHash;
    }

    if (list->tail) {
        list->tail->next = __hwdxCurrentContext->guiFree;
        __hwdxCurrentContext->guiFree = list->head;
    }

    list->nextHash = __hwdxCurrentContext->freeGuiList;
    __hwdxCurrentContext->freeGuiList = list;
}
/*** EOF dx_graph.c ***/
