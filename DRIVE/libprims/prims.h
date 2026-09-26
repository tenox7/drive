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



/*                     SPLINE_CONE
 *
 * This routine draws a cone or cylinder with optional end caps.
 *
 * fildes--int, file descriptor
 * b_rad --float, bottom radius
 * t_rad --float, top radius
 * b_cap --int, cap the bottom?
 * t_cap --int, cap the top?
 * xb,yb,zb -- float, location of the center of the bottom
 * xt,yt,zt -- float, location of the center of the top
 */
extern void spline_cone(
    int fildes,
    float b_rad, float t_rad,
    int   b_cap, int   t_cap,
    float xb, float yb, float zb,
    float xt, float yt, float zt);


/*                   MESH_TO_STRIPS
 *
 * Converts quad mesh to triangle strips.
 */
extern void mesh_to_strips(
    int fildes,
    float *mesh,
    int rows, int columns,
    float *gnormals,
    int cpv, int direction);

/* Same as above, but gets cpv from an inquiry.  This won't work if
 * going into a non-display display-list, since the info isn't up to date.
 */
extern void quad_to_strips(
    int fildes,
    float *mesh,
    int rows, int columns,
    float *gnormals);


/*                     MESH_CONE
 *
 * This routine draws a cone or cylinder with optional end caps.
 *
 * fildes--int, file descriptor
 * b_rad --float, bottom radius
 * t_rad --float, top radius
 * b_cap --int, cap the bottom?
 * t_cap --int, cap the top?
 * facets   -- int, number of facets
 * xb,yb,zb -- float, location of the center of the bottom
 * xt,yt,zt -- float, location of the center of the top
 */
extern void mesh_cone(
    int fildes,
    float b_rad, float t_rad,
    int   b_cap, int   t_cap,
    int facets,
    float xb, float yb, float zb,
    float xt, float yt, float zt);


/*                      ARROW
 *
 * Draws a 3D arrow from x1,y1,z1 to x2,y2,z2 in color rgb with a
 * tip of a mesh cone of height "coneheight".
 */
extern void arrow(
    int fildes,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float coneheight);



/*                     CIRCLE
 *
 * This routine draws a circle.
 *
 * fildes   -- int, file descriptor
 * radius   -- float, circle radius
 * facets   -- int, number of facets
 * partial  -- bool, use partial_polygon instead of polygon
 * xc,yc,zc -- float, location of the center of circle
 * xn,yn,zn -- float, point on normal vector from center
 */
extern void circle(
    int fildes,
    float radius,
    int facets,
    int partial,
    float xc, float yc, float zc,
    float xn, float yn, float zn);



/*                     EXTRUSION
 *
 * This procedure forms an extruded object by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains equal to the normal of the original
 * polygon, thus, the back face will be "parallel" to the front face,
 * regardless of the path. Each triple in "path" is a vector relative
 * to the origin.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 *
 */
extern void extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int front_face, int back_face);

extern void tube_extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int front_face, int back_face);


/*                     SPLINE_EXTRUSION
 *
 * This procedure forms a spline surface by sweeping the polygon
 * given in clist along a path.  At each step of the path, the normal 
 * of the offset polygon remains equal to the normal of the original
 * polygon, thus, the back face will be "parallel" to the front face,
 * regardless of the path. Each triple in "path" is a vector relative
 * to the origin.  The v-knot vectors are set so that the extrusion
 * front and back faces are exactly those specified by clist and 
 * clist offset by the last vector in the path.  A number of vertices
 * may be duplicated at the end of the polygon to insure that the
 * curve is closed.
 *
 * fildes     -- integer; file descriptor
 * clist      -- float*; a simple list of the polygon's vertices
 * poly_size  -- integer; the number of vertices in clist
 * path       -- float*; a list of vectors, relative to the origin,
 *                to offset clist by to get the next set of control
 *                points in the extrusion
 * path_size  -- integer; the number of vectors in path
 * order_u    -- integer; the order of the spline curve going around
 *                the vertices of clist
 * order_v    -- integer; the order of the spline curve in the 
				  direction of the extrusion
 * front_face -- boolean; whether to draw the front face
 * back_face  -- boolean; whether to draw the back face
 * rational   -- boolean; whether the points in clist are rational
 *                b-spline control points (4 floats per vertex)
 * raw        -- boolean; if true, do not duplicate points or change
 *                the u_knot_vector
 *
 */
extern void spline_extrusion(
    int fildes,
    float *clist, int poly_size,
    float *path,  int path_size,
    int order_u,  int order_v,
    int front_face, int back_face,
    int rational,
    int raw);


/*                              SPLINE_SPHERE
 *
 * This procedure draws a rational spline-surface sphere of specified
 * radius centered at x,y,z.
 *
 * fildes -- integer; file descriptor
 * radius -- float; sphere radius
 * x,y,z  -- floats; the center of the sphere
 */
extern void spline_sphere(
    int fildes,
    float radius,
    float x, float y, float z);


/*                     SPLINE_SURFACE_OF_REVOLUTION
 *
 * This procedure forms a rational spline surface by rotation the
 * specified polygon around an arbitrary axis. The control points can
 * be in rational form and u_knot_vector can be specified by the user.
 * If it doesn't look right, try changing the axis to point the other
 * direction.
 *
 * fildes    -- integer; file descriptor
 * poly      -- float*; the control polygon
 * poly_size -- integer; the number of vertices in poly
 * u_order   -- integer;the order of the curve in the u direction, which
 *                corresponds to poly and the cross section of the 
 *                surface.
 * rational  -- boolean; whether the vertices are in rational form
 *                (four floats per vertex)
 * x1,y1,z1,x2,y2,z2 -- floats; the axis of rotation
 */
extern void spline_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int u_order,
    int rational,
    float x1, float y1, float z1,
    float x2, float y2, float z2);


/*                     MESH_SURFACE_OF_REVOLUTION
 *
 * This procedure forms a rational mesh surface by rotation the
 * specified polygon around an arbitrary axis. The control points can
 * be in rational form and u_knot_vector can be specified by the user.
 *
 * fildes    -- integer; file descriptor
 * poly      -- float*; the control polygon
 * poly_size -- integer; the number of vertices in poly
 * facets    -- number of facets in the revolution
 * normals   -- are there normals in the data?
 * x1,y1,z1,x2,y2,z2 -- floats; the axis of rotation
 */
extern void mesh_surface_of_revolution(
    int fildes,
    float *poly, int poly_size,
    int facets,
    int normals,
    float x1, float y1, float z1,
    float x2, float y2, float z2);

extern void mesh_torus(
    int fildes,
    int facets,
    int rev_steps,
    float minor_radius, float major_radius,
    float x1, float y1, float z1,
    float x2, float y2, float z2);

extern void mesh_helix(
    int fildes,
    int facets,
    int rev_steps,
    float minor_radius, float major_radius,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float times_around);


/*
 *                         MESH_SPHERE
 *
 */
extern void mesh_sphere(
    int fildes,
    float radius,
    int latitudes, int longitudes,
    float x, float y, float z);


/*                     TRIMMED_PLANE
 * 
 * This routine draws a planar polygon trimmed by the spline curve
 * defined by clist.  Points will be added as needed to insure that
 * the polygon is closed.
 *
 * fildes    -- integer; file descriptor
 * clist     -- float*; the control points of the trimming curve
 * poly_size -- integer; the number of vertices in clist
 * order     -- integer; the order of the trimming curve
 * rational  -- boolean; whether the control points are rational
 *               (4 floats per vertex)
 * raw       -- boolean; if true, the input polygon is already closed
 *               and use the current u_knot_vector.
 */
extern void trimmed_plane(
    int fildes,
    float *clist, int poly_size,
    int order,
    int rational,
    int raw);


/* THE FOLLOWING ROUTINES KEEP TRACK OF THE CURRENT STATE OF
 * CERTAIN ITEMS FOR PUSH/POP PURPOSES.  THIS COULD BE DONE THROUGH
 * inquires, BUT THAT WOULD NOT WORK IN AN UNDISPLAYED SEGMENT OF
 * THE DISPLAY LIST.  THE ORIGINAL STARBASE ROUTINE IS CALLED AFTER
 * THE STATE IS UPDATED.
 */

#define u_knot_vector _prim_u_knot_vector
#define v_knot_vector _prim_v_knot_vector
#define default_knots _prim_default_knots
#define vertex_format _prim_vertex_format

extern void _prim_vertex_format(
    int fildes,
    int coord,
    int use,
    int rgb,
    int normals,
    int order);
extern void _prim_default_knots(
    int fildes);
extern void _prim_u_knot_vector(
    int fildes,
    float *knot_vector,
    int numpts);
extern void _prim_v_knot_vector(
    int fildes,
    float *knot_vector,
    int numpts);
