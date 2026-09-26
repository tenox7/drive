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


#if defined(MOTIF_GUI)

#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/ScrollBar.h>

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "windows.h"

#define VS_WIDTH		300
#define OKBTN_WIDTH		50
#define COLOR_SCROLLBAR_MAX	100
#define COLORNAME_STRLEN	(8+20)
#define SLIDER_SIZE		5

static char whichVehicle[256] = "";
static float hue,saturation,brightness;

static void vehicleSelectedCallback(
    Widget whichW,
    void *client_data,
    XmListCallbackStruct *cb)
{

    XmString_to_string(cb->item,whichVehicle);

    /* Good!  Display the new one. */
    cstate.car_name = whichVehicle;
}


static void vehicleOKCallback(
    Widget whichW,
    Widget formW,
    XmAnyCallbackStruct *cb)
{
    if (whichVehicle[0] == '\0') {
	strcpy(whichVehicle,DEFAULT_VEHICLE);
	cstate.car_name = whichVehicle;
    }

    /* Good!  Run with it! */
    cstate.car_selected = TRUE;

    XtUnmanageChild(formW);
    XtDestroyWidget(formW);
    XmUpdateDisplay(formW);
}


static void update_color(
    Widget labelW)
{
    Arg arg[MAX_ARG];
    Cardinal ac;
    float r,g,b;
    char str[256];
    XmString xmstr;

    hsv_to_rgb(hue,saturation,brightness,&r,&g,&b);
    cstate.upd.color[0] = r;
    cstate.upd.color[1] = g;
    cstate.upd.color[2] = b;
    sprintf(str,"Color:  %s",lookup_rgb(r,g,b));

    /* and update labelW */
    ac = 0;
    xmstr = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetValues(labelW,arg,ac);
    XmStringFree(xmstr);
}


static void newHueCallback(
    Widget whichW,
    Widget labelW,
    XmScrollBarCallbackStruct *cb)
{
    hue = (float) cb->value / (COLOR_SCROLLBAR_MAX-SLIDER_SIZE);
    update_color(labelW);
}


static void newBrightnessCallback(
    Widget whichW,
    Widget labelW,
    XmScrollBarCallbackStruct *cb)
{
    brightness = (float) cb->value/((COLOR_SCROLLBAR_MAX-SLIDER_SIZE)*2.0)
	+ 0.5;
    update_color(labelW);
}


static void newSaturationCallback(
    Widget whichW,
    Widget labelW,
    XmScrollBarCallbackStruct *cb)
{
    saturation = (float) cb->value / (COLOR_SCROLLBAR_MAX-SLIDER_SIZE);
    update_color(labelW);
}
#endif


void mapVehicleSelectionWindow(
    void)
{
#if defined MOTIF_GUI
    Arg arg[MAX_ARG];
    Cardinal ac;
    int i;
    XmString selStr[MAX_DRIVEABLES],lStr;
    Widget formW,labelW,vehSelW,buttonW,scrollW;
    char str[256];


    /* set color defaults */
    rgb_to_hsv(cstate.upd.color[0], cstate.upd.color[1], cstate.upd.color[2],
	&hue,&saturation,&brightness);
    if (brightness < 0.5) brightness = 0.5;

    ac = 0;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_MODELESS); ++ac;
    XtSetArg(arg[ac],XmNwidth,VS_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNfractionBase,VS_WIDTH); ++ac;
    formW = XmCreateFormDialog(xs.app_shellW,"Vehicle Selection",arg,ac);
    XtManageChild(formW);

    ac = 0;
    sprintf(str,"%-*s",COLORNAME_STRLEN,"Vehicle");
    lStr = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,lStr); ++ac;
    XtSetArg(arg[ac],XmNwidth,VS_WIDTH); ++ac;
    labelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XtManageChild(labelW);
    XmStringFree(lStr);

    for (i=0; i<num_cars; ++i) {
	selStr[i] = XmStringCreateLtoR(driveables[i].name,
	    XmSTRING_DEFAULT_CHARSET);
    }

    ac = 0;
    XtSetArg(arg[ac],XmNitems,selStr); ++ac;
    XtSetArg(arg[ac],XmNitemCount,num_cars); ++ac;
    if (num_cars > 10) {
	XtSetArg(arg[ac],XmNvisibleItemCount,10); ++ac;
    }
    else {
	XtSetArg(arg[ac],XmNvisibleItemCount,num_cars); ++ac;
    }
    XtSetArg(arg[ac],XmNselectionPolicy,XmSINGLE_SELECT); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,labelW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    vehSelW = XmCreateList(formW,"Vehicle Selection",arg,ac);
    XtManageChild(vehSelW);
    for (i=0; i<num_cars; ++i) XmStringFree(selStr[i]);
    XtAddCallback(vehSelW,XmNsingleSelectionCallback,
	(XtCallbackProc) vehicleSelectedCallback, NULL);

    ac = 0;
    sprintf(str,"Color:  %s",lookup_rgb(cstate.upd.color[0],
	cstate.upd.color[1], cstate.upd.color[2]));
    lStr = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNwidth,VS_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNlabelString,lStr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_BEGINNING); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,vehSelW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,10); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    labelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XtManageChild(labelW);
    XmStringFree(lStr);

    ac = 0;
    XtSetArg(arg[ac],XmNminimum,0); ++ac;
    XtSetArg(arg[ac],XmNmaximum,COLOR_SCROLLBAR_MAX); ++ac;
    XtSetArg(arg[ac],XmNorientation,XmHORIZONTAL); ++ac;
    XtSetArg(arg[ac],XmNprocessingDirection,XmMAX_ON_RIGHT); ++ac;
    XtSetArg(arg[ac],XmNsliderSize,SLIDER_SIZE); ++ac;
    XtSetArg(arg[ac],XmNvalue,(COLOR_SCROLLBAR_MAX-SLIDER_SIZE)*hue); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,labelW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    scrollW = XmCreateScrollBar(formW,"scroll",arg,ac);
    XtAddCallback(scrollW,XmNvalueChangedCallback,
	(XtCallbackProc) newHueCallback, (caddr_t) labelW);
    XtAddCallback(scrollW,XmNdragCallback,
	(XtCallbackProc) newHueCallback, (caddr_t) labelW);
    XtManageChild(scrollW);

    ac = 0;
    XtSetArg(arg[ac],XmNminimum,0); ++ac;
    XtSetArg(arg[ac],XmNmaximum,COLOR_SCROLLBAR_MAX); ++ac;
    XtSetArg(arg[ac],XmNorientation,XmHORIZONTAL); ++ac;
    XtSetArg(arg[ac],XmNprocessingDirection,XmMAX_ON_RIGHT); ++ac;
    XtSetArg(arg[ac],XmNsliderSize,SLIDER_SIZE); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,scrollW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNvalue,(COLOR_SCROLLBAR_MAX-SLIDER_SIZE)*saturation);
	++ac;
    scrollW = XmCreateScrollBar(formW,"scroll",arg,ac);
    XtAddCallback(scrollW,XmNvalueChangedCallback,
	(XtCallbackProc) newSaturationCallback, (caddr_t) labelW);
    XtAddCallback(scrollW,XmNdragCallback,
	(XtCallbackProc) newSaturationCallback, (caddr_t) labelW);
    XtManageChild(scrollW);

    ac = 0;
    XtSetArg(arg[ac],XmNminimum,0); ++ac;
    XtSetArg(arg[ac],XmNmaximum,COLOR_SCROLLBAR_MAX); ++ac;
    XtSetArg(arg[ac],XmNorientation,XmHORIZONTAL); ++ac;
    XtSetArg(arg[ac],XmNprocessingDirection,XmMAX_ON_RIGHT); ++ac;
    XtSetArg(arg[ac],XmNsliderSize,SLIDER_SIZE); ++ac;
    XtSetArg(arg[ac],XmNvalue,
	(brightness-0.5)*2*(COLOR_SCROLLBAR_MAX-SLIDER_SIZE)); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,scrollW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    scrollW = XmCreateScrollBar(formW,"scroll",arg,ac);
    XtAddCallback(scrollW,XmNvalueChangedCallback,
	(XtCallbackProc) newBrightnessCallback, (caddr_t) labelW);
    XtAddCallback(scrollW,XmNdragCallback,
	(XtCallbackProc) newBrightnessCallback, (caddr_t) labelW);
    XtManageChild(scrollW);

    ac = 0;
    lStr = XmStringCreateLtoR("OK",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,lStr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    XtSetArg(arg[ac],XmNwidth,OKBTN_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,scrollW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,10); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_POSITION); ++ac;
    XtSetArg(arg[ac],XmNleftPosition,(VS_WIDTH-OKBTN_WIDTH)/2); ++ac;
    buttonW = XmCreatePushButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(buttonW,XmNactivateCallback,
	(XtCallbackProc) vehicleOKCallback,(caddr_t) formW);
    XmStringFree(lStr);
    XtManageChild(buttonW);
#endif
}


void grabCursor(
    void)
{
#if defined(MOTIF_GUI)
    if (cstate.mode & CLIENT_CONSTRAIN_CURSOR_MODE) {
        XGrabPointer(cstate.display, xs.mainWindow,
	    True, 0, GrabModeAsync, GrabModeAsync,
	    xs.graphicsWindow, xs.crosshair, CurrentTime );
    }
    else {
        XGrabPointer(cstate.display, xs.mainWindow,
	    True, 0, GrabModeAsync, GrabModeAsync,
	    xs.mainWindow, xs.crosshair, CurrentTime );
    }
#endif
}


void processXEvents(
    void)
{
#if defined(MOTIF_GUI)
    XEvent event;
    XAnyEvent *anyevent = (XAnyEvent *) &event;

    while (XtAppPending(xs.app_context)) {
	XtAppNextEvent(xs.app_context,&event);
	if (anyevent->window == xs.radarWindow) {
	    radarEventHandler(&event);
	}
	else if (anyevent->window == xs.raceWindow) {
	    raceEventHandler(&event);
	}
	else {
	    XtDispatchEvent(&event);
	}
    }
    if (cstate.use_joystick && (cstate.joystick != NULL) ) {
	/* Tell the daemon we are still here */
	cstate.joystick->Valid = 3; 
    }
#endif
}
