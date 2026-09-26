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
#include <math.h>
#include "prims.h"
#ifdef RAYTRACE
#include <sbrr.c.h>
#endif

#define HS_PARMS FALSE,TRUE
#define PARAM_INC .25
#define WHITE_PERIM 1.0,1.0,1.0
#define RED_PERIM 1.0,0.0,0.0
#define SEGMENT 24
#define ANGLE M_PI/6.0
#define SLEEPTIME 1
#define CAMDIST 3.0

#ifdef WIREFRAME
#define DRAWIT(func)                                \
    {                                               \
	perimeter_color(fildes,RED_PERIM);          \
	hidden_surface(fildes,FALSE,FALSE);         \
	func;                                       \
	hidden_surface(fildes,HS_PARMS);            \
	perimeter_color(fildes,WHITE_PERIM);        \
	func;                                       \
    }
#else
#define DRAWIT(func) func;
#endif

extern char *getenv();

/* global file descriptors */
static int fildes,chcdev,locdev;


static void init()
{
    shade_mode(fildes,CMAP_FULL|INIT,TRUE);
    fill_color(fildes,0.8,0.8,0.8);
    perimeter_color(fildes,WHITE_PERIM);
    light_ambient(fildes,0.3,0.3,0.3);
    light_source(fildes,1,DIRECTIONAL,0.8,0.8,0.8,0.0,1.0,0.0);
    printf("white light from top\n");
    light_source(fildes,2,DIRECTIONAL,0.5,0.0,0.0,-1.0,0.0,0.0);
    printf("red light from left\n");
    light_source(fildes,3,DIRECTIONAL,0.0,0.5,0.0,0.0,0.0,-1.0);
    printf("green light from front\n");
    light_source(fildes,4,DIRECTIONAL,0.4,0.4,0.0,1.0,0.0,0.0);
    printf("yellow light from right\n");
    light_source(fildes,5,DIRECTIONAL,0.0,0.0,0.5,0.0,-1.0,0.0);
    printf("blue light from bottom\n");
    light_source(fildes,6,DIRECTIONAL,0.4,0.0,0.4,0.0,0.0,1.0);
    printf("purple light from behind\n");
    light_switch(fildes,0x7f);

    depth_indicator(fildes,FALSE,FALSE);
    clear_control(fildes,CLEAR_VDC_EXTENT|CLEAR_ZBUFFER);

#ifndef RAYTRACE
    double_buffer(fildes,TRUE|INIT,4);
#endif

#ifdef WIREFRAME
    curve_resolution(fildes,STEP_SIZE,
	PARAM_INC,PARAM_INC,PARAM_INC,PARAM_INC);
    interior_style(fildes,INT_HOLLOW,TRUE);
#else
    curve_resolution(fildes,DC_VALUES,5.0,5.0,2.0,2.0);
    interior_style(fildes,INT_SOLID,FALSE);
    hidden_surface(fildes,TRUE,TRUE);
#endif
}


static void drawit()
{
    static float poly[] = {
	0.6,    0.1,    0.0,
	0.54,   0.1,    0.0,
	0.5,    0.08,   0.0,
	0.46,   0.1,    0.0,
	0.42,   0.1,    0.0,
	0.4,    -0.1,   0.0,
	0.6,    -0.1,   0.0
    };
    static float path[] = {
	0.1,    0.0,    0.1,
	-0.1,   0.1,    0.08
    };
    float star[10][3];
    static float candlestick[] = {
	-.500,  0.650,  0.5,
	-.400,  0.650,  0.5,
	-.410,  0.630,  0.5,
	-.470,  0.630,  0.5,
	-.470,  0.400,  0.5,
	-.440,  0.370,  0.5,
	-.500,  0.370,  0.5
    };
    static float candlestick2[] = {
	-0.5,   0.65,   0.5,
	-0.475, 0.65,   0.5,
	-0.475, 0.68,   0.5,
	-0.45,  0.68,   0.5,
	-0.46,  0.625,  0.5,
	-0.475, 0.625,  0.5,
	-0.475, 0.61,   0.5,
	-0.47,  0.605,  0.5,
	-0.475, 0.59,   0.5,
	-0.475, 0.44,   0.5,
	-0.47,  0.43,   0.5,
	-0.475, 0.425,  0.5,
	-0.475, 0.415,  0.5,
	-0.44,  0.415,  0.5,
	-0.42,  0.40,   0.5,
	-0.44,  0.39,   0.5,
	-0.41,  0.375,  0.5,
	-0.5,   0.375,  0.5
    };
    static float circle[][4] = {
	1.0,        0.0,        0.0,    1.0,
	M_SQRT1_2,  M_SQRT1_2,  0.0,    M_SQRT1_2,
	0.0,        1.0,        0.0,    1.0,
	-M_SQRT1_2, M_SQRT1_2,  0.0,    M_SQRT1_2,
	-1.0,       0.0,        0.0,    1.0,
	-M_SQRT1_2, -M_SQRT1_2, 0.0,    M_SQRT1_2,
	0.0,        -1.0,       0.0,    1.0,
	M_SQRT1_2,  -M_SQRT1_2, 0.0,    M_SQRT1_2,
	1.0,        0.0,        0.0,    1.0
    };
    float circle2[9][4];
    static float circle_knot[] = {
	0.0,0.0,0.0,1.0,1.0,2.0,2.0,3.0,3.0,4.0,4.0,4.0
    };
    static float floor_poly[4][3] = {
	-2.0, -1.0, -2.0,
	 2.0, -1.0, -2.0,
	 2.0, -1.0,  2.0,
	-2.0, -1.0,  2.0
    };
    static float thingee[] = {
	.86,.86,.00,-.1,-.05,0.0,
	.82,.92,.00,-.05,.1,0.0,
	.86,.90,.00,.000,.1,0.0,
	.92,.93,.00,-.8,-.2,0.0,
	.95,.98,.00,.01,.07,0.0,
	.99,.92,.00,.1,.05,0.0,
	.86,.86,.00,-.1,-.05,0.0,
    };
    int poly_size,path_len;
    int i;
    double d;

    poly_size = sizeof(poly)/sizeof(float)/3;
    path_len  = sizeof(path)/sizeof(float)/3;

    /* construct circle2 for trimmed plane */
    for (i=0;i<9;++i) {
	circle2[i][0] = circle[i][0] * .2;
	circle2[i][1] = circle[i][1] * .2;
	circle2[i][2] = circle[i][2] * .2;
	circle2[i][3] = circle[i][3];
    }
    u_knot_vector(fildes,circle_knot,12);
    fill_color(fildes,0.9,0.6,0.4);
    surface_model(fildes,FALSE,1,0.0,0.0,0.0);
    DRAWIT(trimmed_plane(fildes,circle2,9,QUADRATIC,TRUE,TRUE));

    for (i=0,d=0.0;i<10;i+=2,d+=M_PI/2.5) {
	star[i][0] = (float) cos(d) * .2;
	star[i][1] = (float) sin(d) * .2 - .5;
	star[i][2] = star[i][0] - .5;
	star[i+1][0] = (float) cos(d+M_PI/5.0) *.1;
	star[i+1][1] = (float) sin(d+M_PI/5.0) *.1 - .5;
	star[i+1][2] = star[i+1][0] - .5;
    }
    fill_color(fildes,0.9,0.9,0.4);
    surface_model(fildes,TRUE,8,1.0,1.0,1.0);
    DRAWIT(trimmed_plane(fildes,star,10,CUBIC,FALSE,FALSE));

    fill_color(fildes,0.5,0.5,0.5);
    surface_model(fildes,TRUE,4,1.0,1.0,1.0);
    DRAWIT(mesh_cone(fildes,.15,.15,TRUE,TRUE,6,-0.3,-0.3,-0.3,-0.6,-0.6,-0.6));
    for (i=0;i<10;++i) {
	star[i][0] -= .5;
	star[i][1] += .5;
	star[i][2] += .5;
    }
    fill_color(fildes,0.5,0.4,0.8);
    surface_model(fildes,TRUE,4,1.0,1.0,1.0);
    DRAWIT(spline_extrusion(fildes,star,10,path,path_len,QUADRATIC,CUBIC,TRUE,TRUE,FALSE,FALSE));

    fill_color(fildes,0.8,0.8,0.8);
    surface_model(fildes,TRUE,32,1.0,1.0,1.0);
    DRAWIT(mesh_surface_of_revolution(fildes,candlestick,sizeof(candlestick)/sizeof(float)/3,8,FALSE,-0.6,0.5,0.5,-0.6,0.4,0.5)); 

    fill_color(fildes,0.5,1.0,0.5);
    surface_model(fildes,TRUE,16,1.0,1.0,1.0);
    DRAWIT(spline_cone(fildes,0.0,0.2,FALSE,TRUE,0.1,0.5,0.5,-0.2,0.6,0.6));

    fill_color(fildes,0.7,0.5,0.4);
    surface_model(fildes,TRUE,4,0.5,0.5,0.5);
    DRAWIT(spline_sphere(fildes,.2,0.5,0.5,0.5));

    fill_color(fildes,0.9,0.4,0.4);
    surface_model(fildes,FALSE,1,1.0,1.0,1.0);
    DRAWIT(extrusion(fildes,poly,poly_size,path,path_len,TRUE,TRUE));

    for (i=0;i<9;++i) {
	circle2[i][0] = circle[i][0] * .05 + .7 * circle[i][3];
	circle2[i][1] = circle[i][1] * .05 - .5 * circle[i][3];
	circle2[i][2] = circle[i][2] * .05 - .5 * circle[i][3];
	circle2[i][3] = circle[i][3];
    }
    fill_color(fildes,0.4,0.7,0.7);
    surface_model(fildes,TRUE,4,0.6,0.6,0.6);
    DRAWIT(spline_surface_of_revolution(fildes,circle2,9,QUADRATIC,TRUE,.5,-.5,-.5,.5,-.4,-.5));

    DRAWIT(mesh_torus(fildes,5,8,0.05,0.2,
	-.01,1.1,-0.5,-0.01,1.2,-0.5));
    DRAWIT(mesh_helix(fildes,8,16,0.01,0.05,
	-1.1,1.1,-0.5,-1.1,2.1,-0.5,4.0));

    fill_color(fildes,0.8,0.8,0.8);
    surface_model(fildes,TRUE,4,0.5,0.5,0.5);
    DRAWIT(mesh_surface_of_revolution(fildes,thingee,(sizeof(thingee)/6/sizeof(float)),6,TRUE,.8,.8,.0,1.0,.91,0.0));

    fill_color(fildes,0.5,0.5,0.5);
    surface_model(fildes,FALSE,4,0.5,0.5,0.5);
    polygon3d(fildes,floor_poly,4,FALSE);


    close_segment(fildes);

}


main(argc,argv)
int argc;
char *argv[];
{
    float x,y,z,oldx,oldy;
    int valid,value;
    static camera_arg cam = {
	0.0,0.0,0.0,  /* ref */
	0.0,0.0,-CAMDIST, /* cam */
	0.0,1.0,0.0,  /* up  */
	60.0,         /* FOV */
	.005,.005,    /* clip */
	CAM_PERSPECTIVE
    };
    double d,theta,phi;
    int i=0;

    locdev = gopen(getenv("SB_OUTDEV"),INDEV,"",INIT);
    chcdev = locdev;
    fildes = gopen(getenv("SB_OUTDEV"),OUTDEV,"",INIT|THREE_D|MODEL_XFORM);

    init();

    view_camera(fildes,&cam);
    open_segment(fildes,SEGMENT,TRUE,TRUE);
    drawit();
    close_segment(fildes);

#ifdef RAYTRACE
    make_picture_current(fildes);
    sleep(2);
    background_color(fildes,0.1,0.1,0.5);

    {
	int scene_id;
	    

	shadow_switch(fildes,0xffff);
	create_scene(fildes,SEGMENT,&scene_id);
	set_ray_output(fildes,scene_id,TRUE);
	/* turn item buffer off due to sbrr bus error bug */
	set_ray_item_buffer(fildes,scene_id,NO_ITEM_BUFFER); 
	render_scene(fildes,scene_id,COMPUTE_RAY_TRACE);
	make_picture_current(fildes);
    }

#else
    dbuffer_switch(fildes,i++);
    refresh_segment(fildes,SEGMENT);
    init_view(fildes,&cam);

    while (1) {
	dbuffer_switch(fildes,i++);
	change_view(locdev,fildes,NULL);
	refresh_segment(fildes,SEGMENT);
	flush_buffer(fildes);
    }
#endif
}

