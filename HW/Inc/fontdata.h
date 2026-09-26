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

#define FC_CONT_MAX     8       /* Max # of font char contours */

struct FontChar {
    hwInt32
        NumVerts;       /* Number of vertices for this character */
    const hwFloat
        *Verts;         /* Pointer to XYZNNN vertex data */
    hwInt32
        NumTris;        /* Number of facets (triangles) in this character */
    const hwInt32
        *Tris;          /* Pointer to facet description list */
    hwFloat
        CharWidth;      /* Width of this character (Modelling coordinates */
    hwInt32
        NumEdges;       /* Number of outside/inside edge lists for character */
    const hwInt32
        *Edges;         /* Pointer to Edge information array */

    /* Information below here is a result of Cooking... */
    hwInt32
        Cooked,         /* Have we calculated extrusion and tex verts yet? */
        StripCounts[FC_CONT_MAX]; /* Size of *StripData, below */
    hwFloat
        *StripData[FC_CONT_MAX],  /* Pointers to edge tristrip vertex lists */
        *TexVerts,      /* Pointer to Textured (ST or RGB) vertex data */
        *TexStripData[FC_CONT_MAX]; /* Ptrs to tex. edge tristrip vtx lists */
};

extern struct FontChar
    __hwIntFont[];
