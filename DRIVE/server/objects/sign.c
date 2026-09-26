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


/* Code Module for the sign segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

#define TEXT_CULL_SIZE	(20.0)

#define DEFAULT_HEIGHT	(10.0)
#define DEFAULT_LENGTH	(5.0)
#define DEFAULT_WIDTH	(10.0)
#define DEFAULT_RADIUS	(1.5)
#define DEFAULT_RED	(0.0)
#define DEFAULT_GREEN	(1.0)
#define DEFAULT_BLUE	(0.2)

#define SIGN_TEXT_FLOAT	(0.2)
#define POLE_OVERLAP	(0.5)

#define STOP_SIGN 		1
#define SPEED_LIMIT_SIGN 	2
#define LEFT_T_SIGN 		3
#define RIGHT_T_SIGN 		4
#define TOP_T_SIGN 		5
#define LEFT_CURVE_SIGN		6
#define RIGHT_CURVE_SIGN	7
#define POWERSHIFT_SIGN		8
#define GENERIC_SIGN		9
#define TWOPOLE_SIGN		10

#define  deg            *M_PI/180

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Radius			(obj->radius)
#define Red			(obj->color[0])
#define Green			(obj->color[1])
#define Blue			(obj->color[2])

typedef struct _sign_list {
    float signheight,signwidth,poleheight;
    float signradius,poleradius;
    float color[3];
    char label[LABEL_STRLEN];
    int type;
    unsigned int nameset_bits;
    int dl_number;
    struct _sign_list *next;
} SIGN_LIST;
static SIGN_LIST *sign_list=NULL;


static int sign_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    SIGN_LIST *sign = (SIGN_LIST *) (obj->additional_data);

    if (y < sign->poleheight) {
	/* in the realm of the poles only */
	if (sign->type == TWOPOLE_SIGN) {
	    if (x > 0) x -= sign->signwidth/2.0;
	    else x += sign->signwidth/2.0;
	}
	if (x*x + z*z < sign->poleradius*sign->poleradius) {
	    sc->mc_y = sign->poleheight + sign->signheight;
	    get_pole_mc_normal(obj,x,y,z,sc->mc_normal);
	    return(TRUE);
	}
    }
    else if (y < sign->poleheight + sign->signheight) {
	/* In the realm of the sign itself */
	sc->mc_y = sign->poleheight + sign->signheight;
	return(TRUE);
    }

    /* else */
    return(FALSE);
}


static int sign_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    return(checkbounds_bbox(obj,bbox_mc,sc,sign_surface_chars_xyz));
}


static hwObject do_grass(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    float grass_clist[3 * 3 * 8], *grass_ptr;
    float x, y, z;
    int i, blade;
    float pw = sign->poleradius/2.0;
    hwObject curr;
    
    grass_ptr = grass_clist;
    for ( i = 0; i < 3; i++ ) {
	x = BOUNDED_FLOATRAND(-pw,pw);
	y = 0.0;
	z = BOUNDED_FLOATRAND(-pw,pw);
	
	for ( blade=0; blade<3; blade++ ) {
	    *grass_ptr++ = x;
	    *grass_ptr++ = y;
	    *grass_ptr++ = z;
	    *grass_ptr++ = 0.0; /* Move. */
	    
	    *grass_ptr++ = x + floatrand() - 0.5;
	    *grass_ptr++ = y + floatrand() * 2.0;
	    *grass_ptr++ = z + floatrand() - 0.5;
	    *grass_ptr++ = 1.0; /* Draw. */
	}
    }
    curr = hwPolyline->create( hwPolyline );
    curr->name = 0;
    HW_MODIFY_3F( curr, hwStrColor, 0.2, 0.6, 0.1 );
    HW_MODIFY_1B( curr, hwStrHasFlags, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,3*3*2*4),
				grass_clist );
    return curr;
}


static hwObject stop_sign(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    int i;
    float pgon[8][3], *ptr;
    float theta;
    hwObject curr, oList[10];
    int nObjs = 0;

    ptr = (float *) pgon;
    theta = 22.5;

    for(i=0;i<8;i++) {
	*ptr++ = sign->signradius * FCOS(theta deg);  /* X component */
	*ptr++ = sign->signradius + sign->signradius * FSIN(theta deg)
		+ sign->poleheight;
	*ptr++ = 0.0;  /* Z */
	theta += 45.0;
    }
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 0.0, 0.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,8*3), pgon );
    oList[nObjs++] = curr;

    /* draw the back side of the sign */
    ptr = (float *) pgon;
    for(i=0;i<8;i++) {
	theta -= 45.0;
	*ptr++ = Radius * FCOS(theta deg);  /* X component */
	*ptr++ = Radius + Radius * FSIN(theta deg) + sign->poleheight;
	*ptr++ = 0.0;  /* Z */
    }
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 0.6, 0.6, 0.6 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,8*3), pgon );
    oList[nObjs++] = curr;

    /* draw the post */
    sign->poleradius = Radius/8.0;
    curr = post(0.0,0.0,sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = do_grass(obj,sign);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject speed_limit_sign(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    float pgon[4][3], *ptr;
    float wid,height;
    hwObject oList[10], curr;
    int nObjs = 0;

    wid    = sign->signwidth / 2.0;
    height = sign->signwidth * 1.4;

    ptr = (float *) pgon;
    *ptr++ =  wid; *ptr++ = sign->poleheight;  		*ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight + height; 	*ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight + height;  	*ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight;  		*ptr++ = 0.0;
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    /* Draw the back side of the sign */
    ptr = (float *) pgon;
    *ptr++ = -wid; *ptr++ = sign->poleheight;  		*ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight + height; 	*ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight + height;  	*ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight;  		*ptr++ = 0.0;
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    /* draw the post */
    sign->poleradius = sign->signwidth/8.0;
    curr = post(0.0,0.0,sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = do_grass(obj,sign);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs),
			oList );
    return curr;
}


static hwObject yellow_sign(
    DRIVE_OBJECT *obj,
    int rect,  /* Should a small rectangular sign be drawn as well? */
    SIGN_LIST *sign)
{
    float pgon[4][3], *ptr;
    float wid;
    float rect_height, rect_width,height;
    hwObject curr, oList[16];
    int nObjs = 0;

    ptr = (float *) pgon;

    wid         = sign->signwidth / 2.0;
    rect_height = sign->signwidth / 3.0;
    rect_width  = sign->signwidth / 3.0;

    if (rect) height = sign->poleheight - rect_height;
    else height = sign->poleheight;

    *ptr++ =  wid;  *ptr++ = sign->poleheight + wid;  *ptr++ = 0.0;
    *ptr++ =  0.0;  *ptr++ = sign->poleheight + sign->signwidth;  *ptr++ = 0.0;
    *ptr++ = -wid;  *ptr++ = sign->poleheight + wid;  *ptr++ = 0.0;
    *ptr++ =  0.0;  *ptr++ = sign->poleheight;  *ptr++ = 0.0;
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 0.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    if (rect) {
	ptr = (float *) pgon;
	*ptr++ =  rect_width / 2.0;  *ptr++ = height;  *ptr++ = 0.0;
	*ptr++ =  rect_width / 2.0;  *ptr++ = height + rect_height;
	    *ptr++ = 0.0;
	*ptr++ = -rect_width / 2.0;  *ptr++ = height + rect_height;
	    *ptr++ = 0.0;
	*ptr++ = -rect_width / 2.0;  *ptr++ = height;  *ptr++ = 0.0;
	curr = hwPolygon->create( hwPolygon );
	HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 0.0 );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
	curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
	oList[nObjs++] = curr;
    }

    /* the back side */
    ptr = (float *) pgon;
    *ptr++ =  0.0;  *ptr++ = height;  *ptr++ = 0.0;
    *ptr++ = -wid;  *ptr++ = height + wid;  *ptr++ = 0.0;
    *ptr++ =  0.0;  *ptr++ = height + sign->signwidth;  *ptr++ = 0.0;
    *ptr++ =  wid;  *ptr++ = height + wid;  *ptr++ = 0.0;
    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 0.6, 0.6, 0.6 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    /* draw the post */
    sign->poleradius = sign->signwidth/32.0;
    curr = post(0.0,0.0,sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = do_grass(obj,sign);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject sign_board(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    float mesh[10][3];
    float *ptr;
    float len;
    float wid;
    hwObject curr, oList[16];
    int nObjs = 0;

    len = sign->signheight / 2.0;
    wid = sign->signwidth / 2.0;
    ptr = (float *) mesh;

    *ptr++ = -wid;*ptr++ = 0.0; *ptr++ = -len;
    *ptr++ =  wid;*ptr++ = 0.0; *ptr++ = -len;     
    *ptr++ =  wid;*ptr++ = 0.0; *ptr++ =  len;   
    *ptr++ = -wid;*ptr++ = 0.0; *ptr++ =  len;   
    *ptr++ = -wid;*ptr++ = 0.0; *ptr++ = -len;   

    *ptr++ = -wid;*ptr++ = sign->poleheight; *ptr++ = -len;
    *ptr++ =  wid;*ptr++ = sign->poleheight; *ptr++ = -len;     
    *ptr++ =  wid;*ptr++ = sign->poleheight; *ptr++ =  len;   
    *ptr++ = -wid;*ptr++ = sign->poleheight; *ptr++ =  len;   
    *ptr++ = -wid;*ptr++ = sign->poleheight; *ptr++ = -len;   

    curr = hwMesh->create( hwMesh );
    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 1.0 );
    HW_MODIFY_1I( curr, hwStrGraphN, 2 );
    HW_MODIFY_1I( curr, hwStrGraphM, 5 );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), mesh );
    oList[nObjs++] = curr;

    ptr = (float *) mesh;  /* do top and bottom polygons */
    *ptr++ =  wid;*ptr++ = sign->poleheight; *ptr++ = -len;
    *ptr++ =  wid;*ptr++ = sign->poleheight; *ptr++ =  len;
    *ptr++ = -wid;*ptr++ = sign->poleheight; *ptr++ =  len;
    *ptr++ = -wid;*ptr++ = sign->poleheight; *ptr++ = -len;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 1.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), mesh );
    oList[nObjs++] = curr;

    ptr = (float *) mesh;  /* do top and bottom polygons */
    *ptr++ = -wid;*ptr++ = 0.0; *ptr++ = -len;
    *ptr++ = -wid;*ptr++ = 0.0; *ptr++ =  len;
    *ptr++ =  wid;*ptr++ = 0.0; *ptr++ =  len;
    *ptr++ =  wid;*ptr++ = 0.0; *ptr++ = -len;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 1.0 );
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), mesh );
    oList[nObjs++] = curr;

    oList[nObjs++] = do_grass(obj,sign);

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject user_defined_sign(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    float pgon[4][3];
    float *ptr;
    float wid;
    hwObject curr, oList[16];
    int nObjs = 0;

    wid = sign->signwidth / 2.0;

    ptr = (float *) pgon;
    *ptr++ =  wid; *ptr++ = sign->poleheight; 			 *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight + sign->signheight; *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight + sign->signheight; *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight; 			 *ptr++ = 0.0;

    curr = hwPolygon->create( hwPolygon );
    HW_MODIFY_3F( curr, hwStrColor, obj->color[0], obj->color[1], obj->color[2])
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    ptr = (float *) pgon;
    *ptr++ = -wid; *ptr++ = sign->poleheight; 			 *ptr++ = 0.0;
    *ptr++ = -wid; *ptr++ = sign->poleheight + sign->signheight; *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight + sign->signheight; *ptr++ = 0.0;
    *ptr++ =  wid; *ptr++ = sign->poleheight; 			 *ptr++ = 0.0;

    curr = hwPolygon->create( hwPolygon );
    STEEL_HW(curr);
    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
    HW_MODIFY_1B( curr, hwStrFlipNormals, HW_TRUE );
    curr->modify( curr, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*3), pgon );
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject draw_generic_sign(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    hwObject curr, oList[16];
    int nObjs = 0;

    oList[nObjs++] = user_defined_sign(obj,sign);

    sign->poleradius = sign->signwidth/16.0;
    curr = post(0.0,0.0,sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


static hwObject draw_twopole_sign(
    DRIVE_OBJECT *obj,
    SIGN_LIST *sign)
{
    hwObject curr, oList[16];
    int nObjs = 0;

    oList[nObjs++] = user_defined_sign(obj,sign);

    sign->poleradius = sign->signwidth/32.0;
    curr = post(-sign->signwidth/2.0+sign->poleradius,0.0,
        sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = post( sign->signwidth/2.0-sign->poleradius,0.0,
	sign->poleradius/2.0+SIGN_TEXT_FLOAT,
	sign->poleheight+POLE_OVERLAP,sign->poleradius,FALSE,4);
    STEEL_HW(curr);
    oList[nObjs++] = curr;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    return curr;
}


#define X(val) *ptr++ = x + (val);
#define Y(val) *ptr++ = y + (val);
#define Z      *ptr++ = z; n++;

#define c0   (0.0)
#define c1   (0.0 * width)
#define c1_5 (0.1 * width)
#define c2   (0.2 * width)
#define c2_5 (0.3 * width)
#define c3   (0.4 * width)
#define c3_5 (0.5 * width)
#define c4   (0.6 * width)
#define c4_5 (0.7 * width)
#define c5   (0.8 * width)

#define r0   (0.0)
#define r0_5 (0.1)
#define r1   (0.2 * height)
#define r1_5 (0.3 * height)
#define r2   (0.4 * height)
#define r2_5 (0.5 * height)
#define r3   (0.6 * height)
#define r3_5 (0.7 * height)
#define r4   (0.8 * height)
#define r4_5 (0.9 * height)
#define r5   (1.0 * height)

#define POLYGON \
	if( n % 4 ) fprintf( stderr, "QUAD BUG line %d!\n", __LINE__ );

static int draw_letter(
    float *pgon,
    char letter,
    float *advance,
    float x, float y, float z,
    float height, float width,
    int do_draw)
{
    float *ptr;
    int n;
    float ret;

    ret = 0;
    n = 0;
    ptr = pgon;
    switch(letter)
    {
      case 'A':
      case 'a':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r4) Z 
	X(c1) Y(r0) Z 
	POLYGON
	X(c4) Y(r0) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r4) Z
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r4) Z
	POLYGON
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	POLYGON
	ret = c4 ;
	break;
      case 'B':
      case 'b':
	X(c3) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c1) Y(r4) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r4) Z
	X(c1) Y(r4) Z
	X(c1) Y(r1) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c4) Y(r1) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2_5) Z
	POLYGON
	X(c3) Y(r2_5) Z
	X(c4) Y(r3) Z
	X(c4) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	POLYGON
	ret = c4 ;
	break;
      case 'C':
      case 'c':
	X(c4) Y(r0) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c1) Y(r0) Z
	X(c4) Y(r0) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r4) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	X(c3) Y(r4) Z
	X(c3) Y(r3) Z
	X(c4) Y(r3) Z
	X(c4) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'D':
      case 'd':
	X(c3) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c1) Y(r4) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	X(c2) Y(r4) Z
	X(c1) Y(r4) Z
	X(c1) Y(r1) Z
	X(c2) Y(r1) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c3) Y(r4) Z
	X(c3) Y(r1) Z
	X(c4) Y(r1) Z
	POLYGON
	ret = c4 ;
	break;
      case 'E':
      case 'e':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r4) Z
	X(c1) Y(r4) Z
	X(c1) Y(r1) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	POLYGON

	ret = c4 ;
	break;
      case 'F':
      case 'f':
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c2) Y(r0) Z
	X(c2) Y(r4) Z
	X(c1) Y(r4) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	POLYGON
	ret = c4 ;
	break;
      case 'G':
      case 'g':
	X(c4) Y(r0) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c1) Y(r0) Z
	X(c4) Y(r0) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r4) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c2) Y(r4) Z
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'H':
      case 'h':
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	POLYGON
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	POLYGON
	ret = c4 ;
	break;
      case 'I':
      case 'i':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c3) Y(r4) Z
	X(c2) Y(r4) Z
	X(c2) Y(r1) Z
	POLYGON
	ret = c4 ;
	break;
      case 'J':
      case 'j':
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c2) Y(r4) Z
	X(c2) Y(r1) Z
	X(c3) Y(r0) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	X(c3) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case 'K':
      case 'k':
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c2) Y(r2) Z
	X((c2_5 + c3)*.5) Y(r2_5) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2_5) Z
	POLYGON
	X((c2_5 + c3)*.5) Y(r2_5) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c2) Y(r3) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c4) Y(r0) Z
	X((c2_5 + c3)*.5) Y(r2_5) Z
	X(c2) Y(r2) Z
	POLYGON
	ret = c4 ;
	break;
      case 'L':
      case 'l':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r1) Z
	POLYGON
	ret = c4 ;
	break;
      case 'M':
      case 'm':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X((c2+c3)/2.0) Y(r1) Z
	X(c3) Y(r2) Z
	X(c3) Y(r5) Z
	X((c2+c3)/2.0) Y(r3) Z
	POLYGON
	X((c2+c3)/2.0) Y(r3) Z
	X(c2) Y(r5) Z
	X(c2) Y(r2) Z
	X((c2+c3)/2.0) Y(r1) Z
	POLYGON
	ret = c4 ;
	break;
      case 'N':
      case 'n':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c3) Y(r2) Z
	X(c2) Y(r5) Z
	X(c2) Y(r2) Z
	POLYGON
	ret = c4 ;
	break;
      case 'O':
      case 'o':
      case '0': /* Zero */
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r4) Z
	X(c1) Y(r1) Z
	POLYGON
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c2) Y(r0) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	X(c4) Y(r1) Z
	X(c4) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case 'P':
      case 'p':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c4) Y(r3) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'Q':
      case 'q':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r4) Z
	X(c1) Y(r1) Z
	POLYGON
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c2) Y(r0) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c4) Y(r1) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	POLYGON
	X(c4) Y(r0) Z
	X(c5) Y(r0) Z
	X(c4) Y(r1) Z
	X(c3_5) Y(r0_5) Z
	POLYGON
	ret = c5 ;
	break;
      case 'R':
      case 'r':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c4) Y(r3) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	POLYGON
	X(c4) Y(r0) Z
	X(c4) Y(r2) Z
	X(c3) Y(r3) Z
	X(c3) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case 'S':
      case 's':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c4) Y(r2) Z
	X(c4) Y(r3) Z
	X(c1) Y(r3) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c4) Y(r1) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c2) Y(r3) Z
	X(c2) Y(r4) Z
	X(c1) Y(r4) Z
	X(c1) Y(r3) Z
	POLYGON
	ret = c4 ;
	break;
      case 'T':
      case 't':
	X(c2) Y(r4) Z
	X(c2) Y(r0) Z
	X(c3) Y(r0) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case 'U':
      case 'u':
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c1) Y(r0) Z
	X(c4) Y(r0) Z
	POLYGON
	X(c2) Y(r1) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case 'V':
      case 'v':
	X(c3) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c2_5) Y(r2) Z
	POLYGON
	X(c2_5) Y(r2) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c2_5) Y(r2) Z
	X(c2) Y(r0) Z
	X(c2_5) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case 'W':
      case 'w':
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c2) Y(r0) Z
	X(c2_5) Y(r1) Z
	X(c2_5) Y(r3) Z
	X(c2) Y(r2) Z
	POLYGON
	X(c2_5) Y(r1) Z
	X(c3) Y(r0) Z
	X(c3) Y(r2) Z
	X(c2_5) Y(r3) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'X':
      case 'x':
	X(c2) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c4) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'Y':
      case 'y':
	X(c3) Y(r0) Z
	X(c3) Y(r2) Z
	X(c2) Y(r2) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c2) Y(r2) Z
	POLYGON
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case 'Z':
      case 'z':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c1) Y(r1) Z
	X(c2) Y(r1) Z
	X(c4) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case '1':
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c3) Y(r4) Z
	X(c2) Y(r4) Z
	X(c2) Y(r1) Z
	POLYGON
	X(c3) Y(r4) Z
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case '2':
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	POLYGON
	X(c2) Y(r3) Z
	X(c1) Y(r2) Z
	X(c1) Y(r0) Z
	X(c2) Y(r0) Z
	POLYGON
	X(c2) Y(r0) Z
	X(c4) Y(r0) Z
	X(c4) Y(r1) Z
	X(c2) Y(r1) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c4) Y(r3) Z
	X(c4) Y(r4) Z
	X(c3) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case '3':
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r4) Z
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c1) Y(r3) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c3) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case '4':
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c2) Y(r2) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	POLYGON
	ret = c4 ;
	break;
      case '5':
	X(c3) Y(r0) Z
	X(c4) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r1) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c4) Y(r2) Z
	X(c3) Y(r3) Z
	X(c1) Y(r3) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c2) Y(r4) Z
	POLYGON
	X(c1) Y(r5) Z
	X(c1) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case '6':
	X(c2) Y(r0) Z
	X(c2) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r0) Z
	POLYGON
	X(c4) Y(r0) Z
	X(c4) Y(r3) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c4) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r2) Z
	X(c3) Y(r2) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c2) Y(r0) Z
	X(c4) Y(r0) Z
	POLYGON
	ret = c4 ;
	break;
      case '7':
	X(c3) Y(r4) Z
	X(c1) Y(r0) Z
	X(c2) Y(r0) Z
	X(c4) Y(r4) Z
	POLYGON
	X(c4) Y(r4) Z
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c1) Y(r4) Z
	POLYGON
	ret = c4 ;
	break;
      case '8':
	X(c4) Y(r0) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2) Z
	X(c3) Y(r1) Z
	POLYGON
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c1) Y(r0) Z
	X(c4) Y(r0) Z
	POLYGON
	X(c2) Y(r2) Z
	X(c1) Y(r2) Z
	X(c1) Y(r0) Z
	X(c2) Y(r1) Z
	POLYGON
	X(c4) Y(r5) Z
	X(c1) Y(r5) Z
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	POLYGON
	X(c1) Y(r5) Z
	X(c1) Y(r3) Z
	X(c2) Y(r3) Z
	X(c2) Y(r4) Z
	POLYGON
	X(c3) Y(r4) Z
	X(c3) Y(r3) Z
	X(c4) Y(r3) Z
	X(c4) Y(r5) Z
	POLYGON
	X(c2) Y(r2_5) Z
	X(c1) Y(r2) Z
	X(c4) Y(r2) Z
	X(c3) Y(r2_5) Z
	POLYGON
	X(c3) Y(r2_5) Z
	X(c4) Y(r3) Z
	X(c1) Y(r3) Z
	X(c2) Y(r2_5) Z
	POLYGON
	ret = c4 ;
	break;
      case '9':
	X(c4) Y(r0) Z
	X(c4) Y(r5) Z
	X(c3) Y(r5) Z
	X(c3) Y(r0) Z
	POLYGON
	X(c3) Y(r2) Z
	X(c3) Y(r3) Z
	X(c2) Y(r3) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c2) Y(r3) Z
	X(c2) Y(r4) Z
	X(c1) Y(r5) Z
	X(c1) Y(r2) Z
	POLYGON
	X(c2) Y(r4) Z
	X(c3) Y(r4) Z
	X(c3) Y(r5) Z
	X(c1) Y(r5) Z
	POLYGON
	ret = c4 ;
	break;
      case '>':
	X(c5) Y(r2_5) Z
	X(c1_5) Y(r5) Z
	X(c1) Y(r2_5) Z
	X(c1_5) Y(r0) Z
	POLYGON
	ret = c5 ;
	break;
      case '<':
	X(c4_5) Y(r0) Z
	X(c5) Y(r2_5) Z
	X(c4_5) Y(r5) Z
	X(c0) Y(r2_5) Z
	POLYGON
	ret = c5 ;
	break;
      case '-':
	X(c5) Y((r1 + r2) /2.0 ) Z
	X(c5) Y((r4 + r3) /2.0 ) Z
	X(c0) Y((r4 + r3) /2.0 ) Z
	X(c0) Y((r1 + r2) /2.0 ) Z
	POLYGON
	ret = c5 ;
	break;
      case '!':
	X(c3) Y(r2) Z
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r2) Z
	POLYGON
	X(c3) Y(r0) Z
	X(c3) Y(r1) Z
	X(c2) Y(r1) Z
	X(c2) Y(r0) Z
	POLYGON
	ret = c3 ;
	break;
      case '\'':   /* Apostraphe */
	X(c3) Y(r3) Z
	X(c3) Y(r5) Z
	X(c2) Y(r5) Z
	X(c2) Y(r3) Z
	POLYGON
	ret = c3 ;
	break;
      case '.':   /* period */
	X(c2) Y(r0) Z
	X(c2) Y(r1) Z
	X(c1) Y(r1) Z
	X(c1) Y(r0) Z
	POLYGON
	ret = c2 ;
	break;
      case ' ':
      case ',':
	ret = width * 0.2;
	break;
      default:
	ret = 0.0;
	break;
    }

    *advance = ret;

    return n;
}


hwObject draw_string(
    char *string,
    float x, float y, float z,
    float width, float height,
    int align, int path,
    float exp_factor)
{
    float cur_x, cur_y, dx,dy;
    int len,use_width,i;
    float exp_f,tot_width;
    hwObject curr;
    static float pgon[4096*3];
    int nPts = 0;

    exp_f = width * exp_factor;
    len = strlen(string);
    use_width = 0;
    switch(align)
    {
      case TA_CENTER_SB:
	switch(path)
	{
	    case PATH_DOWN:
		cur_x = x;
		tot_width = len * (height + exp_f);
		cur_y = y + (tot_width / 2.0);
		dy = height + exp_f;
		break;
	    case PATH_RIGHT:
	    default:
		 /* Need to calculate the total width of the string. 
		  * Since this is a porportional font and we don't know how 
		  * wide each letter is, call the draw_letter routine to 
		  * figure it out for us -- but dont' draw it yet!
		  */
		tot_width = 0.0; 
		for(i=0;i<len;i++)
		{
		    (void)draw_letter(pgon,string[i],&dx,0.0,0.0,0.0,height, 
			 width,0);
		    tot_width += (dx + exp_f);
		}
		tot_width -= exp_f;  /* subtract off the empty space on right */
		cur_y = y;
		cur_x = x - (tot_width / 2.0);
		use_width = 1;
		dy = 0.0;
		break;
	}
	break;
      case TA_LEFT:
      default:
	cur_x = x;
	cur_y = y;
	switch(path)
	{
	  case PATH_RIGHT:
	      dy = 0;
	      use_width = 1;
	      break;
	  case PATH_DOWN:
	      dx = 0;
	      dy = exp_f + height;
	      break;
	}
	break;
    }

    for(i=0;i<len;i++)
    {
	nPts += draw_letter(pgon+3*nPts,
				string[i],&dx,cur_x, cur_y,z,height,width,1);
	if(use_width) 
	  cur_x += dx + exp_f;
	cur_y -= dy;
    }

    if( nPts ) {
	curr = hwQuads->create( hwQuads );
	HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nPts*3), pgon );
    }
    else {
	curr = 0;
    }
    return curr;
}


#define	TEXT_MIN_LOD	3.0
#define	TEXT_MAX_LOD	100.0

static void create_sign_graphics(
    DRIVE_OBJECT *obj,
    int type,
    SIGN_LIST *sign)
{
    hwObject curr = 0, oList[100];
    hwFloat bounds[200];
    int nObjs = 0, nBounds = 0;

    switch(type) {
	case STOP_SIGN: /* Stop Sign */
	    curr = draw_string("STOP",
		    0.0,sign->poleheight + 0.5 * Radius, -SIGN_TEXT_FLOAT,
		    Radius * 0.6, Radius, TA_CENTER_SB, PATH_RIGHT,0.15);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
		oList[nObjs++] = curr;
	    }

	    curr = stop_sign( obj, sign );
	    oList[nObjs++] = curr;

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    break;

	case SPEED_LIMIT_SIGN:   /* Speed Limit Sign */
	    {
	    float wid,height;

	    sign->signwidth = sign->signheight/1.4;
	    wid = sign->signwidth / 2.0;
	    height = sign->signheight;

	    {   char s[10];
		if (obj->label[0] == '\0') strcpy(s,"55");
		else strcpy(s,obj->label);

		curr = draw_string(s,
		    0.0, sign->poleheight + 0.125 * height, -SIGN_TEXT_FLOAT,
		    sign->signwidth / 3.0, sign->signwidth / 3.0,
		    TA_CENTER_SB,PATH_RIGHT,0.15);
		if( curr ) {
		    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		    bounds[nBounds++] = TEXT_MIN_LOD;
		    bounds[nBounds++] = TEXT_MAX_LOD;
		    oList[nObjs++] = curr;
		}

		curr = draw_string("SPEED",
		    0.0, sign->poleheight + 0.75 * height, -SIGN_TEXT_FLOAT,
		    sign->signwidth / 5.0, sign->signwidth / 5.0,
		    TA_CENTER_SB,PATH_RIGHT,0.15);
		if( curr ) {
		    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		    bounds[nBounds++] = TEXT_MIN_LOD;
		    bounds[nBounds++] = TEXT_MAX_LOD;
		    oList[nObjs++] = curr;
		}

		curr = draw_string("LIMIT",
		    0.0, sign->poleheight + 0.50 * height, -SIGN_TEXT_FLOAT,
		    sign->signwidth / 5.0, sign->signwidth / 5.0,
		    TA_CENTER_SB,PATH_RIGHT,0.15);
		if( curr ) {
		    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		    bounds[nBounds++] = TEXT_MIN_LOD;
		    bounds[nBounds++] = TEXT_MAX_LOD;
		    oList[nObjs++] = curr;
		}
	    }

	    curr = speed_limit_sign(obj,sign);
	    oList[nObjs++] = curr;

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    break;
	    }

	case LEFT_T_SIGN:   /* Left "T" Sign */
	    {
	    float wid;
	    float pgon[20 * 3], *ptr;

	    wid = sign->signwidth / 2.0;

	    ptr = pgon;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.45;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.55;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.55;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.45;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    curr = hwQuads->create( hwQuads );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,24), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    oList[nObjs++] = yellow_sign(obj,FALSE,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case RIGHT_T_SIGN:   /* Right "T" Sign */
	    {
	    float wid;
	    float pgon[20 * 3], *ptr;

	    wid = sign->signwidth / 2.0;

	    ptr = pgon;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.45;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.45;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.55;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.55;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    curr = hwQuads->create( hwQuads );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,24), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    oList[nObjs++] = yellow_sign(obj,FALSE,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case TOP_T_SIGN:   /* TOP "T" Sign */
	    {
	    float wid;
	    float pgon[20 * 3], *ptr;

	    wid = sign->signwidth / 2.0;

	    ptr = pgon;
	    *ptr++ = -sign->signwidth * 0.05;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.05;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.05;
		*ptr++ = sign->poleheight + sign->signwidth * 0.20;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.05;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ =  sign->signwidth * 0.3;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.3;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.3;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.3;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    curr = hwQuads->create( hwQuads );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,24), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    oList[nObjs++] = yellow_sign(obj,FALSE,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case LEFT_CURVE_SIGN:
	    {
	    float wid;
	    float pgon[20 * 3], *ptr;
	    float rect_height, rect_width;
	    char *speed;

	    rect_height = sign->signwidth / 3.0;
	    rect_width  = sign->signwidth / 3.0;

	    wid = sign->signwidth / 2.0;

	    ptr = pgon;

	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.65;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    curr = hwPolygon->create( hwPolygon );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,9), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    ptr = pgon;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.30;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.30;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ =  sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =   sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    curr = hwQuads->create( hwQuads );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,36), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    if (obj->label[0] != '\0') {
		speed = obj->label;
	    }
	    else {
		speed = "50";
	    }

	    curr = draw_string(speed,
		 0.0,sign->poleheight-(rect_height*0.40),-SIGN_TEXT_FLOAT,
		 (rect_width / 3),(rect_width / 3.0),
		 TA_CENTER_SB, PATH_RIGHT,0.20);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
		oList[nObjs++] = curr;
	    }

	    curr = draw_string("MPH",
		 0.0,sign->poleheight-(rect_height * 0.90),-SIGN_TEXT_FLOAT,
		 (rect_width / 3),(rect_width / 3.0),
		 TA_CENTER_SB, PATH_RIGHT,0.20);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
		oList[nObjs++] = curr;
	    }

	    oList[nObjs++] = yellow_sign(obj,TRUE,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case RIGHT_CURVE_SIGN:
	    {
	    float wid;
	    float pgon[20 * 3], *ptr;
	    char *speed;
	    float rect_height, rect_width;

	    rect_height = sign->signwidth / 3.0;
	    rect_width  = sign->signwidth / 3.0;
	    wid = sign->signwidth / 2.0;

	    ptr = pgon;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.65;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.80;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    curr = hwPolygon->create( hwPolygon );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,9), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    ptr = pgon;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.70;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ =  sign->signwidth * 0.0;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;

	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.60;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.2;
		*ptr++ = sign->poleheight + sign->signwidth * 0.30;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.30;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    *ptr++ = -sign->signwidth * 0.1;
		*ptr++ = sign->poleheight + sign->signwidth * 0.50;
		*ptr++ = -SIGN_TEXT_FLOAT;
	    curr = hwQuads->create( hwQuads );
	    HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
	    HW_MODIFY_1B( curr, hwStrBackface, HW_TRUE );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,36), pgon );
	    bounds[nBounds++] = TEXT_MIN_LOD;
	    bounds[nBounds++] = TEXT_MAX_LOD;
	    oList[nObjs++] = curr;

	    if (obj->label[0] != '\0') {
		speed = obj->label;
	    }
	    else {
		speed = "50";
	    }

	    curr = draw_string(speed,
		 0.0,sign->poleheight-(rect_height * 0.40),-SIGN_TEXT_FLOAT,
		 (rect_width / 3),(rect_width / 3.0),
		 TA_CENTER_SB, PATH_RIGHT,0.20);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		oList[nObjs++] = curr;
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
	    }
	    curr = draw_string("MPH",
		 0.0,sign->poleheight-(rect_height * 0.90),-SIGN_TEXT_FLOAT,
		 (rect_width / 3),(rect_width / 3.0),
		 TA_CENTER_SB, PATH_RIGHT,0.20);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		oList[nObjs++] = curr;
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
	    }

	    oList[nObjs++] = yellow_sign(obj,TRUE,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case POWERSHIFT_SIGN:  /* HP, Powershift, double-sided sign */
	    {
	    float wid,len;

	    wid = sign->signwidth  / 2.0;
	    len = sign->signheight / 2.0;

	    curr = draw_string("HP",
		    -0.6 * Width/2.0,Height/10.0,-len - SIGN_TEXT_FLOAT,
		    Width*0.4,Height*0.8,
		    TA_LEFT, PATH_RIGHT,0.3);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
		oList[nObjs++] = curr;
	    }

	    curr = draw_string("POWER SHIFT",
		 0.0,Height/5.0,-len - SIGN_TEXT_FLOAT,
		 Width*0.1,Height*0.6,
		 TA_CENTER_SB,PATH_RIGHT,0.1);
	    if( curr ) {
		HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
		HW_MODIFY_3F( curr, hwStrRotate, 0.0, 180.0, 0.0 );
		bounds[nBounds++] = TEXT_MIN_LOD;
		bounds[nBounds++] = TEXT_MAX_LOD;
		oList[nObjs++] = curr;
	    }

	    oList[nObjs++] = sign_board(obj,sign);

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	case GENERIC_SIGN:
	case TWOPOLE_SIGN:
	    {
	    char labels[10][80];
	    float wid;
	    int i,row,col;
	    float row_height, char_height, char_width,xpos,ypos;
	    int longest;
	    float intensity;

	    wid = sign->signwidth / 2.0;
	    /* len = sign->signheight / 2.0; */


	    /* first, we need to decide how many lines of text there are, and
	     * decide how many characters per line, so we can call the text
	     * generation routines properly
	     */
	    i=0;
	    row = col = 0;
	    while(obj->label[i] != 0 ) {
	       if(obj->label[i] == '\\' ) {
	       /* new line */
		  labels[row][col] = 0;
		  row++;
		  col = 0;
	       }
	       else {
		  labels[row][col++] = obj->label[i];
	       }
	       i++;
	    }
	    labels[row][col++] = 0; /* null terminate the last string */

	    /* Decide how far apart to place the rows of text.  add 2 rows to
	     * allow room at the top and at the botton
	     */

	    row_height = sign->signheight / (row + 2); 
	    char_height = row_height * 0.8;
	    longest = 0;
	    for(i=0;i<=row;i++) {
	      if( strlen(labels[i]) > longest) longest = strlen(labels[i]);
	    }
	    char_width = ((sign->signwidth) / (longest+1 )) / 0.7;

	    /* Make the text color either black or white, depending on
	     * the color specified for the background 
	     */
	    intensity = obj->color[0] * 0.3 + obj->color[1] * 0.59 + 
		obj->color[2] * 0.11;

	    for(i=0;i<=row;i++) {
		xpos = 0.0;
		ypos = sign->signheight - ((i+1.5) * row_height)
		     + sign->poleheight;
		curr = draw_string(labels[i],
		     xpos,ypos,-SIGN_TEXT_FLOAT,
		     char_width,char_height,
		     TA_CENTER_SB,PATH_RIGHT,0.1);
		if( curr ) {
		    if( intensity >= 0.5 ) {
			HW_MODIFY_3F( curr, hwStrColor, 0.0, 0.0, 0.0 );
		    }
		    else {
			HW_MODIFY_3F( curr, hwStrColor, 1.0, 1.0, 1.0 );
		    }
		    bounds[nBounds++] = TEXT_MIN_LOD;
		    bounds[nBounds++] = TEXT_MAX_LOD;
		    oList[nObjs++] = curr;
		}
	    }

	    if (type == GENERIC_SIGN) {
		curr = draw_generic_sign(obj,sign);
	    }
	    else {
		curr = draw_twopole_sign(obj,sign);
	    }
	    oList[nObjs++] = curr;

	    curr = hwGroup->create( hwGroup );
	    curr->modify( curr, hwStrChildren,
			HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
	    curr->modify( curr, hwStrLOD,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,nBounds), bounds );
	    HW_OBJECT_NAMESET( curr, obj );
	    }
	    break;

	default:
	    break;
    }

#if 1 /* def SIGNS_WORKING */
    if( curr ) {
	obj->display_list = createHwSegmentFromObj( &curr, 1 );
    }
    else {
	obj->display_list = INVALID;
    }
#else
    obj->display_list = INVALID;
#endif
}


#define MAT obj->xform
#define  deg   *M_PI/180

static RETURN_CONDITION _sign_update(
    DRIVE_OBJECT *obj,
    float t_interval)
{
    float theta;
    float cos0,sin0;
    float tx,ty,tz;
    static float rot[4][4] = IDENTITY4x4;

    /* This routine simply rotates the sign object by the calculated amount 
     * about the Y axis. The ratio is 36 degrees per second.
     */

    theta = 36.0 * t_interval;
    cos0 = FCOS(theta deg);
    sin0 = FSIN(theta deg);
    tx = MAT[3][0];
    ty = MAT[3][1];
    tz = MAT[3][2];

    MAT[3][0] = 0.0;
    MAT[3][1] = 0.0;
    MAT[3][2] = 0.0;

    rot[0][0] = rot[2][2] = cos0;
    rot[0][2] = -sin0;
    rot[2][0] = sin0;

    concat_matrix(obj->xform,rot,obj->xform);
    MAT[3][0] = tx;
    MAT[3][1] = ty;
    MAT[3][2] = tz;

    return(RETURN_OK);
}


static void init_sign_object(
    DRIVE_OBJECT *obj,
    int type)
{
    SIGN_LIST *sign;

    if (Length == DEFAULT_OBJECT_SIZE)     Length = DEFAULT_LENGTH;
    if (Width  == DEFAULT_OBJECT_SIZE)     Width  = DEFAULT_WIDTH;
    if (Height == DEFAULT_OBJECT_SIZE)     Height = DEFAULT_HEIGHT;
    if (Radius == DEFAULT_OBJECT_RADIUS)   Radius = DEFAULT_RADIUS;
    if ((Red == DEFAULT_OBJECT_COLOR)
	    || (Green == DEFAULT_OBJECT_COLOR)
	    || (Blue == DEFAULT_OBJECT_COLOR)) {
	/* Greenish sign. */
	Red   = DEFAULT_RED;
	Green = DEFAULT_GREEN;
	Blue  = DEFAULT_BLUE;
    }

    obj->num_children = 0;

    /* See if we've created one like this before... */
    sign = sign_list;
    while (sign != NULL) {
	if (IS_NEAR(sign->signheight,Length)
		&& (sign->type == type) 
		&& (strcmp(sign->label,obj->label) == 0)
		&& IS_NEAR(sign->signwidth,Width)
		&& IS_NEAR(sign->color[0],Red)
		&& IS_NEAR(sign->color[1],Green)
		&& IS_NEAR(sign->color[2],Blue)
		&& IS_NEAR(sign->signradius,Radius)
		&& IS_NEAR(sign->poleheight,Height)
		&& (sign->nameset_bits == obj->nameset_bits)) {
	    break;
	}
	/* else */
	sign = sign->next;
    }
    if (sign != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = sign->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((sign = (SIGN_LIST *) malloc(sizeof(SIGN_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	sign->signheight = Length;
	sign->signwidth  = Width;
	sign->signradius = Radius;
	sign->poleheight = Height;
	sign->type   = type;
	sign->next   = sign_list;
	sign->color[0] = Red;
	sign->color[1] = Green;
	sign->color[2] = Blue;
	sign->nameset_bits = obj->nameset_bits;
	strcpy(sign->label,obj->label);
    	create_sign_graphics(obj,type,sign);
	sign->dl_number = obj->display_list;
	sign_list  = sign;

	if(obj->label[0] != 0 )
	{
	   if(debug) printf("Sign Label: %s\n",obj->label);
	}
	if(debug) printf("Red: %f  Green:  %f  Blue:  %f \n",obj->color[0],
	   obj->color[1],  obj->color[2]);
    }

    obj->additional_data = (void *) sign;
    obj->surface_chars_xyz  = sign_surface_chars_xyz;
    obj->surface_chars_bbox = sign_surface_chars_bbox;

    switch( type ) {
	case STOP_SIGN:   /* Stop Sign */
	    /* Just bound the Pole. */
	    obj->bound_mc[0] =  -sign->poleradius;
	    obj->bound_mc[1] = 0.0;
	    obj->bound_mc[2] = -sign->poleradius;
	    obj->bound_mc[3] = sign->poleradius;
	    obj->bound_mc[4] = sign->poleheight + 2.0*sign->signradius;
	    obj->bound_mc[5] = sign->poleradius;
	    break;
	case SPEED_LIMIT_SIGN:
	case LEFT_T_SIGN:
	case RIGHT_T_SIGN:
	case TOP_T_SIGN:  
	case LEFT_CURVE_SIGN:
	case RIGHT_CURVE_SIGN:
	    obj->bound_mc[0] = -sign->poleradius;
	    obj->bound_mc[1] =  0.0;
	    obj->bound_mc[2] =  -sign->poleradius;
	    obj->bound_mc[3] =  sign->poleradius;
	    obj->bound_mc[4] =  sign->poleheight + sign->signheight;
	    obj->bound_mc[5] =  sign->poleradius;
	    break;
	case GENERIC_SIGN:
	case TWOPOLE_SIGN:
	    obj->bound_mc[0] = -sign->signwidth/2.0;
	    obj->bound_mc[1] =  0.0;
	    obj->bound_mc[2] =  -2.0;
	    obj->bound_mc[3] =  sign->signwidth/2.0;
	    obj->bound_mc[4] =  sign->poleheight + sign->signheight;
	    obj->bound_mc[5] =  2.0;
	    break;
	case POWERSHIFT_SIGN:  /* Rotating HP/Powershift  sign */
	    /* Initial (mc) bounding box values */
	    obj->bound_mc[0] =  -1.0 ;
	    obj->bound_mc[1] = 0.0;
	    obj->bound_mc[2] = - 1.0;
	    obj->bound_mc[3] = sign->signwidth + 1.0;
	    obj->bound_mc[4] = sign->poleheight;
	    obj->bound_mc[5] = sign->signheight + 1.0;
	    obj->update_self     = _sign_update;
	    break;
	default:
	  break;
    }

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,FALSE);
}

void init_stop_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,STOP_SIGN);
}

void init_speed_limit_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,SPEED_LIMIT_SIGN);
}

void init_left_t_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,LEFT_T_SIGN);
}

void init_right_t_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,RIGHT_T_SIGN);
}

void init_top_t_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,TOP_T_SIGN);
}

void init_left_curve_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,LEFT_CURVE_SIGN);
}

void init_right_curve_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,RIGHT_CURVE_SIGN);
}

void init_powershift_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,POWERSHIFT_SIGN);
}

void init_generic_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,GENERIC_SIGN);
}

void init_twopole_sign_object(
    DRIVE_OBJECT *obj)
{
  init_sign_object(obj,TWOPOLE_SIGN);
}
