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
#ifndef WIN32
#include <sys/param.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#ifndef MAC
#include <Xm/Xm.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#endif

#endif

/* map to lower case and remove blank space */
void normalize_string(
    char *in,
    char *out)
{

    while (*in != '\0') {
	if ((*in >= 'A') && (*in <= 'Z')) {
	    *out++ = *in++ + ('a' - 'A');
	}
	else if ((*in == ' ') || (*in == '\t') || (*in == '\n')) {
	    /* Just skip it. */
	    ++in;
	}
	else {
	    *out++ = *in++;
	}
    }
    *out = '\0';
}

static void badAcknowledgeCallback(
    void)
{
    exit(1);
}

void popup_error_dialog(
    int  error_type )
{
#if !(defined(WIN32) || defined(MAC))
    XmString xmstr, xmstr2;
    Arg arg[100];
    Cardinal ac;
    Widget w;
    char str[1000];
    int argc = 0;
    char *argv[5];
    XEvent event;
    Display *display;
    Widget app_shellW;

#ifdef __linux__
    app_shellW = XtInitialize("Main", "Drive", NULL, 0, &argc,argv);
#else
    app_shellW = XtInitialize("Main", "Drive", NULL, NULL, &argc,argv);
#endif

    if (app_shellW == NULL) {
	fprintf(stderr,"Could not open Motif1.2 X11R5 Windows on display!!\n");
	exit(1);
    }

    display   = XtDisplay(app_shellW);


    ac = 0;
    xmstr = XmStringCreateLtoR("DRIVE Warning",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_APPLICATION_MODAL); ++ac;
    if( error_type == 1) /* Powershade Missing */
    {
	sprintf(str," DRIVE will not work without Powershade Graphics Software.\n Contact your HP Sales Representative for details on ordering Powershade.");
    }
    else if( error_type == 2) /* Missing services */
    {
	sprintf(str,
" Error!  Your /etc/services file does not contain the required entries for\n DRIVE.  To add the necessary entries, the 'add_drive_services' script (found \n in the same directory as this program) needs to be run by root.");
    }
    xmstr2 = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNmessageString,xmstr2); ++ac;
    w = XmCreateWarningDialog(app_shellW,"DRIVE Warning",arg,ac);
    XmStringFree(xmstr);
    XmStringFree(xmstr2);
    XtAddCallback(w,XmNokCallback,
	(XtCallbackProc) badAcknowledgeCallback,NULL);
    XtManageChild(w);
    XtUnmanageChild(XmMessageBoxGetChild(w,XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(w,XmDIALOG_HELP_BUTTON));
    XSync(display,False);

    while (1) {
	XtNextEvent(&event);
	XtDispatchEvent(&event);
	
    }
#endif
}

#if !defined(WIN32) && !defined(__linux__) && !defined(MAC)
# pragma OPTIMIZE OFF /* [ */
#endif

/* I wanted to call this check_powershade, but that might attract
 * the attention of a "curious" hacker.  :-/
 */
void check_graphics_configuration(
    void)
{
#if 0
    char demo_directory[MAXPATHLEN],*cptr;
#endif

    /* Not to worry -- powershade will be there */
    return;

    /* A lot of this is done painfully so "strings" doesn't give
     * away any secrets.
     */

#if 0
    /* Check and see if powershade exists on the system. */
#ifdef V4_FILE_SYS
    /* Look for /opt/graphics/common/config/.pwrshd.
     */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'o';
    *cptr++ = 'p';
    *cptr++ = 't';
    *cptr++ = '/';
    *cptr++ = 'g';
    *cptr++ = 'r';
    *cptr++ = 'a';
    *cptr++ = 'p';
    *cptr++ = 'h';
    *cptr++ = 'i';
    *cptr++ = 'c';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'c';
    *cptr++ = 'o';
    *cptr++ = 'm';
    *cptr++ = 'm';
    *cptr++ = 'o';
    *cptr++ = 'n';
    *cptr++ = '/';
    *cptr++ = 'c';
    *cptr++ = 'o';
    *cptr++ = 'n';
    *cptr++ = 'f';
    *cptr++ = 'i';
    *cptr++ = 'g';
    *cptr++ = '/';
    *cptr++ = '.';
    *cptr++ = 'p';
    *cptr++ = 'w';
    *cptr++ = 'r';
    *cptr++ = 's';
    *cptr++ = 'h';
    *cptr++ = 'd';
    *cptr++ = '\0';
    if (access(demo_directory,0) == 0) return;

    /* Try to change to /opt/graphics/PEX5/demos/drive, which will
     * short-circuit the .pwrshd check.
     */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'o';
    *cptr++ = 'p';
    *cptr++ = 't';
    *cptr++ = '/';
    *cptr++ = 'g';
    *cptr++ = 'r';
    *cptr++ = 'a';
    *cptr++ = 'p';
    *cptr++ = 'h';
    *cptr++ = 'i';
    *cptr++ = 'c';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'P';
    *cptr++ = 'E';
    *cptr++ = 'X';
    *cptr++ = '5';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'e';
    *cptr++ = 'm';
    *cptr++ = 'o';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'r';
    *cptr++ = 'i';
    *cptr++ = 'v';
    *cptr++ = 'e';
    *cptr++ = '\0';   /* NULL */
    if (chdir(demo_directory) == 0) return;

    /* Hmm, try /opt/graphics/PEX5/demos. */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'o';
    *cptr++ = 'p';
    *cptr++ = 't';
    *cptr++ = '/';
    *cptr++ = 'g';
    *cptr++ = 'r';
    *cptr++ = 'a';
    *cptr++ = 'p';
    *cptr++ = 'h';
    *cptr++ = 'i';
    *cptr++ = 'c';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'P';
    *cptr++ = 'E';
    *cptr++ = 'X';
    *cptr++ = '5';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'e';
    *cptr++ = 'm';
    *cptr++ = 'o';
    *cptr++ = 's';
    *cptr++ = '\0';   /* NULL */
#else
    /* Look for /usr/lib/starbase/.pwrshd.
     */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'u';
    *cptr++ = 's';
    *cptr++ = 'r';
    *cptr++ = '/';
    *cptr++ = 'l';
    *cptr++ = 'i';
    *cptr++ = 'b';
    *cptr++ = '/';
    *cptr++ = 's';
    *cptr++ = 't';
    *cptr++ = 'a';
    *cptr++ = 'r';
    *cptr++ = 'b';
    *cptr++ = 'a';
    *cptr++ = 's';
    *cptr++ = 'e';
    *cptr++ = '/';
    *cptr++ = '.';
    *cptr++ = 'p';
    *cptr++ = 'w';
    *cptr++ = 'r';
    *cptr++ = 's';
    *cptr++ = 'h';
    *cptr++ = 'd';
    *cptr++ = '\0';
    if (access(demo_directory,0) == 0) return;

    /* Try to change to /usr/demos/graphics/drive, which will
     * short-circuit the .pwrshd check.
     */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'u';
    *cptr++ = 's';
    *cptr++ = 'r';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'e';
    *cptr++ = 'm';
    *cptr++ = 'o';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'g';
    *cptr++ = 'r';
    *cptr++ = 'a';
    *cptr++ = 'p';
    *cptr++ = 'h';
    *cptr++ = 'i';
    *cptr++ = 'c';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'r';
    *cptr++ = 'i';
    *cptr++ = 'v';
    *cptr++ = 'e';
    *cptr++ = '\0';   /* NULL */
    if (chdir(demo_directory) == 0) return;

    /* Hmm, try /usr/demos/graphics. */
    cptr = demo_directory;
    *cptr++ = '/';
    *cptr++ = 'u';
    *cptr++ = 's';
    *cptr++ = 'r';
    *cptr++ = '/';
    *cptr++ = 'd';
    *cptr++ = 'e';
    *cptr++ = 'm';
    *cptr++ = 'o';
    *cptr++ = 's';
    *cptr++ = '/';
    *cptr++ = 'g';
    *cptr++ = 'r';
    *cptr++ = 'a';
    *cptr++ = 'p';
    *cptr++ = 'h';
    *cptr++ = 'i';
    *cptr++ = 'c';
    *cptr++ = 's';
    *cptr++ = '\0';   /* NULL */
#endif
    if (chdir(demo_directory) == 0) return;

    popup_error_dialog(1);
    /* It's not going to work without powershade.  Print error and exit. */
    fprintf(stderr,
	"\n\nDRIVE will not work without Powershade Graphics Software\n");
    fprintf(stderr,
	"Contact your HP Sales Representative for demos and details on\n");
    fprintf(stderr,
	"ordering Powershade.\n");
    fprintf(stderr,"\n");
    exit(-1);
#endif
}
#if !defined(WIN32) && !defined(__linux__) && !defined(MAC)
# pragma OPTIMIZE ON /* ] */
#endif
