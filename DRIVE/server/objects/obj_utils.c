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


/* Code Module for object pieces. */


#include "object.h"
#include "obj_common.h"

/* This routine creates a 3- or 4-sided post with faces orthogonal to the XYZ
 * planes.
 * 
 * Vertex format on entry to this routine should be 0,0,0,0,COUNTER_CLOCKWISE.
 *
 * x,y,z = post bottom
 * h     = post height
 * r     = post radius
 */
hwObject post(
    float x, float y, float z,
    float h, float r,
    int do_top,
    int num_sides)
{
    float postmesh[5][2][3];
    float top[4][3];
    int i, nObjs = 0;
    hwObject curr, children[2];
    extern int objPrinting;

    i = 0;
    postmesh[i][0][0]=x+r/2.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z-r/2.0;
    postmesh[i][1][0]=x+r/2.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z-r/2.0;
    ++i;

    postmesh[i][0][0]=x-r/2.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z-r/2.0;
    postmesh[i][1][0]=x-r/2.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z-r/2.0;
    ++i;

    if (num_sides == 4) {
	postmesh[i][0][0]=x-r/2.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z+r/2.0;
	postmesh[i][1][0]=x-r/2.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z+r/2.0;
	++i;

	postmesh[i][0][0]=x+r/2.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z+r/2.0;
	postmesh[i][1][0]=x+r/2.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z+r/2.0;
	++i;
    }
    else {
	postmesh[i][0][0]=0.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z+r/2.0;
	postmesh[i][1][0]=0.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z+r/2.0;
	++i;
    }

    postmesh[i][0][0]=x+r/2.0; postmesh[i][0][1]=y;   postmesh[i][0][2]=z-r/2.0;
    postmesh[i][1][0]=x+r/2.0; postmesh[i][1][1]=y+h; postmesh[i][1][2]=z-r/2.0;

    if( objPrinting ) {
	quadrilateral_mesh( 0, (float *) postmesh, num_sides+1, 2, FALSE );
    }
    else {
	curr = hwMesh->create( hwMesh );
	HW_MODIFY_1I( curr, hwStrGraphN, num_sides+1 );
	HW_MODIFY_1I( curr, hwStrGraphM, 2 );
	curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,(num_sides+1)*2*3), postmesh );
	children[nObjs++] = curr;
    }

    if (do_top) {
	i = 0;
	top[i][0] = x - r/2.0; top[i][1] = y+h; top[i][2] = z - r/2.0;  ++i;
	top[i][0] = x + r/2.0; top[i][1] = y+h; top[i][2] = z - r/2.0;  ++i;
	if (num_sides == 4) {
	    top[i][0] = x + r/2.0; top[i][1] = y+h; top[i][2] = z + r/2.0;  ++i;
	    top[i][0] = x - r/2.0; top[i][1] = y+h; top[i][2] = z + r/2.0;  ++i;
	}
	else {
	    top[i][0] = 0.0; top[i][1] = y+h; top[i][2] = z + r/2.0;  ++i;
	}
	if( objPrinting ) {
	    polygon3d( 0, (float *) top, num_sides, FALSE );
	}
	else {
	    curr = hwPolygon->create( hwPolygon );
	    curr->modify( curr, hwStrData,
			HW_MAKE_TYPE(HW_TYPE_FLOAT,num_sides*3), top );
	    children[nObjs++] = curr;
	}
    }
    if( objPrinting ) {
	curr = 0;
    }
    else {
	curr = hwGroup->create( hwGroup );
	curr->modify( curr, hwStrChildren, HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs),
			children );
    }
    return curr;
}


int checkbounds_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc,
    int (*xyz_routine)(
	DRIVE_OBJECT *r_obj,
	float x, float y, float z,
	MC_SURFACE_CHARACTERISTICS *r_sc))
{
    int good = FALSE;
    MC_SURFACE_CHARACTERISTICS local_sc;
    static int pt_x[8] = {0,0,0,0,3,3,3,3};
    static int pt_y[8] = {1,1,4,4,1,1,4,4};
    static int pt_z[8] = {2,5,2,5,2,5,2,5};
    int i;


    /* Check each point of the MC bounding box. */
    /* Find the point at the maximum Y */

    /* Set defaults */
    sc->mc_y = -(1.0e+30);
    local_sc.mc_normal[0] = 0.0;
    local_sc.mc_normal[1] = 1.0;
    local_sc.mc_normal[2] = 0.0;
    local_sc.friction  = 1.0;
    local_sc.roughness = 0.0;

    for (i=0; i<8; ++i) {
	local_sc.mc_x = bbox_mc[pt_x[i]];
	local_sc.mc_y = bbox_mc[pt_y[i]];
	local_sc.mc_z = bbox_mc[pt_z[i]];
	if ((*xyz_routine)(obj,local_sc.mc_x,local_sc.mc_y,local_sc.mc_z,
		&local_sc)) {
	    if (local_sc.mc_y > sc->mc_y) {
		memcpy(sc,&local_sc,sizeof(MC_SURFACE_CHARACTERISTICS));
		good = TRUE;
	    }
	    /* Reset defaults */
	    local_sc.mc_normal[0] = 0.0;
	    local_sc.mc_normal[1] = 1.0;
	    local_sc.mc_normal[2] = 0.0;
	    local_sc.friction  = 1.0;
	    local_sc.roughness = 0.0;
	}
    }

    return(good);
}


/* For box-like objects, this returns the MC normal at the MC point x,y,z.
 * It does so by seeing which wall of the object the point is closest to,
 * returning the normal of that wall.  Assume that objects are always
 * sitting on something so there's no need to check for bottom intersection.
 */
void get_box_mc_normal(
    DRIVE_OBJECT *obj,
    float mcx, float mcy, float mcz,
    float mc_normal[3])
{
    float COG_mc[3];
    float dx,dy,dz;
    int cx,cy,cz;

    COG_mc[0] = (obj->bound_mc[3] + obj->bound_mc[0]) / 2.0;
    COG_mc[1] = (obj->bound_mc[4] + obj->bound_mc[1]) / 2.0;
    COG_mc[2] = (obj->bound_mc[5] + obj->bound_mc[2]) / 2.0;

    /* Check to see if it's really near one of the corners/edge.
     * If so, there's really no good solution.  Perhaps the best
     * compromise is to return a normal pointing directly out
     * of the corner/edge.
     */
    dx = (obj->bound_mc[3] - obj->bound_mc[0]) / 20.0;
    dy = (obj->bound_mc[4] - obj->bound_mc[1]) / 20.0;
    dz = (obj->bound_mc[5] - obj->bound_mc[2]) / 20.0;

    cx = cy = cz = -1;
    if      (ABS(mcx - obj->bound_mc[0]) < dx) cx = 0;
    else if (ABS(mcx - obj->bound_mc[3]) < dx) cx = 3;

    if      (ABS(mcy - obj->bound_mc[1]) < dy) cy = 1;
    else if (ABS(mcy - obj->bound_mc[4]) < dy) cy = 4;

    if      (ABS(mcz - obj->bound_mc[2]) < dz) cz = 2;
    else if (ABS(mcz - obj->bound_mc[5]) < dz) cz = 5;

    /* If near all three, it's a corner case.  :-) */
    if ((cx != -1) && (cy != -1) && (cz != -1)) {
	mc_normal[0] = obj->bound_mc[cx] - COG_mc[0];
	mc_normal[1] = obj->bound_mc[cy] - COG_mc[1];
	mc_normal[2] = obj->bound_mc[cz] - COG_mc[2];
	NORMALIZE3(mc_normal[0],mc_normal[1],mc_normal[2]);
	return;
    }
    /* If near two, it's an edge case. */
    else if ((cx != -1) && (cz != -1)) {
	mc_normal[0] = obj->bound_mc[cx] - COG_mc[0];
	mc_normal[1] = 0.0;
	mc_normal[2] = obj->bound_mc[cz] - COG_mc[2];
	NORMALIZE2(mc_normal[0],mc_normal[2]);
	return;
    }
    else if ((cx != -1) && (cy != -1)) {
	mc_normal[0] = obj->bound_mc[cx] - COG_mc[0];
	mc_normal[1] = obj->bound_mc[cy] - COG_mc[1];
	mc_normal[2] = 0.0;
	NORMALIZE2(mc_normal[0],mc_normal[1]);
	return;
    }
    else if ((cy != -1) && (cz != -1)) {
	mc_normal[0] = 0.0;
	mc_normal[1] = obj->bound_mc[cy] - COG_mc[1];
	mc_normal[2] = obj->bound_mc[cz] - COG_mc[2];
	NORMALIZE2(mc_normal[1],mc_normal[2]);
	return;
    }


    /* Else it's clearly near one face exclusively */
    if (ABS((mcz - COG_mc[2]) * (obj->bound_mc[3] - COG_mc[0]))
	    < ABS((mcx - COG_mc[0]) * (obj->bound_mc[5] - COG_mc[2]))) {
	if (mcx - COG_mc[0] > 0.0) {
	    /* +X MC axis */
	    mc_normal[0] = 1.0;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = 0.0;
	}
	else {
	    /* -X MC axis */
	    mc_normal[0] = -1.0;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = 0.0;
	}
    }
    else {
	if (mcz - COG_mc[2] > 0.0) {
	    /* Z MC axis */
	    mc_normal[0] = 0.0;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = 1.0;
	}
	else {
	    /* -Z MC axis */
	    mc_normal[0] = 0.0;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = -1.0;
	}
    }

    /* Now check for top */
    if ((mcy - COG_mc[1] > 0.0)
	    && (ABS((mcy - COG_mc[1]) * (obj->bound_mc[3] - COG_mc[0]))
		> ABS((mcx - COG_mc[0]) * (obj->bound_mc[4] - COG_mc[1])))
	    && (ABS((mcy - COG_mc[1]) * (obj->bound_mc[5] - COG_mc[2]))
		> ABS((mcz - COG_mc[2]) * (obj->bound_mc[4] - COG_mc[1])))) {
	/* Y MC axis */
	mc_normal[0] = 0.0;
	mc_normal[1] = 1.0;
	mc_normal[2] = 0.0;
    }
}


#define SQ(x) ((x) * (x))

/* For tall skinny objects, this returns the MC normal at the MC point x,y,z.
 * It assumes the answer is not "down", and it's not "up" unless the point
 * is close to the top center.
 */
void get_pole_mc_normal(
    DRIVE_OBJECT *obj,
    float mcx, float mcy, float mcz,
    float mc_normal[3])
{
    float size = mcx*mcx + mcz*mcz;

    /* If near center top, return "up". */
    if ((mcy > obj->bound_mc[4] - 0.25)
	    && (size < 0.1*(SQ(obj->bound_mc[3] - obj->bound_mc[0]) +
		            SQ(obj->bound_mc[5] - obj->bound_mc[2])))) {
	/* Y MC axis */
	mc_normal[0] = 0.0;
	mc_normal[1] = 1.0;
	mc_normal[2] = 0.0;
    }
    else {
	/* Check if it's real near the center. */
	if (size < 1.0e-10) {
	    /* Pick something arbitrary. */
	    mc_normal[0] = 1.0;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = 0.0;
	}
	else {
	    /* Just normalize the mcx,mcz vector */
	    size = FSQRT(size);
	    mc_normal[0] = mcx/size;
	    mc_normal[1] = 0.0;
	    mc_normal[2] = mcz/size;
	}
    }
}


void random_ground_rgb(
    float *r, float *g, float *b)
{
    float red,grn,blu,lum,curlum,factor;

    red = BOUNDED_FLOATRAND(GROUND_R_MIN,GROUND_R_MAX);
    grn = BOUNDED_FLOATRAND(GROUND_G_MIN,GROUND_G_MAX);
    blu = BOUNDED_FLOATRAND(GROUND_B_MIN,GROUND_B_MAX);

    if ((curlum = 30*red + 59*grn + 11*blu) > (1.0e-5)) {
	lum = GROUND_MIN_LUMINOSITY
	    + BOUNDED_FLOATRAND(GROUND_MIN_LUMINOSITY,GROUND_MAX_LUMINOSITY);
	factor = lum / curlum;
	red *= factor;
	grn *= factor;
	blu *= factor;
    }

    *r = red;
    *g = grn;
    *b = blu;
}

void do_mesh_colors(
    float mesh[GROUND_MESH_SIZE][GROUND_MESH_SIZE][6])
{
    /* Colors must match at corners and on edges. */
    long lrand_restore;
    float r,g,b;
    int i,j, rgbOffs = 3;


#if defined(HPUX_SOURCE)
    lrand_restore = lrand48();
#endif
    seedrand(GROUND_RANDSEED);

    /* The corners must match */
    random_ground_rgb(&r,&g,&b);
    mesh[0][0][rgbOffs+0] =
	mesh[GROUND_MESH_SIZE-1][0][rgbOffs+0] =
	mesh[0][GROUND_MESH_SIZE-1][rgbOffs+0] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][rgbOffs+0] =
	r;
    mesh[0][0][rgbOffs+1] =
	mesh[GROUND_MESH_SIZE-1][0][rgbOffs+1] =
	mesh[0][GROUND_MESH_SIZE-1][rgbOffs+1] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][rgbOffs+1] =
	g;
    mesh[0][0][rgbOffs+2] =
	mesh[GROUND_MESH_SIZE-1][0][rgbOffs+2] =
	mesh[0][GROUND_MESH_SIZE-1][rgbOffs+2] =
	mesh[GROUND_MESH_SIZE-1][GROUND_MESH_SIZE-1][rgbOffs+2] =
	b;

    /* And the edges must match */
    for (j=1; j<(GROUND_MESH_SIZE-1); ++j) {
	random_ground_rgb(&r,&g,&b);
	mesh[0][j][rgbOffs+0] =
	    mesh[GROUND_MESH_SIZE-1][j][rgbOffs+0] =
	    mesh[j][0][rgbOffs+0] =
	    mesh[j][GROUND_MESH_SIZE-1][rgbOffs+0] =
	    r;
	mesh[0][j][rgbOffs+1] =
	    mesh[GROUND_MESH_SIZE-1][j][rgbOffs+1] =
	    mesh[j][0][rgbOffs+1] =
	    mesh[j][GROUND_MESH_SIZE-1][rgbOffs+1] =
	    g;
	mesh[0][j][rgbOffs+2] =
	    mesh[GROUND_MESH_SIZE-1][j][rgbOffs+2] =
	    mesh[j][0][rgbOffs+2] =
	    mesh[j][GROUND_MESH_SIZE-1][rgbOffs+2] =
	    b;
    }

    /* Go back to a real random number. */
    seedrand(lrand_restore);

    /* Now the interior */
    for (i=1; i < (GROUND_MESH_SIZE-1); ++i) {
	for (j=1; j < (GROUND_MESH_SIZE-1); ++j) {
	    random_ground_rgb(&(mesh[i][j][rgbOffs+0]),
		&(mesh[i][j][rgbOffs+1]),&(mesh[i][j][rgbOffs+2]));
	}
    }
}


#if 0
void self_lit_on(
    int fildes)
{
    /* shade_mode(fildes,CMAP_FULL,FALSE); */
    surface_coefficients(fildes,1000.0,0.0,0.0);
}


void self_lit_off(
    int fildes)
{
    surface_coefficients(fildes,1.0,1.0,1.0);
    /* shade_mode(fildes,CMAP_FULL,TRUE); */
}
#endif


float hill_y_wc(
    DRIVE_OBJECT *hill,
    float x, float z)	/* WCs */
{
    float xmc,ymc,zmc;
    float xwc,ywc,zwc;
    MC_SURFACE_CHARACTERISTICS sc;

    wc_to_mc(hill, x,40000.0,z, &xmc,&ymc,&zmc);
    (*hill->surface_chars_xyz)(hill, xmc,ymc,zmc, &sc);
    mc_to_wc(hill, sc.mc_x,sc.mc_y,sc.mc_z, &xwc,&ywc,&zwc);
    return(ywc);
}


float hill_y_mc(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill,
    float x, float z)	/* MCs of "obj" */
{
    float xmc,ymc,zmc;
    float xwc,ywc,zwc;
    MC_SURFACE_CHARACTERISTICS sc;

    mc_to_wc(obj,  x,100000.0,z, &xwc,&ywc,&zwc);
    wc_to_mc(hill, xwc,ywc,zwc, &xmc,&ymc,&zmc);
    (*hill->surface_chars_xyz)(hill, xmc,ymc,zmc, &sc);
    mc_to_wc(hill, sc.mc_x,sc.mc_y,sc.mc_z, &xwc,&ywc,&zwc);
    wc_to_mc(hill, xwc,ywc,zwc, &xmc,&ymc,&zmc);
    return(ymc);
}


DRIVE_OBJECT *find_hill_in_scene(
    SCENE *scene)
{
    DRIVE_OBJECT *obj;

    if (scene == NULL) return(NULL);

    obj = scene->object_head;
    while (obj != NULL) {
	if (strcmp(obj->idptr->lcname,"hill") == 0) break;
	obj = obj->next;
    }

    /* Will return NULL if no hill */
    return(obj);
}


void elevate_object_to_terrain_height(
    SCENE *scene,
    DRIVE_OBJECT *obj,
    boolean_type do_xz_bounds)
{
    DRIVE_OBJECT *hill;
    float x,y,z;
    float xwc,ywc,zwc;
    float ymax,ymin;
    static float elemat[4][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 9.9, 0.0, 1.0 }
    };

    if ((hill = find_hill_in_scene(scene)) == NULL) return;

    if (do_xz_bounds) {
	mc_to_wc(obj, obj->bound_mc[0],100000.0,obj->bound_mc[2],
	    &xwc,&ywc,&zwc);
	ymax = (ymin = hill_y_wc(hill,xwc,zwc));

	mc_to_wc(obj, obj->bound_mc[0],100000.0,obj->bound_mc[5],
	    &xwc,&ywc,&zwc);
	if ((y = hill_y_wc(hill,xwc,zwc)) < ymin) ymin = y;
	else if (y > ymax) ymax = y;

	mc_to_wc(obj, obj->bound_mc[3],100000.0,obj->bound_mc[5],
	    &xwc,&ywc,&zwc);
	if ((y = hill_y_wc(hill,xwc,zwc)) < ymin) ymin = y;
	else if (y > ymax) ymax = y;

	mc_to_wc(obj, obj->bound_mc[3],100000.0,obj->bound_mc[2],
	    &xwc,&ywc,&zwc);
	if ((y = hill_y_wc(hill,xwc,zwc)) < ymin) ymin = y;
	else if (y > ymax) ymax = y;
    }
    else {
	x = (obj->bound_mc[0] + obj->bound_mc[3])/2.0;
	z = (obj->bound_mc[0] + obj->bound_mc[3])/2.0;
	mc_to_wc(obj, x,100000.0,z, &xwc,&ywc,&zwc);
	ymax = (ymin = hill_y_wc(hill,xwc,zwc));
    }

    if (obj->bound_wc[1] >= ymin-0.1) {
	/* already above the level of the hill */
	return;
    }

    if (ymax > 0.5) {
	elemat[3][1] = ymax;
	concat_matrix(obj->xform,elemat,obj->xform);
	elemat[3][1] = -ymax;
	concat_matrix(elemat,obj->ixform,obj->ixform);
	update_wc_bounds(obj);
    }

    if (ymax-ymin > 0.5) {
	/* put a wall underneath the object to fill the gap */
	add_object_to_scene(scene,"fillwall","","",0.0,
	    (obj->bound_wc[0]+obj->bound_wc[3])/2.0 - scene->xscene*SCENE_SIZE,
		ymin,
		obj->bound_wc[2] - scene->zscene*SCENE_SIZE,
	    obj->bound_wc[5] - obj->bound_wc[2],
		obj->bound_wc[3] - obj->bound_wc[0],
		ymax-ymin,
	    0.0,0.0,0.0,0);
    }
}



void init_pobj(
    DRIVE_OBJECT *obj,
    float pounds,
    float torquemult)
{
    PHYSICAL_OBJECT *pobj;
    VEHICLE_AUXDATA *vaux = obj->vehicle_auxdata;
    float length,width,height;


    /***** INIT PHYSICAL OBJECT ****/
    length = ABS(vaux->bbox_mc[PT_UFR][2] - vaux->bbox_mc[PT_UBR][2]);
    width  = ABS(vaux->bbox_mc[PT_UFR][0] - vaux->bbox_mc[PT_UFL][0]);
    height = ABS(vaux->bbox_mc[PT_TFL][1] - vaux->bbox_mc[PT_UFL][1]);

    pobj = (obj->pobj = allocate_physical_object());
    pobj->mass = POUNDS_TO_SLUGS(pounds);

    /* Standard inertial tensor matrix for a rectagular box */
    pobj->I[XD][XD] = pobj->mass
        * (height*height + length*length) / 12.0;
    pobj->I[YD][YD] = pobj->mass
        * (length*length + width*width)   / 12.0;
    pobj->I[ZD][ZD] = pobj->mass
        * (width*width   + height*height) / 12.0;
    pobj->I[XD][YD] = pobj->I[XD][ZD] = pobj->I[YD][ZD] = 0.0;
    pobj->I[YD][XD] = pobj->I[ZD][XD] = pobj->I[ZD][YD] = 0.0;

    pobj->Iinv[XD][XD] = 1.0/pobj->I[XD][XD];
    pobj->Iinv[YD][YD] = 1.0/pobj->I[YD][YD];
    pobj->Iinv[ZD][ZD] = 1.0/pobj->I[ZD][ZD];
    pobj->Iinv[XD][YD] = pobj->Iinv[XD][ZD] = pobj->Iinv[YD][ZD] = 0.0;
    pobj->Iinv[YD][XD] = pobj->Iinv[ZD][XD] = pobj->Iinv[ZD][YD] = 0.0;

    pobj->x[XD] = obj->xform[3][0];
    pobj->x[YD] = obj->xform[3][1];
    pobj->x[ZD] = obj->xform[3][2];

    {   float yangle;
	yangle = FATAN2(obj->xform[2][2],obj->xform[2][0]) - (M_PI/2.0);
	reset_rotation_matrices(obj,yangle);
    }

    ZERO_TRIVECTOR(pobj->p);
    ZERO_TRIVECTOR(pobj->w);
    ZERO_TRIVECTOR(pobj->v);
    ZERO_TRIVECTOR(pobj->L);

    /* Update wheel_torque_mult, now that we know I[YD][YD] */
    vaux->wheel_torque_mult = torquemult * pobj->I[YD][YD];
}
