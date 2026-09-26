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

/* Draw an unlit, red poly on the display */

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        env, poly, light, cam;
    float
        CamAng = 0.0,
        cy, cz;
    int
        i, wireframe = 0, pline = 0;
    static float
        PolyData[] = {
            -1.0, -1.0, 0.0,
            -1.0,  1.0, 0.0,
             1.0,  1.0, 0.0,
             1.0, -1.0, 0.0
        };

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        if( strcmp( argv[i], "-pline" ) == 0 ) {
            pline = 1;
        }
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 100, 100, 256, 256, 0 );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    env = hwEnviron->create( hwEnviron );
    if( !env )          exit( 1 );
    HW_MODIFY_1F( env, hwStrAmbientFactor, 0.5 );
    HW_MODIFY_3F( env, hwStrBackgroundColor, 0.0, 0.0, 1.0 );

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.4, 2.4 );
    HW_MODIFY_1I( cam, hwStrField, 90 );

    if( pline ) {
        poly = hwPolyline->create( hwPolyline );
    }
    else {
        poly = hwPolygon->create( hwPolygon );
    }
    if( !poly ) exit( 1 );
    HW_MODIFY_3F( poly, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1I( poly, hwStrOptFlags, HW_OPT_DL_ATTRS );
    HW_MODIFY_1B( poly, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_1I( poly, hwStrGraphN, 4 );
    poly->modify( poly, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), PolyData );
    HW_MODIFY_1B( poly, hwStrWireframe, wireframe );

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, -1.0 );

    while( 1 ) {
        env->draw( env );

        cy = cos( CamAng * 3.141592653589 / 180.0 );
        cz = sin( CamAng * 3.141592653589 / 180.0 );
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 1.4 * cy, 1.4 * cz );
        HW_MODIFY_3F( cam, hwStrDir, 0.0, -cy, -cz );
        HW_MODIFY_3F( cam, hwStrUp, 0.0, -cz, cy );

        cam->draw( cam );

        light->draw( light );

        poly->draw( poly );
        disp->update( disp, HW_UPDATE_ALL );

        CamAng += 4.0;
        if( CamAng > 360.0 ) CamAng -= 360.0;
    }
}
