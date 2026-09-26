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
#include "hw.h"

/* Draw an unlit, red sphere on the display */

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        env, sphere, light, cam, tm;
    float
        CamAng = 0.0,
        cy, cz,
        Mat[4][4],
        Ang = 0.0;
    int
        graphN = 133, graphM = 165, i, wireframe = 0, sampvert = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        if( strcmp( argv[i], "-rows" ) == 0 ) {
            graphN = atoi( argv[++i] );
        }
        if( strcmp( argv[i], "-cols" ) == 0 ) {
            graphM = atoi( argv[++i] );
        }
        if( strcmp( argv[i], "-rgb" ) == 0 ) {
            sampvert = HW_TRUE;
        }
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 100, 100, 512, 512, 0 );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

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

    tm = hwTexture->create(hwTexture);
    if( ! tm) exit (1);
    tm->modify(tm, hwStrFileName, HW_TYPE_STRING, "Texture/Earth.jpg");
    HW_MODIFY_3F( tm, hwStrScale, 1.0, 0.75, 1.0 );
    HW_MODIFY_3F( tm, hwStrPos, 0.0, 0.25, 0.0 );

    sphere = hwSphere->create( hwSphere );
    if( !sphere )       exit( 1 );
    sphere->modify(sphere, hwStrTexture, HW_TYPE_OBJECT, tm);
    HW_MODIFY_3F( sphere, hwStrColor, 1.0, 0.8, 0.4 );
    HW_MODIFY_1I( sphere, hwStrOptFlags, HW_OPT_DL_ATTRS );
    HW_MODIFY_1B( sphere, hwStrHasNormals, HW_FALSE );
    HW_MODIFY_1B( sphere, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_1I( sphere, hwStrGraphN, graphN );
    HW_MODIFY_1I( sphere, hwStrGraphM, graphM );
    HW_MODIFY_1B( sphere, hwStrWireframe, wireframe );
    HW_MODIFY_2F( sphere, hwStrLatRange, -45.0, 90.0 );
    HW_MODIFY_1B( sphere, hwStrSampVert, sampvert );
    HW_MODIFY_1F( sphere, hwStrShininess, 0.1);

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, -1.0 );

    while( 1 ) {
        env->draw( env );

        cy = cos( CamAng * 3.141592653589 / 180.0 );
        cz = sin( CamAng * 3.141592653589 / 180.0 );
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 2.0 * cy, 2.0 * cz );
        HW_MODIFY_3F( cam, hwStrDir, 0.0, -cy, -cz );
        HW_MODIFY_3F( cam, hwStrUp, 0.0, -cz, cy );

        cam->draw( cam );

        light->draw( light );

        hwRotateY( Mat, Ang );
        disp->pushMatrix( disp, Mat );
        sphere->draw( sphere );
        disp->popMatrix( disp );
        disp->update( disp, HW_UPDATE_ALL );
        Ang += 5.0;
        if( Ang > 360.0 ) Ang -= 360.0;
        CamAng += 4.0;
        if( CamAng > 360.0 ) CamAng -= 360.0;
    }
}
