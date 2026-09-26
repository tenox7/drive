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
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#if defined(MOTIF_GUI)
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/keysym.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeBG.h>
#endif

#include "global.h"
#include "gauge.h"
#include "windows.h"
#include "drive.h"

#define CONTROLS_HELP_SUBJECT	"CONTROLS"
#define MAX_HELP_SUBJECTS	256

#define CULL_STEP_MULTIPLIER	2.0
#define MAX_CULL_MULTIPLIER	(CULL_STEP_MULTIPLIER*CULL_STEP_MULTIPLIER)
#define MIN_CULL_MULTIPLIER	(1.0/MAX_CULL_MULTIPLIER)

#if defined(MOTIF_GUI)
/* Module globals */
static Widget gearBtn,radarBtn,leaderBtn,incDetailBtn,decDetailBtn,
    vehiclePulldownW,vehBtn[MAX_DRIVEABLES];
static Boolean radarVisible = False,
    leaderVisible = False;
static int numVehicles;


/* This is necessary due to bad behavior -- after pressing a mneumonic,
 * the keyboard focus passes to the button instead of to the window the
 * pointer's in.
 */
#define RESET_KEYBOARD_FOCUS \
{ \
    Window _root,_child; \
    int _root_x,_root_y,_win_x,_win_y; \
    unsigned int _keys_buttons; \
    XQueryPointer(xs.display,DefaultRootWindow(xs.display), \
	&_root,&_child,&_root_x,&_root_y,&_win_x,&_win_y,&_keys_buttons); \
    XSetInputFocus(xs.display,_child,RevertToPointerRoot,CurrentTime); \
    if (cstate.mode & CLIENT_CONFINE_CURSOR_MODE) grabCursor(); \
    else XUngrabPointer(xs.display,CurrentTime); \
}
#else

#define RESET_KEYBOARD_FOCUS

#endif


void toggleGearCallback(
    void)
{
#if defined(MOTIF_GUI)
    Arg arg[MAX_ARG];
    Cardinal ac;
    XmString xmstr;
#endif

    if (gear.type == GEAR_AUTOMATIC) {
	gear.type =  GEAR_STANDARD;
	gear.labels[0] = "R";
	gear.labels[1] = "N";
	gear.labels[2] = "1";
	gear.labels[3] = "2";
	gear.labels[4] = "3";
	gear.labels[5] = "4";
	gear.labels[6] = "5";
	gearDraw(&gear);

#if defined(MOTIF_GUI)
	ac = 0;
	xmstr = XmStringCreateLtoR("Automatic Transmission",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(gearBtn,arg,ac);
	XmStringFree(xmstr);
#endif
    }
    else {
	gear.type = GEAR_AUTOMATIC;
	gear.labels[0] = "R";
	gear.labels[1] = "N";
	gear.labels[2] = "L1";
	gear.labels[3] = "L2";
	gear.labels[4] = "L3";
	gear.labels[5] = "D";
	gear.labels[6] = "OD";
	gearDraw(&gear);

#if defined(MOTIF_GUI)
	ac = 0;
	xmstr = XmStringCreateLtoR("Manual Transmission",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(gearBtn,arg,ac);
	XmStringFree(xmstr);
#endif
    }

    RESET_KEYBOARD_FOCUS;
}


void toggleRadarCallback(
    void)
{
#if defined(MOTIF_GUI)
    Arg arg[MAX_ARG];
    Cardinal ac;
    XmString xmstr;

    if (radarVisible) {
	radarVisible = False;
	unmapRadarWindow();
	ac = 0;
	xmstr = XmStringCreateLtoR("Radar Window On",XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(radarBtn,arg,ac);
	XmStringFree(xmstr);
    }
    else {
	radarVisible = True;
	mapRadarWindow();
	ac = 0;
	xmstr = XmStringCreateLtoR("Radar Window Off",XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(radarBtn,arg,ac);
	XmStringFree(xmstr);
    }

    RESET_KEYBOARD_FOCUS;
#endif
}


void toggleLeaderCallback(
    void)
{
#if defined(MOTIF_GUI)
    Arg arg[MAX_ARG];
    Cardinal ac;
    XmString xmstr;

    if (leaderVisible) {
	leaderVisible = False;
	unmapRaceWindow();

	ac = 0;
	xmstr = XmStringCreateLtoR("Leaders Window On",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(leaderBtn,arg,ac);
	XmStringFree(xmstr);
    }
    else {
	leaderVisible = True;
	mapRaceWindow();

	ac = 0;
	xmstr = XmStringCreateLtoR("Leaders Window Off",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(leaderBtn,arg,ac);
	XmStringFree(xmstr);
    }

    RESET_KEYBOARD_FOCUS;
#endif
}

#if defined(MOTIF_GUI)
static void vehicleConfigCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    printf("vehicleConfig callback\n");
    RESET_KEYBOARD_FOCUS;
}
#endif

void smallWindowCallback(
    void)
{
#if defined(MOTIF_GUI)
    XResizeWindow(xs.display,XtWindow(xs.app_shellW),
	WIN_SMALLWIDTH,WIN_SMALLHEIGHT);
    RESET_KEYBOARD_FOCUS;
#endif
}


static void largeWindowCallback(
    void)
{
#if defined(MOTIF_GUI)
    XResizeWindow(xs.display,XtWindow(xs.app_shellW),
	WIN_LARGEWIDTH,WIN_LARGEHEIGHT);
    RESET_KEYBOARD_FOCUS;
#endif
}


#if defined(MOTIF_GUI)
static void toggleCursorConstraintCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    Arg arg[MAX_ARG];
    Cardinal ac;
    XmString xmstr;

    cstate.mode ^= CLIENT_CONFINE_CURSOR_MODE;

    if (cstate.mode & CLIENT_CONFINE_CURSOR_MODE) {
	ac = 0;
	xmstr = XmStringCreateLtoR("Unconstrain Cursor",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(whichW,arg,ac);
	XmStringFree(xmstr);

	grabCursor();
    }
    else {
	ac = 0;
	xmstr = XmStringCreateLtoR("Constrain Cursor",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetValues(whichW,arg,ac);
	XmStringFree(xmstr);

	XUngrabPointer(xs.display,CurrentTime);
    }


    RESET_KEYBOARD_FOCUS;
}


static void increaseDetailCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    if (IS_NEAR(cstate.cull_multiplier,MIN_CULL_MULTIPLIER)) {
	RESET_KEYBOARD_FOCUS;
	return;
    }

    cstate.cull_multiplier /= CULL_STEP_MULTIPLIER;

    XtSetSensitive(incDetailBtn,
	!IS_NEAR(cstate.cull_multiplier,MIN_CULL_MULTIPLIER));
    XtSetSensitive(decDetailBtn,
	!IS_NEAR(cstate.cull_multiplier,MAX_CULL_MULTIPLIER));

    RESET_KEYBOARD_FOCUS;
}


static void decreaseDetailCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    if (IS_NEAR(cstate.cull_multiplier,MAX_CULL_MULTIPLIER)) {
	RESET_KEYBOARD_FOCUS;
	return;
    }

    cstate.cull_multiplier *= CULL_STEP_MULTIPLIER;

    XtSetSensitive(incDetailBtn,
	!IS_NEAR(cstate.cull_multiplier,MIN_CULL_MULTIPLIER));
    XtSetSensitive(decDetailBtn,
	!IS_NEAR(cstate.cull_multiplier,MAX_CULL_MULTIPLIER));

    RESET_KEYBOARD_FOCUS;
}


static void uprightCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    client_upright();
    RESET_KEYBOARD_FOCUS;
}


static void restartCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    client_restart();
    RESET_KEYBOARD_FOCUS;
}


static void helpCallback(
    Widget whichW,
    char *text,
    XmAnyCallbackStruct *cb)
{
    if (strcmp(text,CONTROLS_HELP_SUBJECT) == 0) {
	mapVisCtrlsWindow();
    }
    else {
	genericHelpCallback(xs.app_shellW,text,NULL);
    }

    RESET_KEYBOARD_FOCUS;
}


static void exitProgram(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    client_quit();
    RESET_KEYBOARD_FOCUS;
}


#define BTN_VEHICLE	0
#define BTN_CONFIG	1
#define BTN_UPRIGHT	2
#define BTN_RESTART	3
#define BTN_QUIT	4
#define BTN_HELP	5
#define NUM_BTNS	(BTN_HELP+1)
#endif

#if defined(MOTIF_GUI)
Widget create_menu_bar(
    Widget parent,
    Arg arg[],
    Cardinal ac)
{
    char *help_subject[MAX_HELP_SUBJECTS];
    int num_help_subjects;
    char *help_subject_text[MAX_HELP_SUBJECTS];
    Widget menuBarW,pulldown[NUM_BTNS],cbutton[NUM_BTNS];
    XmString xmstr;
    int i;


    /* add more options */
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNheight,MENUBAR_HEIGHT); ++ac;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_MODELESS); ++ac;
    menuBarW = XmCreateMenuBar(parent,"menu",arg,ac);


    /*** VEHICLE ***/
    ac = 0;
    pulldown[BTN_VEHICLE] = XmCreatePulldownMenu(menuBarW,"pulldown",arg,ac);
    vehiclePulldownW = pulldown[BTN_VEHICLE];
    ac = 0;
    XtSetArg(arg[ac],XmNsubMenuId,pulldown[BTN_VEHICLE]); ++ac;
    xmstr = XmStringCreateLtoR("Vehicle",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNmnemonic,'V'); ++ac;
    cbutton[BTN_VEHICLE] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    XtAddCallback(cbutton[BTN_VEHICLE],XmNactivateCallback,
	(XtCallbackProc) vehicleConfigCallback,NULL);
    if (cstate.mode & (CLIENT_WATCH_MODE|CLIENT_AUTOWATCH_MODE)) {
	XtSetSensitive(cbutton[BTN_VEHICLE],False);
    }
    if (gear.type == GEAR_AUTOMATIC) {
	gearBtn = createButton(pulldown[BTN_VEHICLE],"Manual Transmission",
	    'T', (XtCallbackProc) toggleGearCallback,NULL);
    }
    else {
	gearBtn = createButton(pulldown[BTN_VEHICLE],"Automatic Transmission",
	    'T',(XtCallbackProc) toggleGearCallback,NULL);
    }

    /*** CONFIG ***/
    ac = 0;
    pulldown[BTN_CONFIG] = XmCreatePulldownMenu(menuBarW,"pulldown",arg,ac);
    ac = 0;
    XtSetArg(arg[ac],XmNsubMenuId,pulldown[BTN_CONFIG]); ++ac;
    xmstr = XmStringCreateLtoR("Windows",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNmnemonic,'W'); ++ac;
    cbutton[BTN_CONFIG] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    createButton(pulldown[BTN_CONFIG],"Small Window",'\0',
	(XtCallbackProc) smallWindowCallback,NULL);
    createButton(pulldown[BTN_CONFIG],"Large Window",'\0',
	(XtCallbackProc) largeWindowCallback,NULL);
    createButton(pulldown[BTN_CONFIG],"Constrain Cursor",'C',
	(XtCallbackProc) toggleCursorConstraintCallback,NULL);
    radarBtn = createButton(pulldown[BTN_CONFIG],"Radar Window On",'R',
	(XtCallbackProc) toggleRadarCallback,NULL);
    leaderBtn = createButton(pulldown[BTN_CONFIG],"Leaders Window On",'L',
	(XtCallbackProc) toggleLeaderCallback,NULL);
    if (cstate.pex_version) {
	incDetailBtn = createButton(pulldown[BTN_CONFIG],"Increase Detail",'I',
	    (XtCallbackProc) increaseDetailCallback,NULL);
	decDetailBtn = createButton(pulldown[BTN_CONFIG],"Decrease Detail",'D',
	    (XtCallbackProc) decreaseDetailCallback,NULL);
    }

    /*** UPRIGHT ***/
    ac = 0;
    xmstr = XmStringCreateLtoR("Upright",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNmnemonic,'U'); ++ac;
    cbutton[BTN_UPRIGHT] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    XtAddCallback(cbutton[BTN_UPRIGHT],XmNactivateCallback,
	(XtCallbackProc) uprightCallback,NULL);
    if (cstate.mode & (CLIENT_WATCH_MODE|CLIENT_AUTOWATCH_MODE)) {
	XtSetSensitive(cbutton[BTN_UPRIGHT],False);
    }

    /*** START OVER / NEXT VEHICLE ***/
    ac = 0;
    if (cstate.mode & (CLIENT_WATCH_MODE|CLIENT_AUTOWATCH_MODE)) {
	xmstr = XmStringCreateLtoR("Next Vehicle",XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetArg(arg[ac],XmNmnemonic,'N'); ++ac;
    }
    else {
	xmstr = XmStringCreateLtoR("Start Over",XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
	XtSetArg(arg[ac],XmNmnemonic,'S'); ++ac;
    }
    cbutton[BTN_RESTART] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    XtAddCallback(cbutton[BTN_RESTART],XmNactivateCallback,
	(XtCallbackProc) restartCallback,NULL);
    if (cstate.mode & CLIENT_AUTOWATCH_MODE) {
	XtSetSensitive(cbutton[BTN_RESTART],False);
    }

    /*** QUIT ***/
    ac = 0;
    xmstr = XmStringCreateLtoR("Quit",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNmnemonic,'Q'); ++ac;
    cbutton[BTN_QUIT] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    XtAddCallback(cbutton[BTN_QUIT],XmNactivateCallback,
	(XtCallbackProc) exitProgram,NULL);


    /*** HELP ***/
    parseHelpFile(help_subject,&num_help_subjects,help_subject_text);
    ac = 0;
    pulldown[BTN_HELP] = XmCreatePulldownMenu(menuBarW,"pulldown",arg,ac);
    ac = 0;
    XtSetArg(arg[ac],XmNsubMenuId,pulldown[BTN_HELP]); ++ac;
    xmstr = XmStringCreateLtoR("Help",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNmnemonic,'H'); ++ac;
    cbutton[BTN_HELP] = XmCreateCascadeButtonGadget(menuBarW,"btn",arg,ac);
    XmStringFree(xmstr);
    createButton(pulldown[BTN_HELP],"Controls",'\0',
	(XtCallbackProc) helpCallback,(void *) CONTROLS_HELP_SUBJECT);
    for (i=0; i<num_help_subjects; ++i) {
	createButton(pulldown[BTN_HELP],help_subject[i],'\0',
	    (XtCallbackProc) helpCallback,(void *) help_subject_text[i]);
    }

    XtManageChildren(cbutton,NUM_BTNS);

    ac = 0;
    XtSetArg(arg[ac],XmNmenuHelpWidget,cbutton[BTN_HELP]); ++ac;
    XtSetValues(menuBarW,arg,ac);

    XtManageChild(menuBarW);
    return(menuBarW);
}


static void vehicleSelectionCallback(
    Widget whichW,
    void *client_data,
    XmAnyCallbackStruct *cb)
{
    select_new_vehicle((char *) client_data);
}
#endif

void updateGUIVehicles(
    Driveable *driveables,
    int num_cars)
{
#if defined(MOTIF_GUI)
    int i;

    numVehicles = num_cars;
    /* Add buttons to Vehicle menu selection */
    for (i=0; i<num_cars; ++i) {
	vehBtn[i] = createButton(vehiclePulldownW,driveables[i].name,
	    '\0', (XtCallbackProc) vehicleSelectionCallback,
	    driveables[i].name);
	XtSetSensitive(vehBtn[i],False);
    }
#endif
}


void updateVehicleSensitivity(
    int state)
{
#if defined(MOTIF_GUI)
    int i;

    if ((state == RACE_STATE)
	    || (state == WELCOME_STATE)) {
	for (i=0; i<numVehicles; ++i) XtSetSensitive(vehBtn[i],False);
    }
    else {
	for (i=0; i<numVehicles; ++i) XtSetSensitive(vehBtn[i],True);
    }
#endif
}
