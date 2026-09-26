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
#include "libnum.h"
#include "physics.h"
#include "physics.ih"
#include "hw.h"

#define SIMPLE_MODEL TRUE

#ifdef __linux__
# define VOLATILE
#else
# define VOLATILE volatile
#endif

static VOLATILE int fildes;
static VOLATILE float mc_vertices[8][3];
static VOLATILE float mc_side[4][4][3];
static VOLATILE float elast;

#define BOX_SIZE    10.0
#define XMIN        (-BOX_SIZE)
#define XMAX        (BOX_SIZE)
#define YMIN        (-BOX_SIZE)
#define YMAX        (BOX_SIZE)
#define ZMIN        (-BOX_SIZE)
#define ZMAX        (BOX_SIZE)
#define AMBIENT 0.4,0.4,0.4

#define SHADOWFLOAT	0.1	/* How much to float shadow above floor */

static Display
    *xDisp;
static Window
    win;

static int
   doDump=1;

static hwDisplay
    disp;
static hwDrawable
    draw;
static hwObject
    room,
    box,
    shadow,
    cam,
    light,
    light2,
    sphere,
    env;

extern hwInt32
    hwDefaultOptLevel;

static void dump_state(
    PHYSICAL_OBJECT *obj)
{
    printf("\n");
    printf("x<%5.2f,%5.2f,%5.2f> ", obj->x[XD], obj->x[YD], obj->x[ZD]);
    printf("Q{%5.2f,<%5.2f,%5.2f,%5.2f>} ",obj->Q->s,
        obj->Q->r[XD], obj->Q->r[YD], obj->Q->r[ZD]);
    printf("p<%5.2f,%5.2f,%5.2f> ", obj->p[XD], obj->p[YD], obj->p[ZD]);
    printf("v<%5.2f,%5.2f,%5.2f> ", obj->v[XD], obj->v[YD], obj->v[ZD]);
    printf("w<%5.2f,%5.2f,%5.2f> ", obj->w[XD], obj->w[YD], obj->w[ZD]);
    printf("L<%5.2f,%5.2f,%5.2f> ", obj->L[XD], obj->L[YD], obj->L[ZD]);
    printf("\n");
}


static hwObject build_box_segment(
    float length, float width, float height, 
    int colorit)
{
    hwObject
	boxGroup,
	pgons[6];
    float
	*ptr,
	*data;
    int i;

    mc_vertices[0][0] = mc_vertices[1][0] = mc_vertices[6][0]
	= mc_vertices[7][0] = -length/2.0;
    mc_vertices[2][0] = mc_vertices[3][0] = mc_vertices[4][0]
	= mc_vertices[5][0] = length/2.0;
    mc_vertices[0][1] = mc_vertices[3][1] = mc_vertices[4][1]
	= mc_vertices[7][1] = -height/2.0;
    mc_vertices[1][1] = mc_vertices[2][1] = mc_vertices[5][1]
	= mc_vertices[6][1] = height/2.0;
    mc_vertices[0][2] = mc_vertices[1][2] = mc_vertices[2][2]
	= mc_vertices[3][2] = -width/2.0;
    mc_vertices[4][2] = mc_vertices[5][2] = mc_vertices[6][2]
	= mc_vertices[7][2] = width/2.0;

    mc_side[2][1][0] = mc_side[3][1][0] = mc_vertices[0][0];
    mc_side[2][1][1] = mc_side[3][1][1] = mc_vertices[0][1];
    mc_side[2][1][2] = mc_side[3][1][2] = mc_vertices[0][2];
    mc_side[0][0][0] = mc_side[3][0][0] = mc_vertices[1][0];
    mc_side[0][0][1] = mc_side[3][0][1] = mc_vertices[1][1];
    mc_side[0][0][2] = mc_side[3][0][2] = mc_vertices[1][2];
    mc_side[0][3][0] = mc_side[1][3][0] = mc_vertices[2][0];
    mc_side[0][3][1] = mc_side[1][3][1] = mc_vertices[2][1];
    mc_side[0][3][2] = mc_side[1][3][2] = mc_vertices[2][2];
    mc_side[1][2][0] = mc_side[2][2][0] = mc_vertices[3][0];
    mc_side[1][2][1] = mc_side[2][2][1] = mc_vertices[3][1];
    mc_side[1][2][2] = mc_side[2][2][2] = mc_vertices[3][2];
    mc_side[1][1][0] = mc_side[2][3][0] = mc_vertices[4][0];
    mc_side[1][1][1] = mc_side[2][3][1] = mc_vertices[4][1];
    mc_side[1][1][2] = mc_side[2][3][2] = mc_vertices[4][2];
    mc_side[0][2][0] = mc_side[1][0][0] = mc_vertices[5][0];
    mc_side[0][2][1] = mc_side[1][0][1] = mc_vertices[5][1];
    mc_side[0][2][2] = mc_side[1][0][2] = mc_vertices[5][2];
    mc_side[0][1][0] = mc_side[3][3][0] = mc_vertices[6][0];
    mc_side[0][1][1] = mc_side[3][3][1] = mc_vertices[6][1];
    mc_side[0][1][2] = mc_side[3][3][2] = mc_vertices[6][2];
    mc_side[2][0][0] = mc_side[3][2][0] = mc_vertices[7][0];
    mc_side[2][0][1] = mc_side[3][2][1] = mc_vertices[7][1];
    mc_side[2][0][2] = mc_side[3][2][2] = mc_vertices[7][2];

    {
	float
	    colors[6][3] = {
	       1, 0, 0,
	       0, 1, 0,
	       0, 0, 1,
	       1, 1, 0,
	       1, 0, 1,
	       0, 1, 1,
	    };

	for(i=0; i<6; i++ ) {
	    pgons[i] = hwPolygon->create(hwPolygon);
	    if( colorit ) {
		/* HW_MODIFY_1B(pgons[i], hwStrTwoSided, True); */
		HW_MODIFY_3F(pgons[i], hwStrColor, 
			    colors[i][0], colors[i][1], colors[i][2]);
	    }
	    else {
		HW_MODIFY_3F(pgons[i], hwStrColor,0.1, 0.1, 0.1 );
	    }
	}
    }
    pgons[0]->modify(pgons[0], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),mc_vertices);

    pgons[1]->modify(pgons[1], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),&mc_vertices[4][0] );

    pgons[2]->modify(pgons[2], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),&mc_side[0][0][0]);

    pgons[3]->modify(pgons[3], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),&mc_side[1][0][0]);

    pgons[4]->modify(pgons[4], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),&mc_side[2][0][0]);

    pgons[5]->modify(pgons[5], hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 12),&mc_side[3][0][0]);

    boxGroup = hwGroup->create(hwGroup);
    boxGroup->modify(boxGroup, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,6),
	pgons);

    return(boxGroup);
}


static hwObject  build_room_segment( void )
{
    static float pfloor[] = {
	0.0, 1.0, 0.0,
	XMIN,YMIN,ZMIN,
	XMIN,YMIN,ZMAX,
	XMAX,YMIN,ZMAX,
	XMAX,YMIN,ZMIN,
    };
    static float pleft[] = {
	1.0, 0.0, 0.0,
	XMIN,YMIN,ZMIN,
	XMIN,YMAX,ZMIN,
	XMIN,YMAX,ZMAX,
	XMIN,YMIN,ZMAX,
    };
    static float pceiling[] = {
	0.0, -1.0, 0.0,
	XMIN,YMAX,ZMIN,
	XMAX,YMAX,ZMIN,
	XMAX,YMAX,ZMAX,
	XMIN,YMAX,ZMAX,
    };
    static float pright[] = {
	-1.0, 0.0, 0.0,
	XMAX,YMAX,ZMIN,
	XMAX,YMIN,ZMIN,
	XMAX,YMIN,ZMAX,
	XMAX,YMAX,ZMAX,
    };
    static float pback[] = {
	0.0, 0.0, -1.0,
	XMIN,YMIN,ZMAX,
	XMIN,YMAX,ZMAX,
	XMAX,YMAX,ZMAX,
	XMAX,YMIN,ZMAX,
    };
    int
	i;
    hwFloat
	corners[6] = { -BOX_SIZE, -BOX_SIZE, -BOX_SIZE,
	    BOX_SIZE, BOX_SIZE, BOX_SIZE  };
    float
	*ptrs[5];

    hwObject
	roomGroup,
	fpgons[5];

    ptrs[0] = pfloor;
    ptrs[1] = pleft;
    ptrs[2] = pceiling;
    ptrs[3] = pright;
    ptrs[4] = pback;

    for(i=0; i< 5; i++ ) {
	fpgons[i] = hwPolygon->create(hwPolygon);
	HW_MODIFY_3F(fpgons[i], hwStrColor, 0.5, 0.5, 0.5);
	/*
	HW_MODIFY_1B(fpgons[i], hwStrBackface, True);
	*/
	fpgons[i]->modify(fpgons[i], hwStrData,
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, 12), &(ptrs[i][3]) );
    }

    roomGroup = hwGroup->create(hwGroup);
    roomGroup->modify(roomGroup, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,5),
	fpgons);

    return(roomGroup);
}


#define NO_COLLISION        0
#define LEFT_COLLISION      1
#define RIGHT_COLLISION     2
#define TOP_COLLISION       3
#define BOTTOM_COLLISION    4
#define FRONT_COLLISION     5
#define BACK_COLLISION      6

static int check_collisions(
    PHYSICAL_OBJECT *obj,
    float mtx[4][4])
{
    float wc_vertex[8][3];
    register int p,d;
    int coll_dim = NO_COLLISION;
    float c1[4],c2[4],icoll[4],jcoll[4];
    PHYSICAL_OBJECT *obj2;
    float delta,maxdelta;
    int coll_pt;

    maxdelta = -1.0;
    for (p=0; p<8; ++p) {
	for (d=0; d<3; ++d) {
	    wc_vertex[p][d] =
	        mc_vertices[p][0] * mtx[0][d] +
	        mc_vertices[p][1] * mtx[1][d] +
	        mc_vertices[p][2] * mtx[2][d] +
	        mtx[3][d];
	}

	if ((delta = XMIN - wc_vertex[p][0]) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = LEFT_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}
	else if ((delta = wc_vertex[p][0] - XMAX) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = RIGHT_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}

	if ((delta = YMIN - wc_vertex[p][1]) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = BOTTOM_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}
	else if ((delta = wc_vertex[p][1] - YMAX) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = TOP_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}

	if ((delta = ZMIN - wc_vertex[p][2]) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = FRONT_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}
	else if ((delta = wc_vertex[p][2] - ZMAX) >= 0.0) {
	    if (delta > maxdelta) {
	        coll_dim = BACK_COLLISION;
	        maxdelta = delta;
	        coll_pt = p;
	    }
	}
    }

    if (coll_dim != NO_COLLISION) {
	if(doDump)
	    printf("COLLISION point %d: %f,%f,%f\n",coll_pt,
		wc_vertex[coll_pt][0], wc_vertex[coll_pt][1],
		wc_vertex[coll_pt][2]);

	obj2 = allocate_physical_object();
	obj2->mass = obj->mass*1000000.0;
	obj2->I[XD][XD] = obj2->I[YD][YD] = obj2->I[ZD][ZD] = 100000.0;
	obj2->I[XD][YD] = obj2->I[XD][ZD] = obj2->I[YD][ZD] = 0.0;
	obj2->I[YD][XD] = obj2->I[ZD][XD] = obj2->I[ZD][YD] = 0.0;

	obj2->Iinv[XD][XD] = obj2->Iinv[YD][YD] = obj2->Iinv[ZD][ZD] = 
	    1/100000.0;
	obj2->Iinv[XD][YD] = obj2->Iinv[XD][ZD] = obj2->Iinv[YD][ZD] = 0.0;
	obj2->Iinv[YD][XD] = obj2->Iinv[ZD][XD] = obj2->Iinv[ZD][YD] = 0.0;

	obj2->Q->s = 1.0;
	obj2->Q->r[XD] = obj2->Q->r[YD] = obj2->Q->r[ZD] = 0.0;

	obj2->p[XD] = obj2->p[YD] = obj2->p[ZD] = 0.0;
	obj2->w[XD] = obj2->w[YD] = obj2->w[ZD] = 0.0;
	obj2->v[XD] = obj2->v[YD] = obj2->v[ZD] = 0.0;
	obj2->L[XD] = obj2->L[YD] = obj2->L[ZD] = 0.0;

	obj2->R[XD][XD] = obj2->R[YD][YD] = obj2->R[ZD][ZD] = 1.0;
	obj2->R[XD][YD] = obj2->R[XD][ZD] = obj2->R[YD][ZD] = 0.0;
	obj2->R[YD][XD] = obj2->R[ZD][XD] = obj2->R[ZD][YD] = 0.0;

	obj2->Rinv[XD][XD] = obj2->Rinv[YD][YD] = obj2->Rinv[ZD][ZD] = 1.0;
	obj2->Rinv[XD][YD] = obj2->Rinv[XD][ZD] = obj2->Rinv[YD][ZD] = 0.0;
	obj2->Rinv[YD][XD] = obj2->Rinv[ZD][XD] = obj2->Rinv[ZD][YD] = 0.0;

	c1[XD] = wc_vertex[coll_pt][0] - mtx[3][0];
	c1[YD] = wc_vertex[coll_pt][1] - mtx[3][1];
	c1[ZD] = wc_vertex[coll_pt][2] - mtx[3][2];

	switch (coll_dim) {
	    case LEFT_COLLISION:
		if (doDump) printf("LEFT\n");
	        c2[XD] = 1.0;    c2[YD] = 0.0;    c2[ZD] = 0.0;
	        icoll[XD] = 0.0; icoll[YD] = 1.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 0.0; jcoll[YD] = 0.0; jcoll[ZD] = 1.0;
	        obj->x[XD] += maxdelta;
	        break;

	    case RIGHT_COLLISION:
		if (doDump) printf("RIGHT\n");
	        c2[XD] = -1.0;   c2[YD] = 0.0;    c2[ZD] = 0.0;
	        icoll[XD] = 0.0; icoll[YD] = 1.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 0.0; jcoll[YD] = 0.0; jcoll[ZD] = 1.0;
	        obj->x[XD] -= maxdelta;
	        break;

	    case BOTTOM_COLLISION:
		if (doDump) printf("BOTTOM\n");
	        c2[XD] = 0.0;    c2[YD] = 1.0;    c2[ZD] = 0.0;
	        icoll[XD] = 1.0; icoll[YD] = 0.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 0.0; jcoll[YD] = 0.0; jcoll[ZD] = 1.0;
	        obj->x[YD] += maxdelta;
	        break;

	    case TOP_COLLISION:
		if (doDump) printf("TOP\n");
	        c2[XD]    = 0.0; c2[YD] = -1.0;   c2[ZD] = 0.0;
	        icoll[XD] = 1.0; icoll[YD] = 0.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 0.0; jcoll[YD] = 0.0; jcoll[ZD] = 1.0;
	        obj->x[YD] -= maxdelta;
	        break;

	    case FRONT_COLLISION:
		if (doDump) printf("FRONT\n");
	        c2[XD] = 0.0;    c2[YD] = 0.0;    c2[ZD] = -1.0;
	        icoll[XD] = 0.0; icoll[YD] = 1.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 1.0; jcoll[YD] = 0.0; jcoll[ZD] = 0.0;
	        obj->x[ZD] += maxdelta;
	        break;

	    case BACK_COLLISION:
		if (doDump) printf("BACK\n");
	        c2[XD] = 0.0;    c2[YD] = 0.0;    c2[ZD] = 1.0;
	        icoll[XD] = 0.0; icoll[YD] = 1.0; icoll[ZD] = 0.0;
	        jcoll[XD] = 1.0; jcoll[YD] = 0.0; jcoll[ZD] = 0.0;
	        obj->x[ZD] -= maxdelta;
	        break;
	}
	if (doDump)
	    printf("collision delta %f\n",maxdelta);
	solve_collision(obj,obj2,c1,c2,icoll,jcoll,elast);
	free_physical_object(obj2);
    }
}


static void draw_object(
    PHYSICAL_OBJECT *obj)
{
#ifdef MOVING_CAMERA
    static camera_arg cam = {
	0.0, YMIN/3.0, 0.0, /* ref */
	XMAX/3.0, YMAX/2.0, ZMIN*2.5,   /* cam */
	0.0, 1.0, 0.0,      /* up */
	60.0,               /* fov */
	ZMIN*10.0, ZMIN*10.0,   /* front,back */
	CAM_PERSPECTIVE     /* projection */
    };
#endif
    static float shadow_proj[4][4] = {
	1.0,    0.0,    0.0,    0.0,
	0.0,    0.001,  0.0,    0.0,
	0.0,    0.0,    1.0,    0.0,
	0.0,    (YMIN+SHADOWFLOAT), 0.0, 1.0,
    };
    float mtx[4][4];
    float poly[4][3];
    static int count=0;

#ifdef MOVING_CAMERA
    cam.camx = obj->x[XD];
    cam.camy = obj->x[YD];
    cam.camz = obj->x[ZD];
    cam.refx = obj->R[XD][XD] + obj->x[XD];
    cam.refy = obj->R[XD][YD] + obj->x[YD];
    cam.refz = obj->R[XD][ZD] + obj->x[ZD];
    cam.upx  = obj->R[YD][XD];
    cam.upy  = obj->R[YD][YD];
    cam.upz  = obj->R[YD][ZD];
    view_camera(fildes,&cam);
#endif

    mtx[0][0] = obj->R[XD][XD];
    mtx[0][1] = obj->R[XD][YD];
    mtx[0][2] = obj->R[XD][ZD];
    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0;
    mtx[1][0] = obj->R[YD][XD];
    mtx[1][1] = obj->R[YD][YD];
    mtx[1][2] = obj->R[YD][ZD];
    mtx[2][0] = obj->R[ZD][XD];
    mtx[2][1] = obj->R[ZD][YD];
    mtx[2][2] = obj->R[ZD][ZD];
    mtx[3][0] = obj->x[XD];
    mtx[3][1] = obj->x[YD];
    mtx[3][2] = obj->x[ZD];
    mtx[3][3] = 1.0;

#if 0  /* [ */
    dbuffer_switch(fildes,count++);
    refresh_segment_hsr(fildes,ROOM_SEGMENT_NUMBER);
    push_matrix3d(fildes,mtx);
#ifndef MOVING_CAMERA
    refresh_segment_hsr(fildes,BOX_SEGMENT_NUMBER);
#endif
    light_switch(fildes,0x01);
    concat_transformation3d(fildes,shadow_proj,POST,REPLACE);
    fill_color(fildes,AMBIENT);
    refresh_segment_hsr(fildes,SHADOW_SEGMENT_NUMBER);
    light_switch(fildes,0x07);
    pop_matrix(fildes);
    flush_buffer(fildes);
#endif /* ] */

    env->draw(env);
    cam->draw(cam);
    light->draw(light);
    light2->draw(light2);

    room->draw(room);
    disp->pushMatrix(disp, mtx); 
    box->draw(box);
    disp->popMatrix(disp); 

    disp->pushMatrix(disp, shadow_proj);
    disp->pushMatrix(disp, mtx); 
    shadow->draw(shadow);
    disp->popMatrix(disp); 
    disp->popMatrix(disp); 

    /* sphere->draw(sphere); */

    disp->update(disp, HW_UPDATE_ALL);


    check_collisions(obj,mtx); 
}

/* Dst[] = Src[] * Const; */
#define	VMULC(Dst,Src,Const) do { \
	(Dst)[0] = (Src)[0] * (Const); \
	(Dst)[1] = (Src)[1] * (Const); \
	(Dst)[2] = (Src)[2] * (Const); \
    } while( 0 )

/* Length-squared of Vec[] */
#define	VLEN2(Vec)	((Vec)[0]*(Vec)[0]+(Vec)[1]*(Vec)[1]+(Vec)[2]*(Vec)[2])

/* Length of Vec[] */
#define	VLEN(Vec)	sqrt(VLEN2(Vec))

/* Dst = normalized(Src) */
#define	VNORM(Dst,Src) do { \
	double __L__ = VLEN(Src); \
	if( __L__ != 0.0 ) __L__ = 1.0 / __L__; \
	VMULC(Dst,Src,__L__); \
    } while( 0 )


static void init(void)
{
    float
	dist,
	dir[3];

    cam = hwCamera->create(hwCamera);
    HW_MODIFY_3F(cam, hwStrPos,  20.0, 0.0, -20.0);
    HW_MODIFY_3F(cam, hwStrPos,  XMAX/3.0, YMAX/2.0, ZMIN*2.5);

    HW_MODIFY_3F(cam, hwStrDir, -1.0, 0.0 , 1.0);

    dir[0] = -XMAX/3.0;
    dir[1] = YMIN/3.0-YMAX/2.0;
    dir[2] = -ZMIN*2.5;

    VNORM(dir,dir);


    HW_MODIFY_3F(cam, hwStrDir, dir[0], dir[1], dir[2]);

    printf("Cam pos:  %f %f %f\n", XMAX/3.0, YMAX/2.0, ZMIN*2.5);
    printf("Cam dir:  %f %f %f\n", dir[0], dir[1], dir[2]);

    HW_MODIFY_3F(cam, hwStrUp, 0.0, 1.0 , 0.0);
    HW_MODIFY_2F(cam, hwStrPlanes,  1.0, ZMAX*4.0);
    HW_MODIFY_1B(cam, hwStrPerspective, HW_TRUE );
    HW_MODIFY_1F(cam, hwStrField, 60.0f );

    env = hwEnviron->create(hwEnviron);

    HW_MODIFY_1F(env, hwStrAmbientFactor, 0.4);

    light = hwLight->create(hwLight);
    HW_MODIFY_3F(light, hwStrDir, 0.0,  -1.0,  0.0);
    HW_MODIFY_3F(light, hwStrColor, 0.8, 0.8, 0.8);

    light2 = hwLight->create(hwLight);
    HW_MODIFY_3F(light2, hwStrDir,  -1.4,  -2.0, 1.0);
    HW_MODIFY_3F(light2, hwStrColor, 0.4, 0.4, 0.4);


    sphere = hwSphere->create(hwSphere);
    HW_MODIFY_1F(sphere, hwStrRadius, 5.0);
    HW_MODIFY_3F( sphere, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1F( sphere, hwStrShininess, 0.1f);

#if 0
    static camera_arg cam = {
	0.0, YMIN/3.0, 0.0, /* ref */
	XMAX/3.0, YMAX/2.0, ZMIN*2.5,   /* cam */
	0.0, 1.0, 0.0,      /* up */
	60.0,               /* fov */
	-ZMAX*2.0, ZMAX*2.0,    /* front,back */
	CAM_PERSPECTIVE     /* projection */
    };
    static float identity[4][4] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0,
    };

    push_matrix3d(fildes,identity);
    shade_mode(fildes,CMAP_FULL,TRUE);
    clear_control(fildes,CLEAR_VDC_EXTENT|CLEAR_ZBUFFER);
    double_buffer(fildes,TRUE|INIT,8);
    light_ambient(fildes,AMBIENT);
    light_source(fildes,1,DIRECTIONAL,0.8,0.8,0.8,0.0,1.0,0.0);
    light_source(fildes,2,DIRECTIONAL,0.4,0.4,0.4,1.4,2.0,-1.0);
    light_switch(fildes,0x7);
    surface_model(fildes,FALSE,1,0.0,0.0,0.0);
    hidden_surface(fildes,TRUE,TRUE);
    interior_style(fildes,INT_SOLID,FALSE);
    clear_view_surface(fildes);
    view_camera(fildes,&cam);

#endif

    init_physics_state();
}

void CheckXEvents(void)
{
    int
	i,
	numEvents;
    char
	buff[80];
    XWindowAttributes
	attr;
    KeySym
	KS;
    XEvent
	report;

    numEvents = XPending( xDisp );

    while( numEvents > 0 ) {
	XNextEvent( xDisp, &report );
	switch( report.type ) {
	    case Expose :
		/* Reset the viewport since we resized */
		XGetWindowAttributes( xDisp, win, &attr );
		disp->viewport( disp, 0, 0, attr.width, attr.height );
		break;
	    case KeyPress :
		i = XLookupString( (void *)&report, buff, 16, &KS, 0L );
		if( i != 1 )	break;

		switch( buff[0] ) {
		case 'q' : 
		    exit(0);
		    break;
		case 'd' : 
		    doDump = !doDump;
		    break;
		}
	}
       numEvents--;
   }
}


main(int argc, char **argv)
{
    float 
	length,
	width,
	height,
	temp, t, t2,
	t_interval;
    VECTOR 
	Ftot,
	Ttot;
    PHYSICAL_OBJECT 
	*obj;

    obj = allocate_physical_object();
    Ftot = ALLOCATE_VECTOR(3);
    Ttot = ALLOCATE_VECTOR(3);

    printf("\nmass:  ");    scanf("%f",&obj->mass);
    printf("length: ");     scanf("%f",&length);
    printf("height: ");     scanf("%f",&height);
    printf("width:  ");     scanf("%f",&width);

    xDisp = XOpenDisplay( (char *)0);
    if (! xDisp ) exit(1);

    hwDefaultOptLevel = HW_OPT_USE_DL;

    if (!hwInit(argc, argv)) exit(1);
    disp = hwDefaultDisplay->create(hwDefaultDisplay, NULL, xDisp);
    if (! disp ) exit(1);

    if( !disp->chooseVisual( disp, HW_VIS_DBUFF, 0 ) )	exit( 1 );

    draw = disp->createWindow( disp, "Test", 100, 100, 512, 512, 0 );
    if( !draw )	exit( 1 );

    win = disp->extractWindow( disp, draw );
    XSelectInput( xDisp, win, ExposureMask|KeyPressMask );

    disp->makeCurrent( disp, draw );

    room = build_room_segment();
    box = build_box_segment(length,width,height,1);
    shadow = build_box_segment(length,width,height,0);

    init();

    /* Standard inertial tensor matrix for a box */
    obj->I[XD][XD] = obj->mass * (height*height + width*width) / 12.0;
    obj->I[YD][YD] = obj->mass * (length*length + width*width) / 12.0;
    obj->I[ZD][ZD] = obj->mass * (length*length + height*height) / 12.0;
    obj->I[XD][YD] = obj->I[XD][ZD] = obj->I[YD][ZD] = 0.0;
    obj->I[YD][XD] = obj->I[ZD][XD] = obj->I[ZD][YD] = 0.0;

    obj->Iinv[XD][XD] = 1.0/obj->I[XD][XD];
    obj->Iinv[YD][YD] = 1.0/obj->I[YD][YD];
    obj->Iinv[ZD][ZD] = 1.0/obj->I[ZD][ZD];
    obj->Iinv[XD][YD] = obj->Iinv[XD][ZD] = obj->Iinv[YD][ZD] = 0.0;
    obj->Iinv[YD][XD] = obj->Iinv[ZD][XD] = obj->Iinv[ZD][YD] = 0.0;

    /* start at origin */
    obj->x[XD] = obj->x[YD] = obj->x[ZD] = 0.0;

    /* start with no rotation */
    obj->Q->s     = 1.0;
    obj->Q->r[XD] = 0.0;
    obj->Q->r[YD] = 0.0;
    obj->Q->r[ZD] = 0.0;

    printf("linear momentum p (x y z): ");
	scanf("%f %f %f",&obj->p[XD],&obj->p[YD],&obj->p[ZD]);
    printf("angular speed w (x y z): ");
	scanf("%f %f %f",&obj->w[XD],&obj->w[YD],&obj->w[ZD]);

    printf("Total forces applied to object (x y z): ");
	scanf("%f %f %f",&Ftot[XD],&Ftot[YD],&Ftot[ZD]);

    printf("Total torques applied to object (x y z): ");
	scanf("%f %f %f",&Ttot[XD],&Ttot[YD],&Ttot[ZD]);

    printf("Time interval: "); scanf("%f",&t_interval);
    printf("Elasticity: "); scanf("%f",&elast);

    t2 = 0.0;
    while (1) {
	t = t2;
	t2 += t_interval;
#ifdef SIMPLE_MODEL
	simple_compute_state(obj,Ftot,Ttot,t,t2);
#else
	compute_state(obj,Ftot,Ttot,t,t2);
#endif /* SIMPLE_MODEL else */

	if (doDump) dump_state(obj);
	draw_object(obj);
	CheckXEvents();
    }

}
