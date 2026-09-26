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


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <sys/param.h>
#endif
#include "object.h"
#include "scene.h"
#include "drive_server.h"
#include "filenames.h"
#include "demo_physics.h"


#define MAX_CONSTRUCT_LEVEL	32
#define OBJNAME_STRLEN		64
#define BIGINT			0x7fffffff

#ifndef EPSILON
# define EPSILON (1.0e-6)
#endif /* !EPSILON */

char  objname[OBJNAME_STRLEN];
int   objnum;

static char  construct_tree[MAX_CONSTRUCT_LEVEL][OBJNAME_STRLEN];

static int new_construct;
static int construct_level = 0;

/* Prototype for later use */
static int parse_input(
    SCENE *scene,
    const char * const fname,
    FILE *fileptr,
    int *nextxscene, int *nextzscene,
    int got_sceneloc,
    float global_mat[4][4],
    const OBJECT_SPECS * const defaults,
    boolean_type only_read_courses,
    time_t mtime);

char *add_comment_string(char *current, char *new);
char *assign_comment_field();
char *global_comment=NULL;
char *previous_comment=NULL;
char *scene_comment=NULL;
int  sceneed_construct = 0;

static float identity[4][4] = {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 }
};

static void set_wormhole_characteristics(
    DRIVE_OBJECT *obj,
    WORMHOLE_DATA *data_in,
    char *subtype)
{
    WORMHOLE_DATA *data;
    WORMHOLE_OPERATION *which;
    char tmptype[1024],*type;
    boolean_type negate,invert,scene_relative;

    normalize_string(subtype,tmptype);
    type = tmptype;

    if ((data = malloc(sizeof(WORMHOLE_DATA))) == NULL) {
	return;
    }
    obj->data = (float *) data;

    /* Start the with defaults passed in */
    memcpy(data,data_in,sizeof(WORMHOLE_DATA));

    while ((*type == ' ') || (*type == '\t')) ++type;
    while (*type != '\0') {
	/* Get the thing we want to operate on */
	which = NULL;
	scene_relative = FALSE;

	if (strncmp(type,"positioninscene",15) == 0) {
	    which = &(data->position);
	    type += 15;
	    scene_relative = TRUE;
	}
	else if (strncmp(type,"position_in_scene",17) == 0) {
	    which = &(data->position);
	    type += 17;
	    scene_relative = TRUE;
	}
	else if (strncmp(type,"position",8) == 0) {
	    which = &(data->position);
	    type += 8;
	}
	else if (strncmp(type,"angle",5) == 0) {
	    which = &(data->angle);
	    type += 5;
	}
	else if (strncmp(type,"angular_velocity",16) == 0) {
	    which = &(data->angular_velocity);
	    type += 16;
	}
	else if (strncmp(type,"angularvelocity",15) == 0) {
	    which = &(data->angular_velocity);
	    type += 15;
	}
	else if (strncmp(type,"linear_velocity",15) == 0) {
	    which = &(data->linear_velocity);
	    type += 15;
	}
	else if (strncmp(type,"linearvelocity",14) == 0) {
	    which = &(data->linear_velocity);
	    type += 14;
	}
	else {
	    fprintf(stderr,"Cannot parse wormhole spec: %s\n",
		type);
	    return;
	}
	   
	/* Get the operation */
	invert = negate = FALSE;
	while (*type == ' ') ++type;
	if (*type == '=') {
	    if (scene_relative) which->operation = OPERATION_SCENE_REL_REPLACE;
	    else which->operation = OPERATION_REPLACE;
	    ++type;
	}
	else if ((*type == '+') && (*(type+1) == '=')) {
	    which->operation = OPERATION_ADD;
	    type += 2;
	}
	else if ((*type == '-') && (*(type+1) == '=')) {
	    which->operation = OPERATION_ADD;
	    negate = TRUE;
	    type += 2;
	}
	else if ((*type == '*') && (*(type+1) == '=')) {
	    which->operation = OPERATION_MULTIPLY;
	    type += 2;
	}
	else if ((*type == '/') && (*(type+1) == '=')) {
	    which->operation = OPERATION_MULTIPLY;
	    invert = TRUE;
	    type += 2;
	}
	else {
	    fprintf(stderr,"Cannot parse wormhole spec: %s\n",
		type);
	    return;
	}

	/* Get the operands */
	if (sscanf(type,"%f,%f,%f",
		&(which->operand[0]),
		&(which->operand[1]),
		&(which->operand[2])) == 3) {
	}
	else if (sscanf(type,"%f, %f, %f",
		&(which->operand[0]),
		&(which->operand[1]),
		&(which->operand[2])) == 3) {
	}
	else if (sscanf(type,"%f %f %f",
		&(which->operand[0]),
		&(which->operand[1]),
		&(which->operand[2])) == 3) {
	}
	else if (sscanf(type,"%f",&(which->operand[0])) == 1) {
	    which->operand[1] = which->operand[0];
	    which->operand[2] = which->operand[0];
	}
	else {
	    fprintf(stderr,"Cannot parse wormhole spec: %s\n",
		type);
	    return;
	}

	if (invert) {
	    if (ABS(which->operand[0]) > EPSILON) 
		which->operand[0] = 1.0 / which->operand[0];
	    if (ABS(which->operand[1]) > EPSILON) 
		which->operand[1] = 1.0 / which->operand[1];
	    if (ABS(which->operand[2]) > EPSILON) 
		which->operand[2] = 1.0 / which->operand[2];
	}
	else if (negate) {
	    which->operand[0] = -(which->operand[0]);
	    which->operand[1] = -(which->operand[1]);
	    which->operand[2] = -(which->operand[2]);
	}

#ifdef DO_ADJUSTMENTS_TWICE
	/* Adjust for units */
	if (which == &(data->position)) {
	    /* do nothing */
	}
	else if ((which == &(data->angle))
		|| (which = &(data->angular_velocity))) {
	    which->operand[0] = DEGREES_TO_RADIANS(which->operand[0]);
	    which->operand[1] = DEGREES_TO_RADIANS(which->operand[1]);
	    which->operand[2] = DEGREES_TO_RADIANS(which->operand[2]);
	}
	else if (which == &(data->linear_velocity)) {
	    which->operand[0] = MPH_TO_FPS(which->operand[0]);
	    which->operand[1] = MPH_TO_FPS(which->operand[1]);
	    which->operand[2] = MPH_TO_FPS(which->operand[2]);
	}
#endif	
	/* skip ahead to ';' */
	while ((*type != ';') && (*type != '\0')) ++type;
	if (*type == ';') ++type;
	while ((*type == ' ') || (*type == '\t')) ++type;
    }

    /* Adjust for units */
    if ((data->angle.operation == OPERATION_REPLACE)
	    || (data->angle.operation == OPERATION_HISTORIC)
	    || (data->angle.operation == OPERATION_ADD)) {
	data->angle.operand[0] =
	    DEGREES_TO_RADIANS(data->angle.operand[0]);
	data->angle.operand[1] =
	    DEGREES_TO_RADIANS(data->angle.operand[1]);
	data->angle.operand[2] =
	    DEGREES_TO_RADIANS(data->angle.operand[2]);
    }

    if ((data->angular_velocity.operation == OPERATION_REPLACE)
	    || (data->angular_velocity.operation == OPERATION_ADD)) {
	data->angular_velocity.operand[0] =
	    DEGREES_TO_RADIANS(data->angular_velocity.operand[0]);
	data->angular_velocity.operand[1] =
	    DEGREES_TO_RADIANS(data->angular_velocity.operand[1]);
	data->angular_velocity.operand[2] =
	    DEGREES_TO_RADIANS(data->angular_velocity.operand[2]);
    }

    if ((data->linear_velocity.operation == OPERATION_REPLACE)
	    || (data->linear_velocity.operation == OPERATION_ADD)) {
	data->linear_velocity.operand[0] =
	    MPH_TO_FPS(data->linear_velocity.operand[0]);
	data->linear_velocity.operand[1] =
	    MPH_TO_FPS(data->linear_velocity.operand[1]);
	data->linear_velocity.operand[2] =
	    MPH_TO_FPS(data->linear_velocity.operand[2]);
    }
}

static void build_object_matrix(
    OBJECT_SPECS * myspecs,
    float global_mat[4][4],
    float result[4][4])
{
    /* Apply xrot, yrot, zrot */
    if (myspecs->yrot != 0.0) {
	static float tmpmat[4][4] = {
	    { 1.0, 0.0, 0.0, 0.0 },
	    { 0.0, 1.0, 0.0, 0.0 },
	    { 0.0, 0.0, 1.0, 0.0 },
	    { 0.0, 0.0, 0.0, 1.0 }
	};
	tmpmat[0][0] = tmpmat[2][2] = FCOS(DEGREES_TO_RADIANS(myspecs->yrot));
	tmpmat[0][2] = FSIN(DEGREES_TO_RADIANS(myspecs->yrot));
	tmpmat[2][0] = -tmpmat[0][2];
	concat_matrix(tmpmat,myspecs->mat,myspecs->mat);
    }
    if (myspecs->xrot != 0.0) {
	static float tmpmat[4][4] = {
	    { 1.0, 0.0, 0.0, 0.0 },
	    { 0.0, 1.0, 0.0, 0.0 },
	    { 0.0, 0.0, 1.0, 0.0 },
	    { 0.0, 0.0, 0.0, 1.0 }
	};
	tmpmat[1][1] = tmpmat[2][2] = FCOS(DEGREES_TO_RADIANS(myspecs->xrot));
	tmpmat[1][2] = FSIN(DEGREES_TO_RADIANS(myspecs->xrot));
	tmpmat[2][1] = -tmpmat[1][2];
	concat_matrix(tmpmat,myspecs->mat,myspecs->mat);
    }
    if (myspecs->zrot != 0.0) {
	static float tmpmat[4][4] = {
	    { 1.0, 0.0, 0.0, 0.0 },
	    { 0.0, 1.0, 0.0, 0.0 },
	    { 0.0, 0.0, 1.0, 0.0 },
	    { 0.0, 0.0, 0.0, 1.0 }
	};
	tmpmat[0][0] = tmpmat[1][1] = FCOS(DEGREES_TO_RADIANS(myspecs->zrot));
	tmpmat[0][1] = FSIN(DEGREES_TO_RADIANS(myspecs->zrot));
	tmpmat[1][0] = -tmpmat[0][1];
	concat_matrix(tmpmat,myspecs->mat,myspecs->mat);
    }

    concat_matrix(myspecs->mat,global_mat,result);
}

static void read_construct(
    SCENE *scene,
    OBJECT_SPECS *myspecs,
    float global_mat[4][4])
{
    FILE *fptr;
    char constructfile[MAXPATHLEN+1];
    int junk,i;
    float construct_mat[4][4];
    OBJECT_SPECS construct_specs;


    if (construct_level >= MAX_CONSTRUCT_LEVEL) {
	fprintf(stderr,"Error: too many levels of recursion in construct %s.\n",
	    construct_tree[0]);
	return;
    }
    /* else */

    /* Check for circular definitions. */
    for (i=0; i<construct_level; ++i) {
	if (strcmp(construct_tree[i],objname) == 0) {
	    fprintf(stderr,"Error:  Circular construct definition:  %s\n",
		construct_tree[i]);
	    return;
	}
    }

    strcpy(construct_tree[construct_level],objname);
    ++construct_level;
    build_object_matrix(myspecs,global_mat,construct_mat);

    memcpy(&construct_specs,myspecs,sizeof(OBJECT_SPECS));
    /* Clear out specs matrix since it's already in the global matrix. */
    memcpy(construct_specs.mat,identity,4*4*sizeof(float));
    construct_specs.xrot = construct_specs.yrot = construct_specs.zrot = 0.0;

    sprintf(constructfile,"%s/%s",construct_dir,objname);

    if ((fptr = fopen(constructfile,"r")) == NULL) {
	fprintf(stderr,"Could not read construct file: %s\n",constructfile);
	--construct_level;
	return;
    }

    parse_input(scene,constructfile,fptr,&junk,&junk,FALSE,
	construct_mat,&construct_specs,FALSE,0);

    fclose(fptr);

    --construct_level;
}



/* Outside access to construct readers */
void get_construct(
    SCENE *scene,
    char *construct_name,
    OBJECT_SPECS *specs,
    float mat[4][4])
{
    strcpy(objname,construct_name);
    construct_level = 0;
    new_construct = FALSE;
    read_construct(scene,specs,mat);
}


void got_obj(
    SCENE *scene,
    OBJECT_SPECS *myspecs,
    float global_mat[4][4])
{
    OBJECT_ID *oi;
    DRIVE_OBJECT *obj;

    if (new_construct) {
	new_construct = FALSE;
	read_construct(scene,myspecs,global_mat);
	return;
    }

    if ((objnum != INVALID) || (objname[0] != '\0')) {
	oi = find_object(objnum,objname);

	if (oi->number != INVALID) {
	    if ((obj = (DRIVE_OBJECT *) malloc((sizeof(DRIVE_OBJECT))))
		    == NULL) {
		fprintf(stderr,"Out of malloc space!\n");
	    }
	    else {
		/* Got a good one! */

		if( scene_ed )
		{
		    if (( obj->scene_ed = (SceneEd_OBJECT *) 
			malloc(( sizeof(SceneEd_OBJECT)))) == NULL )
		    {
			fprintf(stderr,"Out of malloc space!\n");
		    }
		    else
		    {
			obj->scene_ed->xrot = myspecs->xrot;
			obj->scene_ed->yrot = myspecs->yrot;
			obj->scene_ed->zrot = myspecs->zrot;
			memcpy(obj->scene_ed->orig_xform,
			    myspecs->mat, sizeof(matrix3d) );
			obj->scene_ed->has_xform =
			    myspecs->object_bits & OBJ_MATRIX;

			/* We need to explicitly save the X, Y, Z as well
			 * as the Length, Width and Height, since the
			 * call to complete_object() below may alter these
			 * values from the ones specified in the file */

			obj->scene_ed->x = myspecs->mat[3][0];
			obj->scene_ed->y = myspecs->mat[3][1];
			obj->scene_ed->z = myspecs->mat[3][2];

			obj->scene_ed->length = myspecs->size[0];
			obj->scene_ed->width  = myspecs->size[1];
			obj->scene_ed->height = myspecs->size[2];

			obj->scene_ed->angle = myspecs->angle;
			obj->scene_ed->comments = 0;
		    }
		}
		build_object_matrix(myspecs,global_mat,obj->xform);

		/* Copy relevant information to object structure */
		obj->idptr	= oi;
		obj->size[0]	= myspecs->size[0];
		obj->size[1]	= myspecs->size[1];
		obj->size[2]	= myspecs->size[2];

		obj->color[0]	= myspecs->color[0];
		obj->color[1]	= myspecs->color[1];
		obj->color[2]	= myspecs->color[2];

		obj->radius	= myspecs->radius;
		obj->nameset_bits = myspecs->nameset_bits;
		obj->angle	= DEGREES_TO_RADIANS(myspecs->angle);
		obj->spacing	= myspecs->spacing;
		obj->count	= myspecs->count;
		obj->dimension	= myspecs->dimension;
		obj->data	= myspecs->data;
		strcpy(obj->subtype,myspecs->subtype);
		strcpy(obj->label,myspecs->label);
		obj->scene	= (void *) scene;

		if (obj->idptr->flags & OBJECTCLASS_WORMHOLE) {
		    /* Copy angle data to wormhole data */
		    myspecs->wormhole_data.angle.operand[0] = myspecs->angle;

		    set_wormhole_characteristics(obj,
			&(myspecs->wormhole_data), obj->subtype);

		    if( scene_ed && (myspecs->object_bits & OBJ_DESTINATION) ){
			/* Wormhole Destination was specifed the old way.
			 * Go ahead and save it off here */
			
			obj->scene_ed->destx = 
			    myspecs->wormhole_data.position.operand[0];
			obj->scene_ed->desty = 
			    myspecs->wormhole_data.position.operand[1];
			obj->scene_ed->destz = 
			    myspecs->wormhole_data.position.operand[2];

		    }
		}


		obj->connection	= 0;

		complete_object(&scene->object_head,obj);


		if (strcmp(obj->idptr->lcname,"hill") == 0)
		    scene->flags &= (~SCENE_ALL_FLAT);
	    }
	}
    }

    /* Assign the comment string to the object, if necessary) */
    if( scene_ed )
    {
	if ((objnum != INVALID) || (objname[0] != '\0')) {
	    obj->scene_ed->comments = assign_comment_field(1);
	}
	else
	{
	    assign_comment_field(1);
	}
    }

    /* Re-initialize the globals to default values */
    objnum     = INVALID;
    objname[0] = '\0';

    /* The "specs" will be re-initialized elsewhere */
}


static int scene_type_number(
    const char scenename[])
{
    char name[4096];
    SCENE_TYPE *st;

    normalize_string((char *) scenename,name);

    st = scene_type;
    while (st->type != INVALID) {
	if (strcmp(name,st->name) == 0) {
	    return(st->type);
	}
	++st;
    }

    return(SCENE_NONRANDOM);
}


#define FIELD_INT		0
#define FIELD_2INT		1
#define FIELD_3INT		2
#define FIELD_FLOAT		3
#define FIELD_2FLOAT		4
#define FIELD_3FLOAT		5
#define FIELD_MATRIX		6
#define FIELD_STRING		7
#define FIELD_OBJECT		8
#define FIELD_SCENELOC		9
#define FIELD_SCENETYPE		10
#define FIELD_CONSTRUCT		11
#define FIELD_SHIFTINT		12
#define FIELD_FLOATLIST		13


/* Returns whether there's more scenes in this file. */
static int parse_input(
    SCENE *scene,
    const char * const fname,
    FILE *fileptr,
    int *nextxscene, int *nextzscene,
    int got_sceneloc,
    float global_mat[4][4],
    const OBJECT_SPECS * const defaults,
    boolean_type only_read_courses,
    time_t mtime)
{
    typedef struct {
	char	*header;
	int	header_strlen;
	int	type;
	void	*addr;
    } FIELD;
    static OBJECT_SPECS myspecs;
    static COURSE mycourse;
    static FIELD object_field[] = {
	/*** Scene-releated attributes ***/
	{ "Scene Location:",	0,	FIELD_SCENELOC,	NULL		},
	{ "Scene Type:",	0,	FIELD_SCENETYPE,NULL		},

	/*** Objects and constructs ***/
	{ "Construct:",		0,	FIELD_CONSTRUCT,objname		},
	{ "Object:",		0,	FIELD_OBJECT,	objname		},

	/*** Object/construct attributes ***/
	{ "Type:",		0,	FIELD_STRING,	
		(void *) myspecs.subtype	},
	{ "Label:",		0,	FIELD_STRING,	
		(void *) myspecs.label		},
	{ "Matrix:",		0,	FIELD_MATRIX,	
		(void *) myspecs.mat		},
	{ "Xrot:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.xrot)	},
	{ "Yrot:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.yrot)	},
	{ "Zrot:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.zrot)	},
	{ "Position:",		0,	FIELD_3FLOAT,	
		(void *) &(myspecs.mat[3][0])	},
	{ "Destination:",	0,	FIELD_3FLOAT,	
		(void *) myspecs.wormhole_data.position.operand },
	{ "X:",			0,	FIELD_FLOAT,	
		(void *) &(myspecs.mat[3][0])	},
	{ "Y:",			0,	FIELD_FLOAT,	
		(void *) &(myspecs.mat[3][1])	},
	{ "Z:",			0,	FIELD_FLOAT,	
		(void *) &(myspecs.mat[3][2])	},
	{ "Size:",		0,	FIELD_3FLOAT,	
		(void *) myspecs.size		},
	{ "Color:",		0,	FIELD_3FLOAT,	
		(void *) myspecs.color		},
	{ "Length:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.size[0])	},
	{ "Width:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.size[1])	},
	{ "Height:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.size[2])	},
	{ "Radius:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.radius)	},
	{ "Animation Bits:",	0,	FIELD_SHIFTINT,	
		(void *) &(myspecs.nameset_bits) },
	{ "Angle:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.angle)	},
	{ "Spacing:",		0,	FIELD_FLOAT,	
		(void *) &(myspecs.spacing)	},
        { "Count:",		0,	FIELD_INT,	
		(void *) &(myspecs.count)	},
        { "Dimension:",		0,	FIELD_INT,	
		(void *) &(myspecs.dimension)	},
        { "Data:",		0,	FIELD_FLOATLIST,
	        (void *) &(myspecs.count)	},
    };

    static FIELD course_field[] = {
	/*** Scene-releated attributes ***/
	{ "Scene Location:",	0,	FIELD_SCENELOC,	NULL		},

	/*** Racecourse related attributes ***/
	{ "Racecourse:",	0,	FIELD_STRING,
	    mycourse.coursename },
	{ "Practice Seconds:",	0,	FIELD_INT,
	    &mycourse.practice_seconds },
	{ "Pre-Race Seconds:",	0,	FIELD_INT,
	    &mycourse.pre_race_seconds },
	{ "Race Seconds:",	0,	FIELD_INT,
	    &mycourse.race_seconds },
	{ "Post-Race Seconds:",	0,	FIELD_INT,
	    &mycourse.post_race_seconds },
	{ "Start SceneX:",	0,	FIELD_INT,
	    &mycourse.start_scene_x },
	{ "Start SceneZ:",	0,	FIELD_INT,
	    &mycourse.start_scene_z },
	{ "Start Angle:",	0,	FIELD_FLOAT, 
	    &mycourse.start_angle },
	{ "Start Position:",	0,	FIELD_3FLOAT,
	    mycourse.start_position },
	{ "Start Positions:",	0,	FIELD_INT, 
	    &mycourse.num_positions },
	{ "Turbo Mode:",	0,	FIELD_INT, 
	    &mycourse.turbo_mode },
	{ "Turbo Boosts:",	0,	FIELD_INT, 
	    &mycourse.turbo_boosts },
	{ "Guns Mode:",		0,	FIELD_INT, 
	    &mycourse.guns_mode },
	{ "Ammo:",		0,	FIELD_INT, 
	    &mycourse.ammo },
    };

    static FIELD *field;

#   define NUM_OBJ_FIELDS (sizeof(object_field)/sizeof(FIELD))
#   define NUM_COURSE_FIELDS (sizeof(course_field)/sizeof(FIELD))
    int i,j,returnval,quitloop;
    char str[4096],*cptr,*cptr2;
    float *fptr;
    int numfields;

    /* If we have a new scene, the scene comment collected in our last */
    /* trip through here needs to be assigned to this scene */
    if( scene && (! only_read_courses) )
    {
	scene->comments = scene_comment;
	scene_comment = NULL;
    }
    global_comment = NULL;
    previous_comment = NULL;


    if( only_read_courses) {
	numfields = NUM_COURSE_FIELDS;
	field = course_field;
    }
    else {
	numfields = NUM_OBJ_FIELDS;
	field = object_field;
    }


    /* Initialize header strlens, if not already done. */
    if (field[0].header_strlen == 0) {
	for (i=0; i<numfields; ++i) {
	    field[i].header_strlen = strlen(field[i].header);
	}
    }

    /* Copy default specs to myspecs */
    memcpy(&myspecs,defaults,sizeof(OBJECT_SPECS));

    /* Copy default course specs to mycourse */
    if( construct_level == 0 )
    {
	memcpy(&mycourse,&default_course,sizeof(COURSE));
	mycourse.start_scene_x = BIGINT;
	mycourse.start_scene_z = BIGINT;
	mycourse.coursename[0] = '\0';
    }

    objnum = INVALID;
    objname[0] = '\0';

    returnval = quitloop = FALSE;

    while (fgets(str,sizeof(str),fileptr) != NULL) {

	/* If we are in SceneEd, and we get a Construct, lets just treat
	 * it (and everything else until an Object or Scene Location statment)
	 * as a comment.
	 */
	if( scene_ed )
	{
	    if(strncmp(str,"Construct",9) == 0 ) {
		sceneed_construct = 1;
	    }
	    else if(strncmp(str,"Object",6) == 0) {
		sceneed_construct = 0;
	    }
	    else if(strncmp(str,"Scene Location",14) == 0) {
		sceneed_construct = 0;
	    }
	    if(sceneed_construct )
	    {
		global_comment = add_comment_string(global_comment,str);
		continue;
	    }
	}
	/* Ignore comments */
	if (str[0] == '#') 
	{
	    if( scene_ed )
	    {
		global_comment = add_comment_string(global_comment,str);
	    }
	    continue;
	}


	/* Skip blank stuff */
	cptr = str;
	while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;

	/* Ignore blank lines */
	if ((*cptr == '\n') || (*cptr == '\0')) continue;

	for (i=0; i<numfields; ++i) {
	    if (strncmp(cptr,field[i].header,field[i].header_strlen) == 0) {
		/* Got a match! */
		switch (field[i].type) {
		    case FIELD_INT:
		    case FIELD_SHIFTINT:
			cptr2 = cptr + field[i].header_strlen;
			while ((*cptr2 == ' ') || (*cptr2 == '\t')) ++cptr2;
			if ((*cptr2 == '0') && (*(cptr2+1) == 'x')) {
			    if (sscanf(cptr2,"%x", (int *) field[i].addr) < 1) {
				/* error */
				i = numfields;
			    }
			}
			else {
			    if (sscanf(cptr2,"%d", (int *) field[i].addr) < 1) {
				/* error */
				i = numfields;
			    }
			}
			if ( (field[i].type == FIELD_SHIFTINT) && (!scene_ed) ){
				*((unsigned int *) field[i].addr) <<= 16;
			}
			break;

		    case FIELD_SCENELOC:
			if (construct_level > 0) {
			    fprintf(stderr,
				"Specifier \"%s\" not valid in constructs.\n",
				field[i].header);
			    break;
			}
			if (got_sceneloc) {
			    /* must be starting a new scene! */
			    if (sscanf(cptr+field[i].header_strlen,"%d %d",
					nextxscene,
					nextzscene)
				    == 2) { 
				/* good! */
				quitloop = TRUE;
				returnval = TRUE;
				scene_comment = global_comment;
				break;
			    }
			    else {
				/* error */
				i = numfields;
			    }
			}
			else {
			    int tmpx, tmpz;
			    if (sscanf(cptr+field[i].header_strlen,"%d %d",
					&tmpx,
					&tmpz)
				    == 2) { 
				if ((scene = find_scene(tmpx,tmpz)) == NULL) {
				    quitloop = TRUE;
				    returnval = FALSE;
				    break;
				}
				if ( ! only_read_courses )
				    scene->comments = assign_comment_field(0);
				global_mat[3][0] = scene->xscene * SCENE_SIZE;
				global_mat[3][2] = scene->zscene * SCENE_SIZE;
				got_sceneloc = TRUE;
			    }
			    else {
				/* error */
				i = numfields;
			    }
			}
			break;

		    case FIELD_SCENETYPE:
			if (construct_level > 0) {
			    fprintf(stderr,
				"Specifier \"%s\" not valid in constructs.\n",
				field[i].header);
			    break;
			}
			cptr += field[i].header_strlen;
			/* skip blank space */
			while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;
			cptr2 = cptr + (strlen(cptr)-1); /* last letter */
			if (*cptr2 == '\n') *cptr2 = '\0';
			if (scene == NULL) {
			    fprintf(stderr,"Must give Scene Location first in scenefile %s\n",fname);
			    quitloop = TRUE;
			    returnval = FALSE;
			    break;
			}
			scene->type = scene_type_number(cptr);
			scene->flags = scene_type[scene->type].flags;

			break;

		    case FIELD_2INT:
			if (sscanf(cptr+field[i].header_strlen,"%d %d",
				    (int *) field[i].addr,
				    (int *) field[i].addr + 1)
				< 2) { 
			    /* error */
			    i = numfields;
			}
			break;

		    case FIELD_3INT:
			if (sscanf(cptr+field[i].header_strlen,"%d %d %d",
				    (int *) field[i].addr,
				    (int *) field[i].addr + 1,
				    (int *) field[i].addr + 2)
				< 3) { 
			    /* error */
			    i = numfields;
			}
			break;

		    case FIELD_FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f",
				    (float *) field[i].addr)
				< 1) {
			    /* error */
			    i = numfields;
			}
			break;

		    case FIELD_2FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f %f",
				    (float *) field[i].addr,
				    (float *) field[i].addr + 1)
				< 2) {
			    /* error */
			    i = numfields;
			}
			break;

		    case FIELD_3FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f %f %f",
				    (float *) field[i].addr,
				    (float *) field[i].addr + 1,
				    (float *) field[i].addr + 2)
				< 3) {
			    /* error */
			    i = numfields;
                            break;
			}
			/* for backwards compatibility */
			if (strncmp(field[i].header,"Destination:",
				field[i].header_strlen) == 0) {
			    myspecs.wormhole_data.position.operation = 
			    myspecs.wormhole_data.angle.operation = 
			    myspecs.wormhole_data.angular_velocity.operation = 
			    myspecs.wormhole_data.linear_velocity.operation = 
				OPERATION_REPLACE;
			    myspecs.wormhole_data.angle.operation = 
				OPERATION_HISTORIC;

			    myspecs.object_bits |= OBJ_DESTINATION;
			    
			}
			break;

		    case FIELD_FLOATLIST:
		    {
			int count, dimension;
			float *array;

			/* Read array size and dimension. */
			sscanf(cptr+field[i].header_strlen, "%d %d",
			       &count, &dimension);

			/* Allocate space for array. */
			array = (float *)malloc(count*dimension*sizeof(float));
			if (array == (float *)NULL)
			{
			    fprintf(stderr,"Malloc failed on scene data.\n");
			    quitloop = TRUE;
			    returnval = FALSE;
			    break;
			}

			/* Read data. */
			fptr = array;
			for (j=0; j<count*dimension; j++)
			{
			    fscanf( fileptr, "%f", fptr );
			    fptr++;
			}
			
			myspecs.count = count;
			myspecs.dimension = dimension;
			myspecs.data = array;
		    }
			break;

		    case FIELD_MATRIX:
			fptr = (float *) field[i].addr;
			myspecs.object_bits |= OBJ_MATRIX;
			/* First try for all 16 values on one line */
			if (sscanf(cptr+field[i].header_strlen,
				"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
				fptr,   fptr+1, fptr+2, fptr+3,
				fptr+4, fptr+5, fptr+6, fptr+7,
				fptr+8, fptr+9, fptr+10,fptr+11,
				fptr+12,fptr+13,fptr+14,fptr+15) != 16) {
			    /* Try it spread over several lines. */
			    if (sscanf(cptr+field[i].header_strlen,
				    "%f %f %f %f",
				    fptr, fptr+1, fptr+2, fptr+3) < 4) {
				/* error */
				i = numfields;
			    }
			    for (j = 0; j<3; ++j) {
				if (fgets(str,sizeof(str),fileptr) == NULL) {
				    /* error */
				    i = numfields;
				    break;
				}
				fptr += 4;
				if (sscanf(str,"%f %f %f %f",
					fptr, fptr+1, fptr+2, fptr+3) < 4) {
				    /* error */
				    i = numfields;
				    break;
				}
			    }
			}
			break;

		    case FIELD_OBJECT:
		    case FIELD_CONSTRUCT:
			got_obj(scene,&myspecs,global_mat);
			new_construct = (field[i].type == FIELD_CONSTRUCT);
			/* Copy default specs to myspecs */
			memcpy(&myspecs,defaults,sizeof(OBJECT_SPECS));
			/* fall through */
		    case FIELD_STRING:
			cptr += field[i].header_strlen;
			/* skip blank space from front */
			while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;
			/* strip blank space from back */
			cptr2 = cptr + (strlen(cptr)-1); /* last letter */
			while ((*cptr2 == '\n') 
				|| (*cptr2 == '\t')
				|| (*cptr2 == ' ')) {
			    *cptr2 = '\0';
			    --cptr2;
			}
			strcpy((char *) field[i].addr,cptr);
			break;
		}
		break;  /* out of string search loop. */
	    }
	}

	if (quitloop) break;

	if (i >= numfields) {
	/*
	    fprintf(stderr,"%s scene file:  bad syntax:  %s.\n",
		fname,str);
	*/
	}
    }

    /* Make sure to pick up the last object. */
    if (!only_read_courses) {
	got_obj(scene,&myspecs,global_mat);
        if (scene) {
            scene->read_in = TRUE;
            if (construct_level == 0) scene->file_mtime = mtime;
        }
    }

    /* Process the course if valid */
    if (only_read_courses && (construct_level == 0) && 
       (mycourse.coursename[0] != '\0') ) {
	COURSE *c;

	/* if start x or y not specified, use the scene's */
	if (got_sceneloc) {
	    if (mycourse.start_scene_x == BIGINT)
		mycourse.start_scene_x = scene->xscene;
	    if (mycourse.start_scene_z == BIGINT)
		mycourse.start_scene_z = scene->zscene;
	}
	/* Malloc space for a copy */
	c = (COURSE *) fastmalloc(sizeof(COURSE));
	memcpy(c,&mycourse,sizeof(COURSE));

	/* Add to linked list */
	c->next = course;
	course = c;
    }

    return(returnval);
}


/* Returns whether there's more scenes in this file. */
static int preview_input(
    SCENE *scene,
    const char * const fname,
    FILE *fileptr,
    int *nextxscene, int *nextzscene,
    int got_sceneloc)
{
    char str[4096],*cptr,*cptr2;

    while (fgets(str,sizeof(str),fileptr) != NULL) {
	/* Ignore comments */
	if (str[0] == '#') continue;

	/* Skip blank stuff */
	cptr = str;
	while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;

	/* Ignore blank lines */
	if ((*cptr == '\n') || (*cptr == '\0')) continue;

	if (strncmp(cptr,"Scene Location:",15) == 0) {
	    cptr += 15;
	    if (got_sceneloc) {
		/* must be starting a new scene! */
		if (sscanf(cptr,"%d %d", nextxscene, nextzscene) == 2) { 
		    return(TRUE);
		}
	    }
	    else {
		int tmpx, tmpz;
		if (sscanf(cptr,"%d %d", &tmpx, &tmpz) == 2) { 
		    scene->xscene = tmpx;
		    scene->zscene = tmpz;
		    got_sceneloc = TRUE;
		}
	    }
	}
	else if (strncmp(cptr,"Scene Type:",11) == 0) {
	    cptr += 11;
	    /* skip blank space */
	    while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;
	    cptr2 = cptr + (strlen(cptr)-1); /* last letter */
	    if (*cptr2 == '\n') *cptr2 = '\0';
	    scene->type = scene_type_number(cptr);
	}
    }

    return(FALSE);
}

void reset_object_specs(
    OBJECT_SPECS *specs)
{
    specs->subtype[0] = '\0';
    specs->label[0] = '\0';
    specs->xrot = specs->yrot = specs->zrot = 0.0;
    specs->size[0] = specs->size[1] = specs->size[2] = DEFAULT_OBJECT_SIZE;
    specs->angle = DEFAULT_OBJECT_ANGLE;
    specs->radius = DEFAULT_OBJECT_RADIUS;
    specs->spacing = DEFAULT_OBJECT_SPACING;
    memcpy(specs->mat,identity,16*sizeof(float));
    specs->color[0] = specs->color[1] = specs->color[2] = DEFAULT_OBJECT_COLOR;
    specs->count = DEFAULT_OBJECT_COUNT;
    specs->wormhole_data.position.operation = OPERATION_NONE;
    specs->wormhole_data.position.operand[0] = 
	specs->wormhole_data.position.operand[1] = 
	specs->wormhole_data.position.operand[2] = 0.0;
    specs->wormhole_data.angle.operation = OPERATION_NONE;
    specs->wormhole_data.angle.operand[0] = 
	specs->wormhole_data.angle.operand[1] = 
	specs->wormhole_data.angle.operand[2] = 0.0;
    specs->wormhole_data.angular_velocity.operation = OPERATION_NONE;
    specs->wormhole_data.angular_velocity.operand[0] = 
	specs->wormhole_data.angular_velocity.operand[1] = 
	specs->wormhole_data.angular_velocity.operand[2] = 0.0;
    specs->wormhole_data.linear_velocity.operation = OPERATION_NONE;
    specs->wormhole_data.linear_velocity.operand[0] = 
	specs->wormhole_data.linear_velocity.operand[1] = 
	specs->wormhole_data.linear_velocity.operand[2] = 0.0;
    specs->dimension = 3;
    specs->data = NULL;
    specs->nameset_bits = ALL_TIMED_NAMESETS;
    if(scene_ed) specs->nameset_bits = specs->nameset_bits >> 16;
    specs->object_bits = 0;
}

#define INVALID_SCENE -1000000

int preview_scenefile(
    const char path[])
{
    SCENE *scene;
    FILE *fptr;
    int theres_more;
    int nextxscene = INVALID_SCENE;
    int nextzscene = INVALID_SCENE;
    int notfirst   = FALSE;
    static float global_mat[4][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 }
    };
    OBJECT_SPECS default_specs;
    
    reset_object_specs(&default_specs);


    if ((fptr = fopen(path,"r")) == NULL) {
	fprintf(stderr,"Could not read scene file: %s\n",path);
	return (-1);
    }

    do {
	if ((scene = (SCENE *) malloc(sizeof(SCENE))) == NULL) {
	    fprintf(stderr,"Out of malloc space!\n");
	    fclose(fptr);
	    return (-1);
	}

	/* set defaults */
	scene->xscene      = nextxscene;
	scene->zscene      = nextzscene;
	scene->type        = SCENE_NONRANDOM;
	scene->flags       = DEFAULT_SCENE_FLAGS;
	scene->random_seed = INVALID;
	scene->object_head = NULL;
	scene->static_seg  = INVALID;
	scene->count       = 0;
	scene->defined     = TRUE;
	scene->read_in     = FALSE;
	scene->comments    = NULL;
	if ((scene->scenefilename = (char *) malloc(strlen(path)+1)) == NULL)
	    break;
	strcpy(scene->scenefilename,path);

	if (notfirst) add_scene_to_list(scene);
	/* Will update offsets in global_mat for first scene
	 * when we read them.
	 */
	theres_more = preview_input(scene,path,fptr,
	    &nextxscene,&nextzscene,notfirst);
	if (!notfirst) 
	{
	    if((scene->xscene!=INVALID_SCENE) &&
		 (scene->zscene != INVALID_SCENE) )
		add_scene_to_list(scene);
	    else
	    {
		return(-1);
		fclose(fptr);
	    }
	}
	notfirst = TRUE;
    } while (theres_more);
    fclose(fptr);

    /* Now, read through the file again, this time reading in ONLY the
     * course information */

    if ((fptr = fopen(path,"r")) == NULL) {
	fprintf(stderr,"Could not read scene file: %s\n",path);
	return(-1);
    }

    do {
	theres_more = parse_input(scene,path,fptr,&nextxscene,&nextzscene,
	    notfirst,global_mat,&default_specs,TRUE,0);
	notfirst = TRUE;
    } while (theres_more);

    fclose(fptr);
    global_comment   = NULL;
    previous_comment = NULL;
    return(0);

}


int read_scenefile(
    const char path[], int *retval)
{
    SCENE *scene;
    FILE *fptr;
    int theres_more;
    int nextxscene = 0;
    int nextzscene = 0;
    int notfirst   = FALSE;
    static float global_mat[4][4] = {
	{ 1.0, 0.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0, 0.0 },
	{ 0.0, 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 0.0, 1.0 }
    };
    OBJECT_SPECS default_specs;
    time_t mtime;
    
    reset_object_specs(&default_specs);

    mtime = scenefile_mtime((char *) path);
	
    if ((fptr = fopen(path,"r")) == NULL) {
	fprintf(stderr,"Could not read scene file: %s\n",path);
	*retval = 0;
	return(0);
    }

    do {
	if (notfirst) {
	    if ((scene = find_scene(nextxscene,nextzscene)) == NULL) break;
	}
	else {
	    /* Don't have a good location yet. */
	    scene = NULL;
	}


	/* Will update offsets in global_mat for first scene
	 * when we read them.
	 */
	theres_more = parse_input(scene,path,fptr,&nextxscene,&nextzscene,
	    notfirst,global_mat,&default_specs,FALSE,mtime);

	notfirst = TRUE;
	global_mat[3][0] = nextxscene * SCENE_SIZE;
	global_mat[3][2] = nextzscene * SCENE_SIZE;
    } while (theres_more);

    fclose(fptr);
    *retval = 1;
    return(1);
}

/* This routine will malloc enough room to hold the strings in both
 * current and new, copy the strings into the new area, and return the
 * new pointer */
char *add_comment_string(char *current, char *new)
{
    char *comment;
    char *insert;

    if (current) {
        comment = malloc( strlen(current)+strlen(new) +3 );
        strcpy(comment, current);
        insert = comment + strlen(current);
        comment[ strlen(current)] = '\n';
        strcpy(insert,new);
        free(current);
    }
    else {
        comment = malloc(strlen(new) + 1);
        strcpy(comment, new);
    }

    /*
    printf("New Comment String: \n%s\n",comment);
    */
    return(comment);
    
}

/* Assign the comment string to the object, if necessary. 
 * Note that certain comments need to be assigned immediately (as
 * in the case of Scene Location ).  In the case of
 * objects, the comments need to be assigned the comment previous.
 */
char *assign_comment_field(int delay)
{
    char *comments;

    if( scene_ed  )
    {

	if( delay )
	{
	    comments = previous_comment;
	    previous_comment = global_comment;
	    global_comment = NULL;
	}
	else
	{
	    comments = global_comment;
	    global_comment = NULL;
	}
    return( comments );
    }

    return( NULL );
}
