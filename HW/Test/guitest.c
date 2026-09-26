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

hwObject
    *objects;
int
    numObjects;
hwDisplay
    disp;

void callback( hwDrawable draw, hwWinEvent *event )
{
    int
        i;

    if( event->type == HW_INPUT_CONFIG ) {
        disp->viewport( disp, 0, 0, event->config.width, event->config.height );
    }
    else if( event->type == HW_INPUT_KEYBOARD ) {
        if( event->keyboard.key == '\033' ) {
            exit(0);
        }
    }

    for( i = 0; i < numObjects; i++ ) {
        objects[i]->modify( objects[i], hwStrEvent, HW_TYPE_EVENT, event );
    }
}

void widget( hwObject obj, hwInt32 reason )
{
    if( reason == HW_CB_COMPLETE ) {
        printf("%s clicked\n", obj->name ? obj->name : "widget" );
    }
}

main( int argc, char **argv )
{
    double
        prevTime,
        saveTime;
    hwDrawable
        draw;
    char
        *filename = 0;
    int
        i,
        numFrames = 0,
        fullscreen = 0,
        doTime = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-time" ) == 0 ) {
            doTime = 1;
        }
        else if( strcmp( argv[i], "-fullscreen" ) == 0 ) {
            fullscreen = HW_WIN_FULLSCREEN;
        }
        else if( *argv[i] != '-' ) {
            filename = argv[i];
        }
    }
    if( !filename )     exit( 1 );

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL);
    if( !disp ) exit( 1 );

    if( !disp->chooseVisual( disp, HW_VIS_DBUFF, HW_VIS_DBUFF ) ) {
        exit( 1 );
    }

    draw = disp->createWindow( disp, "GUI", 100, 100, 640, 480,
                               HW_WIN_INPUT | fullscreen );
    if( !draw ) exit( 1 );

    /* This needs to happen before hwParseFile */
    disp->makeCurrent( disp, draw );

    numObjects = hwParseFile( filename, &objects );
    if( !numObjects )   exit( 1 );

    for( i = 0; i < numObjects; i++ ) {
        objects[i]->modify( objects[i], hwStrCallback, HW_TYPE_CALLBACK,
                                widget );
    }

    disp->inputHandler( disp, callback );

    saveTime = disp->getElapsedTime(disp);
    while( 1 ) {

        for( i = 0; i < numObjects; i++ ) {
            objects[i]->draw( objects[i] );
        }

        disp->update( disp, HW_UPDATE_ALL );

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
