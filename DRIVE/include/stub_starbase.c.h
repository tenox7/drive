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


/* stub_starbase.c.h */

#if !defined(_STUB_STARBASE_C_H_INCLUDED)
#define _STUB_STARBASE_C_H_INCLUDED 1

extern void add_names_to_set(int,int,int*);
extern void close_segment(int);
extern void concat_transformation3d(int,float[4][4],int,int);
extern void cond_execute_segment(int,int,int,int);
extern void cond_return(int,int,int);
extern void curve_resolution(int,int,float,float,float,float);
extern void default_knots(int);
extern void define_trimming_curve(int,float*,int,int,int);
extern void execute_segment(int,int);
extern void fill_color(int, float,float,float);
extern void hidden_surface(int, int, int);
extern void inquire_curve_resolution(int,int*,float*,float*,float*,float*);
extern void inquire_vertex_format(int,int*,int*,int*,int*,int*);
extern void line_color(int,float,float,float);
extern void open_segment(int,int,int,int);
extern void partial_polygon3d(int,float*,int,int,int);
extern void polygon3d(int,float*,int,void *);
extern void polyline3d(int,float*,int,int);
extern void pop_matrix(int);	
extern void quadrilateral_mesh(int,float*,int,int,void *);
extern void remove_all_names_from_set(int);
extern void set_cull_size(int,float);
extern void set_extent(int,float[2][3]);
extern void spline_surface(int,float*,int,int,int,int,int);
extern void surface_model(int,int,int,float,float,float);
extern void triangular_strip(int,float*,int,void*);
extern void u_knot_vector(int,float *,int);
extern void v_knot_vector(int,float *,int);
extern void vertex_format(int, int,int,int,int,int);

#endif /* !_STUB_STARBASE_C_H_INCLUDED) */
