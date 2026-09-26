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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "hwedit.h"

Globals
    edit;

int
    WinX = 0,
    WinY = 0,
    WindowID = -1,
    WinWidth=512,
    WinHeight=512;
extern FILE
    *logFile;

void
    eventHandler( hwDrawable draw, hwWinEvent *event );

void drawHighlight( hwObject );
void handleClick( EventQueue * );
void handleDrag( EventQueue * );
void handleRelease( EventQueue * );


int main( int argc, char **argv )
{
    ObjectList
        curr;
    int
        i, n;

#ifdef DO_LOGGING
    if( (logFile = fopen("hwedit.log", "w")) == NULL ) {
        printf("Cannot open hwedit.log! Exiting\n");
        exit(-1);
    }
    fprintf(logFile,"hwedit beginning\n");
    fflush(logFile);
#endif

    initGUI( argc, argv );
    initGlobals( argc, argv );

    while( 1 ) {
        /* Check for socket attaches */
        n = edit.numClients ? 500 : 500000;
        i = IPCServer( edit.serverSocket, n, edit.socks, NUM_SOCKS );
        if (i > 0) {
            edit.numClients++;
            edit.cmd[i-1] = createConsStream( i-1 );
            if (!edit.cmd[i-1]) {
                edit.numClients--;
                edit.socks[i-1] = -1;
            }
        }

        if( !edit.nq && !edit.animating && edit.numClients ) {
            for (i = 0; i < NUM_SOCKS; i++) {
                if (edit.socks[i] >= 0) {
                    if( edit.cmd[i]->ready( edit.cmd[i] ) ) {
                        interpCommands( edit.cmd[i] );
                    }
                }
            }
        }

        edit.disp->update( edit.disp, HW_CHECK_EVENTS );

        for( i = 0; i < edit.nq; i++ ) {
            switch( edit.eq[i].eventType ) {
            case EVENT_CLICK :
                handleClick( edit.eq + i );
                break;
            case EVENT_RELEASE :
                handleRelease( edit.eq + i );
                break;
            case EVENT_MOVE :
                handleDrag( edit.eq + i );
                break;
            case EVENT_KEY :
                /* TBD */
                break;
            case EVENT_RESIZE :
                edit.winW = edit.eq[i].eventX;
                edit.winH = edit.eq[i].eventY;
                edit.disp->viewport( edit.disp, 0, 0, edit.winW, edit.winH );
                edit.disp->update( edit.disp, HW_UPDATE_ALL );
                edit.needRedraw = 1;
                break;
            }
        }
        edit.nq = 0;

        if( edit.needRedraw ) {
            edit.disp->setVisibility( edit.disp, edit.visibility );
            if( !edit.animating ) {
                edit.needRedraw = 0;
                edit.env->draw( edit.env );
                edit.cam->draw( edit.cam );
                edit.light->draw( edit.light );
            }
            for( curr = edit.olist; curr; curr = curr->next ) {
                curr->obj->draw( curr->obj );
                if( curr->flags & OF_SELECTED ) {
                    drawHighlight( curr->obj );
                }
            }
            edit.disp->update( edit.disp, HW_UPDATE_ALL );
        }
    }
}

void initGUI( int argc, char **argv )
{
    int i;
    for(i=0; i<argc; i++) {
        if( strcmp(argv[i], "-width") == 0 ) {
            WinWidth = atoi(argv[++i]);
            printf("Window Width = %d\n", WinWidth);
        }
        if( strcmp(argv[i], "-height") == 0 ) {
            WinHeight = atoi(argv[++i]);
            printf("Window Height = %d\n", WinHeight);
        }
        if( strcmp(argv[i], "-x") == 0 ) {
            WinX = atoi(argv[++i]);
            printf("Window X = %d\n", WinX);
        }
        if( strcmp(argv[i], "-y") == 0 ) {
            WinY = atoi(argv[++i]);
            printf("Window Y = %d\n", WinY);
        }
        if( strcmp(argv[i], "-windowid") == 0) {
            i++;
            printf("Window Id Specified: %s\n", argv[i]);
            sscanf(argv[i], "%x", &WindowID);
            printf("WindowID: 0x%x\n", WindowID);
        }
    }
    /* TBD */
}

void interpCommands( CmdStream str )
{
    char
        buf[1024], *ptr, *cmd, *res,
        *args[256];
    int
        nargs,
        match;

    edit.activeCmd = str;
    if( str->getString( str, buf, 1024 ) ) {
        nargs = 0; ptr = buf;

        /* Extract command */
        while( isspace( *ptr ) ) ptr++;
        cmd = ptr;
        while( *ptr && !isspace( *ptr ) ) ptr++;
        if( *ptr ) *ptr++ = 0;

        while( *ptr ) {
            while( isspace( *ptr ) ) ptr++;
            if( *ptr ) {
                args[nargs++] = ptr;
                if( *ptr == '\"' ) {
                    /* String argument */
                    do {
                        ptr++;
                    } while( *ptr && (!ptr != '\"') );
                    if( *ptr ) ptr++;
                }
                else if( *ptr == '{' ) {
                    /* Array argument */
                    match = 1;
                    do {
                        ptr++;
                        if( *ptr == '{' ) match++;
                        else if( *ptr == '}' ) match--;
                    } while( *ptr && match );
                    if( *ptr ) ptr++;
                }
                else {
                    /* Otherwise */
                    while( *ptr && !isspace( *ptr ) ) ptr++;
                }
                if( *ptr ) *ptr++ = 0;
            }
        }

        res = execCommand( buf, nargs, args );
        str->putString(str, res);
    }
}

void resetCamera( void )
{
    edit.camPos[0] = 0.0;
    edit.camPos[1] = 0.0;
    edit.camPos[2] = 0.0;
    edit.camDist = 100.0;
    edit.camDir[0] = 0.0;
    edit.camDir[1] = 0.0;
    edit.camDir[2] = -1.0;
    edit.camUp[0] = 0.0;
    edit.camUp[1] = 1.0;
    edit.camUp[2] = 0.0;
    yawCam( 0.0 );
    edit.camField = 60.0;
    edit.camPlanes[0] = 1.0;
    edit.camPlanes[1] = 200.0;
    edit.camPersp = 1;
    edit.needRedraw = 1;
    setCam();
    edit.cam->modify( edit.cam, hwStrField, HW_TYPE_1F, &edit.camField );
    edit.cam->modify( edit.cam, hwStrPlanes, HW_TYPE_2F, edit.camPlanes );
    edit.cam->modify( edit.cam, hwStrPerspective, HW_TYPE_1B, &edit.camPersp );
}

void initGlobals( int argc, char **argv )
{
    int i;

    /* Initialize socket routines */
    if (!PrepareForIPC()) {
        exit(1);
    }
    edit.serverSocket = InitIPC(NULL, HWE_PORT);
    if (edit.serverSocket < 0 ) {
        edit.serverSocket = InitIPC(NULL, HWE_TCP);
    }
    if (edit.serverSocket < 0 ) {
        edit.serverSocket = InitIPC(NULL, HWE_PORT_AUX);
    }
    if (edit.serverSocket < 0 ) {
        edit.serverSocket = InitIPC(NULL, HWE_TCP_AUX);
    }
    if (edit.serverSocket < 0) {
        exit(1);
    }

    for (i = 0; i < NUM_SOCKS; i++) {
        edit.socks[i] = -1;
        edit.cmd[i] = NULL;
    }
    edit.numClients = 0;

    if (!hwInit( argc, argv ) ) exit(1);

    edit.disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !edit.disp ) exit( 1 );

    if( !edit.disp->chooseVisual(edit.disp, HW_VIS_DBUFF|HW_VIS_DEPTH,0)){
        exit( 1 );
    }
    edit.winW = WinWidth;
    edit.winH = WinHeight;
    if(WindowID == -1 ) {
        edit.draw = edit.disp->createWindow( edit.disp, "HW Editor",
                                        WinX, WinY, WinWidth, WinHeight,
                                        HW_WIN_INPUT );
    }
    else {
        edit.draw = edit.disp->createChildWindow( edit.disp, "HW Editor",
                                        (OS_WINDOW_TYPE)WindowID,
                                        WinX, WinY, WinWidth, 
                                        WinHeight, HW_WIN_INPUT );
    }
    if( !edit.draw )    exit( 1 );

    edit.disp->makeCurrent( edit.disp, edit.draw );
    edit.win = edit.disp->extractWindow( edit.disp, edit.draw );

    edit.disp->inputHandler( edit.disp, eventHandler );

    edit.env = hwEnviron->create( hwEnviron );
    if( !edit.env )             exit( 1 );
    HW_MODIFY_3F( edit.env, hwStrBackgroundColor, 0.5, 0.5, 0.5 );

    edit.cam = hwCamera->create( hwCamera );
    if( !edit.cam )             exit( 1 );
    resetCamera();

    edit.light = hwLight->create( hwLight );
    if( !edit.light )   exit( 1 );
    HW_MODIFY_3F( edit.light, hwStrDir, -1.0, -1.0, 1.0 );

    edit.env->draw( edit.env );
    edit.disp->update( edit.disp, HW_UPDATE_ALL );

    edit.wireframe = 0;
    edit.buttonDown = 0;
    edit.mouseCommand = MOUSE_SELECT;
    edit.nq = 0;
    edit.needRedraw = 0;
    edit.dirty = 0;
    edit.visibility = 0xFFFFFFFF;
    edit.renderMode = HW_RENDER_DEFAULT;
    edit.selectMode = HW_SELECT_CULL_FACE;

    edit.disp->setVisibility( edit.disp, edit.visibility );
    edit.disp->renderMode( edit.disp, edit.renderMode);

    registerCommand( "Open", "<filename>", 1, cmdOpen );
    registerCommand( "Revert", "", 0, cmdRevert );
    registerCommand( "New", "", 0, cmdNew );
    registerCommand( "Save", "", 0, cmdSave );
    registerCommand( "SaveAs", "<filename>", 1, cmdSaveAs );
    registerCommand( "Quit", "", 0, cmdQuit );
    registerCommand( "Duplicate", "", 0, cmdDuplicate );
    registerCommand( "Delete", "", 0, cmdDelete );
    registerCommand( "Undo", "", 0, cmdUndo );
    registerCommand( "Redo", "", 0, cmdRedo );
    registerCommand( "SelectAll", "", 0, cmdSelectAll );
    registerCommand( "UnselectAll", "", 0, cmdUnselectAll );
    registerCommand( "SelectXY", "<x> <y>", 2, cmdSelectXY );
    registerCommand( "SelectMouse", "", 0, cmdSelectMouse );
    registerCommand( "SelectObject", "<objname|objnum>", 1, cmdSelectObject );
    registerCommand( "UnselectObject", "<objname|objnum>", 1,
                        cmdUnselectObject );
    registerCommand( "ListSelected", "", 0, cmdListSelected );
    registerCommand( "ListObjects", "", 0, cmdListObjects );
    registerCommand( "PushSubset", "<object> <propname>", 2, cmdPushSubset );
    registerCommand( "PopSubset", "", 0, cmdPopSubset );
    registerCommand( "GetClass", "<objname|objnum>", 1, cmdGetClass );
    registerCommand( "GetProp", "<propname>", 1, cmdGetProp );
    registerCommand( "SetProp", "<propname> <value>", 2, cmdSetProp );
    registerCommand( "GetObjProp", "<objname|objnum> <propname>",
                        2, cmdGetObjProp );
    registerCommand( "SetObjProp", "<objname|objnum> <propname> <value>",
                        3, cmdSetObjProp );
    registerCommand( "ListProps", "<objname|objnum>", 1, cmdListProps );
    registerCommand( "NewObject", "<class>", 1, cmdNewObject );
    registerCommand( "NewNamedObject", "<class> <objname>", 2,
                        cmdNewNamedObject );
    registerCommand( "Translate", "<x> <y> <z>", 3, cmdTranslate );
    registerCommand( "Rotate", "<cx> <cy> <cz> <rx> <ry> <rz>", 6, cmdRotate );
    registerCommand( "Scale", "<cx> <cy> <cz> <sx> <sy> <sz>", 6, cmdScale );
    registerCommand( "ResetView", "", 0, cmdResetView );
    registerCommand( "CamPos", "<x> <y> <z>", 3, cmdCamPos );
    registerCommand( "CamField", "<degrees>", 1, cmdCamField );
    registerCommand( "CamDir", "<x> <y> <z>", 3, cmdCamDir );
    registerCommand( "CamUp", "<x> <y> <z>", 3, cmdCamUp );
    registerCommand( "CamPlanes", "<near> <far>", 2, cmdCamPlanes );
    registerCommand( "CamDist", "<dist>", 1, cmdCamDist );
    registerCommand( "Visibility", "<hex-bits>", 1, cmdVisibility );
    registerCommand( "MouseView", "", 0, cmdMouseView );
    registerCommand( "MouseSelect", "", 0, cmdMouseSelect );
    registerCommand( "MouseMove", "", 0, cmdMouseMove );
    registerCommand( "Wireframe", "{true|false}", 1, cmdWireframe );
    registerCommand( "Animate", "", 0, cmdAnimate );
    registerCommand( "ButtonDown","<x> <y> <button> <state>",4,cmdButtonDown);
    registerCommand( "ButtonUp",  "<x> <y> <button> <state>",4,cmdButtonUp);
    registerCommand( "Motion",  "<x> <y> <state>",3,cmdMotion);
#if 0//ndef WIN32
    registerCommand( "WindowResize",  "<width> <height>",2,cmdWindowResize);
#endif
    registerCommand( "Help", "", 0, cmdHelp );
}

void registerCommand
(
    char *cmd, char *template,
    int nargs, int (*exec)( int, char ** )
)
{
    Command
        result;

    result = malloc( sizeof( struct _cmd ) );
    if( !result ) exit( 1 );

    result->name = cmd;
    result->template = template;
    result->nargs = nargs;
    result->execute = exec;
    result->next = 0;
    if( edit.cmdTail ) {
        edit.cmdTail->next = result;
    }
    else {
        edit.cmdList = result;
    }
    edit.cmdTail = result;
    /* TBD: Hash table for these things */
}

char *execCommand( char *cmd, int nargs, char **args )
{
    Command
        curr;
    char
        *a, *b;

    for( curr = edit.cmdList; curr; curr = curr->next ) {
        a = curr->name;
        b = cmd;
        while( *a && *b && (tolower(*a) == tolower(*b)) ) {
            a++; b++;
        }
        if( !*a && !*b )        break;
    }
    if( !curr ) return COMMAND_INVALID;
    if( curr->nargs != nargs ) return COMMAND_BADARGS;
    if( !curr->execute( nargs, args ) ) return COMMAND_FAILED;
    return COMMAND_DONE;
}

static void interp( hwFloat dst[3], hwFloat s1[3], hwFloat s2[3], hwFloat pct )
{
    dst[0] = (1.f - pct) * s1[0] + pct * s2[0];
    dst[1] = (1.f - pct) * s1[1] + pct * s2[1];
    dst[2] = (1.f - pct) * s1[2] + pct * s2[2];
}

void drawHighlight( hwObject obj )
{
    hwInt32
        type;
    hwFloat
        *bounds,
        sx, sy, sz,
        tx, ty, tz,
        mat[4][4];
    hwSurfaceType
        surf;
    static hwFloat
        poly[48*4] = {
            /* Z=0 face, Y=0 edge */
            -0.05, -0.05, -0.05, 0.00,
             0.25, -0.05, -0.05, 1.00,
             0.75, -0.05, -0.05, 0.00,
             1.05, -0.05, -0.05, 1.00,

            /* Z=0 face, Y=1 edge */
            -0.05,  1.05, -0.05, 0.00,
             0.25,  1.05, -0.05, 1.00,
             0.75,  1.05, -0.05, 0.00,
             1.05,  1.05, -0.05, 1.00,

            /* Z=0 face, X=0 edge */
            -0.05, -0.05, -0.05, 0.00,
            -0.05,  0.25, -0.05, 1.00,
            -0.05,  0.75, -0.05, 0.00,
            -0.05,  1.05, -0.05, 1.00,

            /* Z=0 face, X=1 edge */
             1.05, -0.05, -0.05, 0.00,
             1.05,  0.25, -0.05, 1.00,
             1.05,  0.75, -0.05, 0.00,
             1.05,  1.05, -0.05, 1.00,

            /* Z=1 face, Y=0 edge */
            -0.05, -0.05,  1.05, 0.00,
             0.25, -0.05,  1.05, 1.00,
             0.75, -0.05,  1.05, 0.00,
             1.05, -0.05,  1.05, 1.00,

            /* Z=1 face, Y=1 edge */
            -0.05,  1.05,  1.05, 0.00,
             0.25,  1.05,  1.05, 1.00,
             0.75,  1.05,  1.05, 0.00,
             1.05,  1.05,  1.05, 1.00,

            /* Z=1 face, X=0 edge */
            -0.05, -0.05,  1.05, 0.00,
            -0.05,  0.25,  1.05, 1.00,
            -0.05,  0.75,  1.05, 0.00,
            -0.05,  1.05,  1.05, 1.00,

            /* Z=1 face, X=1 edge */
             1.05, -0.05,  1.05, 0.00,
             1.05,  0.25,  1.05, 1.00,
             1.05,  0.75,  1.05, 0.00,
             1.05,  1.05,  1.05, 1.00,

            /* X=0 face, Y=0 edge */
            -0.05, -0.05, -0.05, 0.00,
            -0.05, -0.05,  0.25, 1.00,
            -0.05, -0.05,  0.75, 0.00,
            -0.05, -0.05,  1.05, 1.00,

            /* X=0 face, Y=1 edge */
            -0.05,  1.05, -0.05, 0.00,
            -0.05,  1.05,  0.25, 1.00,
            -0.05,  1.05,  0.75, 0.00,
            -0.05,  1.05,  1.05, 1.00,

            /* X=1 face, Y=0 edge */
             1.05, -0.05, -0.05, 0.00,
             1.05, -0.05,  0.25, 1.00,
             1.05, -0.05,  0.75, 0.00,
             1.05, -0.05,  1.05, 1.00,

            /* X=1 face, Y=1 edge */
             1.05,  1.05, -0.05, 0.00,
             1.05,  1.05,  0.25, 1.00,
             1.05,  1.05,  0.75, 0.00,
             1.05,  1.05,  1.05, 1.00,
        };

    type = obj->inquire( obj, hwStrBounds, (void **)&bounds );
    if( !type ) return;

    sx = bounds[3] - bounds[0];
    sy = bounds[4] - bounds[1];
    sz = bounds[5] - bounds[2];
    tx = bounds[0];
    ty = bounds[1];
    tz = bounds[2];
    mat[0][0] = sx; mat[0][1] = 0.; mat[0][2] = 0.; mat[0][3] = 0.;
    mat[1][0] = 0.; mat[1][1] = sy; mat[1][2] = 0.; mat[1][3] = 0.;
    mat[2][0] = 0.; mat[2][1] = 0.; mat[2][2] = sz; mat[2][3] = 0.;
    mat[3][0] = tx; mat[3][1] = ty; mat[3][2] = tz; mat[3][3] = 1.;

    hwDefaultSurf( &surf );
    edit.disp->surfAttrs( edit.disp, &surf );
    edit.disp->pushMatrix( edit.disp, mat );
    edit.disp->drawPolyline( edit.disp, poly, HW_DATA_MD_FLAGS, 48 );
    edit.disp->popMatrix( edit.disp );
}

void handleClick( EventQueue *eq )
{
    ObjectList
        curr;
    hwObject
        sel;

    if( edit.animating ) {
        edit.animating = 0;
        return;
    }

    edit.saveX = edit.currX = eq->eventX;
    edit.saveY = edit.currY = eq->eventY;
    edit.buttonDown |= (1 << eq->eventButton);
    switch( edit.mouseCommand ) {
    case MOUSE_SELECT :
        edit.disp->selectionInfo( edit.disp, edit.selectMode,
                        (float)edit.currX, (float)edit.currY,
                        1.0f );
        edit.disp->renderMode( edit.disp, HW_RENDER_SELECT );

        edit.disp->update( edit.disp, HW_UPDATE_MATRIX );
        edit.cam->draw( edit.cam );

        for( curr = edit.olist; curr; curr = curr->next ) {
            if( !eq->eventMods ) {
                curr->flags &= ~OF_SELECTED;
            }
            edit.disp->currentObject( edit.disp, curr->obj );
            curr->obj->draw( curr->obj );
        }
        edit.disp->renderMode( edit.disp, edit.renderMode );
        if( edit.disp->wasSelected( edit.disp, &sel, 0 ) ) {
            curr = findObject( sel );
            if( curr ) {
                curr->flags ^= OF_SELECTED;
            }
        }
        edit.disp->update( edit.disp, HW_UPDATE_MATRIX );

        edit.needRedraw = 1;
        break;
    }
}

void handleDrag( EventQueue *eq )
{
    hwFloat
        dx, dy,
        *oldPos,
        newPos[3],
        dPos[3];
    hwInt32
        type;
    void
        *val;
    ObjectList
        curr;
    hwObject
        obj;

    if( !edit.buttonDown ) return;
    switch( edit.mouseCommand ) {
    case MOUSE_VIEW :
        if( edit.buttonDown == 1 ) {
            dx = (eq->eventX - edit.currX) / (hwFloat)edit.winW;
            dy = (eq->eventY - edit.currY) / (hwFloat)edit.winH;
            if( eq->eventMods ) {
                rollCam( -2.f * dx * 3.141592653589 );
            }
            else {
                yawCam( -2.f * dx * 3.141592653589 );
                pitchCam( -2.f * dy * 3.141592653589 );
            }
            setCam();
            edit.needRedraw = 1;
        }
        else {
            dx = (eq->eventX - edit.currX) / (hwFloat)edit.winW;
            dy = (eq->eventY - edit.currY) / (hwFloat)edit.winH;
            dx *= -edit.camDist; dy *= edit.camDist;
            if( eq->eventMods ) {
                edit.camPos[0] += dx*edit.camRight[0] - dy*edit.camDir[0];
                edit.camPos[1] += dx*edit.camRight[1] - dy*edit.camDir[1];
                edit.camPos[2] += dx*edit.camRight[2] - dy*edit.camDir[2];
            }
            else {
                edit.camPos[0] += dx*edit.camRight[0] + dy*edit.camUp[0];
                edit.camPos[1] += dx*edit.camRight[1] + dy*edit.camUp[1];
                edit.camPos[2] += dx*edit.camRight[2] + dy*edit.camUp[2];
            }
            setCam();
            edit.needRedraw = 1;
        }
        break;
    case MOUSE_MOVE :
        dx = (eq->eventX - edit.currX) / (hwFloat)edit.winW;
        dy = (eq->eventY - edit.currY) / (hwFloat)edit.winH;
        dx *= edit.camDist; dy *= -edit.camDist;
        if( eq->eventMods ) {
            dPos[0] = dx*edit.camRight[0] + dy*edit.camDir[0];
            dPos[1] = dx*edit.camRight[1] + dy*edit.camDir[1];
            dPos[2] = dx*edit.camRight[2] + dy*edit.camDir[2];
        }
        else {
            dPos[0] = dx*edit.camRight[0] + dy*edit.camUp[0];
            dPos[1] = dx*edit.camRight[1] + dy*edit.camUp[1];
            dPos[2] = dx*edit.camRight[2] + dy*edit.camUp[2];
        }
        for( curr = edit.olist; curr; curr = curr->next ) {
            if( !(curr->flags & OF_SELECTED) ) continue;
            obj = curr->obj;
            type = obj->inquire( obj, hwStrPos, &val );
            if( type != HW_TYPE_3F ) continue;
            oldPos = val;
            newPos[0] = oldPos[0] + dPos[0];
            newPos[1] = oldPos[1] + dPos[1];
            newPos[2] = oldPos[2] + dPos[2];
            obj->modify( obj, hwStrPos, HW_TYPE_3F, newPos );
        }
        edit.needRedraw = 1;
        break;
    }
    edit.currX = eq->eventX;
    edit.currY = eq->eventY;
}

void handleRelease( EventQueue *eq )
{
    edit.buttonDown &= ~(1 << eq->eventButton);
}

void eventHandler( hwDrawable draw, hwWinEvent *event )
{
    if( edit.nq >= MAX_EVENT_QUEUE ) return;

    switch( event->type ) {
    case HW_INPUT_CONFIG :
        edit.eq[edit.nq].eventX = event->config.width;
        edit.eq[edit.nq].eventY = event->config.height;
        edit.eq[edit.nq].eventType = EVENT_RESIZE;
        edit.nq++;
        break;
    case HW_INPUT_POINTER :
        if( edit.nq && (edit.eq[edit.nq-1].eventType == EVENT_MOVE) ) {
            /* Merge pending motion events */
            edit.nq--;
        }
        edit.eq[edit.nq].eventX = event->pointer.x;
        edit.eq[edit.nq].eventY = event->pointer.y;
        edit.eq[edit.nq].eventType = EVENT_MOVE;
        edit.eq[edit.nq].eventMods = 0;
        if( event->pointer.kbdMods & HW_KBD_MOD_SHIFT ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_SHIFT;
        }
        if( event->pointer.kbdMods & HW_KBD_MOD_CTRL ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_CTRL;
        }
        edit.nq++;
        break;
    case HW_INPUT_BUTTON_PRESS :
        edit.eq[edit.nq].eventX = event->button.x;
        edit.eq[edit.nq].eventY = event->button.y;
        edit.eq[edit.nq].eventButton = event->button.button;
        edit.eq[edit.nq].eventType = EVENT_CLICK;
        edit.eq[edit.nq].eventMods = 0;
        if( event->button.kbdMods & HW_KBD_MOD_SHIFT ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_SHIFT;
        }
        if( event->button.kbdMods & HW_KBD_MOD_CTRL ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_CTRL;
        }
        edit.nq++;
        break;
    case HW_INPUT_BUTTON_RELEASE :
        edit.eq[edit.nq].eventX = event->button.x;
        edit.eq[edit.nq].eventY = event->button.y;
        edit.eq[edit.nq].eventButton = event->button.button;
        edit.eq[edit.nq].eventType = EVENT_RELEASE;
        edit.eq[edit.nq].eventMods = 0;
        if( event->button.kbdMods & HW_KBD_MOD_SHIFT ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_SHIFT;
        }
        if( event->button.kbdMods & HW_KBD_MOD_CTRL ) {
            edit.eq[edit.nq].eventMods |= EVENT_MOD_CTRL;
        }
        edit.nq++;
        break;
    case HW_INPUT_KEYBOARD :
        edit.eq[edit.nq].eventType = EVENT_KEY;
        edit.eq[edit.nq].eventButton = event->keyboard.key;
        edit.nq++;
        break;
    }
}
