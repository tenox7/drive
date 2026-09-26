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
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include "visDrawArea.h"
#endif

#include "filenames.h"
#include "global.h"
#include "drive.h"
#include "gui.h"
#include "message.h"
#include "windows.h"

#define VISCTRLS_WIDTH			1000
#define VISCTRLS_HEIGHT			350
#define OK_BUTTON_WIDTH			(VISCTRLS_WIDTH/5)

#if defined(MOTIF_GUI)
static Widget vcFormW = NULL;
static unsigned char *ldata = NULL;


#define STRIP_END_GARBAGE(str) \
{   char *_cptr; \
    _cptr = (char *) (str) + strlen(str) - 1; \
    while ((*_cptr == '\n') || (*_cptr == '\t') || (*_cptr == ' ')) { \
	if ((--_cptr) < (str)) break; \
    } \
    *(_cptr+1) = ' '; \
    *(_cptr+2) = '\0'; \
}
#endif


void parseHelpFile(
    char *subject[],
    int *num_subjects,
    char *subject_text[])
{
    FILE *fptr;
    char str[4096],text[16384];
    char *stuff;
    boolean_type need_subject,is_blank_line;

    *num_subjects = 0;

    if ((fptr = fopen(help_filename,"r")) == NULL) {
	return;
    }

    need_subject = TRUE;
    while (fgets(str,sizeof(str),fptr) != NULL) {
	/* ignore comments */
	if (str[0] == '#') continue;

	STRIP_END_GARBAGE(str);
	is_blank_line = ((str[0] == ' ') && (str[1] == '\0'));

	if (need_subject) {
	    /* Ignore blank lines preceding subjects */
	    if (is_blank_line) continue;
	    /* copy to malloced buffer */
	    stuff = (char *) fastmalloc(strlen(str)+1);
	    strcpy(stuff,str);
	    subject[*num_subjects] = stuff;
	    need_subject = FALSE;
	    /* Also copy to text! */
	    sprintf(text,"%s:\n\n",stuff);
	}
	else if (str[0] == '.') {
	    /* End of subject text.  Copy to buffer. */
	    stuff = (char *) fastmalloc(strlen(text)+1);
	    strcpy(stuff,text);
	    subject_text[*num_subjects] = stuff;
	    text[0] = '\0';
	    need_subject = TRUE;
	    ++(*num_subjects);
	}
	else {
	    /* Continuation of text */
	    if (is_blank_line) {
		strcat(text,"\n\n");
	    }
	    else if (str[0] == '^') {
		strcat(text,"\n");
		strcat(text,str+1);
	    }
	    else {
		strcat(text,str);
	    }
	}
    }
}

#if defined(MOTIF_GUI)
static void redrawVisCtrls(
    Widget whichW,
    int x, int y,
    int width, int height)
{
    static XImage xi = {
	VISCTRLS_WIDTH,VISCTRLS_HEIGHT,	/* width,height */
	0,			/* xoffset */
	ZPixmap,		/* format */
	NULL,			/* data -- TO BE FILLED IN */
	MSBFirst,		/* byte_order */
	8,			/* bitmap_unit */
	MSBFirst,		/* bitmap_bit_order */
	0,			/* bitmap_pad */
	8,			/* depth */
	VISCTRLS_WIDTH,		/* bytes_per_line */
	8,			/* bits_per_pixel */
	0xe0,			/* red_mask */
	0x1c,			/* green_mask */
	0x03,			/* blue_mask */
	NULL,			/* obdata */
	{   NULL,		/* f.create_image */
	    NULL,		/* f.destroy_image */
	    NULL,		/* f.get_pixel */
	    NULL,		/* f.put_pixel */
	    NULL,		/* f.sub_image */
	    NULL,		/* f.add_pixel */
	},
    };
    static Window visctrlsWindow = 0;
    static GC visctrlsGC;

    if (visctrlsWindow == 0) {
	visctrlsWindow = XtWindow(whichW);
	/* install_standard_colormap(visctrlsWindow); */
	visctrlsGC = XCreateGC(xs.display,visctrlsWindow,0,NULL);
    }

    if (ldata == NULL) {
	/* Don't downsample */
	getPixmap(visctrls_filename,VISCTRLS_WIDTH,VISCTRLS_HEIGHT,
	    &ldata,NULL,False);
	xi.data   = (char *) ldata;
	INITIALIZE_XIMAGE_PARAMETERS(xi);
	xi.bytes_per_line = xi.width * xi.bits_per_pixel/8;
    }

    /* Update the static visctrls display */
    XPutImage(xs.display,visctrlsWindow,visctrlsGC, &xi,
	x,y,x,y,width,height);
}


static void exposeVisCtrls(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb)
{
    XExposeEvent *event = (XExposeEvent *) (cb->event);

    redrawVisCtrls(whichW,event->x,event->y, event->width,event->height);
}
#endif


void mapVisCtrlsWindow(
    void)
{
#if defined(MOTIF_GUI)
    /* Map the window */
    if (vcFormW != NULL) {
	XtManageChild(vcFormW);
	XmUpdateDisplay(vcFormW);
    }
#endif
}


void unmapVisCtrlsWindow(
    void)
{
#if defined(MOTIF_GUI)
    /* Unmap and destroy the window */
    if (vcFormW != NULL) {
	XtUnmanageChild(vcFormW);
	XmUpdateDisplay(vcFormW);
    }
#endif
}



void createVisCtrlsWindow(
    Widget parentW)
{
#if defined(MOTIF_GUI)
    Arg arg[MAX_ARG];
    Cardinal ac;
    Widget visctrlsW,btnW;
    XmString xmstr;

    ac = 0;
    xmstr = XmStringCreateLtoR("DRIVE Help",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    XtSetArg(arg[ac],XmNfractionBase,VISCTRLS_WIDTH); ++ac;
    vcFormW = XmCreateFormDialog(parentW,"DRIVE Help",arg,ac);
    XmStringFree(xmstr);

    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM);
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM);
    XtSetArg(arg[ac],XmNwidth,VISCTRLS_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNheight,VISCTRLS_HEIGHT); ++ac;
    XtSetArg(arg[ac],XgNvisual,xs.dashInfo.visualInfo.visual); ++ac;
    XtSetArg(arg[ac],XmNcolormap,xs.dashInfo.stdCmap.colormap); ++ac;
    XtSetArg(arg[ac],XmNforeground,xs.white); ++ac;
    XtSetArg(arg[ac],XmNbackground,xs.black); ++ac;
    visctrlsW = XgCreateVisualDrawingArea(vcFormW,"visctrls",arg,ac);
    XtAddCallback(visctrlsW,XmNexposeCallback,
	(XtCallbackProc) exposeVisCtrls,NULL);
    XtAddCallback(visctrlsW,XmNinputCallback,
	(XtCallbackProc) unmapVisCtrlsWindow, NULL);
    XtManageChild(visctrlsW);

    ac = 0;
    xmstr = XmStringCreateLtoR("OK",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNwidth,OK_BUTTON_WIDTH); ++ac;
    XtSetArg(arg[ac],XmNrecomputeSize,False); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,visctrlsW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_POSITION); ++ac;
    XtSetArg(arg[ac],XmNleftPosition,(VISCTRLS_WIDTH-OK_BUTTON_WIDTH)/2); ++ac;
    btnW = XmCreatePushButtonGadget(vcFormW,"btn",arg,ac);
    XtAddCallback(btnW,XmNactivateCallback,
	(XtCallbackProc) unmapVisCtrlsWindow, NULL);
    XmStringFree(xmstr);
    XtManageChild(btnW);
#endif
}


#if defined(MOTIF_GUI)
void genericHelpCallback(
    Widget whichW,
    char *helpMsg,
    XmAnyCallbackStruct *cb)
{
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr;
    Widget formW,textW,btnW;

    ac = 0;
    xmstr = XmStringCreateLtoR("DRIVE Help",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    formW = XmCreateFormDialog(whichW,"DRIVE Help",arg,ac);
    XmStringFree(xmstr);

    ac = 0;
    xmstr = XmStringCreateLtoR("OK",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNrecomputeSize,False); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNbottomAttachment,XmATTACH_FORM); ++ac;
    btnW = XmCreatePushButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(btnW,XmNactivateCallback,
	(XtCallbackProc) destroyWidgetCallback, formW);
    XmStringFree(xmstr);
    XtManageChild(btnW);
	
    ac = 0;
    XtSetArg(arg[ac],XmNeditable,False); ++ac;
    XtSetArg(arg[ac],XmNeditMode,XmMULTI_LINE_EDIT); ++ac;
    XtSetArg(arg[ac],XmNvalue,helpMsg); ++ac;
    XtSetArg(arg[ac],XmNcolumns,80); ++ac;
    XtSetArg(arg[ac],XmNrows,10); ++ac;
    XtSetArg(arg[ac],XmNwordWrap,True); ++ac;
    XtSetArg(arg[ac],XmNscrollHorizontal,False); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNbottomAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNbottomWidget,btnW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    textW = XmCreateScrolledText(formW,"DRIVE Help",arg,ac);
    XtManageChild(textW);

    XtManageChild(formW);
}
#endif
