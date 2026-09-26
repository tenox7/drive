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

void eventHandler( hwDrawable draw, hwWinEvent *event );

int allDone = 0;
int doPerPixel = 1;
int doPause;
int pauseLight;
int pauseCamera;
int pauseObject;
int winWidth = 512;
int winHeight = 512;
int resized;
int doTorus = 0;

int main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    const char
        *texNames[3] = {
            "Texture/earthbump1k.jpg",
            "Texture/earthmap1k.jpg",
            "Texture/earthspec1k.jpg",
        };
    hwObject
        sphereTex[3], /* color, gloss, bump */
        torusTex[3], /* color, gloss, bump */
        bumpObj[2],
        nonBumpObj[2],
        flatSphereTex,
        flatTorusTex,
        env,
        light, cam;
    float
        reliefScale = 4.0,
        CamAng = 0.0,
        LightAng = 0.0,
        cx, cy, cz,
        Mat[4][4],
        Ang = 0.0;
    int
        doSmooth = HW_TRUE,
        vecCount, dataType, vtxFormat, *vfPtr, n, wpv,
        graphN = 17, graphM = 33,
        i, wireframe = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        else if( strcmp( argv[i], "-rows" ) == 0 ) {
            graphN = atoi( argv[++i] );
        }
        else if( strcmp( argv[i], "-cols" ) == 0 ) {
            graphM = atoi( argv[++i] );
        }
        else if( strcmp( argv[i], "-torus" ) == 0 ) {
            doTorus = 1;
        }
        else if( strcmp( argv[i], "-flat" ) == 0 ) {
            doSmooth = HW_FALSE;
        }
        else if( strcmp( argv[i], "-relief" ) == 0 ) {
            reliefScale = atof( argv[++i] );
        }
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if (!disp)  exit( 1 );

    if (!disp->chooseVisual(disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0)) {
        exit( 1 );
    }

    draw = disp->createWindow( disp, "SphereBump",
                                100, 100, winWidth, winHeight,
                                HW_WIN_INPUT );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    disp->inputHandler( disp, eventHandler );

    env = hwEnviron->create( hwEnviron );
    if( !env )          exit( 1 );
    HW_MODIFY_1F( env, hwStrAmbientFactor, 0.5 );
    HW_MODIFY_3F( env, hwStrBackgroundColor, 0.0, 0.0, 1.0 );
    HW_MODIFY_3F( env, hwStrFogColor, 0.0, 0.0, 1.0 );
    HW_MODIFY_1B( env, hwStrFog, HW_TRUE );
    HW_MODIFY_2F( env, hwStrFogPlanes, 1.0, 3.5);
    HW_MODIFY_1I( env, hwStrFogType, HW_FOG_LINEAR);

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.5, 3.5 );
    HW_MODIFY_1I( cam, hwStrField, 90 );
    HW_MODIFY_1B( cam, hwStrPerspective, 1 );

    for( i = 0; i < 3; i++) {
        sphereTex[i] = hwTexture->create( hwTexture );
        torusTex[i] = hwTexture->create( hwTexture );
        if( !(sphereTex[i] && torusTex[i]) )  exit( 1 );

        sphereTex[i]->modify( sphereTex[i], hwStrFileName,
                              HW_TYPE_STRING, texNames[i] );
        torusTex[i]->modify( torusTex[i], hwStrFileName,
                             HW_TYPE_STRING, texNames[i] );
        HW_MODIFY_3F( torusTex[i], hwStrScale, 4.0, 2.0, 1.0 );
        HW_MODIFY_3F( torusTex[i], hwStrPos, 0.0, 0.5, 0.0 );
    }

    flatSphereTex = hwTexture->create( hwTexture );
    flatTorusTex = hwTexture->create( hwTexture );
    if( !(flatSphereTex && flatTorusTex) ) exit( 1 );

    flatSphereTex->modify( flatSphereTex, hwStrFileName,
                           HW_TYPE_STRING, texNames[1] );
    flatTorusTex->modify( flatTorusTex, hwStrFileName,
                          HW_TYPE_STRING, texNames[1] );
    HW_MODIFY_3F( flatTorusTex, hwStrScale, 4.0, 2.0, 1.0 );
    HW_MODIFY_3F( flatTorusTex, hwStrPos, 0.0, 0.5, 0.0 );

    /* Normal map */
    HW_MODIFY_1I( sphereTex[0], hwStrInternalFormat, HW_TM_HEIGHT );
    HW_MODIFY_1I( sphereTex[0], hwStrApply, HW_TM_RELIEF );
    HW_MODIFY_1F( sphereTex[0], hwStrReliefScale, reliefScale );

    /* Color map */
    HW_MODIFY_1I( sphereTex[1], hwStrInternalFormat, HW_TM_COLOR );
    HW_MODIFY_1I( sphereTex[1], hwStrApply, HW_TM_MODULATE );
    HW_MODIFY_1I( sphereTex[1], hwStrCoordMode, HW_TM_STAGE_0 );

    /* Gloss map */
    HW_MODIFY_1I( sphereTex[2], hwStrInternalFormat, HW_TM_COLOR );
    HW_MODIFY_1I( sphereTex[2], hwStrApply, HW_TM_GLOSS );
    HW_MODIFY_1I( sphereTex[2], hwStrCoordMode, HW_TM_STAGE_0 );

    /* Normal map */
    HW_MODIFY_1I( torusTex[0], hwStrInternalFormat, HW_TM_HEIGHT );
    HW_MODIFY_1I( torusTex[0], hwStrApply, HW_TM_RELIEF );
    HW_MODIFY_1F( torusTex[0], hwStrReliefScale, reliefScale );

    /* Color map */
    HW_MODIFY_1I( torusTex[1], hwStrInternalFormat, HW_TM_COLOR );
    HW_MODIFY_1I( torusTex[1], hwStrApply, HW_TM_MODULATE );
    HW_MODIFY_1I( torusTex[1], hwStrCoordMode, HW_TM_STAGE_0 );

    /* Gloss map */
    HW_MODIFY_1I( torusTex[2], hwStrInternalFormat, HW_TM_COLOR );
    HW_MODIFY_1I( torusTex[2], hwStrApply, HW_TM_GLOSS );
    HW_MODIFY_1I( torusTex[2], hwStrCoordMode, HW_TM_STAGE_0 );

    bumpObj[0] = hwSphere->create( hwSphere );
    bumpObj[1] = hwTorus->create( hwTorus );

    if( !(bumpObj[0] && bumpObj[1]) ) exit( 1 );

    for( i = 0; i < 2; i++ ) {
        bumpObj[i]->modify( bumpObj[i], hwStrTexture,
                            HW_MAKE_TYPE(HW_TYPE_OBJECT,3),
                            (i == 0) ? sphereTex : torusTex);
        HW_MODIFY_3F( bumpObj[i], hwStrColor, 1.0, 1.0, 1.0 );
        HW_MODIFY_1I( bumpObj[i], hwStrOptFlags, HW_OPT_DL_ATTRS );
        HW_MODIFY_1B( bumpObj[i], hwStrHasNormals, doSmooth );
        HW_MODIFY_1B( bumpObj[i], hwStrHasUV, HW_TRUE );
        HW_MODIFY_1B( bumpObj[i], hwStrHasTangent, HW_TRUE );
        HW_MODIFY_1I( bumpObj[i], hwStrGraphN, graphN );
        HW_MODIFY_1I( bumpObj[i], hwStrGraphM, graphM );
        HW_MODIFY_1B( bumpObj[i], hwStrWireframe, wireframe );
        HW_MODIFY_1F( bumpObj[i], hwStrShininess, 0.5);
    }

    nonBumpObj[0] = hwSphere->create( hwSphere );
    nonBumpObj[1] = hwTorus->create( hwTorus );
    if( !(nonBumpObj[0] && nonBumpObj[1]) ) exit( 1 );

    for( i = 0; i < 2; i++ ) {
        nonBumpObj[i]->modify( nonBumpObj[i], hwStrTexture,
                               HW_TYPE_OBJECT,
                               (i == 0) ? flatSphereTex : flatTorusTex );
        HW_MODIFY_3F( nonBumpObj[i], hwStrColor, 1.0, 1.0, 1.0 );
        HW_MODIFY_1I( nonBumpObj[i], hwStrOptFlags, HW_OPT_DL_ATTRS );
        HW_MODIFY_1B( nonBumpObj[i], hwStrHasNormals, doSmooth );
        HW_MODIFY_1B( nonBumpObj[i], hwStrHasUV, HW_TRUE );
        HW_MODIFY_1I( nonBumpObj[i], hwStrGraphN, graphN );
        HW_MODIFY_1I( nonBumpObj[i], hwStrGraphM, graphM );
        HW_MODIFY_1B( nonBumpObj[i], hwStrWireframe, wireframe );
        HW_MODIFY_1F( nonBumpObj[i], hwStrShininess, 0.5);
    }

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );

    while( !allDone ) {
       /* Update the window */
        if( resized ) {
            disp->viewport( disp, 0, 0, winWidth, winHeight );
            resized = 0;
        }
        env->draw( env );

        cy = cos( CamAng * 3.141592653589 / 180.0 );
        cz = sin( CamAng * 3.141592653589 / 180.0 );
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 2.0 * cy, 2.0 * cz );
        HW_MODIFY_3F( cam, hwStrDir, 0.0, -cy, -cz );
        HW_MODIFY_3F( cam, hwStrUp, 0.0, -cz, cy );

        cam->draw( cam );

        cx = cos( LightAng * 3.141592653589 / 180.0 );
        cy = sin( LightAng * 3.141592653589 / 180.0 );
        HW_MODIFY_3F( light, hwStrDir, cx, cy, 0.0);
        light->draw( light );

        hwRotateY( Mat, Ang );

        disp->pushMatrix( disp, Mat );

        if( doPerPixel) {
            bumpObj[doTorus]->draw( bumpObj[doTorus] );
        }
        else {
            nonBumpObj[doTorus]->draw( nonBumpObj[doTorus] );
        }

        disp->popMatrix( disp );

        disp->update( disp, HW_UPDATE_ALL );

        if( !doPause ) {
            if( !pauseObject ) {
                Ang += 0.4;
                if( Ang > 360.0 ) Ang -= 360.0;
            }

            if( !pauseCamera ) {
                CamAng += 0.08;
                if( CamAng > 360.0 ) CamAng -= 360.0;
            }

            if( !pauseLight ) {
                LightAng += 1.6;
                if( LightAng > 360.0 ) LightAng -= 360.0;
            }
        }
    }
}

void eventHandler( hwDrawable draw, hwWinEvent *event )
{
    switch( event->type ) {
    case HW_INPUT_CONFIG :
        winWidth = event->config.width;
        winHeight = event->config.height;
        resized = 1;
        break;
    case HW_INPUT_KEYBOARD :
        switch( event->keyboard.key ) {
        case ' ' :
            doPause = !doPause;
            break;
        case 'b' : case 'B' :
            doPerPixel = !doPerPixel;
            break;
        case 'l' : case 'L' :
            pauseLight = !pauseLight;
            break;
        case 'c' : case 'C' :
            pauseCamera = !pauseCamera;
            break;
        case 'r' : case 'R' :
            pauseObject = !pauseObject;
            break;
        case 'o' : case 'O' :
            doTorus = !doTorus;
            break;
        case '\033' :
            allDone = 1;
            break;
        }
        break;
    default :
        break;
    }
}
