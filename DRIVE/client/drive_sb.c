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


/* 
 * drive_sb.c 
 */


#include "hw.h"
#include <stdio.h>
#include <math.h>
#include <errno.h>

#ifndef WIN32
#include <time.h>

#ifndef MAC
#include <X11/X.h>
#include <X11/Xlib.h>
#endif

#endif

#include "global.h"
#include "drive.h"
#if !(defined(WIN32) || defined(MAC))
#include "gui.h"
#include "wsutils.h"
#endif

/****************************** Extern GLOBALS ********************************/
extern radar_type radar;
extern float StarMag1[][3], StarMag2[][3], StarMag3[][3], StarMag4[][3];
extern int NumMag1, NumMag2, NumMag3, NumMag4;
extern float camera_pos[3];
extern int showfps;

/****************************** MODULE GLOBALS ********************************/
static float identity[4][4] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
};


void setup_img_fildes(
    void)
{
}

void auto_rotate_vehicle(
    void)
{
    unsigned int save_camera_mode;
    int i;
    int done=0;
    fd_set server_mask;
    int status;
    static int all_invis_bits = ALL_INVISIBILITY_BITS;
    static int def_invis_bits = DEFAULT_INVISIBILITY_BITS;
#ifndef WIN32
    struct itimerval timer = {0,0,0,0};
#endif
    extern hwDisplay
	disp;

    disp->setInvisibility( disp,
		ALL_INVISIBILITY_BITS, DEFAULT_INVISIBILITY_BITS );

    while (!done
	    && ((cstate.state == POST_RACE_STATE)
		|| (cstate.state == WELCOME_STATE))) {
	/* Set up mask to watch server socket. */
	FD_ZERO( &server_mask );
	FD_SET( server_socket->socketnum, &server_mask );

#ifndef WIN32
	/* Block until we have input on the socket. */
	status = select( FD_SETSIZE, 
	    (int *) &server_mask, (int *)0, (int *)0,
	    &(timer.it_value) );
	if (status < 0 && errno != EINTR) {
	    perror( "Client select" );
	    exit( -1 );
	}
	if (status < 0) continue;	/* signal, not an error */
#endif

	/* Check socket for pending messages. */
	if ( FD_ISSET( server_socket->socketnum, &server_mask ) )
	    done = client_read();
    
	/* Update camera. */
	save_camera_mode = cstate.camera.mode;
	cstate.camera.mode = CAM_MODE_PREVIEW;
	cam_update(img_fildes, &cstate.camera, identity);
	cstate.camera.mode = save_camera_mode;

	{
	    static hwSurfaceType surf;
	    surf.color[0] = cstate.upd.color[0];
	    surf.color[1] = cstate.upd.color[1];
	    surf.color[2] = cstate.upd.color[2];
	    disp->surfAttrs( disp, &surf );
	}

	/* Decide which display list to show off */
	for (i=0; i< num_cars; i++) {
	    if (strcmp(driveables[i].name,cstate.car_name) == 0) {
		draw_hw_seg( (float *)0, driveables[i].display_list );
		disp->update( disp, HW_UPDATE_ALL );
	    }
	}

	/* Let people unconfine their mouse, etc */
	processXEvents();
    }
}


#define SIGHT_SEGMENTS		32

static void draw_sight(
    int fildes,
    int width, int height,
    boolean_type do_crosshair)
{
#if 0
    int radius,xc,yc,dy,y,x2;
    static int circle[SIGHT_SEGMENTS][2];
    static int lastradius = INVALID,
	lastxc = INVALID,
	lastyc = INVALID;

    radius = MIN(width,height)/2 - SIGHT_MARGIN;
    xc = width/2;
    yc = height/2;

    if ((lastradius != radius) || (lastxc != xc) || (lastyc != yc)) {
	double angle = 0.0;
	double d_angle = (M_PI*2.0/(SIGHT_SEGMENTS-1));
	int *iptr = (int *) circle;
	int i;

	/* reconstruct circle */
	for (i=0,angle=0.0; i<SIGHT_SEGMENTS; ++i,angle+=d_angle) {
	    *iptr++ = xc + (int) (FCOS(angle)*radius + 0.5);
	    *iptr++ = yc + (int) (FSIN(angle)*radius + 0.5);
	}

	lastradius = radius;
	lastxc = xc;
	lastyc = yc;
    }
		
    line_color(fildes,1.0,1.0,1.0);
    dcpolyline(fildes,(int *) circle,SIGHT_SEGMENTS,FALSE);

    if (do_crosshair) {
	dcmove(fildes,xc-radius,yc); dcdraw(fildes,xc+radius,yc);
	dcmove(fildes,xc,yc-radius); dcdraw(fildes,xc,yc+radius);

	dy = radius/SIGHT_ELEVATION_MARKS;
	x2 = xc + 20;

	/* elevation marks */
	for (y = yc-dy; y > yc-radius+dy/2; y -= dy) {
	    dcmove(fildes,xc,y); dcdraw(fildes,x2,y);
	}
    }
#endif
}


/*****************************************************************
 * check_background_color
 *	See if we need to change the background color (are we in space yet?)
 */

void check_background_color(
    void)
{
    float num;
    static int last_color = 0;  /* Start out with blue sky */
				/* 0 = blue sky */
				/* 1 = transitions */
				/* 2 = black */
    static float starmat[4][4] = {
	{ 700.0,   0.0,   0.0, 0.0 },
	{   0.0, 700.0,   0.0, 0.0 },
	{   0.0,   0.0, 700.0, 0.0 },
	{   0.0,   0.0,   0.0, 1.0 },
    };
    extern hwDisplay disp;
    float color[3];

    if( radar.ypos < 1000.0)
    {
	if( 1/*last_color != 0*/)
	{
	    daylight_update(img_fildes, &cstate.daylight, TRUE);
	    last_color = 0;
	    disp->doFog( disp, driveFogAmount > 0.0, cstate.daylight.back_clr );
	    disp->backgroundColor( disp, cstate.daylight.back_clr );
	}
    }
    else if( radar.ypos < 1500.0 )
    {
	num = 1.0 - (radar.ypos - 1000.0 ) / 500.0;
	color[0] = cstate.daylight.back_clr[0]*num,
	color[1] = cstate.daylight.back_clr[1]*num,
	color[2] = cstate.daylight.back_clr[2]*num;
	if( 1/*last_color != 1*/)  {
	    disp->doFog( disp, driveFogAmount > 0.0, color );
	    disp->backgroundColor( disp, cstate.daylight.back_clr );
	}
	last_color = 1;
    }
    else {
	if( 1/*last_color != 2*/)
	{
	    last_color =2;
	    color[0] = 0.0;
	    color[1] = 0.0;
	    color[2] = 0.0;
	    disp->doFog( disp, HW_FALSE, color );
	    disp->backgroundColor( disp, color );
	}
    }
    if( last_color > 0 ) {
	/* draw the stars */
	hwSurfaceType surf;

        hwDefaultSurf( &surf );

	surf.transp = 0.0;
	surf.specColor[0] = 1.0;
	surf.specColor[1] = 1.0;
	surf.specColor[2] = 1.0;
	surf.shininess = 0.0;
	surf.visibility = 0xFFFFFFFF;
	surf.flags = HW_SURF_EMISSIVE | HW_SURF_TWOSIDED;
	surf.internalFlags = 0;
	/*surf.texture = 0;*/

	starmat[3][0] = camera_pos[0];
	starmat[3][1] = camera_pos[1];
	starmat[3][2] = camera_pos[2];

	disp->pushMatrix( disp, starmat );

	surf.color[0] = 1.0; surf.color[1] = 1.0; surf.color[2] = 1.0;
	disp->surfAttrs( disp, &surf );
	disp->drawMarkers( disp, StarMag1, 0, NumMag1 );

	surf.color[0] = 0.7; surf.color[1] = 0.7; surf.color[2] = 0.7;
	disp->surfAttrs( disp, &surf );
	disp->drawMarkers( disp, StarMag2, 0, NumMag2 );

	surf.color[0] = 0.5; surf.color[1] = 0.5; surf.color[2] = 0.5;
	disp->surfAttrs( disp, &surf );
	disp->drawMarkers( disp, StarMag3, 0, NumMag3 );

	surf.color[0] = 0.3; surf.color[1] = 0.3; surf.color[2] = 0.3;
	disp->surfAttrs( disp, &surf );
	disp->drawMarkers( disp, StarMag4, 0, NumMag4 );

	disp->popMatrix( disp );
    }
}

/*****************************************************************
 * client_update
 * 
 * 	Draw one frame.
 * 	
 * Inputs:
 *		none.
 * Outputs:
 *		nothing.
 */
void client_update(
    void)
{
    static unsigned long snapshot_frames = 0;
    static struct timeval start_snapshot_time;
    struct timeval stop_snapshot_time;
#ifndef WIN32
    struct timezone tz;
#endif
    unsigned long snapshot_sec, snapshot_usec = 1;
    float fseconds;
    extern hwDisplay
	disp;

#ifdef GLFW
    /* While the vehicle picker is up it owns the window and does its own
     * swap in redraw_spinning_vehicle().  Drawing the world here as well
     * makes the two alternate in the front buffer, which flickers badly.
     */
    if (dashPickerActive()) return;
#endif

    /* Draw frame and send X input to server. */
    daylight_update(img_fildes, &cstate.daylight, FALSE);
    check_background_color();
#ifdef GLFW
    /* Instruments go on top of the scene, just before the buffer swap.
     * drawDashboard() widens the viewport to the whole window so the GUI
     * ortho covers it; put the 3D viewport back once the flush is done.
     */
    if (cstate.car_selected) {
	drawDashboard();
	disp->update( disp, HW_UPDATE_ALL );
	dashEndOverlay();
    }
    else {
	disp->update( disp, HW_UPDATE_ALL );
    }
#else
    disp->update( disp, HW_UPDATE_ALL );
#endif
    cstate.frame_ready = FALSE;

    /* Frame rate display. */
    if ( showfps ) {
	if ( snapshot_frames >= 10 ) {
#ifndef WIN32
	    gettimeofday( &stop_snapshot_time, &tz );
#endif
	    if ( start_snapshot_time.tv_usec > stop_snapshot_time.tv_usec ) {
		stop_snapshot_time.tv_usec += 1000000;
		stop_snapshot_time.tv_sec--;
	    }

	    snapshot_sec = stop_snapshot_time.tv_sec
		         - start_snapshot_time.tv_sec;
	    snapshot_usec = stop_snapshot_time.tv_usec

		          - start_snapshot_time.tv_usec;

	    fseconds = (float)snapshot_sec + (float)snapshot_usec / 1000000.0;
	    fprintf(stderr, "rate: %5.2f\r",(float)snapshot_frames / fseconds);

	    /* Reset frame counter. */
#ifndef WIN32
	    gettimeofday( &start_snapshot_time, &tz );
#endif
	    snapshot_frames = 0;
	}
	else {
	    snapshot_frames++;
	}
    }

    /* Increment client frame count. */
    cstate.frames++;

    /* In autowatch mode, watch the next car every n frames. */
    if ( (cstate.mode & CLIENT_AUTOWATCH_MODE) &&
	 ((cstate.frames % cstate.watch_frames) == 0)) {
	client_restart();
    }

    /* Update automatic camera. */
    if ( cstate.camera.mode == CAM_MODE_AUTOCAM ) {
	cstate.camera.skycam_around_angle++;
	if (cstate.camera.skycam_around_angle > 359) {
	    cstate.camera.skycam_around_angle = 0;
	}

	cstate.camera.skycam_dirty = TRUE;
    }

    /* Draw sight if necessary */
    if ( (cstate.camera.mode == CAM_MODE_ALT_VIEW) || 
	 (cstate.camera.mode == CAM_MODE_ZOOM_VIEW) ) {
	draw_sight(img_fildes,cstate.gWinWidth,cstate.gWinHeight,
	    (cstate.camera.direction == CAM_DIRECTION_FRONT));
    }
}

#if 0

/* Returns whether to force small windows */
int check_sb_device(
    Display *display)
{
    char *sb_dev;
    int fildes;
    int image_banks, image_planes, planes_per_bank, overlay_planes;
    Window win;
    float rev; char model[32]; int type;

    /* the following call creates a window we call spy_window... */

    sb_dev = sb_make_window(display,"100x100+0+0",FALSE,"Thinking..",
	SB_CMAP_TYPE_NORMAL,8,FLEXIBLE,0,NULL,&win);

    if (debug)
	printf("sb_dev = %s\n",sb_dev);

    fildes = gopen(sb_dev,OUTDEV,"",0);
    free((void *) sb_dev);    /* malloced by make_X11_gopen_string */
    inquire_fb_configuration(fildes,&image_banks, &image_planes,
	   &planes_per_bank, &overlay_planes);
    if (debug) {
	printf("image banks: %d\n",image_banks);
	printf("image planes: %d\n",image_planes);
	printf("overlay planes: %d\n",overlay_planes);
    }
    inquire_id(fildes,&rev,model,&type);

    /* Turn transparency off by default, then turn it on if
     * we're on one of the known fast devices.  It would be better
     * to do this programmatically, but there's not time to
     * code it right now.
     *
     * Note that this flag needs to be set before display lists
     * containing transparency are received.
     */
    cstate.disable_transparency = True;
    if (  (strcmp(model,"hpA1659a") == 0)		/* Elk */
       || (strcmp(model,"hpA2269a") == 0)) {		/* Dual Elk */
        cstate.twisted_elk = True;
    }
    else if (  (strcmp("hpA2091A",model) == 0)		/* Stinger */
	    || (strcmp("hp98766",model)  == 0)) {	/* Falcon */
        cstate.disable_transparency = False;
    }
    else if (strcmp("hpA1439a",model) == 0) {		/* Rattler/Pirahna */
	/* Could be Piranha, could be Rattler */
	char flags[SIZE_OF_CAPABILITIES];

	inquire_capabilities(fildes,sizeof(flags),flags);
	if (flags[PERF_HINTS_1_CAPABILITIES] & IC_HW_ZBUFFER) {
	    cstate.disable_transparency = False;
	}
    }
    gclose(fildes);

    XDestroyWindow(display,win);
    XSync(display,0);

    if ((image_banks == 1)
	    && (planes_per_bank == 8)) {
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}

#endif


void redraw_spinning_vehicle(
    int which_dl,
    boolean_type draw_half_transparent)
{
    unsigned int save_camera_mode;
    extern hwDisplay
	disp;

    /* Update camera. */
    save_camera_mode = cstate.camera.mode;
    cstate.camera.mode = CAM_MODE_PREVIEW;
    cam_update(img_fildes, &cstate.camera, identity);
    cstate.camera.mode = save_camera_mode;

    {
	static hwSurfaceType surf;
	surf.color[0] = cstate.upd.color[0];
	surf.color[1] = cstate.upd.color[1];
	surf.color[2] = cstate.upd.color[2];
	if( draw_half_transparent ) {
	    surf.transp = 0.5;
	}
	else {
	    surf.transp = 0.0;
	}
	disp->surfAttrs( disp, &surf );
    }
    disp->setInvisibility( disp,
		ALL_INVISIBILITY_BITS, DEFAULT_INVISIBILITY_BITS );
    draw_hw_seg( identity, which_dl );
#ifdef GLFW
    drawVehiclePicker();
#endif
    disp->update( disp, HW_UPDATE_ALL );
}


void resize_sb_window(
    int width, int height)
{
    extern hwDisplay
	disp;

    if( disp ) disp->viewport( disp, 0, 0, width, height );
}


#if 1 /* [ SB HACK! */

/* And concat_matrix */
void concat_matrix( float m1[4][4], float m2[4][4], float m3[4][4] )
{
    float
	t1[4][4],
	t2[4][4];

    (void)memcpy( t1, m1, 4*4*sizeof(float) );
    (void)memcpy( t2, m2, 4*4*sizeof(float) );
    hwMatMult( m3, t1, t2 );
}

#endif /* ] */
