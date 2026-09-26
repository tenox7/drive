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

/* Draw a textured, red disc on the display */

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        tm[2], quad, light, cam;
    float
        CamAng = 0.0,
        cy, cz,
        Mat[4][4];
    int
        i, sampVert = 0, Wireframe = 0, tess = 0;
    unsigned char
        /* An 8x8 checkerboard */
        Image[8][8] = {
            {0x55,0x55,0xFF,0xFF,0x55,0x55,0xFF,0xFF},
            {0x55,0x55,0xFF,0xFF,0x55,0x55,0xFF,0xFF},
            {0xFF,0xFF,0x55,0x55,0xFF,0xFF,0x55,0x55},
            {0xFF,0xFF,0x55,0x55,0xFF,0xFF,0x55,0x55},
            {0x55,0x55,0xFF,0xFF,0x55,0x55,0xFF,0xFF},
            {0x55,0x55,0xFF,0xFF,0x55,0x55,0xFF,0xFF},
            {0xFF,0xFF,0x55,0x55,0xFF,0xFF,0x55,0x55},
            {0xFF,0xFF,0x55,0x55,0xFF,0xFF,0x55,0x55},
        },
        /* A colored checkerboard */
        Image1[2][2][3] = {
            { {0xFF,0x00,0x00}, {0x00,0xFF,0x00} },
            { {0x00,0x00,0xFF}, {0xFF,0xFF,0xFF} }
        };
    hwFloat
        data[] = {
            -1.0, -1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0,  0.0, 0.0,
             1.0, -1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 1.0,  0.0, 1.0,
             1.0,  1.0, 0.0,  0.0, 0.0, -1.0,  1.0, 1.0,  1.0, 1.0,
            -1.0,  1.0, 0.0,  0.0, 0.0, -1.0,  1.0, 0.0,  1.0, 0.0,
        };

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-rgb" ) == 0 ) {
            sampVert = HW_TRUE;
        }
        if( strcmp( argv[i], "-wire" ) == 0 ) {
            Wireframe = HW_TRUE;
        }
        if( strcmp( argv[i], "-tess" ) == 0 ) {
            tess = atoi( argv[++i] );
        }
    }

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 100, 100, 256, 256, 0 );
    if( !draw ) exit( 1 );

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.4, 2.4 );
    HW_MODIFY_1I( cam, hwStrField, 90 );
    HW_MODIFY_1B( cam, hwStrPerspective, HW_TRUE );

    tm[0] = hwTexture->create( hwTexture );
    HW_MODIFY_1I( tm[0], hwStrRows, 8 );
    HW_MODIFY_1I( tm[0], hwStrColumns, 8 );
    HW_MODIFY_1I( tm[0], hwStrComponents, 1 );
    HW_MODIFY_1I( tm[0], hwStrType, HW_IMG_UBYTE );
    tm[0]->modify( tm[0], hwStrImage, HW_MAKE_TYPE(HW_TYPE_BYTE,8*8), Image );
    HW_MODIFY_1I( tm[0], hwStrFilter, 0 );
    HW_MODIFY_1I( tm[0], hwStrScale, 2 );

    tm[1] = hwTexture->create( hwTexture );
    HW_MODIFY_1I( tm[1], hwStrRows, 2 );
    HW_MODIFY_1I( tm[1], hwStrColumns, 2 );
    HW_MODIFY_1I( tm[1], hwStrComponents, 3 );
    HW_MODIFY_1I( tm[1], hwStrType, HW_IMG_UBYTE );
    tm[1]->modify( tm[1],hwStrImage, HW_MAKE_TYPE(HW_TYPE_BYTE,2*2*3), Image1 );
    HW_MODIFY_1I( tm[1], hwStrFilter, 0 );
    HW_MODIFY_1I( tm[1], hwStrScale, 2 );

    quad = hwQuads->create( hwQuads );
    if( !quad ) exit( 1 );
    HW_MODIFY_3F( quad, hwStrColor, 1.0, 1.0, 1.0 );
    HW_MODIFY_1I( quad, hwStrOptFlags, HW_OPT_DL_ATTRS );
    HW_MODIFY_1B( quad, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrHasST0, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrHasST1, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrWireframe, Wireframe );
    HW_MODIFY_1B( quad, hwStrSampVert, sampVert );
    if( tess > 0 ) {
        HW_MODIFY_1I( quad, hwStrGraphN, tess );
        HW_MODIFY_1I( quad, hwStrGraphM, tess );
    }
    quad->modify( quad, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*10), data );
    quad->modify( quad, hwStrTexture, HW_MAKE_TYPE(HW_TYPE_OBJECT,2), tm );

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, -1.0 );

    disp->makeCurrent( disp, draw );

    while( 1 ) {
        cy = cos( CamAng * 3.141592653589 / 180.0 );
        cz = sin( CamAng * 3.141592653589 / 180.0 );
        CamAng += 4.0; if( CamAng > 360.0 ) CamAng -= 360.0;
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 1.4 * cy, 1.4 * cz );
        HW_MODIFY_3F( cam, hwStrDir, 0.0, -cy, -cz );
        HW_MODIFY_3F( cam, hwStrUp, 0.0, -cz, cy );

        cam->draw( cam );
        light->draw( light );
        quad->draw( quad );
        disp->update( disp, HW_UPDATE_ALL );
    }
}
