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
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <sys/param.h>

#include <Xm/Xm.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#endif

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "windows.h"
#include "filenames.h"

static Widget selW;

static char local_help[] = "If you press \"OK\", the DRIVE server will be\
 started on the local system.  If the server is run elsewhere, your local\
 system can devote its resources to the graphics display from your vehicle,\
 so you may get smoother motion through more frames per second.";

static char server_help[] = "DRIVE consists of one server process controlling\
 the interactions between objects in the virtual world and one process per\
 user to handle display of the graphics and processing of input.\
 The DRIVE server must be running before anyone can hook up to the\
 virtual world.  This window allows you to select a system on which the\
 server is running.  By selecting the system (or typing it in), you\
 can have any accessible system run the server.  The \"Search\" button\
 will check all of the machines on the local subnet (up to 256 of them)\
 and update the list to reflect all machines currently running the\
 DRIVE server.  If you press \"Cancel\", this program will exit.";

static char selected_system[256];

#if defined(MOTIF_GUI)
static void exitProgramCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{
    exit(0);
}
	

static void startLocalCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{

#ifdef FORK_METHOD
    if (fork() == 0) {
	setpgrp();
	close(0);
	close(1);
	close(2);
	execl(server_programname,SERVER_PROGRAM,0);
	exit(0);
    }
#else
    {	
	char cmd[MAXPATHLEN+256];
	sprintf(cmd,"%s &",server_programname);
	system(cmd);
    }
#endif /* FORK_METHOD */

    sleep(2);
    gethostname(selected_system,sizeof(selected_system)-1);
    XtDestroyWidget(whichW);
}


static void searchCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{
    char server[256][256];
    XmString serverStr[256];
    int num_servers;
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr,xmstr2;
    Widget w;
    int i;

    find_active_servers(server,&num_servers);

    if (num_servers == 0) {	
	ac = 0;
	xmstr = XmStringCreateLtoR("Server?",XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
	xmstr2 = XmStringCreateLtoR(
	    "No running servers found!  Start one on the local system?",
	    XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNmessageString,xmstr2); ++ac;
	w = XmCreateQuestionDialog(whichW,"Server?",arg,ac);
	XmStringFree(xmstr);
	XmStringFree(xmstr2);
	XtAddCallback(w,XmNcancelCallback,
	    (XtCallbackProc) destroyWidgetCallback,w);
	XtAddCallback(w,XmNokCallback,
	    (XtCallbackProc) startLocalCallback,NULL);
	XtAddCallback(w,XmNhelpCallback,
	    (XtCallbackProc) genericHelpCallback,local_help);
	XtManageChild(w);
    }
    else {
	for (i=0; i<num_servers; ++i) {
	    serverStr[i] =
		XmStringCreateLtoR(&(server[i][0]),XmSTRING_DEFAULT_CHARSET);
	}
	ac = 0;
	XtSetArg(arg[ac],XmNlistItems,serverStr); ++ac;
	XtSetArg(arg[ac],XmNlistItemCount,num_servers); ++ac;
	XtSetValues(selW,arg,ac);
    }
}


static void okCallback(
    Widget whichW,
    caddr_t data,
    XmSelectionBoxCallbackStruct *cb)
{
    XmString_to_string(cb->value,selected_system);
}
#endif


char *findServer(
    void)
{
#if defined(MOTIF_GUI)
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr,xmstr2,xmstr3,xmstr4;
    char server[256][256];
    XmString serverStr[256];
    int num_servers,i;

    find_active_servers(server,&num_servers);
    for (i=0; i<num_servers; ++i) {
	serverStr[i] =
	    XmStringCreateLtoR(&(server[i][0]),XmSTRING_DEFAULT_CHARSET);
    }

    selected_system[0] = '\0';

    ac = 0;
    xmstr = XmStringCreateLtoR("Server Selection",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    xmstr2 = XmStringCreateLtoR("Search",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNapplyLabelString,xmstr2); ++ac;
    xmstr3 = XmStringCreateLtoR("Server:",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNselectionLabelString,xmstr3); ++ac;
    xmstr4 = XmStringCreateLtoR("Server Systems",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlistLabelString,xmstr4); ++ac;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_APPLICATION_MODAL); ++ac;
    if (num_servers) { XtSetArg(arg[ac],XmNlistItems,serverStr); ++ac; }
    XtSetArg(arg[ac],XmNlistItemCount,num_servers); ++ac;
    XtSetArg(arg[ac],XmNlistVisibleItemCount,10); ++ac;
    XtSetArg(arg[ac],XmNtextColumns,12); ++ac;
    XtSetArg(arg[ac],XmNautoUnmanage,False); ++ac;
    selW = XmCreateSelectionDialog(xs.app_shellW,"Server Selection",arg,ac);
    XmStringFree(xmstr);
    XmStringFree(xmstr2);
    XmStringFree(xmstr3);
    XmStringFree(xmstr4);
    XtAddCallback(selW,XmNokCallback,
	(XtCallbackProc) okCallback,NULL);
    XtAddCallback(selW,XmNapplyCallback,
	(XtCallbackProc) searchCallback,NULL);
    XtAddCallback(selW,XmNcancelCallback,
	(XtCallbackProc) exitProgramCallback,NULL);
    XtAddCallback(selW,XmNhelpCallback,
	(XtCallbackProc) genericHelpCallback,server_help);
    XtManageChild(selW);

    while (selected_system[0] == '\0') {
	processXEvents();
    }

    XtDestroyWidget(selW);
    XmUpdateDisplay(xs.app_shellW);
    return(selected_system);
#else
    return NULL;
#endif
}

#if defined(MOTIF_GUI)
static int got_ok;

static void badAcknowledgeCallback(
    void)
{
    got_ok = TRUE;
}


void badServerMessage(
    char *bad_server_name)
{
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr,xmstr2;
    char str[512];
    Widget w;

    got_ok = FALSE;
	
    ac = 0;
    xmstr = XmStringCreateLtoR("Server Warning",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_APPLICATION_MODAL); ++ac;
    sprintf(str,"No server running on system \"%s\"",bad_server_name);
    xmstr2 = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNmessageString,xmstr2); ++ac;
    w = XmCreateWarningDialog(xs.app_shellW,"Server Warning",arg,ac);
    XmStringFree(xmstr);
    XmStringFree(xmstr2);
    XtAddCallback(w,XmNokCallback,
	(XtCallbackProc) badAcknowledgeCallback,NULL);
    XtManageChild(w);

    XtUnmanageChild(XmMessageBoxGetChild(w,XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(w,XmDIALOG_HELP_BUTTON));

    XmUpdateDisplay(w);

    while (!got_ok) {
	processXEvents();
    }

    XtDestroyWidget(w);
    XmUpdateDisplay(xs.app_shellW);
}
#endif
