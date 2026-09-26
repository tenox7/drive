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
#endif

#include "global.h"
#include "drive.h"
#include "gui.h"
#include "windows.h"

#define PLACE_X		20
#define PLACE_Y		60
#define PLACE_YTOP	45
#define PLACE_YBOTTOM	90

#define REDRAW_CHECKPOINT	0
#define REDRAW_STATUS		1

static int last_redraw_type = REDRAW_STATUS;
static int last_place = INVALID;
static int last_state = RACE_STATE;


void redrawCheckpoint(
    void)
{
#if defined(MOTIF_GUI)
    XTextItem item;
    char txt[256], *ptr;
    float chktime;
    int i;
    int hours,minutes,seconds,tenths;

    item.delta = 0;
    item.font  = xs.textDfont;

    XClearWindow(xs.display,xs.textWindow);

    XSetForeground(xs.display,xs.textGC,xs.white);
    sprintf(txt,"Number of Checkpoints: %3d",cstate.checkpoint.num_checkpoints);
    item.chars = txt;
    item.nchars = strlen(item.chars);
    XDrawText(xs.display,xs.textWindow,xs.textGC,20,20,&item,1);

    sprintf(txt,"Checkpoints Visited:");
    ptr = txt + strlen(txt);
    *ptr++ = ' ';
    for (i=1; i <= cstate.checkpoint.num_checkpoints; i++) {
	if ( cstate.checkpoint.visited & (1 << (i-1) ) ) {
	   if( i < 10 ) {
	       *ptr ++ = i + '0';
	       *ptr ++ = ' ';
	   }
	   else {
	       *ptr ++ = '1';
	       *ptr ++ = i - 10 + '0';
	       *ptr ++ = ' ';
	   }
	}
    }
    *ptr = 0;
    item.chars = txt;
    item.nchars = strlen(item.chars);
    XDrawText(xs.display,xs.textWindow,xs.textGC,20,40,&item,1);

    chktime = cstate.checkpoint.time;
    hours = (int)(chktime / 3600.0 );
    minutes = (int)( (chktime - hours * 3600 ) / 60.0 );
    seconds = (int)( chktime - hours * 3600 - minutes * 60 );
    tenths  = (int)( (chktime - (int)(chktime)) * 10);
    sprintf(txt,"Time to Checkpoint: %02d:%02d:%02d.%02d",
	  hours,minutes,seconds,tenths);
    item.chars = txt;
    item.nchars = strlen(item.chars);
    XDrawText(xs.display,xs.textWindow,xs.textGC,20,60,&item,1);

    chktime = cstate.lap_time;
    hours = (int)(chktime / 3600.0 );
    minutes = (int)( (chktime - hours * 3600 ) / 60.0 );
    seconds = (int)( chktime - hours * 3600 - minutes * 60 );
    tenths  = (int)( (chktime - (int)(chktime)) * 10);
    sprintf(txt,"Lap Time: %02d:%02d:%02d.%02d",
	  hours,minutes,seconds,tenths);
    item.chars = txt;
    item.nchars = strlen(item.chars);
    XDrawText(xs.display,xs.textWindow,xs.textGC,20,80,&item,1);

    last_redraw_type = REDRAW_CHECKPOINT;
#endif
}





void updateStateWindow(
    int state)
{
#if defined(MOTIF_GUI)
    char txt[100];
    XTextItem item;

    XClearWindow(xs.display,xs.textWindow);

    item.delta = 0;
    item.font = xs.textBfont;
    XSetForeground(xs.display,xs.textGC,xs.white);

    switch (state) {
     case WELCOME_STATE:
	sprintf(txt,"WELCOME!");
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_Y,&item,1);
        break;
     case RACE_STATE:
	sprintf(txt,"RACING");
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_Y,&item,1);
        break;
     case PRACTICE_STATE:
	sprintf(txt,"PRACTICING");
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_Y,&item,1);
        break;
     case PRE_RACE_STATE:
	sprintf(txt,"PREPARE TO");
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_YTOP,&item,1);
	sprintf(txt,"RACE......");
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_YBOTTOM,&item,1);
        break;
     case POST_RACE_STATE:
	switch (last_place) {
	    case 1:
		sprintf(txt,"WINNER!!!!");
		break;
	    case 2:
		sprintf(txt,"2nd PLACE");
		break;
	    case 3:
		sprintf(txt,"3rd PLACE");
		break;
	    case 4:
		sprintf(txt,"4th PLACE");
		break;
	    case INVALID:
		sprintf(txt,"RACE COMPLETE");
		break;
	    default:
		sprintf(txt,"DIDN'T PLACE");
		break;
	}
	item.chars = txt;
	item.nchars = strlen(item.chars);
	XDrawText(xs.display,xs.textWindow,xs.textGC,
	    PLACE_X,PLACE_Y,&item,1);
        break;
    }

    XFlush(xs.display);
    last_redraw_type = REDRAW_STATUS;
    last_state = state;

    /* Allow/disallow the user changing vehicles. */
    updateVehicleSensitivity(state);
#endif
}


void showPlace(
    int place)
{
    last_place = place;
    updateStateWindow(last_state);
}


#if defined(MOTIF_GUI)
void exposeTextWindow(
    Widget whichW,
    void *client_data,
    XmDrawingAreaCallbackStruct *cb)
{

    switch (last_redraw_type) {
	case REDRAW_CHECKPOINT:
	    redrawCheckpoint();
	    break;
	case REDRAW_STATUS:
	    updateStateWindow(last_state);
	    break;
    }
}
#endif
