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


#ifndef _DISPLAY_CONTROL_INCLUDED
#define _DISPLAY_CONTROL_INCLUDED

/************************* MACROS and enums ***********************************/
typedef enum {
    CONDITION_DONT_CARE		= 0,
    CONDITION_CHECK_PRUNE	= 1,
    CONDITION_CHECK_CULL	= 2
} DISPLAY_NODE_CONDITION;

typedef enum {
    OPERATION_EXECUTE		= 0,
    OPERATION_RETURN		= 1
} DISPLAY_NODE_OPERATION;


/************************** FUNCTION PROTOTYPES *******************************/
#ifdef PEXDRIVE
/*** From ??? ***/
extern void draw_pex_structure(
    int structno,
    boolean_type dynamic,
    float matrix[4][4]);
#endif /* PEXDRIVE else */

/*** From disp_ctrl.c ***/
extern void add_display_node(
    int parent_segment_number,
    int segment_number,
    boolean_type has_matrix,	/* local matrix passed is valid */
    float matrix[4][4],		/* local to node, concat'ed with parents' */
    float mc_extent[6],		/* MCs */
    float cull_size,		/* DCs */
    DISPLAY_NODE_CONDITION condition,
    boolean_type expected_condition_value,
    DISPLAY_NODE_OPERATION operation,
#ifdef PEXDRIVE
    int pex_structure_number);	/* INVALID if not leaf node */
#else
    boolean_type is_leaf_node);
#endif /* PEXDRIVE else */

extern void draw_tree(
    int segment_number,
    camera_arg *cam,
    boolean_type dynamic,
    matrix3d *topmatrix);



#endif /* _DISPLAY_CONTROL_INCLUDED */
