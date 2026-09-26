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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>

#include "global.h"
#include "drive.h"
#include "windows.h"
#include "gui.h"


Boolean StarbaseGetVisuals(
    XVisualInfo *vdash,
    XVisualInfo *vgraphics)
{
    XVisualInfo *visual,def;
    int nvisuals,i;
    int score, maxscore_dash,maxscore_graphics;

    return(1);

    vdash->depth = 0;

    maxscore_dash = (maxscore_graphics = -1);
    def.screen = xs.screenNum;
    visual = XGetVisualInfo(xs.display,VisualScreenMask,&def,&nvisuals);
    if (nvisuals == 0) {
	fprintf(stderr,"ERROR: XServer does not return any visuals!\n");
	exit(1);
    }

    /* score:  DDDDDMMMMMMMMMCCC where D=depth, M=cmap size, c=class */
    for (i=0; i<nvisuals; ++i) {
	switch (visual[i].class) {
	    case TrueColor:
		score = 6;
		break;
	    case DirectColor:
		score = 5;
		break;
	    case StaticColor:
		score = 4;
		break;
	    case PseudoColor:
		score = 3;
		break;
	    case StaticGray:
		score = 2;
		break;
	    case GrayScale:
	    default:
		score = 1;
		break;
	}

	score |= (visual[i].depth << 12);
	if (visual[i].depth <= 8) score |= (visual[i].colormap_size << 3);

	if (score > maxscore_graphics) {
	    maxscore_graphics = score;
	    memcpy(vgraphics,&(visual[i]),sizeof(XVisualInfo));
	}
	if ((score > maxscore_dash) && (visual[i].depth != 12)) {
	    maxscore_dash = score;
	    memcpy(vdash,&(visual[i]),sizeof(XVisualInfo));
	}
    }

    XFree((char *) visual);

    return(vdash->depth != 0);
}
#endif
