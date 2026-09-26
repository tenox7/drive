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
#include <X11/keysym.h>
#include "hw.h"

#define DEG * (3.14159 / 180.0)

Display *display;
Window  window;

float view_up_dir = 0.0;
float view_up_angle = -90.0;
float view_around_angle = 0.0;
float view_dist = 3.0;
float scale = 1.0;

/* Draw an unlit, red sphere on the display */

void UpdateCamera( hwObject camera)
{
    float x, y, z;
    float r2;

    if( view_up_angle > 89.0) view_up_angle = 89.0;
    if( view_up_angle < -89.0) view_up_angle = -89.0;
    if( view_dist <= 0.1) view_dist = 0.1;

    z =  sin( view_up_angle DEG) * view_dist;
    r2 = cos( view_up_angle DEG);

    x = sin( view_around_angle DEG) * r2 * view_dist;

    y = cos( view_around_angle DEG) * r2 * view_dist;

    HW_MODIFY_3F(camera, hwStrPos, x, y, z);
    HW_MODIFY_3F( camera, hwStrDir,  -x,  -y,  -z );

    HW_MODIFY_2F( camera, hwStrPlanes, 0.1, 10*view_dist );


}

void processXEvents(
    void)
{
    XEvent event;
    XAnyEvent *anyevent = (XAnyEvent *) &event;
    KeySym ks;
    int keyState;

    if( XPending(display)) {
       while(XPending(display) )
           XNextEvent(display, &event);

    }

    switch(event.type) {
        case KeyPress:
           ks = XKeycodeToKeysym(display,event.xkey.keycode,0);
           keyState = event.xkey.state;

           switch(ks) {
              case XK_Left:
                  if( keyState & ShiftMask )
                      view_around_angle += 10.0;
                  else
                      view_around_angle += 1.0;
                  break;
              case XK_Right:
                  if( keyState & ShiftMask )
                      view_around_angle -= 10.0;
                  else
                      view_around_angle -= 1.0;
                  break;
              case XK_Up:
                  if( keyState & ShiftMask )
                      view_up_angle += 10.0;
                  else
                      view_up_angle += 1.0;
                  break;
              case XK_Down:
                  if( keyState & ShiftMask )
                      view_up_angle -= 10.0;
                  else
                      view_up_angle -= 1.0;
                  break;
              case XK_Prior:
                  if( keyState & ShiftMask )
                      view_dist += 1.0 * scale;
                  else
                      view_dist += 0.1 * scale;
                  break;
              case XK_Next:
                  if( keyState & ShiftMask )
                      view_dist -= 1.0 * scale;
                  else
                      view_dist -= 0.1 * scale;
                  break;
              case XK_q:
                  exit(1);
                  break;
           }
    }

}

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        env, light, cam,
        *objects;
    float
        CamAng = 0.0,
        CamDist = 1.4,
        cy, cz,
        Mat[4][4],
        Ang = 0.0;
    int
        doSleep = 0,
        numObjects,
        sampVert = 0,
        wireframe = 0,
        bigWindow = 0,
        optFlags = 0,
        i;
    char
        *dispname = 0,
        *filename = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-dist" ) == 0 ) {
            CamDist = atof( argv[++i] );
            view_dist = CamDist;
        }
        else if( strcmp( argv[i], "-scale" ) == 0 ) {
            scale = atof( argv[++i] );
        }
        else if( strcmp( argv[i], "-opt" ) == 0 ) {
            optFlags = atoi( argv[++i] );
        }
        else if( strcmp( argv[i], "-big" ) == 0 ) {
            bigWindow = 1;
        }
        else if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        else if( strcmp( argv[i], "-rgb" ) == 0 ) {
            sampVert = HW_TRUE;
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

    if( dispname ) {
        display = XOpenDisplay( dispname );
        if( !display )  exit( 1 );
        disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, display );
    }
    else {
        display = XOpenDisplay( "" );
        disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    }
    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    if(bigWindow)
        draw = disp->createWindow( disp, "Test", 100, 100, 1024, 1024, 0 );
    else
        draw = disp->createWindow( disp, "Test", 100, 100, 512, 512, 0 );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    window = disp->extractWindow( disp, draw );

    XSelectInput(display, window, KeyPressMask );

    numObjects = hwParseFile( filename, &objects );
    if( !numObjects )   exit( 1 );

    for( i = 0; i < numObjects; i++ ) {
        if( wireframe ) HW_MODIFY_1B( objects[i], hwStrWireframe, wireframe );
        if( sampVert ) HW_MODIFY_1B( objects[i], hwStrSampVert, sampVert );
        if( optFlags ) HW_MODIFY_1I( objects[i], hwStrOptFlags, optFlags );
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
    HW_MODIFY_3F( cam, hwStrUp, 0.0, 0.0, -1.0 );

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, 1.0 );

    while( 1 ) {
        env->draw( env );

        UpdateCamera( cam );

        cam->draw( cam );
        light->draw( light );

        for( i = 0; i < numObjects; i++ ) {
            objects[i]->draw( objects[i] );
        }

        disp->update( disp, HW_UPDATE_ALL );
        if( doSleep ) {
            sleep( doSleep );
        }

        processXEvents( );
    }
}
