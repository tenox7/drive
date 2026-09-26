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


/* Code Module for the hill crossroad segment object */


#include <stdio.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"
#include "scene.h"

#define DELTA			(SCENE_SIZE/(GROUND_MESH_SIZE-1))
#define DEFAULT_ROAD_LENGTH	DELTA
#define DEFAULT_ROAD_HEIGHT	0.0
#define EPSILON 		(1e-3)

#define Length				(obj->size[SIZE_LENGTH])
#define Width				(obj->size[SIZE_WIDTH])

typedef struct {
    float yhmaxa,yhmaxb;
    float yvmaxa,yvmaxb;
    float ycmax;
} HILLXROAD_INFO;


static int hillxroad_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    int on_h,on_v;
    float xc,zc,t;
    HILLXROAD_INFO *info = (HILLXROAD_INFO *) (obj->additional_data);

    on_h = (ABS(z) < (Width/2.0));
    on_v = (ABS(x) < (Width/2.0));

    if (!on_h && !on_v) return(FALSE);
    /* else */

    sc->friction = 1.0;
    sc->roughness = 0.0;
    sc->roughness_frequency = 0.0;
    if (on_h && on_v) {
	sc->mc_y = info->ycmax + ROAD_FLOAT;
	return(TRUE);
    }
    /* else */

    if (on_h) {
	if (x < 0.0) {
	    xc = (-Width/2.0);
	    t = (xc - x) / (xc + (Length/2.0));
	    sc->mc_y = (1.0-t)*info->ycmax + t*info->yhmaxa;
	}
	else {
	    xc = (Width/2.0);
	    t = (x - xc) / ((Length/2.0) - xc);
	    sc->mc_y = (1.0-t)*info->ycmax + t*info->yhmaxb;
	}
    }
    else {
	if (z < 0.0) {
	    zc = (-Width/2.0);
	    t = (zc - z) / (zc + (Length/2.0));
	    sc->mc_y = (1.0-t)*info->ycmax + t*info->yvmaxa;
	}
	else {
	    zc = (Width/2.0);
	    t = (z - zc) / ((Length/2.0) - zc);
	    sc->mc_y = (1.0-t)*info->ycmax + t*info->yvmaxb;
	}
    }
    sc->mc_y += ROAD_FLOAT;

    return(TRUE);
}


static float draw_hill_crossroads(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill)
{
    float xc1,xc2,xc3,xc4,yc1,yc2,yc3,yc4,zc1,zc2,zc3,zc4,ycmax;
    float xh1,xh2,xh3,xh4,yh1,yh2,yh3,yh4,zh1,zh2,zh3,zh4,yhmaxa,yhmaxb;
    float xv1,xv2,xv3,xv4,yv1,yv2,yv3,yv4,zv1,zv2,zv3,zv4,yvmaxa,yvmaxb;
    float mesh[16][3],*fptr;
    HILLXROAD_INFO *info;
    hwObject curr, oList[10];
    int nObjs = 0;


    /* Crossroads intersection area */
    xc1 = -Width/2.0;   zc1 = -Width/2.0;  yc1 = hill_y_mc(obj,hill,xc1,zc1);
    xc2 = -Width/2.0;   zc2 =  Width/2.0;  yc2 = hill_y_mc(obj,hill,xc2,zc2);
    xc3 =  Width/2.0;   zc3 =  Width/2.0;  yc3 = hill_y_mc(obj,hill,xc3,zc3);
    xc4 =  Width/2.0;   zc4 = -Width/2.0;  yc4 = hill_y_mc(obj,hill,xc4,zc4);
    ycmax = yc1;
    if (yc2 > ycmax) ycmax = yc2;
    if (yc3 > ycmax) ycmax = yc3;
    if (yc4 > ycmax) ycmax = yc4;
    ycmax += ROAD_FLOAT;

    /* X-parallel road */
    xh1 = -Length/2.0;  zh1 = -Width/2.0;  yh1 = hill_y_mc(obj,hill,xh1,zh1);
    xh2 = -Length/2.0;  zh2 =  Width/2.0;  yh2 = hill_y_mc(obj,hill,xh2,zh2);
    xh3 =  Length/2.0;  zh3 =  Width/2.0;  yh3 = hill_y_mc(obj,hill,xh3,zh3);
    xh4 =  Length/2.0;  zh4 = -Width/2.0;  yh4 = hill_y_mc(obj,hill,xh4,zh4);
    yhmaxa = MAX(yh1,yh2) + ROAD_FLOAT;
    yhmaxb = MAX(yh3,yh4) + ROAD_FLOAT;

    /* Z-parallel road */
    xv1 = -Width/2.0;  zv1 = -Length/2.0;  yv1 = hill_y_mc(obj,hill,xv1,zv1);
    xv2 = -Width/2.0;  zv2 =  Length/2.0;  yv2 = hill_y_mc(obj,hill,xv2,zv2);
    xv3 =  Width/2.0;  zv3 =  Length/2.0;  yv3 = hill_y_mc(obj,hill,xv3,zv3);
    xv4 =  Width/2.0;  zv4 = -Length/2.0;  yv4 = hill_y_mc(obj,hill,xv4,zv4);
    yvmaxa = MAX(yv1,yv4) + ROAD_FLOAT;
    yvmaxb = MAX(yv2,yv3) + ROAD_FLOAT;

    fptr = (float *) mesh;
    *fptr++ = xv4; *fptr++ = yv4; *fptr++ = zv4;
    *fptr++ = xc4; *fptr++ = yc4; *fptr++ = zc4;
    *fptr++ = xc3; *fptr++ = yc3; *fptr++ = zc3;
    *fptr++ = xv3; *fptr++ = yv3; *fptr++ = zv3;

    *fptr++ = xv4; *fptr++ = yvmaxa; *fptr++ = zv4;
    *fptr++ = xc4; *fptr++ = ycmax;  *fptr++ = zc4;
    *fptr++ = xc3; *fptr++ = ycmax;  *fptr++ = zc3;
    *fptr++ = xv3; *fptr++ = yvmaxb; *fptr++ = zv3;

    *fptr++ = xv1; *fptr++ = yvmaxa; *fptr++ = zv1;
    *fptr++ = xc1; *fptr++ = ycmax;  *fptr++ = zc1;
    *fptr++ = xc2; *fptr++ = ycmax;  *fptr++ = zc2;
    *fptr++ = xv2; *fptr++ = yvmaxb; *fptr++ = zv2;

    *fptr++ = xv1; *fptr++ = yv1; *fptr++ = zv1;
    *fptr++ = xc1; *fptr++ = yc1; *fptr++ = zc1;
    *fptr++ = xc2; *fptr++ = yc2; *fptr++ = zc2;
    *fptr++ = xv2; *fptr++ = yv2; *fptr++ = zv2;
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 4 );
    HW_MODIFY_1I( curr, hwStrGraphM, 4 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*4*3), mesh );
    oList[nObjs++] = curr;

    fptr = (float *) mesh;
    *fptr++ = xh1; *fptr++ = yh1; *fptr++ = zh1;
    *fptr++ = xc1; *fptr++ = yc1; *fptr++ = zc1;

    *fptr++ = xh1; *fptr++ = yhmaxa; *fptr++ = zh1;
    *fptr++ = xc1; *fptr++ = ycmax;  *fptr++ = zc1;

    *fptr++ = xh2; *fptr++ = yhmaxa; *fptr++ = zh2;
    *fptr++ = xc2; *fptr++ = ycmax;  *fptr++ = zc2;

    *fptr++ = xh2; *fptr++ = yh2; *fptr++ = zh2;
    *fptr++ = xc2; *fptr++ = yc2; *fptr++ = zc2;
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 4 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*2*3), mesh );
    oList[nObjs++] = curr;

    fptr = (float *) mesh;
    *fptr++ = xc4; *fptr++ = yc4; *fptr++ = zc4;
    *fptr++ = xh4; *fptr++ = yh4; *fptr++ = zh4;

    *fptr++ = xc4; *fptr++ = ycmax;  *fptr++ = zc4;
    *fptr++ = xh4; *fptr++ = yhmaxb; *fptr++ = zh4;

    *fptr++ = xc3; *fptr++ = ycmax;  *fptr++ = zc3;
    *fptr++ = xh3; *fptr++ = yhmaxb; *fptr++ = zh3;

    *fptr++ = xc3; *fptr++ = yc3; *fptr++ = zc3;
    *fptr++ = xh3; *fptr++ = yh3; *fptr++ = zh3;
    curr = hwMesh->create( hwMesh );
    HW_MODIFY_1I( curr, hwStrGraphN, 4 );
    HW_MODIFY_1I( curr, hwStrGraphM, 2 );
    curr->modify( curr, hwStrData,
		HW_MAKE_TYPE(HW_TYPE_FLOAT,4*2*3), mesh );
    oList[nObjs++] = curr;

    if ((info = (HILLXROAD_INFO *) malloc(sizeof(HILLXROAD_INFO))) == NULL) {
	fprintf(stderr,"Out of malloc space.\n");
	exit(1);
    }
    obj->additional_data = (void *) info;

    info->yhmaxa = yhmaxa; info->yhmaxb = yhmaxb;
    info->yvmaxa = yvmaxa; info->yvmaxb = yvmaxb;
    info->ycmax  = ycmax;

    /* Return the highest y */
    if (yhmaxa > ycmax) ycmax = yhmaxa;
    if (yhmaxb > ycmax) ycmax = yhmaxb;
    if (yvmaxa > ycmax) ycmax = yvmaxa;
    if (yvmaxb > ycmax) ycmax = yvmaxb;

    curr = hwGroup->create( hwGroup );
    curr->modify( curr, hwStrChildren,
		HW_MAKE_TYPE(HW_TYPE_OBJECT,nObjs), oList );
    HW_OBJECT_NAMESET(curr,obj);
    ASPHALT_HW(curr);

    obj->display_list = createHwSegmentFromObj( &curr, 1 );

    return ycmax;
}


static float create_hillxroad_graphics(
    DRIVE_OBJECT *obj,
    DRIVE_OBJECT *hill)
{
    float ymax;

    ymax = draw_hill_crossroads(obj,hill);

    return(ymax);
}


void init_hillxroad_object(
    DRIVE_OBJECT *obj)
{
    DRIVE_OBJECT *hill;
    float ymax;

    if (Length <= 0.0) Length = DEFAULT_ROAD_LENGTH;
    if (Width <= 0.0)  Width  = DEFAULT_ROAD_WIDTH;

    if ((hill = find_hill_in_scene((SCENE *) (obj->scene))) == NULL) {
	return;
    }

    ymax = create_hillxroad_graphics(obj,hill);

    obj->num_children = 0;
    obj->surface_chars_xyz  = hillxroad_surface_chars_xyz;

    /* Initialize bounding box values */
    obj->bound_mc[0] = -Length/2.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = -Length/2.0;
    obj->bound_mc[3] = Length/2.0;
    obj->bound_mc[4] = ymax + BBOX_MARGIN;
    obj->bound_mc[5] = Length/2.0;

    update_wc_bounds(obj);
}
