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


/* Code Module for the bridge segment object */

#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"

static hwObject high_res_model();
#if !defined(HOVERWARE_MODEL)
static void mid_res_model();
static void low_res_model();
#endif

#define DEFAULT_LENGTH		(120.0*2 + DEFAULT_ROAD_WIDTH + 4.0)
#define DEFAULT_WIDTH		DEFAULT_ROAD_WIDTH
#define DEFAULT_HEIGHT		10.0

#define RAIL_HEIGHT		3.0
#define RAIL_WIDTH		0.5
#define DECK_THICKNESS		0.5
#define BRIDGE_ROAD_LINE_FLOAT	(ROAD_LINE_FLOAT*2.0)
#define DIVISIONS_HIGH		8
#define DIVISIONS_MED		4

#define Length			(obj->size[SIZE_LENGTH])
#define Width			(obj->size[SIZE_WIDTH])
#define Height			(obj->size[SIZE_HEIGHT])
#define Gap			(Width + 4.0)
#define BankWidth		(Height/2.0)

#define YHEIGHT(z) 							\
    (((z) < (-Gap/2.0)) ? 						\
	((Height/2.0) * (1.0 + FCOS(((z)+(Gap/2.0))*M_PI/Length))	\
	    + ((Height/500.0) + BRIDGE_ROAD_LINE_FLOAT))		\
    : (((z) < (Gap/2.0)) ?						\
	    (Height + BRIDGE_ROAD_LINE_FLOAT)				\
	: ((Height/2.0) * (1.0 + FCOS(((z)-(Gap/2.0))*M_PI/Length))	\
	    + ((Height/500.0) + BRIDGE_ROAD_LINE_FLOAT))))

typedef struct _bridge_list {
    float length,width,height;
    unsigned int nameset_bits;
    int dl_number;
    struct _bridge_list *next;
} BRIDGE_LIST;
static BRIDGE_LIST *bridge_list=NULL;


static int scxyz_firstramp(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float z2,top;
    float nbank_x,nbank_y,nbank_z;
    float yp,nx,ny,nz,fullwidth;
    float absx = ABS(x);
    float absz = ABS(z);

    if (absx <= Width/2.0) {
	z2 = z + Gap/2.0 + Length;
	yp = YHEIGHT(z) - BRIDGE_ROAD_LINE_FLOAT; 
	ny = 1.0;
	nz = -(Height/2.0)*(M_PI/Length)*FSIN(z2*M_PI/Length);
	NORMALIZE2(ny,nz);
	sc->mc_y = yp;
	if (y > sc->mc_y - BBOX_MARGIN) {
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = ny;
	    sc->mc_normal[2] = nz;
	}
	else {
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = 1.0;
	}
	return(TRUE);
    }
    else {
	fullwidth = Width/2.0 + BankWidth - BankWidth/Length*(absz-Gap/2.0);
	if (absx < fullwidth) {
	    /* intersecting a bank */
	    /* normal to bank polygon is cross product of two edges */
	    nbank_x = Length*Height;
	    nbank_y = Length*BankWidth;
	    nbank_z = Height*BankWidth;
	    NORMALIZE3(nbank_x,nbank_y,nbank_z);
	    top = Height * ((absz-Gap/2.0)/(Length-Gap/2.0));
	    yp = (1.0 - (absx-Width/2.0)/(fullwidth-Width/2.0)) * top;
	    /* To keep people from climbing the walls, bump this to 5.0
	     * if we're far enough into the ramp.
	     */
	    if ((yp < 5.0) && (top > 5.0)) yp = 5.0;
	    sc->mc_y = yp;
	    if ((absz < Gap/2.0+3.0)
		    && (absx < Width/2.0+BankWidth - 3.0)) {
		/* hit end of abutment */
		sc->mc_normal[0] = 0.0;
		sc->mc_normal[1] = 0.0;
		sc->mc_normal[2] = 1.0;
	    }
	    else {
		if (x >= 0.0) nx = nbank_x;
		else nx = -nbank_x;
		ny = nbank_y;
		nz = -nbank_z;
		sc->mc_normal[0] = nx;
		sc->mc_normal[1] = ny;
		sc->mc_normal[2] = nz;
	    }
	    return(TRUE);
	}
    }

    /* else */
    return(FALSE);
}



static int scxyz_bridge(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    /* On the bridge */
    sc->mc_y = Height;
    return(TRUE);
}


static int scxyz_gr(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    /* ran into a guard rail */
    sc->mc_y = Height + RAIL_HEIGHT;
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int scxyz_secondramp(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float z2,top;
    float nbank_x,nbank_y,nbank_z;
    float yp,nx,ny,nz,fullwidth;
    float absx = ABS(x);
    float absz = ABS(z);

    if (absx <= Width/2.0) {
	/* on down-slope */
	z2 = Length + Gap/2.0 - z;
	yp = YHEIGHT(z) - BRIDGE_ROAD_LINE_FLOAT; 
	ny = 1.0;
	nz = (Height/2.0)*(M_PI/Length)*FSIN(z2*M_PI/Length);
	NORMALIZE2(ny,nz);
	sc->mc_y = yp;
	if (y > sc->mc_y - BBOX_MARGIN) {
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = ny;
	    sc->mc_normal[2] = nz;
	}
	else {
	    sc->mc_normal[0] = 0.0;
	    sc->mc_normal[1] = 0.0;
	    sc->mc_normal[2] = -1.0;
	}
	return(TRUE);
    }
    else {
	fullwidth = Width/2.0 + BankWidth - BankWidth/Length*(absz-Gap/2.0);
	if (absx < fullwidth) {
	    /* intersecting a bank */
	    /* normal to bank polygon is cross product of two edges */
	    nbank_x = Length*Height;
	    nbank_y = Length*BankWidth;
	    nbank_z = Height*BankWidth;
	    NORMALIZE3(nbank_x,nbank_y,nbank_z);
	    top = Height * (1.0 - (absz-Gap/2.0)/(Length-Gap/2.0));
	    yp = (1.0 - (absx-Width/2.0)/(fullwidth-Width/2.0)) * top;
	    /* To keep people from climbing the walls, bump this to 5.0
	     * if we're far enough into the ramp.
	     */
	    if ((yp < 5.0) && (top > 5.0)) yp = 5.0;
	    sc->mc_y = yp;
	    if ((absz < Gap/2.0+3.0)
		    && (absx < Width/2.0+BankWidth - 3.0)) {
		/* hit end of abutment */
		sc->mc_normal[0] = 0.0;
		sc->mc_normal[1] = 0.0;
		sc->mc_normal[2] = -1.0;
	    }
	    else {
		if (x >= 0.0) nx = nbank_x;
		else nx = -nbank_x;
		ny = nbank_y;
		nz = nbank_z;
		sc->mc_normal[0] = nx;
		sc->mc_normal[1] = ny;
		sc->mc_normal[2] = nz;
	    }
	    return(TRUE);
	}
    }
    /* else */
    return(FALSE);
}


static int scbbox_firstramp(
    DRIVE_OBJECT *obj,
    float bbox[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float z2;
    float yp,ny,nz;

    if ((bbox[3] >= -Width/2.0)
	    && (bbox[0] <= Width/2.0)) {
	z2 = bbox[5] + Gap/2.0 + Length;
	yp = YHEIGHT(bbox[5]) - BRIDGE_ROAD_LINE_FLOAT; 
	ny = 1.0;
	nz = -(Height/2.0)*(M_PI/Length)*FSIN(z2*M_PI/Length);
	NORMALIZE2(ny,nz);
	sc->mc_y = yp;
	sc->mc_normal[0] = 0.0;
	sc->mc_normal[1] = ny;
	sc->mc_normal[2] = nz;
	return(TRUE);
    }
    else if (bbox[3] < -Width/2.0) {
	return(scxyz_firstramp(obj,bbox[3],bbox[1],bbox[5],sc));
    }
    else {
	return(scxyz_firstramp(obj,bbox[0],bbox[1],bbox[5],sc));
    }
}


static int scbbox_bridge(
    DRIVE_OBJECT *obj,
    float bbox[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    /* on bridge */
    if ((bbox[1] < Height)
	    && (bbox[4] > Height)) {
	sc->mc_y = Height;
	return(TRUE);
    }
    /* else */
    return(FALSE);
}


static int scbbox_gr(
    DRIVE_OBJECT *obj,
    float bbox[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    /* ran into a guard rail */
    if ((bbox[1] < (Height+RAIL_HEIGHT))
	    && (bbox[4] > Height)) {
	sc->mc_y = Height + RAIL_HEIGHT;
	return(TRUE);
    }
    /* else */
    return(FALSE);
}


static int scbbox_secondramp(
    DRIVE_OBJECT *obj,
    float bbox[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    float z2;
    float yp,ny,nz;

    if ((bbox[3] >= -Width/2.0)
	    && (bbox[0] <= Width/2.0)) {
	z2 = bbox[2] + Gap/2.0 + Length;
	yp = YHEIGHT(bbox[2]) - BRIDGE_ROAD_LINE_FLOAT; 
	ny = 1.0;
	nz = -(Height/2.0)*(M_PI/Length)*FSIN(z2*M_PI/Length);
	NORMALIZE2(ny,nz);
	sc->mc_y = yp;
	sc->mc_normal[0] = 0.0;
	sc->mc_normal[1] = ny;
	sc->mc_normal[2] = nz;
	return(TRUE);
    }
    else if (bbox[3] < -Width/2.0) {
	return(scxyz_secondramp(obj,bbox[3],bbox[1],bbox[2],sc));
    }
    else {
	return(scxyz_secondramp(obj,bbox[0],bbox[1],bbox[2],sc));
    }
}


static void create_bridge_graphics(
    DRIVE_OBJECT *obj)
{
#if defined(HOVERWARE_MODEL)
    hwObject
	group;

    group = high_res_model(obj);
    obj->display_list = createHwSegmentFromObj( &group, 1 );
#else
    int BridgeSeg;
    int HighResSeg;
    int MidResSeg;
    int LowResSeg;
    float mc_extent[2][3];


    /* Make it wider than real to prevent early culling when approaching. */
    mc_extent[0][0] = -Width*2;
    mc_extent[0][1] = 0.0;
    mc_extent[0][2] = -Gap/2.0 - Length;
    mc_extent[1][0] = Width*2;
    mc_extent[1][1] = Height + RAIL_HEIGHT;
    mc_extent[1][2] = Gap/2.0 + Length;


    BridgeSeg = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg = get_dl_segment();
    LowResSeg = get_dl_segment();

    obj->display_list = BridgeSeg;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
        high_res_model(obj);
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
        mid_res_model(obj);
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
	HANDLE_OBJECT_NAMESET(img_fildes,obj);
        low_res_model(obj);
	DEFAULT_OBJECT_NAMESET(img_fildes,obj);
    close_segment(img_fildes);

    open_segment(img_fildes,BridgeSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      set_cull_size(img_fildes,100.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      set_cull_size(img_fildes,200.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,MidResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);
#endif
}


static hwObject high_res_model(
    DRIVE_OBJECT *obj)
{
    float deck_and_rails[18][3];
    float railcap[4][3];
    float bankcap[4][3];
    float y[DIVISIONS_HIGH],mesh[DIVISIONS_HIGH*2][3],*fptr;
    int i;
    float x,z;
    hwObject objList[20], group;
#if !defined(HOVERWARE_MODEL)
    float ytmp,ztmp;
    float pgon[4][6];
#endif

    i = 0;
    deck_and_rails[i][0] = -Width/2.0;
	deck_and_rails[i][1] = Height - DECK_THICKNESS;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = -Width/2.0;
	deck_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = -Width/2.0 + RAIL_WIDTH;
	deck_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = -Width/2.0 + RAIL_WIDTH;
	deck_and_rails[i][1] = Height;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = Width/2.0 - RAIL_WIDTH;
	deck_and_rails[i][1] = Height;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = Width/2.0 - RAIL_WIDTH;
	deck_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = Width/2.0;
	deck_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = Width/2.0;
	deck_and_rails[i][1] = Height - DECK_THICKNESS;
	deck_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_and_rails[i][0] = -Width/2.0;
	deck_and_rails[i][1] = Height - DECK_THICKNESS;
	deck_and_rails[i][2] = Gap/2.0;
    for (i=0; i<18; i+=2) {
	deck_and_rails[i+1][0] = deck_and_rails[i][0];
	deck_and_rails[i+1][1] = deck_and_rails[i][1];
	deck_and_rails[i+1][2] = -deck_and_rails[i][2];
    }

    i = 0;
    railcap[i][0] = -Width/2.0 + RAIL_WIDTH;
	railcap[i][1] = Height - DECK_THICKNESS;
	railcap[i][2] = -Gap/2.0;
	++i;
    railcap[i][0] = -Width/2.0 + RAIL_WIDTH;
	railcap[i][1] = Height + RAIL_HEIGHT;
	railcap[i][2] = -Gap/2.0;
	++i;
    railcap[i][0] = -Width/2.0;
	railcap[i][1] = Height + RAIL_HEIGHT;
	railcap[i][2] = -Gap/2.0;
	++i;
    railcap[i][0] = -Width/2.0;
	railcap[i][1] = Height - DECK_THICKNESS;
	railcap[i][2] = -Gap/2.0;
	++i;

    i = 0;
    bankcap[i][0] = -Width/2.0 - BankWidth;
	bankcap[i][1] = 0.0;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = -Width/2.0;
	bankcap[i][1] = Height;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = Width/2.0;
	bankcap[i][1] = Height;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = Width/2.0 + BankWidth;
	bankcap[i][1] = 0.0;
	bankcap[i][2] = -Gap/2.0;
	++i;

    for (i=0,z=0.0; i<DIVISIONS_HIGH; ++i,z+=(1.0/(DIVISIONS_HIGH-1.0))) {
	y[i] = (Height/2.0) * (1.0 - FCOS(z*M_PI)) + 0.02;
    }

    fptr = (float *) mesh;
    for (i = 0, z = (-Gap/2.0-Length);
	    i < DIVISIONS_HIGH;
	    ++i, z += (Length/(DIVISIONS_HIGH-1.0))) {
	*fptr++ = (-Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;

	*fptr++ = (Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;
    }

    objList[0] = hwMesh->create( hwMesh );
    objList[0]->name = 0;
    ASPHALT_HW( objList[0] );
    HW_MODIFY_1I( objList[0], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[0], hwStrGraphM, 2 );
    objList[0]->modify( objList[0], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    NEGATE_XZ(mesh,DIVISIONS_HIGH*2);
    objList[1] = hwMesh->create( hwMesh );
    objList[1]->name = 0;
    ASPHALT_HW( objList[1] );
    HW_MODIFY_1I( objList[1], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[1], hwStrGraphM, 2 );
    objList[1]->modify( objList[1], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    objList[2] = hwMesh->create( hwMesh );
    objList[2]->name = 0;
    ASPHALT_HW( objList[2] );
    HW_MODIFY_1I( objList[2], hwStrGraphN, 9 );
    HW_MODIFY_1I( objList[2], hwStrGraphM, 2 );
    objList[2]->modify( objList[2], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, 9*2*3),
			deck_and_rails );

    objList[3] = hwPolygon->create( hwPolygon );
    objList[3]->name = 0;
    ASPHALT_HW( objList[3] );
    HW_MODIFY_1B( objList[3], hwStrBackface, HW_TRUE );
    objList[3]->modify( objList[3], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3),
			railcap );

    NEGATE_X(railcap,4);
    objList[4] = hwPolygon->create( hwPolygon );
    objList[4]->name = 0;
    ASPHALT_HW( objList[4] );
    HW_MODIFY_1B( objList[4], hwStrBackface, HW_TRUE );
    objList[4]->modify( objList[4], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3),
			railcap );

    NEGATE_Z(railcap,4);
    objList[5] = hwPolygon->create( hwPolygon );
    objList[5]->name = 0;
    ASPHALT_HW( objList[5] );
    HW_MODIFY_1B( objList[5], hwStrBackface, HW_TRUE );
    objList[5]->modify( objList[5], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3),
			railcap );

    NEGATE_X(railcap,4);
    objList[6] = hwPolygon->create( hwPolygon );
    objList[6]->name = 0;
    ASPHALT_HW( objList[6] );
    HW_MODIFY_1B( objList[6], hwStrBackface, HW_TRUE );
    objList[6]->modify( objList[6], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3),
			railcap );

    objList[7] = hwPolygon->create( hwPolygon );
    objList[7]->name = 0;
    ASPHALT_HW( objList[7] );
    HW_MODIFY_1B( objList[7], hwStrBackface, HW_TRUE );
    objList[7]->modify( objList[7], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, NUMPTS(bankcap)*3),
			bankcap );

    NEGATE_Z(bankcap,4);
    objList[8] = hwPolygon->create( hwPolygon );
    objList[8]->name = 0;
    ASPHALT_HW( objList[8] );
    HW_MODIFY_1B( objList[8], hwStrBackface, HW_TRUE );
    objList[8]->modify( objList[8], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, NUMPTS(bankcap)*3),
			bankcap );

    fptr = (float *) mesh;
    for (i = 0, x = (-Width/2.0), z = (-Gap/2.0-Length);
	    i < DIVISIONS_HIGH;
	    ++i,
		x -= (BankWidth)/(DIVISIONS_HIGH-1.0),
		z += (Length/(DIVISIONS_HIGH-1.0))) {
	*fptr++ = x;
	*fptr++ = 0.0;
	*fptr++ = z;

	*fptr++ = (-Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;
    }

    objList[9] = hwMesh->create( hwMesh );
    objList[9]->name = 0;
    GRASS_HW( objList[9] );
    HW_MODIFY_1I( objList[9], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[9], hwStrGraphM, 2 );
    HW_MODIFY_1B( objList[9], hwStrBackface, HW_FALSE );
    objList[9]->modify( objList[9], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    for (i=0; i<DIVISIONS_HIGH*2; ++i) mesh[i][0] = -mesh[i][0];

    objList[10] = hwMesh->create( hwMesh );
    objList[10]->name = 0;
    GRASS_HW( objList[10] );
    HW_MODIFY_1I( objList[10], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[10], hwStrGraphM, 2 );
    HW_MODIFY_1B( objList[10], hwStrBackface, HW_TRUE );
    objList[10]->modify( objList[10], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    for (i=0; i<DIVISIONS_HIGH*2; ++i) mesh[i][2] = -mesh[i][2];

    objList[11] = hwMesh->create( hwMesh );
    objList[11]->name = 0;
    GRASS_HW( objList[11] );
    HW_MODIFY_1I( objList[11], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[11], hwStrGraphM, 2 );
    HW_MODIFY_1B( objList[11], hwStrBackface, HW_FALSE );
    objList[11]->modify( objList[11], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    for (i=0; i<DIVISIONS_HIGH*2; ++i) mesh[i][0] = -mesh[i][0];

    objList[12] = hwMesh->create( hwMesh );
    objList[12]->name = 0;
    GRASS_HW( objList[12] );
    HW_MODIFY_1I( objList[12], hwStrGraphN, DIVISIONS_HIGH );
    HW_MODIFY_1I( objList[12], hwStrGraphM, 2 );
    HW_MODIFY_1B( objList[12], hwStrBackface, HW_TRUE );
    objList[12]->modify( objList[12], hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT, DIVISIONS_HIGH*2*3),
			mesh );

    group = hwGroup->create( hwGroup );
    group->modify( group, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,13),
			objList );
    return group;

#if !defined(HOVERWARE_MODEL)
    /*** Road lines ***/
    ROAD_LINE_YELLOW(img_fildes);
    vertex_format(img_fildes,3,3,0,0,COUNTER_CLOCKWISE);
    for (z=(-Gap/2.0-Length); z<(Gap/2.0+Length); z+=ROAD_LINE_SPACING) {
	fptr = (float *) pgon;
	*fptr++ = (-ROAD_LINE_WIDTH/2.0);
	ytmp = *fptr++ = YHEIGHT(z);
	*fptr++ = z;
	*fptr++ = 0.0;
	*fptr++ = 0.707;
	*fptr++ = 0.707;

	*fptr++ = (ROAD_LINE_WIDTH/2.0);
	*fptr++ = ytmp;
	*fptr++ = z;
	*fptr++ = 0.0;
	*fptr++ = 0.707;
	*fptr++ = -0.707;

	if ((ztmp = z + ROAD_LINE_LENGTH) > (Gap/2.0+Length)) 
	    ztmp = (Gap/2.0+Length);
	*fptr++ = (ROAD_LINE_WIDTH/2.0);
	ytmp = *fptr++ = YHEIGHT(ztmp);
	*fptr++ = ztmp;
	*fptr++ = 0.0;
	*fptr++ = 0.707;
	*fptr++ = -0.707;

	*fptr++ = (-ROAD_LINE_WIDTH/2.0);
	*fptr++ = ytmp;
	*fptr++ = ztmp;
	*fptr++ = 0.0;
	*fptr++ = 0.707;
	*fptr++ = 0.707;

	polygon3d(img_fildes,pgon,4,FALSE);
    }
    RESTORE_DEFAULT_VERTEX_FORMAT(img_fildes);
    DIFFUSE_LIGHTING_OFF(img_fildes);
#endif
}


#if !defined(HOVERWARE_MODEL)
static void mid_res_model(
    DRIVE_OBJECT *obj)
{
    float deck_t[4][3];
    float deck_b_and_rails[8][3];
    float bankcap[4][3];
    float y[DIVISIONS_MED],mesh[DIVISIONS_MED*2][3],*fptr;
    int i;
    float x,z;

    i = 0;
    deck_t[i][0] = -Width/2.0;
	deck_t[i][1] = Height;
	deck_t[i][2] = Gap/2.0;
	++i;
    deck_t[i][0] = -Width/2.0;
	deck_t[i][1] = Height;
	deck_t[i][2] = -Gap/2.0;
	++i;
    deck_t[i][0] = Width/2.0;
	deck_t[i][1] = Height;
	deck_t[i][2] = -Gap/2.0;
	++i;
    deck_t[i][0] = Width/2.0;
	deck_t[i][1] = Height;
	deck_t[i][2] = Gap/2.0;
	++i;

    i = 0;
    deck_b_and_rails[i][0] = Width/2.0;
	deck_b_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_b_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_b_and_rails[i][0] = Width/2.0;
	deck_b_and_rails[i][1] = Height;
	deck_b_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_b_and_rails[i][0] = -Width/2.0;
	deck_b_and_rails[i][1] = Height;
	deck_b_and_rails[i][2] = Gap/2.0;
	i += 2;
    deck_b_and_rails[i][0] = -Width/2.0;
	deck_b_and_rails[i][1] = Height + RAIL_HEIGHT;
	deck_b_and_rails[i][2] = Gap/2.0;
	i += 2;
    for (i=0; i<8; i+=2) {
	deck_b_and_rails[i+1][0] = deck_b_and_rails[i][0];
	deck_b_and_rails[i+1][1] = deck_b_and_rails[i][1];
	deck_b_and_rails[i+1][2] = -deck_b_and_rails[i][2];
    }

    i = 0;
    bankcap[i][0] = -Width/2.0 - BankWidth;
	bankcap[i][1] = 0.0;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = -Width/2.0;
	bankcap[i][1] = Height;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = Width/2.0;
	bankcap[i][1] = Height;
	bankcap[i][2] = -Gap/2.0;
	++i;
    bankcap[i][0] = Width/2.0 + BankWidth;
	bankcap[i][1] = 0.0;
	bankcap[i][2] = -Gap/2.0;
	++i;

    for (i=0,z=0.0; i<DIVISIONS_MED; ++i,z+=(1.0/(DIVISIONS_MED-1.0))) {
	y[i] = (Height/2.0) * (1.0 - FCOS(z*M_PI)) + (Height/500.0);
    }

    fptr = (float *) mesh;
    for (i = 0, z = (-Gap/2.0-Length);
	    i < DIVISIONS_MED;
	    ++i, z += (Length/(DIVISIONS_MED-1.0))) {
	*fptr++ = (-Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;

	*fptr++ = (Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;
    }

    ASPHALT(img_fildes);
    quadrilateral_mesh(img_fildes,mesh,DIVISIONS_MED,2,NULL);
    NEGATE_XZ(mesh,DIVISIONS_MED*2);
    quadrilateral_mesh(img_fildes,mesh,DIVISIONS_MED,2,NULL);
    polygon3d(img_fildes,deck_t,NUMPTS(deck_t),FALSE);
    quadrilateral_mesh(img_fildes,deck_b_and_rails,4,2,NULL);
    polygon3d(img_fildes,bankcap,NUMPTS(bankcap),FALSE);
    NEGATE_Z(bankcap,4);
    polygon3d(img_fildes,bankcap,NUMPTS(bankcap),FALSE);

    GRASS(img_fildes);
    fptr = (float *) mesh;
    for (i = 0, x = (-Width/2.0), z = (-Gap/2.0-Length);
	    i < DIVISIONS_MED;
	    ++i,
		x -= (BankWidth)/(DIVISIONS_MED-1.0),
		z += (Length/(DIVISIONS_MED-1.0))) {
	*fptr++ = x;
	*fptr++ = 0.0;
	*fptr++ = z;

	*fptr++ = (-Width/2.0);
	*fptr++ = y[i];
	*fptr++ = z;
    }
    mesh_to_strips(img_fildes,mesh,DIVISIONS_MED,2,NULL,3,CLOCKWISE);
    for (i=0; i<DIVISIONS_MED*2; ++i) mesh[i][0] = -mesh[i][0];
    mesh_to_strips(img_fildes,mesh,DIVISIONS_MED,2,NULL,3,COUNTER_CLOCKWISE);
    for (i=0; i<DIVISIONS_MED*2; ++i) mesh[i][2] = -mesh[i][2];
    mesh_to_strips(img_fildes,mesh,DIVISIONS_MED,2,NULL,3,CLOCKWISE);
    for (i=0; i<DIVISIONS_MED*2; ++i) mesh[i][0] = -mesh[i][0];
    mesh_to_strips(img_fildes,mesh,DIVISIONS_MED,2,NULL,3,COUNTER_CLOCKWISE);
}


static void low_res_model(
    DRIVE_OBJECT *obj)
{
    float ramp_plus_deck[8][3];
    float bankcap[4][3];
    float bank[3][3];

    ramp_plus_deck[0][0] = -Width/2.0;
	ramp_plus_deck[0][1] = 0.0;
	ramp_plus_deck[0][2] = -Gap/2.0 - Length;
    ramp_plus_deck[1][0] = Width/2.0;
	ramp_plus_deck[1][1] = 0.0;
	ramp_plus_deck[1][2] = -Gap/2.0 - Length;

    ramp_plus_deck[2][0] = -Width/2.0;
	ramp_plus_deck[2][1] = Height;
	ramp_plus_deck[2][2] = -Gap/2.0;
    ramp_plus_deck[3][0] = Width/2.0;
	ramp_plus_deck[3][1] = Height;
	ramp_plus_deck[3][2] = -Gap/2.0;

    ramp_plus_deck[4][0] = -Width/2.0;
	ramp_plus_deck[4][1] = Height;
	ramp_plus_deck[4][2] = Gap/2.0;
    ramp_plus_deck[5][0] = Width/2.0;
	ramp_plus_deck[5][1] = Height;
	ramp_plus_deck[5][2] = Gap/2.0;

    ramp_plus_deck[6][0] = -Width/2.0;
	ramp_plus_deck[6][1] = 0.0;
	ramp_plus_deck[6][2] = Gap/2.0 + Length;
    ramp_plus_deck[7][0] = Width/2.0;
	ramp_plus_deck[7][1] = 0.0;
	ramp_plus_deck[7][2] = Gap/2.0 + Length;

    bankcap[0][0] = -Width/2.0 - BankWidth;
	bankcap[0][1] = 0.0;
	bankcap[0][2] = -Gap/2.0;
    bankcap[1][0] = -Width/2.0;
	bankcap[1][1] = Height;
	bankcap[1][2] = -Gap/2.0;
    bankcap[2][0] = Width/2.0;
	bankcap[2][1] = Height;
	bankcap[2][2] = -Gap/2.0;
    bankcap[3][0] = Width/2.0 + BankWidth;
	bankcap[3][1] = 0.0;
	bankcap[3][2] = -Gap/2.0;

    bank[0][0] = -Width/2.0;
	bank[0][1] = 0.0;
	bank[0][2] = -Gap/2.0-Length;
    bank[1][0] = -Width/2.0;
	bank[1][1] = Height;
	bank[1][2] = -Gap/2.0;
    bank[2][0] = -Width/2.0-BankWidth;
	bank[2][1] = 0.0;
	bank[2][2] = -Gap/2.0;

    ASPHALT(img_fildes);
    quadrilateral_mesh(img_fildes,ramp_plus_deck,4,2,NULL);
    polygon3d(img_fildes,bankcap,4,FALSE);
    NEGATE_Z(bankcap,4);
    polygon3d(img_fildes,bankcap,4,FALSE);

    GRASS(img_fildes);
    polygon3d(img_fildes,bank,3,FALSE);
    NEGATE_X(bank,3);
    polygon3d(img_fildes,bank,3,FALSE);
    NEGATE_Z(bank,3);
    polygon3d(img_fildes,bank,3,FALSE);
    NEGATE_X(bank,3);
    polygon3d(img_fildes,bank,3,FALSE);
}
#endif /* !HOVERWARE_MODEL */


void init_bridge_object(
    DRIVE_OBJECT *obj)
{
    BRIDGE_LIST *rl;
    DRIVE_OBJECT *firstramp,*bridge,*secondramp,*gr1,*gr2;

    if (Length <= 0.0) Length = DEFAULT_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_WIDTH;
    if (Height <= 0.0) Height = DEFAULT_HEIGHT;

    /* length really needs to be the length of one of the ramps */
    Length = (Length - Gap) / 2.0;

    if(debug) printf(" inside init_bridge_object() routine \n");

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = bridge_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,Length)
		&& IS_NEAR(rl->width,Width)
		&& IS_NEAR(rl->height,Height)
		&& (rl->nameset_bits == obj->nameset_bits)) {
	    break;
	}
	/* else */
	rl = rl->next;
    }
    if (rl != NULL) {
	/* Good -- I have one like this already. */
	obj->display_list = rl->dl_number;
    }
    else {
	/* Nope -- gotta create a new one. */
	if ((rl = (BRIDGE_LIST *) malloc(sizeof(BRIDGE_LIST))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = Length;
	rl->width  = Width;
	rl->height = Height;
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = bridge_list;
    	create_bridge_graphics(obj);
	rl->dl_number = obj->display_list;
	bridge_list  = rl;
    }

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = -Width/2.0 - BankWidth;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length - Gap/2.0;
    obj->bound_mc[3] = Width/2.0 + BankWidth;
    obj->bound_mc[4] = Height+RAIL_HEIGHT+BBOX_MARGIN;
    obj->bound_mc[5] = Length + Gap/2.0;
    update_wc_bounds(obj);

    /* Now create the subobjects */
    if ((firstramp = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* else */
    memcpy(firstramp,obj,sizeof(DRIVE_OBJECT));
    firstramp->num_children = 0;
    firstramp->child_list = NULL;
    firstramp->display_list = 0;
    firstramp->bound_mc[0] = -Width/2.0 - BankWidth;
    firstramp->bound_mc[1] = 0.0;
    firstramp->bound_mc[2] = -Length - Gap/2.0;
    firstramp->bound_mc[3] = Width/2.0 + BankWidth;
    firstramp->bound_mc[4] = Height+BBOX_MARGIN;
    firstramp->bound_mc[5] = -Gap/2.0;
    update_wc_bounds(firstramp);
    firstramp->surface_chars_xyz  = scxyz_firstramp;
    firstramp->surface_chars_bbox = scbbox_firstramp;
    add_object_to_list(&(obj->child_list),firstramp);

    if ((bridge = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* else */
    memcpy(bridge,obj,sizeof(DRIVE_OBJECT));
    bridge->num_children = 0;
    bridge->child_list = NULL;
    bridge->display_list = 0;
    bridge->bound_mc[0] = -Width/2.0;
    bridge->bound_mc[1] = Height-0.5;
    bridge->bound_mc[2] = -Gap/2.0;
    bridge->bound_mc[3] = Width/2.0;
    bridge->bound_mc[4] = Height+BBOX_MARGIN;
    bridge->bound_mc[5] = Gap/2.0;
    update_wc_bounds(bridge);
    bridge->surface_chars_xyz  = scxyz_bridge;
    bridge->surface_chars_bbox = scbbox_bridge;
    add_object_to_list(&(obj->child_list),bridge);

    if ((gr1 = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* else */
    memcpy(gr1,obj,sizeof(DRIVE_OBJECT));
    gr1->num_children = 0;
    gr1->child_list = NULL;
    gr1->display_list = 0;
    gr1->bound_mc[0] = -Width/2.0 - RAIL_WIDTH;
    gr1->bound_mc[1] = Height - 0.5;
    gr1->bound_mc[2] = -Gap/2.0;
    gr1->bound_mc[3] = -Width/2.0;
    gr1->bound_mc[4] = Height + RAIL_HEIGHT+BBOX_MARGIN;
    gr1->bound_mc[5] = Gap/2.0;
    update_wc_bounds(gr1);
    gr1->surface_chars_xyz  = scxyz_gr;
    gr1->surface_chars_bbox = scbbox_gr;
    add_object_to_list(&(obj->child_list),gr1);

    if ((gr2 = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* else */
    memcpy(gr2,obj,sizeof(DRIVE_OBJECT));
    gr2->num_children = 0;
    gr2->child_list = NULL;
    gr2->display_list = 0;
    gr2->bound_mc[0] = Width/2.0;
    gr2->bound_mc[1] = Height - 0.5;
    gr2->bound_mc[2] = -Gap/2.0;
    gr2->bound_mc[3] = Width/2.0 + RAIL_WIDTH;
    gr2->bound_mc[4] = Height + RAIL_HEIGHT+BBOX_MARGIN;
    gr2->bound_mc[5] = Gap/2.0;
    update_wc_bounds(gr2);
    gr2->surface_chars_xyz  = scxyz_gr;
    gr2->surface_chars_bbox = scbbox_gr;
    add_object_to_list(&(obj->child_list),gr2);

    if ((secondramp = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* else */
    memcpy(secondramp,obj,sizeof(DRIVE_OBJECT));
    secondramp->num_children = 0;
    secondramp->child_list = NULL;
    secondramp->display_list = 0;
    secondramp->bound_mc[0] = -Width/2.0 - BankWidth;
    secondramp->bound_mc[1] = 0.0;
    secondramp->bound_mc[2] = Gap/2.0;
    secondramp->bound_mc[3] = Width/2.0 + BankWidth;
    secondramp->bound_mc[4] = Height+BBOX_MARGIN;
    secondramp->bound_mc[5] = Length + Gap/2.0;
    update_wc_bounds(secondramp);
    secondramp->surface_chars_xyz  = scxyz_secondramp;
    secondramp->surface_chars_bbox = scbbox_secondramp;
    add_object_to_list(&(obj->child_list),secondramp);
}
