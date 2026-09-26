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

#include "global.h"
#include "drive.h"
#include "disp_ctrl.h"
#include "debug.h"

#define SEGMENT_NODE_BLOCKSIZE	500
#define EPSILON			(1.0e-6)


extern void concat_matrix(float[4][4],float[4][4],float[4][4]);

typedef struct _segment_instance_struct {
    matrix3d *local_xform;
    int segnum;			/* If valid, index off of "segnode" array */
    /* SEGMENT_NODE */ void  *segnode;
	/* if segnum==INVALID, a pointer to malloced segnode */
    struct _segment_instance_struct *next;
} SEGMENT_INSTANCE;

typedef struct _segnode {
    float mc_extent[6];
    float cull_size;
    DISPLAY_NODE_CONDITION condition;
    boolean_type expected_condition_value;
    boolean_type is_leaf_node;	/* has graphics to draw */
    DISPLAY_NODE_OPERATION operation;
    SEGMENT_INSTANCE *firstchild;
} SEGMENT_NODE;

static SEGMENT_NODE *segnode = NULL;
static int current_segnode_array_size = 0;

static matrix3d identity = {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 }
};


typedef struct {
    int x,y,z;
} TRIPLE;
static TRIPLE corner[8] = {
    { 0,1,2 },
    { 0,1,5 },
    { 3,1,2 },
    { 3,1,5 },
    { 0,4,2 },
    { 0,4,5 },
    { 3,4,2 },
    { 3,4,5 },
};


#define BIG 	(1e+20)

static void transform_extent(
    float extent_in[6],
    matrix3d *xform,
    float extent_out[6])
{
    float point_mc[8][3];
    float x,y,z,w;
    float minx,miny,minz,maxx,maxy,maxz;
    int i;

    for (i=0; i<8; ++i) {
	point_mc[i][0] = extent_in[corner[i].x];
	point_mc[i][1] = extent_in[corner[i].y];
	point_mc[i][2] = extent_in[corner[i].z];
    }

    minx = miny = minz = BIG;
    maxx = maxy = maxz = -BIG;

    for (i=0; i<8; ++i) {
	x = point_mc[i][0] * (*xform)[0][0] +
	    point_mc[i][1] * (*xform)[1][0] +
	    point_mc[i][2] * (*xform)[2][0] +
	    (*xform)[3][0];
	y = point_mc[i][0] * (*xform)[0][1] +
	    point_mc[i][1] * (*xform)[1][1] +
	    point_mc[i][2] * (*xform)[2][1] +
	    (*xform)[3][1];
	z = point_mc[i][0] * (*xform)[0][2] +
	    point_mc[i][1] * (*xform)[1][2] +
	    point_mc[i][2] * (*xform)[2][2] +
	    (*xform)[3][2];
	w = point_mc[i][0] * (*xform)[0][3] +
	    point_mc[i][1] * (*xform)[1][3] +
	    point_mc[i][2] * (*xform)[2][3] +
	    (*xform)[3][3];

	if (w < EPSILON) {
	    /* behind the eyepoint */
	    if (w < 0.0) {
		x = -x; y = -y;
	    }
	    z = -BIG;
	}
	else if (ABS(w-1.0) > EPSILON) {
	    x /= w; y /= w; z /= w;
	}

	if (x < minx) minx = x;
	if (x > maxx) maxx = x;
	if (y < miny) miny = y;
	if (y > maxy) maxy = y;
	if (z < minz) minz = z;
	if (z > maxz) maxz = z;
    }

    extent_out[0] = minx;
    extent_out[1] = miny;
    extent_out[2] = minz;
    extent_out[3] = maxx;
    extent_out[4] = maxy;
    extent_out[5] = maxz;
}


static void realloc_segnode_array(
    int maxnode)
{
    int newsize,i;

    /* Make sure there's room */
    if (maxnode >= current_segnode_array_size) {
	newsize = SEGMENT_NODE_BLOCKSIZE * (maxnode/SEGMENT_NODE_BLOCKSIZE + 1);
	if (current_segnode_array_size == 0) {
	    if ((segnode = (SEGMENT_NODE *)
		    malloc(sizeof(SEGMENT_NODE)*newsize))
		    == NULL) {
		fprintf(stderr,"Out of malloc space!");
		exit(1);
	    }
	}
	else {
	    if ((segnode = (SEGMENT_NODE *) realloc(segnode,
			sizeof(SEGMENT_NODE)*newsize))
		    == NULL) {
		fprintf(stderr,"Out of malloc space!");
		exit(1);
	    }
	}
	for (i=current_segnode_array_size; i<newsize; ++i) {
	    segnode[i].condition = CONDITION_DONT_CARE;
	    segnode[i].operation = OPERATION_EXECUTE;
	    segnode[i].is_leaf_node = FALSE;
	    segnode[i].firstchild = NULL;
	}
	current_segnode_array_size = newsize;
    }
}


void add_display_node(
    int parent_segment_number,
    int node_segment_number,	/* If INVALID, no associated segment */
    boolean_type has_matrix,    /* local matrix passed is valid */
    matrix3d local_xform,	/* local to node, concat'ed with parents' */
    float mc_extent[6],         /* MCs */
    float cull_size,            /* DCs */
    DISPLAY_NODE_CONDITION condition,
    boolean_type expected_condition_value,
    DISPLAY_NODE_OPERATION operation,
    boolean_type is_leaf_node)
{
    SEGMENT_NODE *sn,*parent;
    SEGMENT_INSTANCE *si;
    int maxnode;
    matrix3d *mat;

    DPRINTF8("Node %3d, parent %3d, %s = %d -> %s.  Mat? %d.  PEX #%d\n",
	    node_segment_number,
	    parent_segment_number,
	    (condition == CONDITION_DONT_CARE) ? "DONTCARE" :
		((condition == CONDITION_CHECK_PRUNE) ? "PRUNE" : "CULL"),
	    expected_condition_value,
	    (operation == OPERATION_EXECUTE) ? "EXEC" : "RETURN",
	    has_matrix,
	    0);
    DFLUSH;
    if ((node_segment_number < 0) && (node_segment_number != INVALID)) {
	DPRINTF1("NEGATIVE SEGMENT NUMBER IN add_display_node!!!\n");
	return;
    }
    if (parent_segment_number < 0) {
	DPRINTF1("NEGATIVE PARENT SEGMENT NUMBER IN add_display_node!!!\n");
	return;
    }

    /* Malloc space for this instance of a segment */
    if ((si = (SEGMENT_INSTANCE *)
	    fastmalloc(sizeof(SEGMENT_INSTANCE))) == NULL) {
	fprintf(stderr,"Out of malloc space!");
	exit(1);
    }

    /* Make sure the node array is big enough for it and its parent */
    maxnode = MAX(node_segment_number,parent_segment_number);
    if (maxnode >= current_segnode_array_size) {
	realloc_segnode_array(maxnode);
    }

    if (node_segment_number == INVALID) {
	if ((sn = (SEGMENT_NODE *)
		fastmalloc(sizeof(SEGMENT_NODE))) == NULL) {
	    fprintf(stderr,"Out of malloc space!");
	    exit(1);
	}
	sn->condition = CONDITION_DONT_CARE;
	sn->operation = OPERATION_EXECUTE;
	sn->is_leaf_node = FALSE;
	sn->firstchild = NULL;
	si->segnum = INVALID;
	si->segnode = (void *) sn;
    }
    else {
	/* Get a pointer to the appropriate node. */
	sn = segnode + node_segment_number;
	si->segnum = node_segment_number;
	si->segnode = NULL;
    }

    /* Update si */
    si->next = NULL;
    if (has_matrix) {
	if ((mat = (matrix3d *) fastmalloc(sizeof(matrix3d))) == NULL) {
	    fprintf(stderr,"Out of malloc space!");
	    exit(1);
	}
	memcpy((void *) mat,(void *) local_xform,sizeof(float)*16);
	si->local_xform = mat;
    }
    else si->local_xform = NULL;

    /* Attach it to the tree. */
    parent = segnode + parent_segment_number;
    if (parent->firstchild == NULL) {
	parent->firstchild = si;
	/* nodes with children do not display things */
	/* parent->pex_structure_number = INVALID; */
    }
    else {
	SEGMENT_INSTANCE *tsi;

	/* find last sibling of the parent's first child */
	tsi = parent->firstchild;
	while (tsi->next != NULL) tsi = tsi->next;
	tsi->next = si;
    }


    /* Copy the data to the node */
    memcpy((void *) sn->mc_extent,(void *) mc_extent,sizeof(float)*6);
    sn->cull_size = cull_size;
    sn->condition = condition;
    sn->expected_condition_value = expected_condition_value;
    sn->operation = operation;
    sn->is_leaf_node = is_leaf_node;
}


/*---------------------------------------------------------------------------*/
static void project_vector(
    float vx, float vy, float vz,
    float nx, float ny, float nz,
    float *px, float *py, float *pz)
{
    float nxsq,nysq,nzsq,nxvx,nyvy,nzvz,nmagxyz,vmagxyz,tx,ty,tz;

    nmagxyz = FSQRT(nx*nx + ny*ny + nz*nz);
    nx = nx/nmagxyz;   ny = ny/nmagxyz;   nz = nz/nmagxyz;
    vmagxyz = FSQRT(vx*vx + vy*vy + vz*vz);
    vx = vx/vmagxyz;   vy = vy/vmagxyz;   vz = vz/vmagxyz;
    nxsq = nx*nx;   nysq = ny*ny;   nzsq = nz*nz;
    nxvx = nx*vx;   nyvy = ny*vy;   nzvz = nz*vz;
    tx = vx * (nysq + nzsq) - nx * (nyvy + nzvz);
    ty = vy * (nxsq + nzsq) - ny * (nxvx + nzvz);
    tz = vz * (nxsq + nysq) - nz * (nxvx + nyvy);
    nmagxyz = FSQRT(tx*tx + ty*ty + tz*tz);
    if (((nx==vx) && (ny==vy) && (nz==vz)) || (nmagxyz == 0)) return;
    *px = tx/nmagxyz;   *py = ty/nmagxyz;   *pz = tz/nmagxyz;
}



static void view_camera_to_view_matrix(
    camera_arg *cam,
    matrix3d *cxform)
{
    float tmat[4][4];
    float nx,ny,nz;
    float s,magyz,magxyz;
    float sinx,cosx,siny;
    float cosy,sinz,cosz;
    float upx,upy,upz;
    float frontdist,backdist;
    float mindist;
    float zscale;


    /*camera sets the positional viewpoint for perspective back cull*/
    nx = cam->refx - cam->camx;		
    ny = cam->refy - cam->camy;		
    nz = cam->refz - cam->camz;

    project_vector(cam->upx,cam->upy,cam->upz,nx,ny,nz,&upx,&upy,&upz);
    magyz   = FSQRT(ny*ny + nz*nz);
    magxyz  = FSQRT(nx*nx + ny*ny + nz*nz);
    mindist = magxyz/(float)100.0;
    if (cam->front == cam->back) {
	frontdist = mindist;
	backdist  = magxyz*10.0;
    }
    else {
	if ((frontdist = magxyz+cam->front) < mindist)
	    frontdist = mindist;
	if ((backdist = magxyz+cam->back) < frontdist)
	    backdist = 1.001*frontdist;
    }
    zscale = 1.0/(backdist-frontdist);
    s = frontdist * FTAN(DEGREES_TO_RADIANS(cam->field_of_view)*0.5);

    memcpy(cxform,&identity,sizeof(matrix3d));
    /* Assume CAM_PERSPECTIVE */
    /* want z range to be 0-1 */
    frontdist *= zscale;  backdist *= zscale;
    s *= zscale;

#ifdef ADD_DC_COMPONENT
    /*Need isotropic mapping to the view_port*/
    xval = cstate.gWinWidth;  yval = cstate.gWinHeight;
    if (xval < yval) {
	scale = xval /((float)2.0*size);   
	yval  = -size*yval/xval ;   
	xval  = -size; 
    }
    else { 
	scale = yval/((float)2.0*size);   
	xval  = -size*xval/yval;   
	yval  = -size; 
    }

    (*cxform)[0][0] = scale;
    (*cxform)[1][1] = scale;
    (*cxform)[3][0] = -scale*xval;
    (*cxform)[3][1] = -scale*yval;
#endif /* ADD_DC_COMPONENT */
   
    /*   This section computes a 3D perspective matrix.  S is the
     *   size of 1/2 the front face of the pyramid, and frontdist &
     *   backdist are the distances to the front face and back face
     *   from the "eye".  See Newman and Sproull, Principles of
     *   Interactive Computer Graphics second edition, pg 356-359.
     */
    memcpy(&tmat,&identity,sizeof(matrix3d));
    tmat[2][2] = (s/(frontdist*(1.0 - frontdist/backdist)));
    tmat[2][3] = (s/frontdist);
    tmat[3][2] = -s/(1.0 - frontdist/backdist);
    tmat[3][3] = 0.0;
    concat_matrix(tmat,*cxform,*cxform);

    memcpy(&tmat,&identity,sizeof(matrix3d));
    tmat[0][0] = tmat[1][1] = tmat[2][2] = zscale;
    tmat[3][2] = zscale*magxyz;
    concat_matrix(tmat,*cxform,*cxform);

    /* Rotate and translate xyz into uvw */
    if (magyz != 0.0) {
	cosx = nz / magyz;
	sinx = ny / magyz;
    }
    else {
	cosx = 1.0;
	sinx = 0.0;
    }
    cosy = magyz / magxyz;
    siny = -nx / magxyz;
    sinz = upx*cosy + upy*sinx*siny + upz*cosx*siny; /*up already normalized*/
    cosz = upy*cosx - upz*sinx;
    memcpy(&tmat,&identity,sizeof(matrix3d));
    tmat[0][0] = cosy*cosz;
    tmat[0][1] = cosy*sinz;
    tmat[0][2] = (-siny);
    tmat[1][0] = sinx*siny*cosz - cosx*sinz;
    tmat[1][1] = (sinx*siny*sinz + cosx*cosz);
    tmat[1][2] = (sinx*cosy);
    tmat[2][0] = cosx*siny*cosz + sinx*sinz;
    tmat[2][1] = cosx*siny*sinz - sinx*cosz;
    tmat[2][2] = (cosx*cosy);
    concat_matrix(tmat,*cxform,*cxform);

    memcpy(&tmat,&identity,sizeof(matrix3d));
    tmat[3][0] = -cam->refx;
    tmat[3][1] = -cam->refy;
    tmat[3][2] = -cam->refz;
    concat_matrix(tmat,*cxform,*cxform);
}


/* Return TRUE iff the extent crosses into the view volume */
static boolean_type extent_in_view_volume(
    float wc_extent[6],
    matrix3d *viewmat)
{
    float predc_extent[6];

    /* Turn the wc_extent into [-1..1,-1..1,-1..1] post-viewing space. */
    transform_extent(wc_extent,viewmat,predc_extent);

    /* If it intersects that space, it's visible. */
    if ((predc_extent[0] <= 1.0) && (predc_extent[3] >= -1.0)
	    && (predc_extent[1] <= 1.0) && (predc_extent[4] >= -1.0)
	    && (predc_extent[2] <= 1.0) && (predc_extent[5] >= -1.0)) {
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}


static float diagonal_size(
    float wc_extent[6],
    matrix3d *viewmat)
{
    float predc_extent[6];

    /* Turn the wc_extent into [-1..1,-1..1,-1..1] post-viewing space. */
    transform_extent(wc_extent,viewmat,predc_extent);

    /* Return the diagonal in that space multiplied by DC factors */
    return(FHYPOT2(
	(predc_extent[3] - predc_extent[0]) * cstate.gWinWidth,
	(predc_extent[4] - predc_extent[1]) * cstate.gWinHeight));
}



typedef enum {
    CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES = 0,
    CONTINUE_WITH_SIBLINGS_DREW_NO_PRIMITIVES = 1,
    ABORT_SIBLINGS = 2
} SUBTREE_RESULT;

/* Internal recursive call to draw a SEGMENT_NODE tree */
static SUBTREE_RESULT draw_subtree(
    SEGMENT_INSTANCE *si,
    matrix3d *cumulative_xform,
    matrix3d *viewmat,
    boolean_type dynamic)
{
    matrix3d newmat,*cmat;
    float wc_extent[6];
    float dcdiagonal;
    SEGMENT_NODE *sn;
    SEGMENT_INSTANCE *child;
    int result,draw_parent_struct;

    if (si->segnum != INVALID) {
	sn = segnode + si->segnum;
    }
    else {
	sn = (SEGMENT_NODE *) (si->segnode);
    }

    DPRINTF2("draw_subtree(%d)",sn->segnum);

    if (si->local_xform != NULL) {
	concat_matrix(*(si->local_xform),*cumulative_xform,newmat);
	cmat = &(newmat);
	DPRINTF1(", + local matrix");
    }
    else {
	cmat = cumulative_xform;
    }

    if (sn->condition == CONDITION_CHECK_PRUNE) {
	transform_extent(sn->mc_extent,cmat,wc_extent);
	result = !extent_in_view_volume(wc_extent,viewmat);
	DPRINTF4(", prune = %d, %s if %d",
		result,
		(sn->operation == OPERATION_EXECUTE) ? "exec" : "return",
		sn->expected_condition_value);
	if (sn->operation == OPERATION_EXECUTE) {
	    if (result != sn->expected_condition_value) {
		DPRINTF1(", as if I drew them.\n");
		return(CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES);
	    }
	}
	else {
	    if (result == sn->expected_condition_value) {
		DPRINTF1(", abort siblings.\n");
		return(ABORT_SIBLINGS);
	    }
	    else {
		DPRINTF1(", continue sibs, drew nothing.\n");
		return(CONTINUE_WITH_SIBLINGS_DREW_NO_PRIMITIVES);
	    }
	}
    }
    else if (sn->condition == CONDITION_CHECK_CULL) {
	transform_extent(sn->mc_extent,cmat,wc_extent);
	dcdiagonal = diagonal_size(wc_extent,viewmat);
	result = (dcdiagonal <= sn->cull_size*cstate.cull_multiplier);
	DPRINTF4(", cull = %d, %s if %d\n",
		result,
		(sn->operation == OPERATION_EXECUTE) ? "exec" : "return",
		sn->expected_condition_value);
	if (sn->operation == OPERATION_EXECUTE) {
	    if (result != sn->expected_condition_value) {
		DPRINTF1(", as if I drew them.\n");
		return(CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES);
	    }
	}
	else {
	    if (result == sn->expected_condition_value) {
		DPRINTF1(", abort siblings.\n");
		return(ABORT_SIBLINGS);
	    }
	    else {
		DPRINTF1(", continue sibs, drew nothing.\n");
		return(CONTINUE_WITH_SIBLINGS_DREW_NO_PRIMITIVES);
	    }
	}
    }

    DPRINTF1("\n");
    DFLUSH;

    /* Call this on my children */
    draw_parent_struct = TRUE;
    child = (SEGMENT_INSTANCE *) (sn->firstchild);
    while (child != NULL) {
	if ((result = draw_subtree(child,cmat,viewmat,dynamic))
		== ABORT_SIBLINGS) {
	    DPRINTF2("  (%d) aborting child siblings\n",sn->segnum);
	    draw_parent_struct = FALSE;
	    break;
	}
	else if (result == CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES) {
	    DPRINTF2("  (%d) child drew, disabling parent pexstruct\n",
		sn->segnum);
	    draw_parent_struct = FALSE;
	}

	child = child->next;
    }

    /* If my children have already done the drawing, return that result */
    if (draw_parent_struct == FALSE) {
	DPRINTF2("segnum %d -- no draw (children drawn)!\n",sn->segnum);
	return(CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES);
    }
    else if (!(sn->is_leaf_node)) {
	return(CONTINUE_WITH_SIBLINGS_DREW_NO_PRIMITIVES);
    }
    else {
#if 0
	/* TBD: Dunno what this does... */
	refresh_segment(img_fildes,si->segnum);
#endif
	return(CONTINUE_WITH_SIBLINGS_DREW_PRIMITIVES);
    }
}


void draw_tree(
    int segment_number,
    camera_arg *cam,
    boolean_type dynamic,
    matrix3d *topmatrix)
{
    matrix3d viewxform;
    SEGMENT_INSTANCE si;
#ifdef TEST_CODE
    SEGMENT_NODE *n;
    SEGMENT_INSTANCE *i;
#endif /* TEST_CODE */

#ifdef TEST_CODE
    {
    camera_arg cam2;

    DPRINTF2("\nDRAW_TREE(%d)\n",segment_number);
    memcpy(&cam2,cam,sizeof(camera_arg));
    cam2.front *= .5;
    cam2.back *= .5;
    cam2.field_of_view *= .5;
    view_camera_to_view_matrix(&cam2,&viewxform);
    }
#else
    view_camera_to_view_matrix(cam,&viewxform);
#endif

    si.local_xform = NULL;
    si.segnum = segment_number;
    si.segnode = NULL;
    si.next = NULL;

    draw_subtree(&si,topmatrix,&viewxform,dynamic);
}

#ifdef TEST_CODE
static void pxp(
    matrix3d *xform,
    float x, float y, float z)
{
    float xt,yt,zt,wt;

    xt = x * (*xform)[0][0] +
	 y * (*xform)[1][0] +
	 z * (*xform)[2][0] +
	 (*xform)[3][0];
    yt = x * (*xform)[0][1] +
	 y * (*xform)[1][1] +
	 z * (*xform)[2][1] +
	 (*xform)[3][1];
    zt = x * (*xform)[0][2] +
	 y * (*xform)[1][2] +
	 z * (*xform)[2][2] +
	 (*xform)[3][2];
    wt = x * (*xform)[0][3] +
	 y * (*xform)[1][3] +
	 z * (*xform)[2][3] +
	 (*xform)[3][3];

     printf("%f,%f,%f,%f\n",xt,yt,zt,wt);
     if (ABS(wt) > 1e-12) printf("  %f,%f,%f\n",xt/wt,yt/wt,zt/wt);
     else printf("  wt too small for division\n");
     printf("\n");
}
#endif /* TEST_CODE */
