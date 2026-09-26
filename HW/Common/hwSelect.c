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

#include "hw.h"
#include "hw_internal.h"
#include "hw_vecmath.h"

#define EPSILON 0.00001f

hwInt32 __hwIntPickMarkers
(
    hwFloat *verts, hwInt32 numVerts, hwInt32 vn,
    hwSelectionType *sel
);

static hwFloat paramInterp(
    hwFloat v0, hwFloat v1, hwFloat v2,
    hwFloat a, hwFloat b
)
{
    return v0 + a*v1 + b*v2;
}

static hwInt32 lineSelected(
    hwFloat *p0, hwFloat *p1, hwSelectionType *sel
)
{
    hwFloat
        px, py,
        dx, dy,
        sx, sy,
        t, d;

    px = p0[0];         py = p0[1];
    dx = p1[0] - px;    dy = p1[1] - py;
    sx = px - sel->point[0];    sy = py - sel->point[1];

    t = dx*dx + dy*dy;
    if( t >= EPSILON ) {
        /* Find time of closest approach */
        t = -(dx*sx + dy*sy) / t;
        if( t < 0.f ) t = 0.f;
        if( t > 1.f ) t = 1.f;

        /* Calculate point on line */
        px += t * dx;
        py += t * dy;
    }
    else {
        if( p0[2] < p1[2] )     t = 0.f;
        else                    t = 1.f;
    }

    /* See if it's close enough */
    dx = px - sel->point[0];
    dy = py - sel->point[1];
    d = dx*dx + dy*dy;
    if( d > (sel->aperture*sel->aperture) ) {
        /* Nope */
        return 0;
    }

    /* Calculate Z depth */
    d = p0[2] + t*(p1[2] - p0[2]);

    if( (d >= 0.f) && (d < sel->dist) ) {
        /* Aha! A pick. */
        sel->dist = d;
        return 1;
    }

    return 0;
}

static hwInt32 triSelected(
    hwSelectionType *sel, hwFloat *v0, hwFloat *v1, hwFloat *v2
)
{
    hwFloat
        vec1[3], vec2[3],
        sx, sy, denom,
        a, b, t;
    hwInt32
        selEdges = 0,
        result;

    /* Create vectors for the two edges */
    if( !(sel->flags & HW_SELECT_EDGES) ) {
        VSUB( vec1, v1, v0 );
        VSUB( vec2, v2, v0 );

        /* Create parametric ratios */
        denom = vec2[0]*vec1[1] - vec2[1]*vec1[0];
        if( (denom > -EPSILON) && (denom < EPSILON) ) {
            /* Oops - zero area triangle.  Do a line selection */
            selEdges = 1;
        }
    }
    else {
        selEdges = 1;
    }
    if( selEdges ) {
        result = lineSelected( v0, v1, sel );
        result |= lineSelected( v1, v2, sel );
        result |= lineSelected( v2, v0, sel );
        return result;
    }

    denom = 1.f / denom;
    sx = sel->point[0] - v0[0];
    sy = sel->point[1] - v0[1];
    a = (sy*vec2[0] - sx*vec2[1]) * denom;
    b = (sx*vec1[1] - sy*vec1[0]) * denom;

    /* Check for outside of triangle */
    if( (a < 0.f) || (b < 0.f) || ((a + b) > 1.f) ) return 0;

    t = paramInterp( v0[2], vec1[2], vec2[2], a, b );
    if( (t < 0.f) || (t > sel->dist) )  return 0;

    sel->dist = t;
    return 1;
}


hwInt32 __hwIntPickStrip
(
    hwFloat *data, hwInt32 vn, hwInt32 n,
    hwSelectionType *sel
)
{
    hwInt32
        i,
        found;
    hwFloat
        p0[3], p1[3], p2[3],
        *ptr0, *ptr1, *ptr2;

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( data, n, vn, sel );
    }

    /* Initialize output record */
    found = 0;

    hwTransformPersp( p0, data, sel->matrix );          data += vn;
    hwTransformPersp( p1, data, sel->matrix );          data += vn;
    hwTransformPersp( p2, data, sel->matrix );

    /* For triangle in the strip */
    ptr0 = p0; ptr1 = p1; ptr2 = p2;
    for( i = 2; i < n; i++ ) {
        if( triSelected( sel, p0, p1, p2 ) ) found = 1;
        ptr0 = ptr1; ptr1 = ptr2;
        switch( i % 3 ) {
        case 0 :        ptr2 = p1;      break;
        case 1 :        ptr2 = p2;      break;
        case 2 :        ptr2 = p0;      break;
        }
        data += vn;
        hwTransformPersp( ptr2, data, sel->matrix );
    }
    return found;
}

hwInt32 __hwIntPickMesh
(
    hwFloat *data, hwInt32 vn, hwInt32 n, hwInt32 m,
    hwSelectionType *sel
)
{
    hwInt32
        i, j,
        found;
    hwFloat
        p0[3], p1[3],
        p2[3], p3[3],
        *ptr0, *ptr1,
        *ptr2, *ptr3,
        *src0, *src1;

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( data, n*m, vn, sel );
    }

    /* Get pointers to first two rows */
    src0 = data;        src1 = data + m*vn;

    /* Initialize output record */
    found = 0;

    /* For each row of triangles in the mesh */
    for( i = 1; i < n; i++ ) {
        /* For each pair of triangles in the row */
        ptr0 = p0; ptr1 = p1;
        ptr2 = p2; ptr3 = p3;
        hwTransformPersp( p0, src0, sel->matrix ); src0 += vn;
        hwTransformPersp( p1, src0, sel->matrix );
        hwTransformPersp( p2, src1, sel->matrix ); src1 += vn;
        hwTransformPersp( p3, src1, sel->matrix );
        for( j = 1; j < m; j++ ) {
            if( triSelected( sel, ptr0, ptr2, ptr1 ) ) found = 1;
            if( triSelected( sel, ptr2, ptr1, ptr3 ) ) found = 1;
            ptr0 = ptr1; ptr2 = ptr3;
            if( j & 1 ) {
                ptr1 = p0; ptr3 = p2;
            }
            else {
                ptr1 = p1; ptr3 = p3;
            }
            src0 += vn; src1 += vn;
            hwTransformPersp( ptr1, src0, sel->matrix );
            hwTransformPersp( ptr3, src1, sel->matrix );
        }
    }
    return found;
}

hwInt32 __hwIntPickPolygon
(
    hwFloat *polyData, hwInt32 vn, hwInt32 numVerts,
    hwSelectionType *sel
)
{
    hwInt32
        i,
        found;
    hwFloat
        p0[3], p1[3], p2[3];

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( polyData, numVerts, vn, sel );
    }

    /* Initialize output record */
    found = 0;

    hwTransformPersp( p0, polyData, sel->matrix );      polyData += vn;
    hwTransformPersp( p1, polyData, sel->matrix );      polyData += vn;
    hwTransformPersp( p2, polyData, sel->matrix );

    /* For each triangle in the polygon... */
    for( i = 2; i < numVerts; i++ ) {
        if( triSelected( sel, p0, p1, p2 ) ) found = 1;
        /* Skip to next triangle */
        polyData += vn;
        if( i & 1 )     hwTransformPersp( p2, polyData, sel->matrix );
        else            hwTransformPersp( p1, polyData, sel->matrix );
    }
    return found;
}

hwInt32 __hwIntPickPolyline
(
    hwFloat *data, hwInt32 vn, hwInt32 n,
    hwInt32 closed, hwSelectionType *sel
)
{
    hwInt32
        i, found;
    hwFloat
        p0[3], p1[3];

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( data, n, vn, sel );
    }

    found = 0;
    hwTransformPersp( p0, data, sel->matrix );
    if( closed ) {
        hwTransformPersp( p1, data + (n-1)*vn, sel->matrix );
        if( lineSelected( p0, p1, sel ) ) found = 1;
    }
    for( i = 1; i < n; i++, data += vn ) {
        if( lineSelected( p0, p1, sel ) ) found = 1;
        if( i & 1 )     hwTransformPersp( p0, data, sel->matrix );
        else            hwTransformPersp( p1, data, sel->matrix );
    }

    return found;
}

hwInt32 __hwIntPickQuads
(
    hwFloat *data, hwInt32 vn, hwInt32 n,
    hwSelectionType *sel
)
{
    hwInt32
        i,
        found;
    hwFloat
        p0[3], p1[3],
        p2[3], p3[3];

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( data, 4*n, vn, sel );
    }

    /* Initialize output record */
    found = 0;

    /* For each triangle in the mesh... */
    for( i = 0; i < n; i++ ) {
        hwTransformPersp( p0, data     , sel->matrix );
        hwTransformPersp( p1, data+  vn, sel->matrix );
        hwTransformPersp( p2, data+2*vn, sel->matrix );
        hwTransformPersp( p3, data+3*vn, sel->matrix );

        if( triSelected( sel, p0, p1, p2 ) )    found = 1;
        if( triSelected( sel, p0, p2, p3 ) )    found = 1;

        data += 4*vn;
    }
    return found;
}

hwInt32 __hwIntPickIndexedTris
(
    hwFloat *verts, hwInt32 numVerts, hwInt32 vn,
    hwInt32 *indexList, hwInt32 numTris,
    hwSelectionType *sel
)
{
    hwInt32
        i,
        found;
    hwFloat
        p0[3], p1[3], p2[3];

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( verts, numVerts, vn, sel );
    }

    found = 0;

    /* For each triangle in the mesh... */
    for( i = 0; i < numTris; i++ ) {
        hwTransformPersp( p0, verts + vn*indexList[0], sel->matrix );
        hwTransformPersp( p1, verts + vn*indexList[1], sel->matrix );
        hwTransformPersp( p2, verts + vn*indexList[2], sel->matrix );
        if( triSelected( sel, p0, p1, p2 ) ) found = 1;

        /* Skip to next triangle */
        indexList += 3;
    }
    return found;
}

hwInt32 __hwIntPickPolyhedron
(
    hwFloat *verts, hwInt32 numVerts, hwInt32 vn,
    hwInt32 *indexList, hwInt32 numPolys,
    hwSelectionType *sel
)
{
    hwInt32
        i, n,
        *ptr,
        found;
    hwFloat
        p0[3], p1[3], p2[3], p3[3];

    if( sel->flags & HW_SELECT_VERTICES ) {
        return __hwIntPickMarkers( verts, numVerts, vn, sel );
    }

    found = 0;

    /* For each polygon in the mesh (we only handle tris + quads)... */
    ptr = indexList;
    for( i = 0; i < numPolys; i++ ) {
        ptr++; n = *ptr++;
        switch( n ) {
        case 3 :
            hwTransformPersp( p0, verts + vn*ptr[0], sel->matrix );
            hwTransformPersp( p1, verts + vn*ptr[1], sel->matrix );
            hwTransformPersp( p2, verts + vn*ptr[2], sel->matrix );
            if( triSelected( sel, p0, p1, p2 ) ) found = 1;
            break;
        case 4 :
            hwTransformPersp( p0, verts + vn*ptr[0], sel->matrix );
            hwTransformPersp( p1, verts + vn*ptr[1], sel->matrix );
            hwTransformPersp( p2, verts + vn*ptr[2], sel->matrix );
            hwTransformPersp( p2, verts + vn*ptr[3], sel->matrix );
            if( triSelected( sel, p0, p1, p2 ) ) found = 1;
            if( triSelected( sel, p0, p2, p3 ) ) found = 1;
            break;
        }
        ptr += n;
    }
    return found;
}

hwInt32 __hwIntPickMarkers
(
    hwFloat *verts, hwInt32 numVerts, hwInt32 vn,
    hwSelectionType *sel
)
{
    hwInt32
        i,
        best;
    hwFloat
        dist, d2,
        pt[3],
        dx, dy,
        sx, sy,
        minDist;

    minDist = sel->dist;
    best = -1;

    sx = sel->point[0];
    sy = sel->point[1];
    d2 = sel->aperture*sel->aperture;

    /* Step through list, seeing if we meet intersection crieteria... */
    for( i = 0; i < numVerts; i++, verts += vn ) {
        hwTransformPersp( pt, verts, sel->matrix );
        dx = pt[0] - sx;        dy = pt[1] - sy;
        dist = dx*dx + dy*dy;
        if( (dist < d2) && (verts[2] > 0.f) && (verts[2] < minDist) ) {
            /* A possible hit */
            best = i;
            minDist = pt[2];
        }
    }

    /* See if we found a "hit" */
    if( best < 0 ) return 0;

    /* Finally! Return success. */
    sel->dist = minDist;
    sel->vert = best;
    return 1;
}

/*** EOF hwSelect.c ***/
