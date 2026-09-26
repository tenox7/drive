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

#if defined(WIN32)
#       include <windows.h>
#endif

#if defined(USE_X11)
#       include <GL/glx.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>

#if defined(MAC)
#include <OpenGL/OpenGL.h>
#elif defined(GLFW)
#include "GLFW/glfw3.h"
#else
#include <GL/gl.h>
#endif
#include "hw.h"
#include "timer.h"

#if defined(WIN32)
#       include "glHpNt.h"
PFNGLVISIBILITYBUFFERHPPROC glVisibilityBufferHP;
PFNGLNEXTVISIBILITYTESTHPPROC glNextVisibilityTestHP;

#ifndef GLFW
void
    eventHandler( HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam );
extern void
    (*__hwglEventHandler)( HWND hwnd, UINT msg, LPARAM wParam, LPARAM lParam );
#endif

#endif

extern hwInt32
    hwDefaultOptFlags;

float 
    CamAng = 0.0,
    CamDist = 35.0;

hwObject
    cam;

typedef struct _Ball {
    hwObject    hwobj;
    float       bounds[6];
    int         bounds_id;
    int         visible_now;
    int         visible_last;
    int         fixup;
} BallType;

#define NUM_BALLS       300
#define BALL_N           16
#define BALL_M           32

hwDisplay
    disp;

#define OCCLUDE_MODE_NONE       0
#define OCCLUDE_MODE_OCCLUDE    1
#define OCCLUDE_MODE_VISTEST    2

int
    winWidth, winHeight, resized,
    Done = 0,
    num_tests = 0,
    num_occludes = 0,
    ShowBoxes = 0,
    PrintOccStats = 0,
    OccludeMode = OCCLUDE_MODE_NONE,
    Paused=0,
    DoOcclusion = 0;

void draw_balls_occlusion( BallType *balls, int num_balls)
{
    GLboolean
        result;
    int 
        i;
    float
        transMat[4][4];

    for(i=0; i < NUM_BALLS; i++ ) {
        num_tests++;

#ifdef USE_OCCLUSION_TEST
        glEnable(GL_OCCLUSION_TEST_HP); 
#endif
        glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING);
        glColorMask(GL_FALSE, GL_FALSE,GL_FALSE,GL_FALSE);

        glCallList(balls[i].bounds_id);
        
#ifdef USE_OCCLUSION_TEST
        glGetBooleanv(GL_OCCLUSION_RESULT_HP, &result); 
        glDisable(GL_OCCLUSION_TEST_HP);
#else
        result = 1;
#endif
        glEnable(GL_LIGHTING);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        if(result) {
            balls[i].hwobj->draw(balls[i].hwobj);
            if(ShowBoxes)
                glCallList(balls[i].bounds_id);
        }
        else {
            num_occludes++;
        }
    }

}

void draw_balls_all( BallType *balls, int num_balls)
{
    int 
        i;

    for(i=0; i < NUM_BALLS; i++ ) {
        num_tests++;

        balls[i].hwobj->draw(balls[i].hwobj);
        if(ShowBoxes)
            glCallList(balls[i].bounds_id);
    }

}


void draw_balls_new( BallType *balls, int num_balls)
{
    GLboolean
        visBuffer[NUM_BALLS],
        visstuff[2];
    int 
        i;

    /* Draw what was visible last time */

    for(i=0; i < NUM_BALLS; i++ ) {
        if( balls[i].visible_now ) {
            if(ShowBoxes)
                glCallList(balls[i].bounds_id);
            balls[i].hwobj->draw(balls[i].hwobj);
        }
    }

     /* glFlush();  */
    /* glFinish(); */

    /* Look to see who is visible for next time */

#ifdef USE_OCCLUSION_TEST /* [ */
    glVisibilityBufferHP(NUM_BALLS,visBuffer,0);

    glEnable(GL_VISIBILITY_TEST_HP);
    glDepthMask(GL_FALSE);
    glColorMask(GL_FALSE, GL_FALSE,GL_FALSE,GL_FALSE);
    glDisable(GL_LIGHTING);

    for(i=0; i < NUM_BALLS; i++ ) {
        num_tests++;
        glCallList(balls[i].bounds_id);
        glNextVisibilityTestHP();
    }

    glDisable(GL_VISIBILITY_TEST_HP);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_LIGHTING);


        
#ifdef TIME_GETS
    stop_timer();
    while(1) {
        start_timer();
        glGetBooleanv(GL_VISIBILITY_TEST_HP, visstuff); 
        stop_timer();
        print_elapsed_time();
    }
#else
    glGetBooleanv(GL_VISIBILITY_TEST_HP, visstuff); 
#endif

#endif /* ] */
    for(i=0; i < NUM_BALLS; i++ ) {
        if(  visBuffer[i] ) {   
            balls[i].visible_now = 1;
        }
        else {
            num_occludes++;
            balls[i].visible_now = 0;
        }
    }
}

void TestGetLoop(void )
{
    int i, j, count;
    float time;
    GLboolean result[100];

#ifdef USE_OCCLUSION_TEST
    count = 0;
    time = 0.0;
    for(i=0; i < 1000; i++ ) {
        start_timer();
        for(j=0; j < 1000; j++ ) {
            /* glGetBooleanv(GL_DEPTH_BITS, result);    */
            glGetBooleanv(GL_OCCLUSION_RESULT_HP, result);
            count++;
        }
        stop_timer ();
        time += get_elapsed_time();
        if( time > 0.0 ) {
            printf("GL Gets Per Second:  %f\n", count / time );
            time = 0.0;
            count = 0;
        }
    }
#endif
}

void PrintHelp()
{
  printf("\n\n");
  printf("vistest recognizes the following keys: \n");
  printf("  ' ': (space)   Pause object rotation (but keep rendering)\n");
  printf("  '.': (period)  Rotate object Counter Clockwise\n");
  printf("  ',': (comma)   Rotate object Clockwise\n");
  printf("  'i':           Move Camera in toward object \n");
  printf("  'o':           Move Camera away from object \n");
  printf("  'm':           Cycle occlusion mode (none/occlusion/vis test \n");
  printf("  'p':           Toggle printing of occlusion statistics\n");
  printf("  'p':           Toggle rendering of bounding boxes\n");
  printf("  'h':           Print this Help summary \n");
  printf("  'q':           Quit (terminate program) \n");
  printf("\n");
}

void handleKeyPress( int key )
{

    switch( key ) {
    case ' ':
        Paused = !Paused;
        break;
    case 'b':
        ShowBoxes = !ShowBoxes;
        break;
    case '.':
        CamAng += 1.0;
        break;
    case 'm':
        OccludeMode++;
        OccludeMode %= 3;
#if defined(WIN32)
        if( !glVisibilityBufferHP || !glNextVisibilityTestHP ) {
            OccludeMode = 0;
        }
#endif
        switch( OccludeMode ) {
            case OCCLUDE_MODE_NONE:
                printf("\nOcclude Mode: NONE\n");
            break;
            case OCCLUDE_MODE_OCCLUDE:
                printf("\nOcclude Mode: Occlusion Cull\n");
            break;
            case OCCLUDE_MODE_VISTEST:
                printf("\nOcclude Mode: Visibility Test\n");
            break;
        }
        break;
    case ',':
        CamAng -= 1.0;
        break;
    case 'i':
        CamDist -= 1.0;
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 0.0, -CamDist);
        printf("\nCamDist: %f\n", CamDist);
        break;
    case 'o':
        CamDist += 1.0;
        HW_MODIFY_3F( cam, hwStrPos, 0.0, 0.0, -CamDist);
        printf("\nCamDist: %f\n", CamDist);
        break;
    case 'p':
        PrintOccStats = !PrintOccStats;
        break;
    case 'h':
        PrintHelp();
        break;
    case 'q':
    case 'Q':
        Done = 1;
        break;
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
        handleKeyPress( event->keyboard.key );
        break;
    }
}

main( int argc, char **argv )
{
    hwDrawable
        draw;
    BallType
        balls[NUM_BALLS];
    hwObject
        env, light, light2, 
        plane1,
        plane2,
        lcap,
        rcap,
        oring,
        iring;
    int 
        numRows,
        frames=0, i,j;
    float
        elapsed = 0.0,
        rotMat[4][4],
        tmpMat[4][4],
        x, y, z,
        x1, y1, z1,
        x2, y2, z2,
        cx, cy, cz,
        theta,
        radius,
        *bbox,
        *ptr,
        *shape,
        *path,
        cosTheta, sinTheta;



    PrintHelp();

    hwDefaultOptFlags = HW_OPT_DL_ATTRS;

    for(i=0; i < argc; i++ ) {
        if(strcmp(argv[i], "-printstats") == 0) {
            PrintOccStats=1;
        }
        else if(strcmp(argv[i],"-showboxes") == 0 ) {
           ShowBoxes = 1;
        }
        else if(strcmp(argv[i],"-vistest") == 0 ) {
            OccludeMode = OCCLUDE_MODE_VISTEST;
        }
        else if(strcmp(argv[i],"-occlusion") == 0 ) {
            OccludeMode = OCCLUDE_MODE_OCCLUDE;
        }
        else if(strcmp(argv[i],"-all") == 0 ) {
            OccludeMode = OCCLUDE_MODE_NONE;
        }
        else if((strcmp(argv[i], "-opt") == 0) && (i < (argc-1))) {
            i++;
            hwDefaultOptFlags = atoi(argv[i]);
        }
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );

    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 100, 100, 512, 512,
                               HW_WIN_INPUT );

    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

#if defined(WIN32)
    /* We need the pointers to the visibility test functions */
    glVisibilityBufferHP
                = (PFNGLVISIBILITYBUFFERHPPROC)wglGetProcAddress
                        ( "glVisibilityBufferHP" );
    glNextVisibilityTestHP
                = (PFNGLNEXTVISIBILITYTESTHPPROC)wglGetProcAddress
                        ( "glNextVisibilityTestHP" );
#endif

    disp->update( disp, HW_UPDATE_ALL );        /* Incl. swap */
#if 0
    disp->draw2Dtext( disp,
                txfLoadStaticFont(16),
                0x00FF0000,
                HW_TEXT_ALIGN_CENTER, HW_TEXT_ALIGN_CENTER,
                16, 128, 128, "Hit return" );
#endif
    disp->update( disp, HW_UPDATE_GUI );        /* Just flush */

    /* Create the environment, camera, light */
    env = hwEnviron->create( hwEnviron );
    if( !env )          exit( 1 );
    HW_MODIFY_1F( env, hwStrAmbientFactor, 0.5 );
    HW_MODIFY_3F( env, hwStrBackgroundColor, 0.0, 0.0, 0.3 );

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.4, 60.0 );
    HW_MODIFY_1I( cam, hwStrField, 60 );
    HW_MODIFY_3F( cam, hwStrPos, 0.0, 0.0, -CamDist);
    HW_MODIFY_3F( cam, hwStrDir, 0.0, 0.0, 1.0);

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, -1.0 );

    light2 = hwLight->create( hwLight );
    if( !light2 )       exit( 1 );
    HW_MODIFY_3F( light2, hwStrDir, 1.0, 1.0,  1.0 );

    disp->makeCurrent( disp, draw );




    /* Create a couple of occluding surfaces */

    plane1 = hwBox->create(hwBox);
    HW_MODIFY_3F(plane1, hwStrPos, 0.0, 0.0, 5.0);
    HW_MODIFY_3F(plane1, hwStrColor, 1.0, 0.0, 1.0);
    HW_MODIFY_3F(plane1, hwStrScale, 15.0,10.0, 0.1);

    plane2 = hwBox->create(hwBox);
    HW_MODIFY_3F(plane2, hwStrPos, 0.0, 0.0, -6.0);
    HW_MODIFY_3F(plane2, hwStrColor, 1.0, 1.0, 0.0);
    HW_MODIFY_3F(plane2, hwStrScale, 15.0,10.0, 0.1);

#define DO_SWEEPS

#ifdef DO_SWEEPS
    iring = hwSweep->create(hwSweep);

    shape = (float *)malloc( 2 * 65 * sizeof(float) );

    ptr = shape;
    theta = 0.0;
    for(i=0; i<= 64; i++ ) {
        cosTheta = (float)cos((double)(theta * 3.14159 / 180.0));
        sinTheta = (float)sin((double)(theta * 3.14159 / 180.0));
        *ptr++ = cosTheta * 8.0;
        *ptr++ = sinTheta * 8.0;
        theta += (360.0 / 64.0);
    }
    iring->modify(iring, hwStrShape, HW_MAKE_TYPE(HW_TYPE_FLOAT, 2*65), 
         (void*) shape );

/*
    *ptr++ = -5.0; *ptr++ = 0.0;
    *ptr++ =  5.0; *ptr++ = 0.0;

    iring->modify(iring, hwStrShape, HW_MAKE_TYPE(HW_TYPE_FLOAT, 2*2), 
         (void*) shape );
*/

    path = (float *)malloc( 3 * 10 * sizeof(float));
    ptr = path;
    *ptr++ = 0.0; *ptr++ =0.0; *ptr++ = -20.0;
    *ptr++ = 0.0; *ptr++ =0.0; *ptr++ =  20.0;

    iring->modify(iring, hwStrPath, HW_MAKE_TYPE(HW_TYPE_FLOAT, 2*3), 
        (void *)path );

    HW_MODIFY_1B(iring, hwStrTwoSided, HW_FALSE);
    HW_MODIFY_1B(iring, hwStrTwoSided, HW_TRUE);

    HW_MODIFY_3F(iring, hwStrColor, 0.0, 1.0, 0.0);
    HW_MODIFY_1B(iring, hwStrHasCaps, HW_TRUE);

#else
    /* Create the outside ring of the bearing */

    oring = hwCone->create(hwCone);
    HW_MODIFY_1I(oring, hwStrGraphN, 32);
    HW_MODIFY_2F(oring, hwStrRadius, 8.0, 8.0);
    HW_MODIFY_1F(oring, hwStrHeight, 30);
    HW_MODIFY_3F(oring, hwStrColor, 0.0, 1.0, 0.0);
    HW_MODIFY_1F(oring, hwStrShininess, 0.1);
    HW_MODIFY_3F(oring, hwStrPos, 0.0, 0.0, -15.0);


    /* Create inside ring of the bearing */

    iring = hwCone->create(hwCone);
    HW_MODIFY_1I(iring, hwStrGraphN, 32);
    HW_MODIFY_2F(iring, hwStrRadius, 7.3, 7.3);
    HW_MODIFY_1F(iring, hwStrHeight, 30);
    HW_MODIFY_1B(iring, hwStrBackface, HW_TRUE);
    HW_MODIFY_3F(iring, hwStrColor, 0.0, 1.0, 0.0);
    HW_MODIFY_1F(iring, hwStrShininess, 0.1);
    HW_MODIFY_3F(iring, hwStrPos, 0.0, 0.0, -15.0);

    /* Create a cap on the egde of the two rings */

    lcap = hwRing->create(hwRing);
    HW_MODIFY_1I(lcap, hwStrGraphN, 32);
    HW_MODIFY_2F(lcap, hwStrRadius, 7.3, 8.0);
    HW_MODIFY_3F(lcap, hwStrColor, 0.0, 1.0, 0.0);
    HW_MODIFY_1F(lcap, hwStrShininess, 0.1);
    HW_MODIFY_3F(lcap, hwStrPos, 0.0, 0.0, -15.0);

    rcap = hwRing->create(hwRing);
    HW_MODIFY_1I(rcap, hwStrGraphN, 32);
    HW_MODIFY_2F(rcap, hwStrRadius, 7.3, 8.0);
    HW_MODIFY_3F(rcap, hwStrColor, 0.0, 1.0, 0.0);
    HW_MODIFY_1B(rcap, hwStrBackface, HW_TRUE);
    HW_MODIFY_1F(rcap, hwStrShininess, 0.1);
    HW_MODIFY_3F(rcap, hwStrPos, 0.0, 0.0, 15.0);
#endif

    /* Create the ball bearings */

    theta = 0.0;
    radius = 9.0;
    numRows = (float)NUM_BALLS / 24.0;

    for(i=0; i< NUM_BALLS; i++ ) {
        cosTheta = (float)cos((double)(theta * 3.14159 / 180.0));
        sinTheta = (float)sin((double)(theta * 3.14159 / 180.0));

        x = radius * cosTheta;
        y = radius * sinTheta;
        z = (i / 25 - (numRows / 2.0)) * 3.0;

        if( theta >= 360.0 ) 
            theta = 0.0;

        balls[i].hwobj = hwSphere->create(hwSphere);
        HW_MODIFY_3F( balls[i].hwobj, hwStrColor, 1.0, 0.0, 0.0);
        HW_MODIFY_3F( balls[i].hwobj, hwStrPos, x,y,z);
        HW_MODIFY_1F( balls[i].hwobj, hwStrRadius, 1.0);
        HW_MODIFY_1F(balls[i].hwobj, hwStrShininess, 0.2);
        HW_MODIFY_1I(balls[i].hwobj, hwStrGraphN, BALL_N);
        HW_MODIFY_1I(balls[i].hwobj, hwStrGraphM, BALL_M);

        balls[i].hwobj->draw( balls[i].hwobj );
        balls[i].hwobj->inquire(balls[i].hwobj, hwStrBounds,  
           (void **)(&bbox) );

        memcpy( balls[i].bounds, bbox, 6 * sizeof(float) );
        /*
        printf("Bounds for Sphere %d :  ", i);
        for(j=0; j<6; j++)
            printf("%3.2f, ", balls[i].bounds[j]);
        printf("\n");
        */

        /* Generate a display list for the wire frame bounding box */

        balls[i].bounds_id = glGenLists( 1 );

        x1 = balls[i].bounds[0];
        y1 = balls[i].bounds[1];
        z1 = balls[i].bounds[2];
        x2 = balls[i].bounds[3];
        y2 = balls[i].bounds[4];
        z2 = balls[i].bounds[5];

        glNewList( balls[i].bounds_id, GL_COMPILE );
            glBegin(GL_LINE_STRIP);
                glVertex3f(x1,y1,z1);
                glVertex3f(x1,y1,z2);
                glVertex3f(x1,y2,z2);
                glVertex3f(x1,y2,z1);
                glVertex3f(x1,y1,z1);

                glVertex3f(x2,y1,z1);
                glVertex3f(x2,y1,z2);
                glVertex3f(x2,y2,z2);
                glVertex3f(x2,y2,z1);
                glVertex3f(x2,y1,z1);

                glVertex3f(x1,y2,z1);
                glVertex3f(x2,y2,z1);
                glVertex3f(x2,y1,z2);
                glVertex3f(x1,y1,z2);
                glVertex3f(x2,y2,z2);
                glVertex3f(x1,y2,z2);

            glEnd();
        glEndList();
        balls[i].visible_now = 1;
        balls[i].visible_last = 1;
        balls[i].fixup = 0;

        theta += 15;
    }

    disp->inputHandler( disp, eventHandler );

    while( !Done ) {
        if( resized ) {
            disp->viewport( disp, 0, 0, winWidth, winHeight );
            resized = 0;
        }
        env->draw( env );

        start_timer();

        cam->draw( cam );

        light->draw( light );
        light2->draw( light2 );

        hwRotateY(rotMat, CamAng );
        disp->pushMatrix(disp, rotMat);

#ifdef DO_SWEEPS
        iring->draw(iring );
#else
        iring->draw(iring);
        oring->draw(oring);
        lcap->draw(lcap);
        rcap->draw(rcap);
#endif

        plane1->draw(plane1);
        plane2->draw(plane2);

        switch( OccludeMode ) {
            case OCCLUDE_MODE_NONE:
                draw_balls_all(balls, NUM_BALLS);
            break;
            case OCCLUDE_MODE_OCCLUDE:
                draw_balls_occlusion(balls, NUM_BALLS);
            break;
            case OCCLUDE_MODE_VISTEST:
                draw_balls_new(balls, NUM_BALLS);
            break;
        }

        disp->update( disp, HW_UPDATE_ALL );

        stop_timer();

        frames++;
        elapsed += get_elapsed_time();

        if(elapsed > 1.0 ) {
            if(PrintOccStats ) {
                printf("Tests: %d  Occluded: %d  Ratio: %1.2f ",
                num_tests, num_occludes, (float)num_occludes / num_tests);
                num_tests =0;
                num_occludes = 0;
            }
            printf("FPS: %.2f  Angle: %d \r", (frames / elapsed), (int)CamAng );
            fflush(stdout);
            frames = 0;
            elapsed = 0;
        }


        if(! Paused)
            CamAng += 1.0;
        if( CamAng > 360.0 ) CamAng -= 360.0;
        if( CamAng < 0.0 ) CamAng += 360.0;
    }

    exit( 0 );
}
