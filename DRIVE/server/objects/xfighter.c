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


/* Code Module for the xfighter car object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "prims.h"
#include "obj_common.h"
#include "demo_physics.h"

#define	XSCALE	1.5
#define	YSCALE	1.5
#define	ZSCALE	1.5

#define DEFAULT_RED	0.4
#define DEFAULT_GRN	0.4
#define DEFAULT_BLU	0.4

#define	XS(X)		((X)*(XSCALE))
#define	YS(Y)		((Y)*(YSCALE))
#define	ZS(Z)		((Z)*(ZSCALE))

static int xfighter_dl = -1, shadow_dl = -1;

static VEHICLE_AUXDATA *init_vaux(
    DRIVE_OBJECT *obj)
{
    static VEHICLE_AUXDATA *vaux = NULL;

    if (vaux != NULL) {
	return(vaux);
    }

    /***** INIT AUXILIARY DATA *****/
    if ((vaux = (VEHICLE_AUXDATA *) malloc(sizeof(VEHICLE_AUXDATA))) == NULL) {
	return(NULL);
    }

    /***** INIT AUXILIARY DATA *****/
    vaux->bbox_mc[PT_UFL][0] = XS(-4.5);
	    vaux->bbox_mc[PT_UFL][1] = YS(-4.5);
	    vaux->bbox_mc[PT_UFL][2] = ZS(7.0);
    vaux->bbox_mc[PT_UFR][0] =  XS(4.5);
	    vaux->bbox_mc[PT_UFR][1] = YS(-4.5);
	    vaux->bbox_mc[PT_UFR][2] = ZS(7.0);
    vaux->bbox_mc[PT_UBR][0] =  XS(4.5);
	    vaux->bbox_mc[PT_UBR][1] = YS(-4.5);
	    vaux->bbox_mc[PT_UBR][2] = ZS(-7.0);
    vaux->bbox_mc[PT_UBL][0] = XS(-4.5);
	    vaux->bbox_mc[PT_UBL][1] = YS(-4.5);
	    vaux->bbox_mc[PT_UBL][2] = ZS(-7.0);
    vaux->bbox_mc[PT_TFL][0] = XS(-4.5);
	    vaux->bbox_mc[PT_TFL][1] = YS(4.5);
	    vaux->bbox_mc[PT_TFL][2] = ZS(7.0);
    vaux->bbox_mc[PT_TFR][0] = XS(4.5);
	    vaux->bbox_mc[PT_TFR][1] = YS(4.5);
	    vaux->bbox_mc[PT_TFR][2] = ZS(7.0);
    vaux->bbox_mc[PT_TBR][0] = XS(4.5);
	    vaux->bbox_mc[PT_TBR][1] = YS(4.5);
	    vaux->bbox_mc[PT_TBR][2] = ZS(-7.0);
    vaux->bbox_mc[PT_TBL][0] = XS(-4.5);
	    vaux->bbox_mc[PT_TBL][1] = YS(4.5);
	    vaux->bbox_mc[PT_TBL][2] = ZS(-7.0);

    vaux->COG_mc[0] =
	    (vaux->bbox_mc[PT_UFR][0] + vaux->bbox_mc[PT_UFL][0]) / 2.0;
    vaux->COG_mc[1] =
	    (vaux->bbox_mc[PT_TFL][1] + vaux->bbox_mc[PT_UFL][1]) / 2.0;
    vaux->COG_mc[2] =
	    (vaux->bbox_mc[PT_UBR][2] + vaux->bbox_mc[PT_UFR][2]) / 2.0;

    vaux->coefficient_of_drag = 0.030;
    vaux->wheel_torque_mult   = 75.0 /* *pobj->I[2][2] */;
    vaux->best_turn_speed     = MPH_TO_FPS(25.0);
    vaux->max_obstacle_height = 1.0;

    vaux->horsepower          = 2000.0;
    vaux->peak_power_rpm      = 3500.0;
    vaux->two_peak_power      = 0.0;
    vaux->gear_best_speed[0]  = MPH_TO_FPS(15.0);	/* reverse */
    vaux->gear_best_speed[1]  = 0.0;			/* neutral */
    vaux->gear_best_speed[2]  = MPH_TO_FPS(20.0);	/* 1st */
    vaux->gear_best_speed[3]  = MPH_TO_FPS(45.0);	/* 2nd */
    vaux->gear_best_speed[4]  = MPH_TO_FPS(70.0);	/* 3rd */
    vaux->gear_best_speed[5]  = MPH_TO_FPS(100.0);	/* 4th */
    vaux->gear_best_speed[6]  = MPH_TO_FPS(130.0);	/* 5th */
    vaux->offroad_performance = 0.0;
    vaux->thrust_mechanism    = THRUSTER;

    vaux->left_gauge_class  = GAUGE_DIGITAL_MPH;
    vaux->right_gauge_class = GAUGE_DIGITAL_ALTITUDE;
    vaux->max_speed         = 500.0;
    vaux->max_rpm           = 8000.0;
    vaux->max_altitude      = 25000.0;

    return(vaux);
}


static int xfighter_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = YS(5.0);
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int xfighter_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = YS(4.5);
    return(TRUE);
}


#define SHADOW_MARGIN 0.4
static int create_shadow_dl(
    void)
{
    static float body[] = {
	-3.00,	0.00,	6.43,
	 3.00,	0.00,	6.43,

	-1.52,	2.29,	-2.77,
	 1.52,	2.29,	-2.77,

	-2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	 2.75,	1.00,	-7.17 + SHADOW_MARGIN,

	-3.00,	0.00,	-7.17 + SHADOW_MARGIN,
	 3.00,	0.00,	-7.17 + SHADOW_MARGIN,

	-3.00,	0.00,	6.43,
	 3.00,	0.00,	6.43,
    };
    static float side1[] = {
	-3.00,	0.00,	-7.17 + SHADOW_MARGIN,
	-2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	-1.52,	2.29,	-2.77,
	-3.00,	0.00,	6.43,
    };
    static float side2[] = {
	 3.00,	0.00,	6.43,
	 1.52,	2.29,	-2.77,
	 2.75,	1.00,	-7.17 + SHADOW_MARGIN,
	 3.00,	0.00,	-7.17 + SHADOW_MARGIN,
    };
    /* Make it small so it isn't seen unless center is seen. */
    static float mc_extent[2][3] = {
	{ -0.1, 0.0, -0.1 },
	{  0.1, 0.1,  0.1 }
    };
    int seg,graphics_seg;

    seg = get_dl_segment();
    graphics_seg = get_dl_segment();

    open_segment(img_fildes,graphics_seg,TRUE,FALSE);
	SHADOW(img_fildes);
	TRANSPARENT_ON(img_fildes,0.5);
	quadrilateral_mesh(img_fildes,body,5,2,NULL);
	polygon3d(img_fildes,side1,NUMPTS(side1),NULL);
	polygon3d(img_fildes,side2,NUMPTS(side2),NULL);
	TRANSPARENT_OFF(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,seg,TRUE,FALSE);
	set_extent(img_fildes,mc_extent);
	cond_execute_segment(img_fildes,CI_PRUNE,FALSE,graphics_seg);
    close_segment(img_fildes);

    return(seg);
}


#if !defined(HOVERWARE_MODEL)
static void SetPoint( Poly, X, Y, Z )
float
    *Poly,
    X, Y, Z;
{
    *Poly++ = X; *Poly++ = Y; *Poly = Z;
}
#define BODY_COLOR(fildes) \
	surface_model((fildes),TRUE,4,1.0,1.0,1.0);

#define	HIWING(gfd) \
    SetPoint( WingPoly   , XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-2.0) ); \
    SetPoint( WingPoly+ 3, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-1.0) ); \
    SetPoint( WingPoly+ 6, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-4.0) ); \
    SetPoint( WingPoly+ 9, XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-6.0) ); \
    vertex_format( gfd, 0, 0, 0, FALSE, CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    vertex_format( gfd, 0, 0, 0, FALSE, COUNTER_CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    mesh_cone( gfd, ZS(0.5), ZS(0.5), 1, 1, NM, \
		XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-7.0), \
		XS(5.0)*COS45, YS(5.0)*SIN45, ZS( 0.0) ); \
    SetPoint( WingPoly   , XS(0.3)*COS45, YS(0.3)*SIN45, ZS( 5.8) ); \
    SetPoint( WingPoly+ 3, XS(1.0)*COS45, YS(1.0)*SIN45, ZS( 6.0) ); \
    SetPoint( WingPoly+ 6, XS(1.0)*COS45, YS(1.0)*SIN45, ZS( 5.3) ); \
    SetPoint( WingPoly+ 9, XS(0.3)*COS45, YS(0.3)*SIN45, ZS( 5.0) ); \
    vertex_format( gfd, 0, 0, 0, FALSE, CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    vertex_format( gfd, 0, 0, 0, FALSE, COUNTER_CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    mesh_cone( gfd, ZS(0.1), ZS(0.1), 1, 1, NM, \
		XS(1.0)*COS45, YS(1.0)*SIN45, ZS( 5.0), \
		XS(1.0)*COS45, YS(1.0)*SIN45, ZS( 6.0) )

#define	MEDWING(gfd) \
    SetPoint( WingPoly   , XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-2.0) ); \
    SetPoint( WingPoly+ 3, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-1.0) ); \
    SetPoint( WingPoly+ 6, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-4.0) ); \
    SetPoint( WingPoly+ 9, XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-6.0) ); \
    vertex_format( gfd, 0, 0, 0, FALSE, CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    vertex_format( gfd, 0, 0, 0, FALSE, COUNTER_CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    mesh_cone( gfd, ZS(0.5), ZS(0.5), 1, 1, NM, \
		XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-7.0), \
		XS(5.0)*COS45, YS(5.0)*SIN45, ZS( 0.0) )

#define	LOWING(gfd) \
    SetPoint( WingPoly   , XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-2.0) ); \
    SetPoint( WingPoly+ 3, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-1.0) ); \
    SetPoint( WingPoly+ 6, XS(5.0)*COS45, YS(5.0)*SIN45, ZS(-4.0) ); \
    SetPoint( WingPoly+ 9, XS(1.0)*COS45, YS(1.0)*SIN45, ZS(-6.0) ); \
    vertex_format( gfd, 0, 0, 0, FALSE, CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE ); \
    vertex_format( gfd, 0, 0, 0, FALSE, COUNTER_CLOCKWISE ); \
    polygon3d( gfd, WingPoly, 4, FALSE )

static void high_res_model( gfd )
int
    gfd;
{
    static float
	WingPoly[8*3];

#define	NM	6

    BODY_COLOR(gfd);	/* Make it shine */

    /* The body */
    mesh_cone( gfd, ZS(1.0), ZS(1.5), 1, 0, NM,
		0.0, 0.0, ZS(-7.0),	0.0, 0.0, ZS(-6.0) );
    mesh_cone( gfd, ZS(1.5), ZS(1.5), 0, 0, NM,
		0.0, 0.0, ZS(-6.0),	0.0, 0.0, ZS(-1.0) );
    mesh_cone( gfd, ZS(1.5), ZS(1.0), 0, 0, NM,
		0.0, 0.0, ZS(-1.0),	0.0, 0.0, ZS( 0.0) );
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 0, 0, NM,
		0.0, 0.0, ZS( 0.0),	0.0, 0.0, ZS( 4.0) );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 0, NM,
		0.0, 0.0, ZS( 4.0),	0.0, 0.0, ZS( 5.0) );
    mesh_cone( gfd, ZS(0.5), ZS(0.5), 0, 0, NM,
		0.0, 0.0, ZS( 5.0),	0.0, 0.0, ZS( 6.0) );
    mesh_cone( gfd, ZS(0.5), ZS(0.0), 0, 0, NM,
		0.0, 0.0, ZS( 6.0),	0.0, 0.0, ZS( 7.0) );

    /* The turret/cockpit assembly */
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 0, 0, NM,
		0.0, YS(-2.0), ZS( 2.0),	0.0, YS(2.0), ZS( 2.0) );

    /* The turret */
    fill_color( gfd, 0.7, 0.7, 0.7 );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 1, NM,
		0.0, YS(-2.0), ZS( 2.0),	0.0, YS(-2.5), ZS( 2.0) );
    mesh_cone( gfd, ZS(0.25), ZS(0.25), 0, 1, NM,
		0.0, YS(-2.25), ZS( 2.0),	0.0, YS(-2.25), ZS( 3.5) );

    /* The intakes */
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 1, 1, NM,
		XS(1.5), 0.0, ZS(-6.0),	XS(1.5), 0.0, ZS(-1.0) );
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 1, 1, NM,
		XS(-1.5), 0.0, ZS(-6.0),	XS(-1.5), 0.0, ZS(-1.0) );

    /* The wings */
    HIWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    HIWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	-1.5
#define	YSCALE	1.5
    HIWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    HIWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	1.5
#define	YSCALE	1.5

    /* The cockpit */
    fill_color( gfd, 0.0, 0.0, 0.0 );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 1, NM,
		0.0, YS( 2.0), ZS( 2.0),	0.0, YS( 2.5), ZS( 2.0) );

    RESTORE_DEFAULT_SURFACE_MODEL(gfd);

#undef NM
}


static void med_res_model( gfd )
int
    gfd;
{
    static float
	WingPoly[8*3];

#define	NM	4

    BODY_COLOR(gfd);	/* Make it shine */

    /* The body */
    mesh_cone( gfd, ZS(1.0), ZS(1.5), 1, 0, NM,
		0.0, 0.0, ZS(-7.0),	0.0, 0.0, ZS(-6.0) );
    mesh_cone( gfd, ZS(1.5), ZS(1.5), 0, 0, NM,
		0.0, 0.0, ZS(-6.0),	0.0, 0.0, ZS(-1.0) );
    mesh_cone( gfd, ZS(1.5), ZS(1.0), 0, 0, NM,
		0.0, 0.0, ZS(-1.0),	0.0, 0.0, ZS( 0.0) );
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 0, 0, NM,
		0.0, 0.0, ZS( 0.0),	0.0, 0.0, ZS( 4.0) );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 0, NM,
		0.0, 0.0, ZS( 4.0),	0.0, 0.0, ZS( 5.0) );
    mesh_cone( gfd, ZS(0.5), ZS(0.5), 0, 0, NM,
		0.0, 0.0, ZS( 5.0),	0.0, 0.0, ZS( 6.0) );
    mesh_cone( gfd, ZS(0.5), ZS(0.0), 0, 0, NM,
		0.0, 0.0, ZS( 6.0),	0.0, 0.0, ZS( 7.0) );

    /* The turret/cockpit assembly */
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 0, 0, NM,
		0.0, YS(-2.0), ZS( 2.0),	0.0, YS(2.0), ZS( 2.0) );

    /* The turret */
    fill_color( gfd, 0.7, 0.7, 0.7 );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 1, NM,
		0.0, YS(-2.0), ZS( 2.0),	0.0, YS(-2.5), ZS( 2.0) );

    /* The intakes */
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 1, 1, NM,
		XS(1.5), 0.0, ZS(-6.0),	XS(1.5), 0.0, ZS(-1.0) );
    mesh_cone( gfd, ZS(1.0), ZS(1.0), 1, 1, NM,
		XS(-1.5), 0.0, ZS(-6.0),	XS(-1.5), 0.0, ZS(-1.0) );

    /* The wings */
    MEDWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    MEDWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	-1.5
#define	YSCALE	1.5
    MEDWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    MEDWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	1.5
#define	YSCALE	1.5

    /* The cockpit */
    fill_color( gfd, 0.0, 0.0, 0.0 );
    mesh_cone( gfd, ZS(1.0), ZS(0.5), 0, 1, NM,
		0.0, YS( 2.0), ZS( 2.0),	0.0, YS( 2.5), ZS( 2.0) );

    RESTORE_DEFAULT_SURFACE_MODEL(gfd);
#undef NM
}

static void low_res_model( gfd )
int
    gfd;
{
    static float
	WingPoly[8*3];

#define	NM	4

    BODY_COLOR(gfd);	/* Make it shine */

    /* The body */
    mesh_cone( gfd, ZS(1.5), ZS(0.0), 0, 0, NM,
		0.0, 0.0, ZS(-6.0),	0.0, 0.0, ZS( 7.0) );

    /* The wings */
    fill_color( gfd, 0.7, 0.7, 0.7 );

    LOWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    LOWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	-1.5
#define	YSCALE	1.5
    LOWING(gfd);
#undef	YSCALE
#define	YSCALE	-1.5
    LOWING(gfd);
#undef	XSCALE
#undef	YSCALE
#define	XSCALE	1.5
#define	YSCALE	1.5

    RESTORE_DEFAULT_SURFACE_MODEL(gfd);
#undef NM
}
#endif /* !HOVERWARE_MODEL */

static void create_xfighter_graphics(
    DRIVE_OBJECT *obj)
{
#if defined(HOVERWARE_MODEL)
    hwObject
	*objects;
    int
	numObjects;

    numObjects = hwParseFile( "objects/RedTeam.hw", &objects );
    obj->display_list = createHwSegmentFromObj( objects, numObjects );
#else
    int FighterSeg;
    int HighResSeg;
    int MidResSeg;
    int LowResSeg;

    float mc_extent[2][3];

    mc_extent[0][0] = XS(-4.5);
    mc_extent[0][1] = YS(-4.5);
    mc_extent[0][2] = ZS(-7.0);
    mc_extent[1][0] = XS( 4.5);
    mc_extent[1][1] = YS( 4.5);
    mc_extent[1][2] = ZS( 7.0);

    FighterSeg = get_dl_segment();
    HighResSeg = get_dl_segment();
    MidResSeg = get_dl_segment();
    LowResSeg = get_dl_segment();

    obj->display_list = FighterSeg;

    open_segment(img_fildes,HighResSeg,FALSE,FALSE);
      high_res_model(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,MidResSeg,FALSE,FALSE);
      med_res_model(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,LowResSeg,FALSE,FALSE);
      low_res_model(img_fildes);
    close_segment(img_fildes);

    open_segment(img_fildes,FighterSeg,FALSE,FALSE);
      set_extent(img_fildes,mc_extent);
      cond_return(img_fildes,CI_PRUNE,TRUE);

      set_cull_size(img_fildes,30.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,LowResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);

      set_cull_size(img_fildes,60.0);
      cond_execute_segment(img_fildes,CI_CULL,TRUE,MidResSeg);
      cond_return(img_fildes,CI_CULL,TRUE);
      /* if we get to here, it means the thing is big...draw the full model */
      execute_segment(img_fildes,HighResSeg);
    close_segment(img_fildes);
#endif
}


void init_xfighter_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *child;

    if(debug) printf(" inside init_xfighter_object() routine \n");

    if (obj->color[0] == DEFAULT_OBJECT_COLOR) obj->color[0] = DEFAULT_RED;
    if (obj->color[1] == DEFAULT_OBJECT_COLOR) obj->color[1] = DEFAULT_GRN;
    if (obj->color[2] == DEFAULT_OBJECT_COLOR) obj->color[2] = DEFAULT_BLU;

    obj->num_children = 0;

    if (xfighter_dl == -1) {
    	create_xfighter_graphics(obj);
	xfighter_dl = obj->display_list;
	shadow_dl = create_shadow_dl();
    }
    else {
	obj->display_list = xfighter_dl;
    }

    if ((obj->vehicle_auxdata = init_vaux(obj)) == NULL) {
	return;
    }
    init_pobj(obj,2000.0,75.0);
    obj->upd = (UpdateDisp_ptr)malloc( sizeof(UpdateDisp) );

    obj->upd->invis_words = 1;
    /* These are things you want to draw */
    obj->upd->color[0] = obj->color[0];
    obj->upd->color[1] = obj->color[1];
    obj->upd->color[2] = obj->color[2];
    obj->upd->invis[0] = LIGHTS_OFF_BRAKES_ON|BACKUP_LIGHTS_OFF|WHEELS_CENTER;

    obj->update_self = apply_xfighter_physics;

    obj->bound_mc[0] = XS(-4.5);
    obj->bound_mc[1] = YS(-4.5);
    obj->bound_mc[2] = ZS(-7.0);
    obj->bound_mc[3] = XS( 4.5);
    obj->bound_mc[4] = YS( 4.5);
    obj->bound_mc[5] = ZS( 7.0);

    update_wc_bounds(obj);

    /* apply brake */
    obj->controls.pointer_x = 0.0;
    obj->controls.pointer_y = -1.0;

    obj->surface_chars_xyz  = xfighter_surface_chars_xyz;
    obj->surface_chars_bbox = xfighter_surface_chars_bbox;

    /* Create shadow child */
    obj->num_children = 1;
    if ((child = (DRIVE_OBJECT *) malloc(sizeof(DRIVE_OBJECT))) == NULL) {
	fprintf(stderr,"Out of malloc space!\n");
	return;
    }
    /* First clone myself */
    memcpy(child,obj,sizeof(DRIVE_OBJECT));
    child->idptr = &(object_id[SHADOW_ID_NUMBER]);
    child->num_children = 0;
    child->child_list = NULL;
    child->display_list = shadow_dl;
    /* Use same intersection routines. */
    /* I have no physics or controls */
    child->update_controls = NULL;
    child->pobj = NULL;
    child->vehicle_auxdata = NULL;
    child->aeroplane = NULL;
    child->update_self = NULL;
    child->upd = NULL;
    child->connection = 0;
    add_object_to_list(&(obj->child_list),child);

    obj->aim_xform = &(obj->xform);
    obj->fire_point[0] = 0.0;
    obj->fire_point[1] = 2.0;
    obj->fire_point[2] = ZS(7.5);
}
