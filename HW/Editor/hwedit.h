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

/* Header file for HW editor */
#include <stdio.h>
#include <stdlib.h>
#include "hw.h"
#include "ipc.h"

/* Flag definitions */
#define OF_SELECTED     0x00000001      /* This object is selected */

typedef struct _ol {
    hwObject
        obj;            /* The selected object */
    int
        flags;
    struct _ol
        *prev,          /* Previous in double-linked list */
        *next;          /* Next in double-linked list */
} *ObjectList;

typedef struct _ed {
    ObjectList
        list;           /* Previous list of objects */
    int
        scalar;         /* Is this a scalar object? */
    hwObject
        obj;            /* The object whose property was pushed */
    struct _ed
        *next;          /* Next on the stack */
    char
        prop[1];        /* Which property was pushed */
} *EditStack;

typedef struct _cmd {
    char
        *name, *template;
    int
        nargs;
    int
        (*execute)( int nargs, char **args );
    struct _cmd
        *next;
} *Command;

typedef struct _cmd_stream {
    int
        (*ready)( struct _cmd_stream * );       /* Is input pending? */
    int
        (*getString)( struct _cmd_stream *, char *, int );/* Get a string */
    int
        (*putString)( struct _cmd_stream *, char * );   /* Ouput a string */
    void
        (*destroy)( struct _cmd_stream * );     /* Close+destroy the stream */
} *CmdStream;

typedef struct {
    int
        eventType,
        eventButton,
        eventMods,
        eventX, eventY;
} EventQueue;
#define MAX_EVENT_QUEUE 256

/* Mouse commands */
#define MOUSE_SELECT    0
#define MOUSE_VIEW      1
#define MOUSE_MOVE      2
#define MOUSE_SIZE      3
#define MOUSE_ROTATE    4

/* Event types */
#define EVENT_NONE      0
#define EVENT_CLICK     1
#define EVENT_RELEASE   2
#define EVENT_MOVE      3
#define EVENT_KEY       4
#define EVENT_RESIZE    5

/* Event modifiers */
#define EVENT_MOD_SHIFT 0x00000001
#define EVENT_MOD_CTRL  0x00000002

/* Number of command streams supported */
#define NUM_SOCKS       8

/* Global data structure */
typedef struct {
    char
        *fname;         /* File name */
    ObjectList
        olist;          /* List of all objects */
    EditStack
        stack;          /* Stack of nested edits */
    char
        *fileName;      /* Name of file to save */
    hwDisplay
        disp;           /* Display object */
    hwDrawable
        draw;           /* Window to draw HW objects on */
    hwObject
        cam, env, light;        /* Camera + global environment + a light */
    float
        camPos[3],      /* Camera parms: position of point-of-interest... */
        camDir[3],      /*              direction... */
        camUp[3],       /*              up vector... */
        camRight[3],    /*              right vector... */
        camField,       /*              field of view, deg. */
        camDist,        /*              dist. to point-of-interest */
        camPlanes[2];   /*              front + back planes */
    int
        camPersp,       /*              perspective view? */
        wireframe,      /* Wireframe mode? */
        saveX, saveY,   /* Orig. mouse click locations */
        currX, currY,   /* Current mouse motion locations */
        winW, winH,     /* Size of edit window */
        buttonDown,     /* Is a button down? */
        mouseCommand,   /* Which command is mouse doing? */
        needRedraw,     /* Need to redraw scene? */
        animating,      /* Run animations? */
        dirty,          /* Scene's been modified... */
        nq;             /* Number of events queued */
    EventQueue
        eq[MAX_EVENT_QUEUE];    /* The event queue */
    hwInt32
        renderMode,
        selectMode,
        visibility;
    Command
        cmdList,        /* List of possible commands */
        cmdTail;        /* Tail of list */
    OS_DISPLAY_TYPE
        *xDisp;
    OS_DRAWABLE_TYPE
        win;
    CmdStream
        activeCmd,              /* Currently active command stream */
        cmd[NUM_SOCKS];         /* I/O stream for commands */
    IPC_SOCK
        serverSocket,
        socks[NUM_SOCKS];
    int
        numClients;
} Globals;

extern Globals
    edit;

/* Utility functions */
extern void
    initGUI( int, char ** ),
    initGlobals( int, char ** ),
    resetCamera( void ),
    interpCommands( CmdStream );

/* Command stream routines */
CmdStream
    /* *createFilterStream( char *path ) TBD */
    /* *createGuiStream( void ) TBD */
    createConsStream( int ),
    createCharStream( char *buf ),
    createFileStream( FILE *inbuf, FILE *outbuf );

/* Command functions */
extern void
    registerCommand( char *cmd, char *template,
                        int nargs, int (*exec)( int, char ** ) );
extern char
    *execCommand( char *cmd, int nargs, char **args );

/* Parsing functions */
extern char
    *objectToString( hwObject );
extern hwObject
    stringToObject( char * );
extern ObjectList
    findObject( hwObject );
extern hwInt32
    getValue( char **, void **, int * );
extern void
    printProp( hwInt32, void * );

/* Camera functions */
extern void
    setCam( void ),
    yawCam( hwFloat ),
    pitchCam( hwFloat ),
    rollCam( hwFloat );

/* The commands themselves */
extern int
    cmdOpen( int, char ** ),
    cmdRevert( int, char ** ),
    cmdNew( int, char ** ),
    cmdSave( int, char ** ),
    cmdSaveAs( int, char ** ),
    cmdQuit( int, char ** ),
    cmdDuplicate( int, char ** ),
    cmdDelete( int, char ** ),
    cmdUndo( int, char ** ),
    cmdRedo( int, char ** ),
    cmdSelectAll( int, char ** ),
    cmdUnselectAll( int, char ** ),
    cmdSelectXY( int, char ** ),
    cmdSelectMouse( int, char ** ),
    cmdSelectObject( int, char ** ),
    cmdUnselectObject( int, char ** ),
    cmdListSelected( int, char ** ),
    cmdListObjects( int, char ** ),
    cmdGetClass( int, char ** ),
    cmdPushSubset( int, char ** ),
    cmdPopSubset( int, char ** ),
    cmdGetProp( int, char ** ),
    cmdSetProp( int, char ** ),
    cmdGetObjProp( int, char ** ),
    cmdSetObjProp( int, char ** ),
    cmdListProps( int, char ** ),
    cmdNewObject( int, char ** ),
    cmdNewNamedObject( int, char ** ),
    cmdTranslate( int, char ** ),
    cmdRotate( int, char ** ),
    cmdScale( int, char ** ),
    cmdResetView( int, char ** ),
    cmdCamPos( int, char ** ),
    cmdCamField( int, char ** ),
    cmdCamDir( int, char ** ),
    cmdCamUp( int, char ** ),
    cmdCamPlanes( int, char ** ),
    cmdCamDist( int, char ** ),
    cmdVisibility( int, char ** ),
    cmdMouseView( int, char ** ),
    cmdMouseSelect( int, char ** ),
    cmdMouseMove( int, char ** ),
    cmdMouseSize( int, char ** ),
    cmdWireframe( int, char ** ),
    cmdAnimate( int, char ** ),
    cmdButtonDown(int, char **),
    cmdButtonUp(  int, char **),
    cmdMotion(  int, char **),
    cmdHelp( int, char ** );
