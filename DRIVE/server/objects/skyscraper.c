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


/* Code Module for the skyscraper segment object */


#include <stdio.h>
#include <math.h>
#include "object.h"
#include "physics.h"
#include "libnum.h"
#include "obj_common.h"


#define DEFAULT_WIDTH (50.0)
#define DEFAULT_LENGTH (50.0)
#define DEFAULT_HEIGHT (150.0)

#define STORY_HEIGHT 15.0
#define FIRST_GLASS 2
#define DOORS 2
#define H_STRIP 2.0
#define V_STRIP 0.5
#define V_SPACING 10.0

typedef struct _skyscraper_list {
    float length,width,height;
    int stories;
    unsigned int nameset_bits;
    int dl_number;
    struct _skyscraper_list *next;
} SKYSCRAPER_LIST;

static SKYSCRAPER_LIST *skyscraper_list=NULL;


static int skyscraper_surface_chars_xyz(
    DRIVE_OBJECT *obj,
    float x, float y, float z,
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    get_box_mc_normal(obj,x,y,z,sc->mc_normal);
    return(TRUE);
}


static int skyscraper_surface_chars_bbox(
    DRIVE_OBJECT *obj,
    float bbox_mc[6],
    MC_SURFACE_CHARACTERISTICS *sc)
{
    sc->mc_y = obj->size[SIZE_HEIGHT];
    return(TRUE);
}


static hwObject high_res_model(
    int stories,
    float length, float width, float height,
    float gr, float gg, float gb,
    float sr, float sg, float sb)
{
    float *ptr,x,y,z,top;
    int i;
    int fg;
    float bg,delta;
    float pts[4*2000];
    int numpts;
    int
	this;
    hwObject
	skyGroup,
	objList[4];


    DIFFUSE_LIGHTING_ON(img_fildes);
    /* Note that if FIRST_GLASS
     * is <= 2, it is set to 2 (the first story is not glass ) 
     */
    if (FIRST_GLASS < 2) {
      bg = STORY_HEIGHT;
      fg = 2;
    }else{
      bg = (FIRST_GLASS - 1) * STORY_HEIGHT;
      fg = FIRST_GLASS;
    }

    top = STORY_HEIGHT * stories;
    /* vertex_format(img_fildes,0,0,0,0,CLOCKWISE); */

    this = 0;

    /* First, fill in all the glass */
    if (top > bg) {
	objList[this] = hwMesh->create(hwMesh);
	objList[this]->name = 0;
	HW_MODIFY_3F(objList[this],hwStrColor, gr,gg,gb);

	/* first row of quad mesh */
	ptr = pts;
	delta = 1.0;
	*ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = delta;
	*ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = width - delta;
	*ptr++ = length - delta; *ptr++ = bg - 1.0; *ptr++ = width - delta;
	*ptr++ = length - delta; *ptr++ = bg - 1.0; *ptr++ = delta;
	*ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = delta;

	/* second row of quad mesh */
	*ptr++ = delta; *ptr++ = top; *ptr++ = delta;
	*ptr++ = delta; *ptr++ = top; *ptr++ = width - delta;
	*ptr++ = length - delta; *ptr++ = top; *ptr++ = width - delta;
	*ptr++ = length - delta; *ptr++ = top; *ptr++ = delta;
	*ptr++ = delta; *ptr++ = top; *ptr++ = delta;

	HW_MODIFY_1I(objList[this], hwStrGraphN, 2);
	HW_MODIFY_1I(objList[this], hwStrGraphM, 5);
	HW_MODIFY_1B(objList[this], hwStrBackface, HW_TRUE);
	objList[this]->modify(objList[this],hwStrData, 
	    HW_MAKE_TYPE(HW_TYPE_FLOAT, 2*5*3),
	    pts);
	this++;
    }

    /* Now, draw the stories that aren't all glass.  */
    /* fill_color(img_fildes,sr,sg,sb);
    line_color(img_fildes,sr,sg,sb); */


    objList[this] = hwPolygon->create(hwPolygon);
    objList[this]->name = 0;
    HW_MODIFY_3F(objList[this],hwStrColor, sr,sg,sb);
    /* top */
    ptr = pts;
    *ptr++ = 0.0;	*ptr++ = top;	*ptr++ = width;
    *ptr++ = length;	*ptr++ = top;	*ptr++ = width;
    *ptr++ = length;	*ptr++ = top;	*ptr++ = 0.0;
    *ptr++ = 0.0;	*ptr++ = top;	*ptr++ = 0.0;
    objList[this]->modify(objList[this],hwStrData, 
	HW_MAKE_TYPE(HW_TYPE_FLOAT, 4*3), pts);
    this++;

    ptr = pts;
    objList[this] = hwMesh->create(hwMesh);
    objList[this]->name = 0;
    HW_MODIFY_3F(objList[this],hwStrColor, sr,sg,sb);

    /* first row of quad mesh */
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = width;
    *ptr++ = length; *ptr++ = 0.0; *ptr++ = width;
    *ptr++ = length; *ptr++ = 0.0; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = 0.0;

    /* second row of quad mesh */
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = width;
    *ptr++ = length; *ptr++ = bg; *ptr++ = width;
    *ptr++ = length; *ptr++ = bg; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = 0.0;

    HW_MODIFY_1I(objList[this], hwStrGraphN, 2);
    HW_MODIFY_1I(objList[this], hwStrGraphM, 5);
    HW_MODIFY_1B(objList[this], hwStrBackface, HW_TRUE);
    objList[this]->modify(objList[this],hwStrData, 
	 HW_MAKE_TYPE(HW_TYPE_FLOAT,2*5*3), pts);
    this++;

    skyGroup = hwGroup->create(hwGroup);
    skyGroup->name = 0;
    skyGroup->modify(skyGroup, hwStrChildren, 
	HW_MAKE_TYPE(HW_TYPE_OBJECT, this), objList);
 
    return(skyGroup);

    /* Now, while the color is that of the building, draw the horizontal and
     * vertical strips */
    
#ifdef HORIZONTAL_STORY_MESHES    
    /* First the horizontal...the values used for x and z are already correct.
     * Just loop for each story, altering the y values */
    for(i = fg; i<= stories; i++)
    {
	y = i * STORY_HEIGHT - H_STRIP;
	ptr = pts; ptr++;  /* point at first Y value*/
	for(j=0;j<5;j++)
	{
	  *ptr = y;
	  ptr += 3;
	}
	y = i * STORY_HEIGHT;
	for(j=0;j<5;j++)
	{
	  *ptr = y;
	  ptr += 3;
	}
	quadrilateral_mesh(img_fildes,pts,2,5,NULL);
    }
    /* Now, put the lid on it. Just use the points already stored */
    ptr = pts + (3*5);
    polygon3d(img_fildes,ptr,4,NULL);
#else
	
    for(i = fg; i<= stories; i++)
    {
	y = i * STORY_HEIGHT;
	ptr = pts;
	
	*ptr++ = 0.0; *ptr++ = y; *ptr++ = 0.0;
	*ptr++ = 0.0; *ptr++ = y; *ptr++ = width;
	*ptr++ = length; *ptr++ = y; *ptr++ = width;
	*ptr++ = length; *ptr++ = y; *ptr++ = 0.0;
	*ptr++ = 0.0; *ptr++ = y; *ptr++ = 0.0;
	polyline3d(img_fildes,pts,5,FALSE);
    }
	
    /* Now, put the lid on it. Just use the points already stored */
    polygon3d(img_fildes,pts,4,NULL);
#endif


    /* Okay, now do the vertical strips. -- use polylines! */

    ptr = pts;
    /* Front of building */
    for(z=0.0; z<= (width - V_SPACING); z += V_SPACING)
    {
	*ptr++ = 0.0; *ptr++ = bg; *ptr++ = z;  *ptr++ = 0;  /* move */
	*ptr++ = 0.0; *ptr++ = top; *ptr++ = z; *ptr++ = 1;  /* draw */
    }
    /* put one on the end of this wall */
    z = width - V_STRIP;
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = z; *ptr++ = 0;  /* move */
    *ptr++ = 0.0; *ptr++ = top; *ptr++ = z; *ptr++ = 1;  /* draw */
    
    /* back of building */
    for(z=0.0; z<= (width - V_SPACING); z += V_SPACING)
    {
	*ptr++ = length; *ptr++ = bg; *ptr++ = z; *ptr++ = 0;  /* move */
	*ptr++ = length; *ptr++ = top; *ptr++ = z; *ptr++ = 1;  /* draw */
    }
    /* put one on the end of this wall */
    z = width + V_STRIP;
    *ptr++ = length; *ptr++ = bg; *ptr++ = z; *ptr++ = 0;  /* move */
    *ptr++ = length; *ptr++ = top; *ptr++ = z; *ptr++ = 1;  /* draw */

    /* Left side of building */
    for(x=0.0; x<= (length - V_SPACING); x += V_SPACING)
    {
	*ptr++ = x; *ptr++ = bg; *ptr++ = 0.0; *ptr++ = 0;  /* move */
	*ptr++ = x; *ptr++ = top; *ptr++ = 0.0; *ptr++ = 1;  /* draw */
    }
    /* put one on the end of this wall */
    x = length; 
    *ptr++ = x; *ptr++ = bg; *ptr++ = 0.0; *ptr++ = 0;  /* move */
    *ptr++ = x; *ptr++ = top; *ptr++ = 0.0;  *ptr++ = 1;  /* draw */

    /* Right side of building */
    for(x=0.0; x<= (length - V_SPACING); x += V_SPACING)
    {
	*ptr++ = x; *ptr++ = bg; *ptr++ = width; *ptr++ = 0;  /* move */
	*ptr++ = x; *ptr++ = top; *ptr++ = width; *ptr++ = 1;  /* draw */

    }
    /* put one on the end of this wall */
    x = length - V_STRIP;
    *ptr++ = x; *ptr++ = bg; *ptr++ = width; *ptr++ = 0;  /* move */
    *ptr++ = x; *ptr++ = top; *ptr++ = width; *ptr++ = 1;  /* draw */

    numpts = ((int)ptr - (int)pts) / (4*4);
    polyline3d(img_fildes,pts,numpts, TRUE);
    RESTORE_DEFAULT_VERTEX_FORMAT(img_fildes);
    DIFFUSE_LIGHTING_OFF(img_fildes);
}


#ifdef USE_LOW_RES_MODEL
static void low_res_model(
    int stories,
    float length, float width, float height,
    float gr, float gg, float gb,
    float sr, float sg, float sb)
{

    float *ptr,top;
    float bg,delta;
    static float pts[3*20];


    /* First, draw the stories that aren't all glass.  Note that if FIRST_GLASS
     * is <= 2, it is set to 2 (the first story is not glass ) 
     */
    
    ptr = pts;
    if (FIRST_GLASS < 2) {
      bg = STORY_HEIGHT;
    }else{
      bg = (FIRST_GLASS - 1) * STORY_HEIGHT;
    }

    top = STORY_HEIGHT * stories;

    /* first row of quad mesh */
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = width;
    *ptr++ = length; *ptr++ = 0.0; *ptr++ = width;
    *ptr++ = length; *ptr++ = 0.0; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = 0.0; *ptr++ = 0.0;

    /* second row of quad mesh */
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = width;
    *ptr++ = length; *ptr++ = bg; *ptr++ = width;
    *ptr++ = length; *ptr++ = bg; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = bg; *ptr++ = 0.0;

    fill_color(img_fildes,sr,sg,sb);
    vertex_format(img_fildes,0,0,0,0,CLOCKWISE);
    quadrilateral_mesh(img_fildes,pts,2,5,NULL);

    /* Now, put the lid on it. Just use the points already stored */
    ptr = pts; 
    *ptr++ = 0.0; *ptr++ = top; *ptr++ = 0.0;
    *ptr++ = 0.0; *ptr++ = top; *ptr++ = width;
    *ptr++ = length; *ptr++ = top; *ptr++ = width;
    *ptr++ = length; *ptr++ = top; *ptr++ = 0.0;
    polygon3d(img_fildes,pts,4,NULL);

    /* Finally, fill in all the glass */


    /* first row of quad mesh */
    ptr = pts;
    delta = 1.0;
    *ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = delta;
    *ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = width - delta;
    *ptr++ = length - delta; *ptr++ = bg - 1.0; *ptr++ = width - delta;
    *ptr++ = length - delta; *ptr++ = bg - 1.0; *ptr++ = delta;
    *ptr++ = delta; *ptr++ = bg - 1.0; *ptr++ = delta;

    /* second row of quad mesh */
    *ptr++ = delta; *ptr++ = top; *ptr++ = delta;
    *ptr++ = delta; *ptr++ = top; *ptr++ = width - delta;
    *ptr++ = length - delta; *ptr++ = top; *ptr++ = width - delta;
    *ptr++ = length - delta; *ptr++ = top; *ptr++ = delta;
    *ptr++ = delta; *ptr++ = top; *ptr++ = delta;

    fill_color(img_fildes,gr,gg,gb);
    surface_model(img_fildes,TRUE,2,1.0,1.0,1.0);
    quadrilateral_mesh(img_fildes,pts,2,5,NULL);
    RESTORE_DEFAULT_SURFACE_MODEL(img_fildes);
    RESTORE_DEFAULT_VERTEX_FORMAT(img_fildes);

}
#endif /* USE_LOW_RES_MODEL */


static void create_skyscraper_graphics(
    DRIVE_OBJECT *obj,
    int stories,
    float length, float width, float height)
{
    hwObject
	hwGrfx;
    typedef struct {
	float r,g,b;
    } RGB;
    static RGB glass_color[] = {
	{ 0.05, 0.05, 0.05 },	/* Grey 1 */
	{ 0.1, 0.1, 0.1 },	/* Grey 2 */
	{ 0.2, 0.2, 0.2 },	/* Grey 3 */
	{ 0.3, 0.3, 0.3 },	/* Grey 4 */
	{ 0.1, 0.3, 0.1 },	/* Greenish */
	{ 0.3, 0.2, 0.05 },	/* Brownish */
	{ 0.1, 0.1, 0.3 },	/* Blueish */
    };
#   define NUM_GLASS_COLORS (sizeof(glass_color)/sizeof(RGB))
    static RGB structure_color[] = {
	{ 0.2, 0.2, 0.2 },	/* Grey 1 */
	{ 0.6, 0.6, 0.6 },	/* Grey 2 */
	{ 0.3, 0.3, 0.3 },	/* Grey 3 */
	{ 0.8, 0.8, 0.8 },	/* Grey 4 */
	{ 0.2, 0.1, 0.05 },	/* Brownish */
	{ 0.3, 0.1, 0.1 },	/* Reddish */
    };
#   define NUM_STRUCTURE_COLORS (sizeof(structure_color)/sizeof(RGB))
    int gclr,sclr;


    gclr = ZINTRAND(NUM_GLASS_COLORS);
    sclr = ZINTRAND(NUM_STRUCTURE_COLORS);

        hwGrfx = high_res_model(stories,length,width,height,
	    glass_color[gclr].r,glass_color[gclr].g,glass_color[gclr].b,
	    structure_color[sclr].r,structure_color[sclr].g,
		structure_color[sclr].b);
        
	obj->display_list = createHwSegmentFromObj(&hwGrfx,1);

}


void init_skyscraper_object(
    DRIVE_OBJECT *obj)
{
    SKYSCRAPER_LIST *rl;
    int stories;


    if (obj->size[SIZE_LENGTH] <= 0.0)
	obj->size[SIZE_LENGTH] = DEFAULT_LENGTH;
    if (obj->size[SIZE_WIDTH] <= 0.0)
	obj->size[SIZE_WIDTH] = DEFAULT_WIDTH;
    if (obj->size[SIZE_HEIGHT] <= STORY_HEIGHT)
	obj->size[SIZE_HEIGHT] = DEFAULT_HEIGHT;

    stories = (int)obj->size[SIZE_HEIGHT]/STORY_HEIGHT;
    obj->size[SIZE_HEIGHT] = stories * STORY_HEIGHT;

    if (debug)
	printf(" inside init_skyscraper_%d_object() routine \n",(int)stories);

    obj->num_children = 0;

    /* See if we've created one like this before... */
    rl = skyscraper_list;
    while (rl != NULL) {
	if (IS_NEAR(rl->length,obj->size[SIZE_LENGTH])
		&& IS_NEAR(rl->width,obj->size[SIZE_WIDTH])
		&& IS_NEAR(rl->height,obj->size[SIZE_HEIGHT])
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
	if ((rl = (SKYSCRAPER_LIST *) malloc(sizeof(SKYSCRAPER_LIST)))==NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    return;
	}
	rl->length = obj->size[SIZE_LENGTH];
	rl->height = obj->size[SIZE_HEIGHT];
	rl->width  = obj->size[SIZE_WIDTH];
	rl->nameset_bits = obj->nameset_bits;
	rl->next   = skyscraper_list;
	rl->stories = stories;
    	create_skyscraper_graphics(obj, stories, obj->size[SIZE_LENGTH],
	    obj->size[SIZE_WIDTH],obj->size[SIZE_HEIGHT]);
	rl->dl_number = obj->display_list;
	skyscraper_list  = rl;
    }

    obj->surface_chars_xyz  = skyscraper_surface_chars_xyz;
    obj->surface_chars_bbox = skyscraper_surface_chars_bbox;

    /* Initial (mc) bounding box values */
    obj->bound_mc[0] = 0.0;
    obj->bound_mc[1] = 0.0;
    obj->bound_mc[2] = 0.0;
    obj->bound_mc[3] = obj->size[SIZE_LENGTH];
    obj->bound_mc[4] = obj->size[SIZE_HEIGHT] + BBOX_MARGIN;
    obj->bound_mc[5] = obj->size[SIZE_WIDTH];

    /* apply the object's xform matrix to the bounding box to put it in
     * world coordinates */
    update_wc_bounds(obj);
    elevate_object_to_terrain_height((SCENE *) obj->scene,obj,TRUE);
}
