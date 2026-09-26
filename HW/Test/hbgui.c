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
#include <string.h>
#include "hw.h"

#define MAX_ADDR_TEXT   6
typedef struct {
    hwObject
        hbNumPad, hbAddr0, hbAddr1, hbAddr2, hbAddr3,
        hbDrawWalls, hbDrawFloor, hbDrawRoof, hbDrawLines,
        hbWireframe, hbSolid, hbTextured,
        hbLoRes, hbMedRes, hbHiRes, hbExtRes,
        activeAddr,
        *objects;
    char
        activeAddrText[MAX_ADDR_TEXT+1];
    int
        activeAddrLen,
        menuNum,
        numObjects;
    hwDisplay
        disp;
} hbMenuStruct;

hbMenuStruct
    hbMenu;

/* In all menus */
#define ID_BACK         1000

/* Main menu */
#define MENU_MAIN       0
#define ID_GAME         100
#define ID_OPTIONS      110
#define ID_ABOUT        120
#define ID_HUD          130

/* Game menu */
#define MENU_GAME       1
#define ID_JOIN         200
#define ID_NEXT_PLAYER  210
#define ID_CONNECT      220
#define ID_QUIT         230

/* Options menu */
#define MENU_OPTIONS    2
#define ID_DRAW         300
#define ID_MODE         310
#define ID_DETAIL       320

/* Draw meu */
#define MENU_DRAW       3
#define ID_WALLS        400
#define ID_FLOOR        410
#define ID_ROOF         420
#define ID_LINES        430

/* Mode menu */
#define MENU_MODE       4
#define ID_WIREFRAME    500
#define ID_SOLID        510
#define ID_TEXTURED     520

/* Detail menu */
#define MENU_DETAIL     5
#define ID_LOW_RES      600
#define ID_MED_RES      610
#define ID_HI_RES       620
#define ID_EXT_RES      630

/* About menu */
#define MENU_ABOUT      6

/* Connect menu */
#define MENU_CONNECT    7
#define ID_ADDR_0       800
#define ID_ADDR_1       810
#define ID_ADDR_2       820
#define ID_ADDR_3       830
#define ID_DO_CONN      840

/* Command menu */
#define MENU_COMMAND    8
#define ID_TURBO        900
#define ID_JUMP         910
#define ID_SINK         920
#define ID_OPT_MENU     930
#define ID_WATCH_BALL   940
#define ID_ALT_VIEW     950
#define ID_SHOOT        960

#define ID_KEYPAD       10000

/* Callbacks into Hoverball */
void SwitchMode( int );
void SwitchResolutionMode( int );
int ConnectToHost( char * );

void hbMenuCB( hwDrawable draw, hwWinEvent *event )
{
    int
        i;
    hwObject
        obj;

    if( event->type == HW_INPUT_CONFIG ) {
        hbMenu.disp->viewport( hbMenu.disp, 0, 0,
                                event->config.width, event->config.height );

        /* Send config to all menus */
        for( i = 0; i < hbMenu.numObjects; i++ ) {
            obj = hbMenu.objects[i];
            obj->modify( obj, hwStrEvent, HW_TYPE_EVENT, event );
        }
    }
    else {
        obj = hbMenu.objects[hbMenu.menuNum];
        obj->modify( obj, hwStrEvent, HW_TYPE_EVENT, event );
    }
}

static void setActiveAddr( hwObject obj )
{
    void
        *ptr;
    char
        *s;
    hwInt32
        color[4];

    hbMenu.activeAddr = obj;
    if( obj->inquire( obj, hwStrLabel, &ptr ) ) {
        s = ptr;
        strncpy( hbMenu.activeAddrText, s, MAX_ADDR_TEXT );
        hbMenu.activeAddrText[MAX_ADDR_TEXT] = 0;
        hbMenu.activeAddrLen = strlen(hbMenu.activeAddrText);
    }
    color[0] = (obj == hbMenu.hbAddr0) ? 0xFFFF0000 : 0xFF808080;
    color[1] = (obj == hbMenu.hbAddr1) ? 0xFFFF0000 : 0xFF808080;
    color[2] = (obj == hbMenu.hbAddr2) ? 0xFFFF0000 : 0xFF808080;
    color[3] = (obj == hbMenu.hbAddr3) ? 0xFFFF0000 : 0xFF808080;
    HW_MODIFY_1I( hbMenu.hbAddr0, hwStrBackgroundColor, color[0] );
    HW_MODIFY_1I( hbMenu.hbAddr1, hwStrBackgroundColor, color[1] );
    HW_MODIFY_1I( hbMenu.hbAddr2, hwStrBackgroundColor, color[2] );
    HW_MODIFY_1I( hbMenu.hbAddr3, hwStrBackgroundColor, color[3] );
}

static void widget( hwObject obj, hwInt32 reason )
{
    void
        *ptr;
    hwInt32
        c, id, val;
    char
        *s0, *s1, *s2, *s3,
        ipaddr[128];

    if( reason == HW_CB_COMPLETE ) {
        if( obj->inquire( obj, hwStrID, &ptr ) != HW_TYPE_1I ) return;
        id = *(hwInt32 *)ptr;
        if( obj->inquire( obj, hwStrValue, &ptr ) == HW_TYPE_1I ) {
            val = *(hwInt32 *)ptr;
        }

        switch( id ) {
        case ID_GAME :
            hbMenu.menuNum = MENU_GAME;
            break;
        case ID_CONNECT :
            hbMenu.menuNum = MENU_CONNECT;
            setActiveAddr( hbMenu.hbAddr0 );
            break;
        case ID_ADDR_0 :
            setActiveAddr( hbMenu.hbAddr0 );
            break;
        case ID_ADDR_1 :
            setActiveAddr( hbMenu.hbAddr1 );
            break;
        case ID_ADDR_2 :
            setActiveAddr( hbMenu.hbAddr2 );
            break;
        case ID_ADDR_3 :
            setActiveAddr( hbMenu.hbAddr3 );
            break;
        case ID_DO_CONN :
            hbMenu.hbAddr0->inquire(hbMenu.hbAddr0, hwStrLabel, &ptr); s0 = ptr;
            hbMenu.hbAddr1->inquire(hbMenu.hbAddr1, hwStrLabel, &ptr); s1 = ptr;
            hbMenu.hbAddr2->inquire(hbMenu.hbAddr2, hwStrLabel, &ptr); s2 = ptr;
            hbMenu.hbAddr3->inquire(hbMenu.hbAddr3, hwStrLabel, &ptr); s3 = ptr;
            sprintf(ipaddr, "%s.%s.%s.%s", s0, s1, s2, s3);
            (void)ConnectToHost( ipaddr );
            hbMenu.menuNum = MENU_GAME;
            break;
        case ID_OPT_MENU :
            hbMenu.menuNum = MENU_MAIN;
            break;
        case ID_OPTIONS :
            hbMenu.menuNum = MENU_OPTIONS;
            break;
        case ID_ABOUT :
            hbMenu.menuNum = MENU_ABOUT;
            break;
        case ID_HUD :
            /*cstate.drawHUD = val;*/
            break;
        case ID_DRAW :
            hbMenu.menuNum = MENU_DRAW;
            break;
        case ID_MODE :
            hbMenu.menuNum = MENU_MODE;
            break;
        case ID_DETAIL :
            hbMenu.menuNum = MENU_DETAIL;
            break;
        case ID_BACK :
            switch( hbMenu.menuNum ) {
            case MENU_MAIN :
                hbMenu.menuNum = MENU_COMMAND;
                break;
            case MENU_GAME :
                hbMenu.menuNum = MENU_MAIN;
                break;
            case MENU_OPTIONS :
                hbMenu.menuNum = MENU_MAIN;
                break;
            case MENU_ABOUT :
                hbMenu.menuNum = MENU_MAIN;
                break;
            case MENU_DRAW :
                hbMenu.menuNum = MENU_OPTIONS;
                break;
            case MENU_MODE :
                hbMenu.menuNum = MENU_OPTIONS;
                break;
            case MENU_DETAIL :
                hbMenu.menuNum = MENU_OPTIONS;
                break;
            case MENU_CONNECT :
                hbMenu.menuNum = MENU_GAME;
                hbMenu.activeAddr = NULL;
                break;
            }
            break;
        case ID_JOIN :
            /* AddCommand( CMD_Join ); */
            break;
        case ID_NEXT_PLAYER :
            /* AddCommand( CMD_NextTarget ); */
            break;
        case ID_QUIT :
            /* ShutdownAudio(); */
            exit( 0 );
            break;
        case ID_WALLS :
            if( val )   /* cstate.DontDrawBits &= ~DONT_DRAW_WALLS */;
            else        /* cstate.DontDrawBits |= DONT_DRAW_WALLS */;
            break;
        case ID_FLOOR :
            if( val )   /* cstate.DontDrawBits &= ~DONT_DRAW_FLOOR */;
            else        /* cstate.DontDrawBits |= DONT_DRAW_FLOOR */;
            break;
        case ID_ROOF :
            if( val )   /* cstate.DontDrawBits &= ~DONT_DRAW_ROOF */;
            else        /* cstate.DontDrawBits |= DONT_DRAW_ROOF */;
            break;
        case ID_LINES :
            if( val )   /* cstate.DontDrawBits &= ~DONT_DRAW_LINES */;
            else        /* cstate.DontDrawBits |= DONT_DRAW_LINES */;
            break;
        case ID_LOW_RES :
            if( val ) SwitchResolutionMode( 1 );
            break;
        case ID_MED_RES :
            if( val ) SwitchResolutionMode( 2 );
            break;
        case ID_HI_RES :
            if( val ) SwitchResolutionMode( 3 );
            break;
        case ID_EXT_RES :
            if( val ) SwitchResolutionMode( 4 );
            break;
        case ID_WIREFRAME :
            if( val ) SwitchMode( 3 );
            break;
        case ID_SOLID :
            if( val ) SwitchMode( 1 );
            break;
        case ID_TEXTURED :
            if( val ) SwitchMode( 2 );
            break;
        default :
            if( id >= ID_KEYPAD ) {
                c = id - ID_KEYPAD;
                if( c == 8 ) {
                    /* Backspace */
                    if( hbMenu.activeAddrLen > 0 ) {
                        hbMenu.activeAddrText[--hbMenu.activeAddrLen] = 0;
                    }
                    hbMenu.activeAddr->modify( hbMenu.activeAddr, hwStrLabel,
                                        HW_TYPE_STRING, hbMenu.activeAddrText );
                }
                else if( c == '.' ) {
                    if( hbMenu.activeAddr == hbMenu.hbAddr0 ) {
                        setActiveAddr( hbMenu.hbAddr1 );
                    }
                    else if( hbMenu.activeAddr == hbMenu.hbAddr1 ) {
                        setActiveAddr( hbMenu.hbAddr2 );
                    }
                    else if( hbMenu.activeAddr == hbMenu.hbAddr2 ) {
                        setActiveAddr( hbMenu.hbAddr3 );
                    }
                }
                else if( hbMenu.activeAddrLen < MAX_ADDR_TEXT ) {
                    hbMenu.activeAddrText[hbMenu.activeAddrLen++] = c;
                    hbMenu.activeAddrText[hbMenu.activeAddrLen] = 0;
                    hbMenu.activeAddr->modify( hbMenu.activeAddr, hwStrLabel,
                                        HW_TYPE_STRING, hbMenu.activeAddrText );
                }
            }
            break;
        }
    }
}

void hbMenuInit( hwDisplay disp, char *filename )
{
    int
        i;
    hwObject
        obj;

    hbMenu.numObjects = hwParseFile( filename, &hbMenu.objects );
    if( !hbMenu.numObjects )    exit( 1 );

    for( i = 0; i < hbMenu.numObjects; i++ ) {
        obj = hbMenu.objects[i];
        obj->modify( obj, hwStrCallback, HW_TYPE_CALLBACK, widget );
    }

    hbMenu.hbNumPad = hwFindObject( "hbNumPad" );

    hbMenu.hbAddr0 = hwFindObject( "hbAddr0" );
    hbMenu.hbAddr1 = hwFindObject( "hbAddr1" );
    hbMenu.hbAddr2 = hwFindObject( "hbAddr2" );
    hbMenu.hbAddr3 = hwFindObject( "hbAddr3" );

    hbMenu.hbDrawWalls = hwFindObject( "hbDrawWalls" );
    hbMenu.hbDrawFloor = hwFindObject( "hbDrawFloor" );
    hbMenu.hbDrawRoof = hwFindObject( "hbDrawRoof" );

    hbMenu.hbWireframe = hwFindObject( "hbWireframe" );
    hbMenu.hbSolid = hwFindObject( "hbSolid" );
    hbMenu.hbTextured = hwFindObject( "hbTextured" );

    hbMenu.hbLoRes = hwFindObject( "hbLoRes" );
    hbMenu.hbMedRes = hwFindObject( "hbMedRes" );
    hbMenu.hbHiRes = hwFindObject( "hbHiRes" );
    hbMenu.hbExtRes = hwFindObject( "hbExtRes" );

    /* Set defaults */
    obj = hbMenu.hbDrawWalls;
    HW_MODIFY_1I( obj, hwStrValue, 1 );

    obj = hbMenu.hbDrawFloor;
    HW_MODIFY_1I( obj, hwStrValue, 1 );

    obj = hbMenu.hbDrawRoof;
    HW_MODIFY_1I( obj, hwStrValue, 1 );

    obj = hbMenu.hbTextured;
    HW_MODIFY_1I( obj, hwStrValue, 1 );

    obj = hbMenu.hbHiRes;
    HW_MODIFY_1I( obj, hwStrValue, 1 );

    hbMenu.disp = disp;
    hbMenu.menuNum = MENU_COMMAND;
}

void hbMenuDisplay( void )
{
    hwObject
        obj;

    obj = hbMenu.objects[hbMenu.menuNum];
    obj->draw( obj );
}

main( int argc, char **argv )
{
    double
        prevTime,
        saveTime;
    hwDrawable
        draw;
    char
        *filename = "hbgui.hw";
    hwObject
        env, cam, tm, sphere, light,
        obj;
    float
        cy, cz, Ang = 0.0, CamAng = 0.0, Mat[4][4];
    int
        i,
        numFrames = 0,
        doTime = 0;
    hwDisplay
        disp;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-time" ) == 0 ) {
            doTime = 1;
        }
        else if( *argv[i] != '-' ) {
            filename = argv[i];
        }
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );

    if( !disp->chooseVisual( disp, HW_VIS_DBUFF, 0 ) )  exit( 1 );

    draw = disp->createWindow( disp, "GUI", 80, 80, 640, 480, HW_WIN_INPUT );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    hbMenuInit( disp, filename );

    /* Create an interesting backdrop */
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
    HW_MODIFY_1B( sphere, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_1B( sphere, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_1I( sphere, hwStrGraphN, 14 );
    HW_MODIFY_1I( sphere, hwStrGraphM, 37 );
    HW_MODIFY_2F( sphere, hwStrLatRange, -45.0, 90.0 );
    HW_MODIFY_1F( sphere, hwStrShininess, 0.1);

    light = hwLight->create( hwLight );
    if( !light )        exit( 1 );
    HW_MODIFY_3F( light, hwStrDir, 1.0, 1.0, -1.0 );

    disp->inputHandler( disp, hbMenuCB );

    saveTime = disp->getElapsedTime(disp);
    while( 1 ) {

        /* Draw the scene */
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
        Ang += 5.0; if( Ang > 360.0 ) Ang -= 360.0;
        CamAng += 4.0; if( CamAng > 360.0 ) CamAng -= 360.0;

        hbMenuDisplay();

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

void SwitchResolutionMode( int mode )
{
    printf("ResMode: %d\n", mode);
}

void SwitchMode( int mode )
{
    printf("Mode: %d\n", mode);
}

int ConnectToHost( char *ip )
{
    printf("Connect: %s\n", ip );
}
