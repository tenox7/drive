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
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/keysym.h>

#include <X11/Xutil.h>

#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#endif

#include "global.h"
#include "drive.h"
#include "windows.h"
#include "gui.h"

#if defined(MOTIF_GUI)
#define OVERRIDE_HP_332 1   /* If set, don't allow 332 on HP 8-plane devices. */

#define READ_ACCESS	04
#define WRITE_ACCESS	02
#define EXECUTE_ACCESS	01
#define ACCESS_OKAY	0
#define ACCESS_ERROR	(-1)

/*
    Interesting values for the interpolation_ramp field.
*/

#define HPPEX_INTERP_RAMP_884_CRX     0x011 
#define HPPEX_INTERP_RAMP_666_CRX     0x012

#define IS_332_CMAP(stdCmap) \
    ((  (stdCmap)->red_max   == 7) && ((stdCmap)->red_mult   == 32) \
    && ((stdCmap)->green_max == 7) && ((stdCmap)->green_mult == 4) \
    && ((stdCmap)->blue_max  == 3) && ((stdCmap)->blue_mult  == 1) \
    && ((stdCmap)->base_pixel == 0))
#define IS_666_CMAP(stdCmap) \
    ((  (stdCmap)->red_max   == 5) && ((stdCmap)->red_mult   == 36) \
    && ((stdCmap)->green_max == 5) && ((stdCmap)->green_mult == 6) \
    && ((stdCmap)->blue_max  == 5) && ((stdCmap)->blue_mult  == 1) \
    && ((stdCmap)->base_pixel == 40))


static char sharedCmapHelp[] = "HP Servers on low-end devices can be run in\
 several modes.  When the SB_X_SHARED_CMAP environment variable is set before\
 the X server is started, the colormap is set up so that it can be shared \
 between the window system and Starbase.  When it is not set, no sharing \
 is done.  If it is not set when the server is started, but it gets set \
 before DRIVE is started, colormaps will not be correctly allocated and \
 shared.";


static Boolean got_ok = False;

static void acknowledgeSharedCallback(
    void)
{
    got_ok = True;
}

static void sharedWarning(
    void)
{
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr,xmstr2;
    char str[512];
    Widget w;

    got_ok = FALSE;

    ac = 0;
    xmstr = XmStringCreateLtoR("Shared CMAP Warning",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNdialogTitle,xmstr); ++ac;
    XtSetArg(arg[ac],XmNdialogStyle,XmDIALOG_APPLICATION_MODAL); ++ac;
    sprintf(str,"Server is not in SB_X_SHARED_CMAP mode, yet SB_X_SHARED_CMAP is set.\nYou may see colormap anomalies.");
    xmstr2 = XmStringCreateLtoR(str,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNmessageString,xmstr2); ++ac;
    w = XmCreateWarningDialog(xs.app_shellW,"Shared Colormap Warning",arg,ac);
    XmStringFree(xmstr);
    XmStringFree(xmstr2);
    XtAddCallback(w,XmNokCallback,
        (XtCallbackProc) acknowledgeSharedCallback,NULL);
    XtAddCallback(w,XmNhelpCallback,
        (XtCallbackProc) genericHelpCallback,(void *)  sharedCmapHelp);
    XtManageChild(w);

    XtUnmanageChild(XmMessageBoxGetChild(w,XmDIALOG_CANCEL_BUTTON));

    XmUpdateDisplay(w);

    while (!got_ok) {
        processXEvents();
    }

    XtDestroyWidget(w);
    XmUpdateDisplay(xs.app_shellW);
}




static short *get_special_ELK_cmap(
    XStandardColormap *stdCmap,
    XVisualInfo *visinfo)
{
    static short hp_crx_666_dithertable[256] = {
    /*   0: */   0,   1,   2,   3,   4,   5,   6,   7,
    /*   8: */   8,   9,  10,  11,  12,  13,  14,  15,
    /*  16: */  16,  17,  18,  19,  20,  21,  22,  23,
    /*  24: */  24,  25,  26,  27,  28,  29,  30,  31,
    /*  32: */  32,  33,  34,  35,  36,  37,  38,  39,
    /*  40: */ 255, 251, 191, 187, 127, 123, 253, 249,
    /*  48: */ 189, 185, 125, 121, 239, 235, 175, 171,
    /*  56: */ 111, 107, 237, 233, 173, 169, 109, 105,
    /*  64: */ 223, 219, 159, 155,  95,  91, 221, 217,
    /*  72: */ 157, 153,  93,  89, 254, 250, 190, 186,
    /*  80: */ 126, 122, 252, 248, 188, 184, 124, 120,
    /*  88: */ 238, 234, 174, 170, 110, 106, 236, 232,
    /*  96: */ 172, 168, 108, 104, 222, 218, 158, 154,
    /* 104: */  94,  90, 220, 216, 156, 152,  92,  88,
    /* 112: */ 247, 243, 183, 179, 119, 115, 245, 241,
    /* 120: */ 181, 177, 117, 113, 231, 227, 167, 163,
    /* 128: */ 103,  99, 229, 225, 165, 161, 101,  97,
    /* 136: */ 215, 211, 151, 147,  87,  83, 213, 209,
    /* 144: */ 149, 145,  85,  81, 246, 242, 182, 178,
    /* 152: */ 118, 114, 244, 240, 180, 176, 116, 112,
    /* 160: */ 230, 226, 166, 162, 102,  98, 228, 224,
    /* 168: */ 164, 160, 100,  96, 214, 210, 150, 146,
    /* 176: */  86,  82, 212, 208, 148, 144,  84,  80,
    /* 184: */ 207, 203, 143, 139,  79,  75, 205, 201,
    /* 192: */ 141, 137,  77,  73, 199, 195, 135, 131,
    /* 200: */  71,  67, 197, 193, 133, 129,  69,  65,
    /* 208: */  63,  59,  55,  51,  47,  43,  61,  57,
    /* 216: */  53,  49,  45,  41, 206, 202, 142, 138,
    /* 224: */  78,  74, 204, 200, 140, 136,  76,  72,
    /* 232: */ 198, 194, 134, 130,  70,  66, 196, 192,
    /* 240: */ 132, 128,  68,  64,  62,  58,  54,  50,
    /* 248: */  46,  42,  60,  56,  52,  48,  44,  40,
    };

    static short hp_crx_884_dithertable[256] = {
    /*   0: */ 255, 251, 223, 219, 253, 249, 221, 217,
    /*   8: */ 239, 235, 207, 203, 237, 233, 205, 201,
    /*  16: */ 127, 123,  95,  91, 125, 121,  93,  89,
    /*  24: */ 111, 107,  79,  75, 109, 105,  77,  73,
    /*  32: */ 254, 250, 222, 218, 252, 248, 220, 216,
    /*  40: */ 238, 234, 206, 202, 236, 232, 204, 200,
    /*  48: */ 126, 122,  94,  90, 124, 120,  92,  88,
    /*  56: */ 110, 106,  78,  74, 108, 104,  76,  72,
    /*  64: */ 247, 243, 215, 211, 245, 241, 213, 209,
    /*  72: */ 231, 227, 199, 195, 229, 225, 197, 193,
    /*  80: */ 119, 115,  87,  83, 117, 113,  85,  81,
    /*  88: */ 103,  99,  71,  67, 101,  97,  69,  65,
    /*  96: */ 246, 242, 214, 210, 244, 240, 212, 208,
    /* 104: */ 230, 226, 198, 194, 228, 224, 196, 192,
    /* 112: */ 118, 114,  86,  82, 116, 112,  84,  80,
    /* 120: */ 102,  98,  70,  66, 100,  96,  68,  64,
    /* 128: */ 191, 187, 159, 155, 189, 185, 157, 153,
    /* 136: */ 175, 171, 143, 139, 173, 169, 141, 137,
    /* 144: */  63,  59,  31,  27,  61,  57,  29,  25,
    /* 152: */  47,  43,  15,  11,  45,  41,  13,   9,
    /* 160: */ 190, 186, 158, 154, 188, 184, 156, 152,
    /* 168: */ 174, 170, 142, 138, 172, 168, 140, 136,
    /* 176: */  62,  58,  30,  26,  60,  56,  28,  24,
    /* 184: */  46,  42,  14,  10,  44,  40,  12,   8,
    /* 192: */ 183, 179, 151, 147, 181, 177, 149, 145,
    /* 200: */ 167, 163, 135, 131, 165, 161, 133, 129,
    /* 208: */  55,  51,  23,  19,  53,  49,  21,  17,
    /* 216: */  39,  35,   7,   3,  37,  33,   5,   1,
    /* 224: */ 182, 178, 150, 146, 180, 176, 148, 144,
    /* 232: */ 166, 162, 134, 130, 164, 160, 132, 128,
    /* 240: */  54,  50,  22,  18,  52,  48,  20,  16,
    /* 248: */  38,  34,   6,   2,  36,  32,   4,   0,
    };

    /* 
     * This is the value type for the _HP_RGB_SHADING_MAP property.
     * 	There may be one entry per Visual.
     */
    typedef struct {
	    VisualID	visualid;
	    int		interpolation_ramp;
    } hp_rgb_shading_map_type;
    hp_rgb_shading_map_type *shading_map_data;
    int shading_map_count;
    Atom property_atom,actual_type;
    unsigned long item_count,item_count_return,bytes_unread;
    int actual_format,result,ramp_index,i;


    /* Maybe our "spy window" already knows the answer. */
    if (cstate.twisted_elk) {
	if (IS_666_CMAP(stdCmap)) {
	    return(hp_crx_666_dithertable);
	}
	else {
	    return(hp_crx_884_dithertable);
	}
    }

    if (!(cstate.vendor & VENDOR_HP)) {
	return(NULL);
    }

    if ((property_atom = XInternAtom(xs.display,"_HP_RGB_SHADING_MAP",True))
	    == 0) {
	return(NULL);
    }

    item_count = sizeof(hp_rgb_shading_map_type)/4;
    bytes_unread = 0;
    do {
	item_count += (bytes_unread+3)/4;
	result = XGetWindowProperty(xs.display,RootWindowOfScreen(xs.screen),
	    property_atom,
	    0, item_count,False,property_atom,
	    &actual_type, &actual_format,
	    &item_count_return, &bytes_unread,
	    (unsigned char **) &shading_map_data);
    } while ((result == Success) && (bytes_unread > 0));
    if (result != Success) return(NULL);

    ramp_index = 0;
    shading_map_count = item_count_return/(sizeof(hp_rgb_shading_map_type)/4);
    for (i=0; i<shading_map_count; ++i) {
	if (visinfo->visualid == shading_map_data[i].visualid) {
	    ramp_index = shading_map_data[i].interpolation_ramp;
	    break;
	}
    }

    if (item_count_return == 0) XFree(shading_map_data);
    if (ramp_index == 0) return(NULL);

    if ((ramp_index == HPPEX_INTERP_RAMP_884_CRX)
	    && IS_332_CMAP(stdCmap)) {
	cstate.twisted_elk = TRUE;
	return(hp_crx_884_dithertable);
    }
    else if ((ramp_index == HPPEX_INTERP_RAMP_666_CRX) 
	    && IS_666_CMAP(stdCmap)) {
	cstate.twisted_elk = TRUE;
	return(hp_crx_666_dithertable);
    }
    else {
	return(NULL);
    }
}


static void twist_cmap(
    XStandardColormap *stdCmap,
    Colormap cmap,
    int num_colors,
    short *dither_table)
{
    XColor source_colors[256],new_colors[256];
    int i,j;

    for (j=0,i=stdCmap->base_pixel; j<num_colors; ++i,++j) {
	source_colors[j].pixel = i;
    }
    XQueryColors(xs.display,cmap,source_colors,num_colors);

    /* Copy old to new, but shuffle them. */
    for (j=0,i=stdCmap->base_pixel; j<num_colors; ++i,++j) {
	new_colors[j].flags = DoRed|DoGreen|DoBlue;
	new_colors[j].pixel = dither_table[i];
	new_colors[j].red   = source_colors[j].red;
	new_colors[j].green = source_colors[j].green;
	new_colors[j].blue  = source_colors[j].blue;
    }
    XStoreColors(xs.display,cmap,new_colors,num_colors);
}


/* Look up the best color in the xs colormap */
static Pixel rgbToPixel(
    XVisualInfo *vinfo,
    XStandardColormap *stdCmap,
    XColor *cmapContents,
    int cmap_size,
    int r, int g, int b)	/* Range is 0-COLORMAX */
{
    if ((vinfo->class == StaticGray)
	    || (vinfo->class == GrayScale)) {
	return((Pixel) (stdCmap->base_pixel + 
	    (r*GRAY_RED_WEIGHT + g*GRAY_GREEN_WEIGHT + b*GRAY_BLUE_WEIGHT)
		* stdCmap->red_max/(GRAY_WEIGHT_BASE*COLORMAX)
		* stdCmap->red_mult));
    }
    else if (cmap_size > 256) {
	/* Treat as Directcolor or Truecolor */
	return((Pixel) (stdCmap->base_pixel +
	    ((unsigned long) (0.5 + ((float) r/COLORMAX*stdCmap->red_max)) *
		stdCmap->red_mult) +
	    ((unsigned long) (0.5 + ((float) g/COLORMAX*stdCmap->green_max)) *
		stdCmap->green_mult) +
	    ((unsigned long) (0.5 + ((float) b/COLORMAX*stdCmap->blue_max)) *
		stdCmap->blue_mult)));
    }
    else {
	/* Though we'd like to trust the information in stdCmap,
	 * we know ELK lies and the information is bogus, so
	 * do a colormap search.
	 */
	Pixel best = cmapContents->pixel;
	int i;
	XColor *xc;
	int dr,dg,db,mindist;

	mindist = 0x7fffffff;
	for (i=0,xc=cmapContents; i<cmap_size; ++i,++xc) {
	    if ((dr = r - (int) xc->red) < 0) dr = -dr;
	    if (dr > mindist) continue;
	    if ((dg = g - (int) xc->green) < 0) dg = -dg;
	    if ((dr += dg) > mindist) continue;
	    if ((db = b - (int) xc->blue) < 0) db = -db;
	    if ((dr += db) > mindist) continue;

	    if (dr == 0) return(xc->pixel);
	    /* else dr < mindist:  best so far */
	    mindist = dr;
	    best = xc->pixel;
	}
	return(best);
    }
}


static void create_translation_table(
    XVisualInfo *vinfo,
    XStandardColormap *stdCmap,
    XColor *cmapContents,
    int cmap_size,
    Pixel map332ToPixel[256])
{
    static int eightval[] = {
	COLORMAX*0/7,
	COLORMAX*1/7,
	COLORMAX*2/7,
	COLORMAX*3/7,
	COLORMAX*4/7,
	COLORMAX*5/7,
	COLORMAX*6/7,
	COLORMAX*7/7,
    };
    static int fourval[] = {
	COLORMAX*0/3,
	COLORMAX*1/3,
	COLORMAX*2/3,
	COLORMAX*3/3,
    };
    int r,g,b,i;

    i = 0;
    for (r=0; r<8; ++r) {
	for (g=0; g<8; ++g) {
	    for (b=0; b<4; ++b) {
		map332ToPixel[i++] =
		    rgbToPixel(vinfo,stdCmap,cmapContents,cmap_size,
			eightval[r],eightval[g],fourval[b]);
	    }
	}
    }
}


static void create_standard_colormap(
    XStandardColormap *stdCmap,
    XVisualInfo *vinfo,
    Atom which_std_cmap)
{
    int count;
    Colormap cmap;
    XColor defs[256];
    int i,j,k,l,r,g;
    short *dither_table;

    /* Initialize the cmap if it's not too big */
    count = (stdCmap->red_max+1)
	* (stdCmap->green_max+1)  
	* (stdCmap->blue_max+1);
    if (count <= 256) {
	l = 0;
	for (i=0; i<=stdCmap->red_max; ++i) {
	    r = COLORMAX * i / stdCmap->red_max;
	    for (j=0; j<=stdCmap->green_max; ++j) {
		g = COLORMAX * j / stdCmap->green_max;
		for (k=0; k<=stdCmap->blue_max; ++k) {
		    defs[l].pixel = l+stdCmap->base_pixel;
		    defs[l].flags = (DoRed|DoGreen|DoBlue);
		    defs[l].red   = r;
		    defs[l].green = g;
		    defs[l].blue  = COLORMAX * k / stdCmap->blue_max;
		    ++l;
		}
	    }
	}
    }

    /* Create the colormap */
    if ((count <= 256) || ((vinfo->class & 0x01) == 0)) {
	cmap = XCreateColormap(xs.display,RootWindowOfScreen(xs.screen),
	    vinfo->visual,AllocNone);
    }
    else {
	cmap = XCreateColormap(xs.display,RootWindowOfScreen(xs.screen),
	    vinfo->visual,AllocAll);
    }

    switch (vinfo->class) {
	case PseudoColor:
	case DirectColor:
    	case GrayScale:
	    if (count <= 256) {
		unsigned long pixels[256],planes[8];
		for (i=0; i<count+stdCmap->base_pixel; ++i) pixels[i] = i;
		/* allocate them all */
		if (XAllocColorCells(xs.display,cmap,True,planes,0,pixels,
			count+stdCmap->base_pixel)) {
		    /* If we're not using the whole colormap... */
		    if (stdCmap->base_pixel > 0) {
			/* First, initialize the unused ones to the same
			 * as the default colormap.
			 */
			XColor defs2[256];
			for (i=0; i<stdCmap->base_pixel; ++i) defs2[i].pixel=i;
			XQueryColors(xs.display,
			    DefaultColormapOfScreen(xs.screen),
			    defs2,stdCmap->base_pixel);
			XStoreColors(xs.display,cmap,defs2,stdCmap->base_pixel);
			/* Now free them */
#ifdef FREE_COLORS_WHEN_POSSIBLE
			/* If FREE_COLORS_WHEN_POSSIBLE isn't set, it might
			 * be because doing so messed up ELK colors,
			 * so we'll just keep them.
			 */
			XFreeColors(xs.display,cmap,pixels,
			    stdCmap->base_pixel,0);
#endif /* FREE_COLORS_WHEN_POSSIBLE */
		    }
		    XStoreColors(xs.display,cmap,defs,count);
		    if ((dither_table = get_special_ELK_cmap(stdCmap,vinfo))) {
			/* Warp that colormap! */
			twist_cmap(stdCmap,cmap,count,dither_table);
		    }
		}
	    }
	    break;
    }

    /* Verify good match between server mode and SB_X_SHARED_CMAP env var */
    if (count <= 256) {
	/* Set "SB_X_SHARED_CMAP" appropriately. */
	if (IS_666_CMAP(stdCmap)
	    	&& !getenv("SB_X_SHARED_CMAP")) {
	     putenv("SB_X_SHARED_CMAP=TRUE");
	}
	else if (IS_332_CMAP(stdCmap)
	    	&& getenv("SB_X_SHARED_CMAP")) {
	    sharedWarning();
	}
    }

    /* Let everyone else know you've created this. */
    stdCmap->colormap = cmap;
    if (which_std_cmap != 0) {
#ifdef SHARE_STANDARD_COLORMAP
	/* Must set it up to be retained after I'm dead. */
	XSetRGBColormaps(xs.display,RootWindowOfScreen(xs.screen),
	    stdCmap,1,which_std_cmap);
#endif /* SHARE_STANDARD_COLORMAP */
    }
}


static Boolean check_standard_colormap_type(
    Atom which_std_cmap,
    VisualID visualid,
    XStandardColormap *goodCmap)
{
    XStandardColormap *returned;
    int count,i;

    if (XGetRGBColormaps(xs.display, RootWindowOfScreen(xs.screen),
	    &returned, &count, which_std_cmap)) {
	for (i=0; i<count; ++i) {
	     if (((returned+i)->visualid == visualid)
		    && ((returned+i)->red_max > 0)
		    && ((returned+i)->red_mult > 0)
		    && ((returned+i)->green_max > 0)
		    && ((returned+i)->green_mult > 0)
		    && ((returned+i)->blue_max > 0)
		    && ((returned+i)->blue_mult > 0)) {
		/* A good standard colormap! */
		memcpy(goodCmap,returned+i,sizeof(XStandardColormap));
		return(True);
	    }
	}
    }
    /* else */
    return(False);
}


static void constructStdCmapFromVisualInfo(
    XVisualInfo *vinfo,
    XStandardColormap *stdCmap)
{
    unsigned int mask,mult;
    static XStandardColormap defaultStdCmap666 = {
	0,		/* colormap */
	5,36,		/* red */
	5,6,		/* green */
	5,1,		/* blue */
	40,		/* base_pixel */
	0,		/* visualid */
	0		/* killid */
    };
    static XStandardColormap defaultStdCmap12 = {
	0,		/* colormap */
	0x0f,0x00100,	/* red */
	0x0f,0x00010,	/* green */
	0x0f,0x00001,	/* blue */
	0,		/* base_pixel */
	0,		/* visualid */
	0		/* killid */
    };
    static XStandardColormap defaultStdCmap24 = {
	0,		/* colormap */
	0xff,0x10000,	/* red */
	0xff,0x00100,	/* green */
	0xff,0x00001,	/* blue */
	0,		/* base_pixel */
	0,		/* visualid */
	0		/* killid */
    };

    if ((vinfo->red_mask == 0)
	    || (vinfo->green_mask == 0)
	    || (vinfo->blue_mask == 0)) {
	if (vinfo->depth >= 24) {
	    memcpy(stdCmap,&defaultStdCmap24,sizeof(XStandardColormap));
	}
	else if (vinfo->depth == 12) {
	    memcpy(stdCmap,&defaultStdCmap12,sizeof(XStandardColormap));
	}
	else {
	    memcpy(stdCmap,&defaultStdCmap666,sizeof(XStandardColormap));
	}
	stdCmap->visualid = vinfo->visualid;
	return;
    }

    mask = vinfo->red_mask; mult = 1;
    while ((mask & 0x01) == 0) {
	mask >>= 1; mult <<= 1;
    }
    stdCmap->red_max = mask;
    stdCmap->red_mult = mult;

    mask = vinfo->green_mask; mult = 1;
    while ((mask & 0x01) == 0) {
	mask >>= 1; mult <<= 1;
    }
    stdCmap->green_max = mask;
    stdCmap->green_mult = mult;

    mask = vinfo->blue_mask; mult = 1;
    while ((mask & 0x01) == 0) {
	mask >>= 1; mult <<= 1;
    }
    stdCmap->blue_max = mask;
    stdCmap->blue_mult = mult;

    stdCmap->colormap = 0;
    stdCmap->base_pixel = 0;
    stdCmap->visualid = vinfo->visualid;
    stdCmap->killid = 0;
}
#endif


void get_standard_colormap(
    XVisualInfo *vinfo,
    XStandardColormap *stdCmap,
    int *cmap_size,
    Pixel map332ToPixel[256])
{
#if defined(MOTIF_GUI)
    int i;
    static XStandardColormap defaultStdCmap666 = {
	0,		/* colormap */
	5,36,		/* red */
	5,6,		/* green */
	5,1,		/* blue */
	40,		/* base_pixel */
	0,		/* visualid */
	0		/* killid */
    };
    static XStandardColormap defaultStdCmap332 = {
	0,		/* colormap */
	7,0x20,		/* red */
	7,0x04,		/* green */
	3,0x01,		/* blue */
	0,		/* base_pixel */
	0,		/* visualid */
	0		/* killid */
    };
    XColor cmapContents[256];
    Atom which_std_cmap;

    if (check_standard_colormap_type(XA_RGB_DEFAULT_MAP,vinfo->visualid,
	    stdCmap)) {
	which_std_cmap = XA_RGB_DEFAULT_MAP;
    }
    else if (check_standard_colormap_type(XA_RGB_BEST_MAP,vinfo->visualid,
	    stdCmap)) {
	which_std_cmap = XA_RGB_BEST_MAP;
    }
    else if (check_standard_colormap_type(XA_RGB_GRAY_MAP,vinfo->visualid,
	    stdCmap)) {
	which_std_cmap = XA_RGB_GRAY_MAP;
    }
    else {
	if ((vinfo->depth > 8)
		|| !(cstate.vendor & VENDOR_HP)) {
	    constructStdCmapFromVisualInfo(vinfo,stdCmap);
	}
	else {
	    if (getenv("SB_X_SHARED_CMAP")) {
		/* Use 666 colormap */
		memcpy(stdCmap,&defaultStdCmap666,
		    sizeof(XStandardColormap));
	    }
	    else {
		/* Use 332 colormap */
		memcpy(stdCmap,&defaultStdCmap332,
		    sizeof(XStandardColormap));
	    }
	}
	stdCmap->visualid = vinfo->visualid;
	which_std_cmap = 0;
    }

#ifdef OVERRIDE_HP_332
    /* See if we need to, alas, override 332 on HP.  Sigh.
     * But, only do it for Starbase.  If we force 666 for PEX, and
     * the server is not started in 666 mode, the colors are HOSED --
     * PEX will generate 332 pixels unless the server was started with a
     * shared colormap!
     */

    if(!  cstate.pex_version)
	if ((cstate.vendor & VENDOR_HP)
		&& (vinfo->depth == 8)
		&& (MaxCmapsOfScreen(xs.screen) == 1)
		&& IS_332_CMAP(stdCmap)) {
	    putenv("SB_X_SHARED_CMAP=true");
	    memcpy(stdCmap,&defaultStdCmap666,sizeof(XStandardColormap));
	    stdCmap->visualid = vinfo->visualid;
	    which_std_cmap = 0;
	}
#endif /* OVERRIDE_HP_332 */

    /* If no one has created it, we'll have to create it ourself */
#ifdef CHECK_FOR_CMAP_REUSE
    if (stdCmap->colormap == 0)
#endif /* CHECK_FOR_CMAP_REUSE */
    {
	create_standard_colormap(stdCmap,vinfo,which_std_cmap);
    }
    /* else just use the one we've already got. */

    /* Got a good colormap.  Query it, if it's not too big. */
    *cmap_size = (stdCmap->red_max+1)
	* (stdCmap->green_max+1)  
	* (stdCmap->blue_max+1);
    if (*cmap_size <= 256) {
	for (i=0; i<(*cmap_size); ++i) {
	    cmapContents[i].pixel = i + stdCmap->base_pixel;
	}
	XQueryColors(xs.display,stdCmap->colormap,
	    cmapContents,*cmap_size);
    }

    create_translation_table(vinfo,stdCmap,cmapContents,*cmap_size,map332ToPixel);
#endif
}


void colornameToRGB(
    char *colorname,
    float *r, float *g, float *b)	/* 0.0 to 1.0 */
{
#if defined(MOTIF_GUI)
    XColor rgb_db_def,hardware_def;

    /* In case we're called before the GUI is created... */
    if (xs.display == NULL) {
	*r = *g = *b = 1.0;
	return;
    }

    XLookupColor(xs.display, xs.dashInfo.stdCmap.colormap, colorname,
	&rgb_db_def, &hardware_def);
    *r = (float) rgb_db_def.red   / COLORMAX;
    *g = (float) rgb_db_def.green / COLORMAX;
    *b = (float) rgb_db_def.blue  / COLORMAX;
#endif
}


#if defined(MOTIF_GUI)
/* Change pixmap from 332 to the appropriate Pixels */
static void convertPixmap(
    unsigned char *data,
    int width, int height)
{
    int count;
    unsigned char *tdata,*cptr;
    unsigned long *ldata;

    count = width*height;

    if (xs.dashInfo.cmap_size > 256) {
	/* Make temporary copy */
	if ((tdata = (unsigned char *) malloc(count)) == NULL) return;
	memcpy(tdata,data,count);

	/* Build the thing from the temporary copy */
	ldata = (unsigned long *) data;
	cptr = tdata;
	while (count--) {
	    *ldata++ = (unsigned long) xs.dashInfo.map332ToPixel[*cptr++];
	}

	/* free up the temporary copy */
	free(tdata);
    }
    else {
	while (count--) {
	    *data = (unsigned char) xs.dashInfo.map332ToPixel[*data];
	    ++data;
	}
    }
}


#define DOWNGRAN	2
#define K_RED	(DOWNGRAN*DOWNGRAN*(1<<5))
#define K_GRN	(DOWNGRAN*DOWNGRAN*(1<<2))
#define K_BLU	(DOWNGRAN*DOWNGRAN)

static void downsamplePixmap(
    unsigned char *in,
    unsigned char *out,
    int width, int height)
{
    unsigned char *cptr;
    int x,y,yp;
    unsigned char *p0,*p1;
    unsigned int r,g,b,indx,rt,gt,bt;


    /* 8-bit 332 CMAP_FULL */
    cptr = out;
    for (y=0,yp=0; y<height/DOWNGRAN; ++y,yp+=DOWNGRAN) {
	p0 = in + yp     * width;
	p1 = in + (yp+1) * width;
	r = (K_RED/2); g = (K_GRN/2); b = (K_BLU/2);
	for (x=0; x<width/DOWNGRAN; ++x) {
	    indx = *p0++;
	    r += indx & 0xe0;
	    g += indx & 0x1c;
	    b += indx & 0x03;
	    indx = *p1++;
	    r += indx & 0xe0;
	    g += indx & 0x1c;
	    b += indx & 0x03;
	    indx = *p0++;
	    r += indx & 0xe0;
	    g += indx & 0x1c;
	    b += indx & 0x03;
	    indx = *p1++;
	    r += indx & 0xe0;
	    g += indx & 0x1c;
	    b += indx & 0x03;

	    rt = r / K_RED; gt = g / K_GRN; bt = b / K_BLU;
	    r %= K_RED; g %= K_GRN; b %= K_BLU;
	    *cptr++ = (rt << 5) | (gt << 2) | bt;
	}
    }
}
#endif

void getPixmap(
    char *filename,
    int width, int height,
    unsigned char **ldata,
    unsigned char **sdata,
    Boolean downsample)
{
#if defined(MOTIF_GUI)
    FILE *fptr;
    char cfilename[MAXPATHLEN],
	tmpfilename[L_tmpnam],
	cmd[MAXPATHLEN+L_tmpnam+20];
    boolean_type made_tmpfile = FALSE;
    int size;

    if (xs.dashInfo.cmap_size > 256) size = width*height*4;
    else size = width*height;

    if (*ldata == NULL) {
	if ((*ldata = (unsigned char *) malloc(size)) == NULL) return;
    }
    if (downsample && (*sdata == NULL)) {
	if ((*sdata = (unsigned char *) malloc((size+3)/4)) == NULL) {
	    free(*ldata);
	    *ldata = NULL;
	    return;
	}
    }

    if ((fptr = fopen(filename,"r")) == NULL) {
	static char *uncompressor[] = {
	    "/usr/bin/uncompress",
	    "/bin/uncompress",
	    "/bin/gunzip",
	    "/usr/bin/gunzip",
	};
#define UNCOMPRESSOR_COUNT (sizeof(uncompressor)/sizeof(char *))
	int i;

	/* See if we can find a compressed version */
	sprintf(cfilename,"%s.Z",filename);
	if (access(cfilename,READ_ACCESS) == ACCESS_ERROR) {
	    fprintf(stderr,"Cannot open file %s\n",filename);
	    free(*ldata); free(*sdata);
	    *ldata = *sdata = NULL;
	    return;
	}
	/* else */

	for (i=0; i<UNCOMPRESSOR_COUNT; ++i) {
	    if (access(uncompressor[i], EXECUTE_ACCESS) == ACCESS_OKAY) {
	        break;
	    }
	}
	if (i == UNCOMPRESSOR_COUNT) {
	    fprintf(stderr,"Cannot find uncompressor program for file %s.Z\n",
	        filename);
	    free(*ldata); free(*sdata);
	    *ldata = *sdata = NULL;
	    return;
	}
	/* else */

	tmpnam(tmpfilename);
	sprintf(cmd,"%s <%s >%s",uncompressor[i],cfilename,tmpfilename);
	system(cmd);
	if ((fptr = fopen(tmpfilename,"r")) == NULL) {
	    fprintf(stderr,"Cannot open file %s\n",filename);
	    free(*ldata); free(*sdata);
	    *ldata = *sdata = NULL;
	    unlink(tmpfilename);
	    return;
	}
	made_tmpfile = TRUE;
    }

    if (fread((void *) (*ldata),1,width*height,fptr) != (width*height)) {
	fprintf(stderr,"Cannot read file %s\n",filename);
	fclose(fptr);
	free(*ldata); free(*sdata);
	*ldata = *sdata = NULL;
	if (made_tmpfile) unlink(tmpfilename);
	return;
    }

    fclose(fptr);
    if (made_tmpfile) unlink(tmpfilename);

    if (downsample) downsamplePixmap(*ldata,*sdata,width,height);

    convertPixmap(*ldata,width,height);
    if (downsample) convertPixmap(*sdata,width/2,height/2);
#endif
}
