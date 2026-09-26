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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hw.h"

/* Draw an unlit, red sphere on the display */

main( int argc, char **argv )
{
    double
        prevTime,
        saveTime;
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        env, light, cam,
        *objects;
    float
        rotDelta = 5.0,
        camDelta = 4.0,
        CamAng = 0.0,
        CamDist = 1.4,
        cy, cz,
        Mat[4][4],
        Ang = 0.0;
    int
        doSleep = 0,
        numFrames = 0,
        doTime = 1,
        numObjects,
        sampVert = 0,
        wireframe = 0,
        dbuff = 1,
        optFlags = 0,
        optSet = 0,
        i;
    hwInt32
        invisibility = 0, excl = 0,
        visibility = 0xFFFFFFFF;
    char
        *dispname = 0,
        *filename = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-dist" ) == 0 ) {
            CamDist = atof( argv[++i] );
        }
        else if( strcmp( argv[i], "-sleep" ) == 0 ) {
            doSleep = atoi( argv[++i] );
        }
        else if( strcmp( argv[i], "-opt" ) == 0 ) {
            optFlags = atoi( argv[++i] );
            optSet = 1;
        }
        else if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        else if( strcmp( argv[i], "-nodb" ) == 0 ) {
            dbuff = 0;
        }
        else if( strcmp( argv[i], "-vis" ) == 0 ) {
            sscanf( argv[i+1], "%x", &visibility );
            i++;
        }
        else if( strcmp( argv[i], "-invis" ) == 0 ) {
            sscanf( argv[i+1], "%x", &invisibility );
            sscanf( argv[i+2], "%x", &excl);
            visibility = 0;
            i += 2;
        }
        else if( strcmp( argv[i], "-rot" ) == 0 ) {
            sscanf( argv[i+1], "%f", &rotDelta );
            i++;
        }
        else if( strcmp( argv[i], "-camrot" ) == 0 ) {
            sscanf( argv[i+1], "%f", &camDelta );
            i++;
        }
        else if( strcmp( argv[i], "-rgb" ) == 0 ) {
            sampVert = HW_TRUE;
        }
        else if( strcmp( argv[i], "-time" ) == 0 ) {
            doTime = 1;
        }
        else if( strcmp( argv[i], "-display" ) == 0 ) {
            dispname = argv[++i];
        }
        else if( *argv[i] != '-' ) {
            filename = argv[i];
        }
    }
    if( !filename )     exit( 1 );

    if (!hwInit(argc, argv)) exit(1);

#if defined(X11)
    if( dispname ) {
        Display
            *display;
        display = XOpenDisplay( dispname );
        if( !display )  exit( 1 );
        disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, display );
    }
#else
    if( 0 ) {
    }
#endif
    else {
        disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    }
    if( !disp ) exit( 1 );

    if( dbuff ) {
        if( !disp->chooseVisual( disp, HW_VIS_DBUFF | HW_VIS_DEPTH, 0 ) )       
            exit( 1 );
    }
    else {
        if( !disp->chooseVisual( disp, HW_VIS_DEPTH, 0 ) )      
            exit( 1 );
    }

    draw = disp->createWindow( disp, "Test", 100, 100, 512, 512, 0 );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    numObjects = hwParseFile( filename, &objects );
    if( !numObjects )   exit( 1 );

    for( i = 0; i < numObjects; i++ ) {
        if( wireframe ) HW_MODIFY_1B( objects[i], hwStrWireframe, wireframe );
        if( sampVert ) HW_MODIFY_1B( objects[i], hwStrSampVert, sampVert );
        if( optSet )HW_MODIFY_1I( objects[i], hwStrOptFlags, optFlags );
    }

    env = hwEnviron->create( hwEnviron );
    if( !env )          exit( 1 );
    HW_MODIFY_1F( env, hwStrAmbientFactor, 0.5 );
    HW_MODIFY_3F( env, hwStrBackgroundColor, 0.0, 0.0, 1.0 );

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.1, 2*CamDist );
    HW_MODIFY_1I( cam, hwStrField, 60 );
    HW_MODIFY_1B( cam, hwStrPerspective, HW_TRUE );
    HW_MODIFY_3F( cam, hwStrPos, 0.0, 0.0, -CamDist );
    HW_MODIFY_3F( cam, hwStrDir, 0.0, 0.0, 1.0 );
    HW_MODIFY_3F( cam, hwStrUp, 0.0, 1.0, 0.0 );

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, -1.0, -1.0, 1.0 );

    if( visibility ) {
        disp->setVisibility( disp, visibility );
    }
    else {
        disp->setInvisibility( disp, invisibility, excl );
    }

    while( 1 ) {
        env->draw( env );

        hwRotateY( Mat, CamAng );
        disp->pushMatrix( disp, Mat );
            cam->draw( cam );
            light->draw( light );
        disp->popMatrix( disp );


        hwRotateX( Mat, Ang );
        disp->pushMatrix( disp, Mat );

        for( i = 0; i < numObjects; i++ ) {
            objects[i]->draw( objects[i] );
        }

        disp->popMatrix( disp );
        disp->update( disp, HW_UPDATE_ALL );
        if( doSleep ) {
#ifndef WIN32
            sleep( doSleep );
#endif
        }

        Ang += rotDelta;
        if( Ang > 360.0 ) Ang -= 360.0;
        CamAng += camDelta;
        if( CamAng > 360.0 ) CamAng -= 360.0;

        /* Perform frames-per-second report */
        prevTime = disp->getElapsedTime(disp);
        if( doTime ) {
            numFrames++;
            if( (numFrames > 0) && ((prevTime - saveTime) > 2.) ) {
                (void)printf( "FPS: %f\r", numFrames / (prevTime - saveTime) );
                (void)fflush( stdout );
                saveTime = prevTime;
                numFrames = 0;
            }
        }

    }
}
