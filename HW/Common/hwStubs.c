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

/* Stubs to build a "libhwstub.a" in case you want all the fun Hoverware
 * utilities but none of the objects (*cough*OADL*cough*).
 */

#include "hw.h"

int __hwIntInitOrient(void) {return 1;}

int __hwIntInitSurf(void) {return 1;}

hwObject
    hwBox,
    hwButton,
    hwCamera,
    hwCone,
    hwDisc,
    hwEnviron,
    hwFile,
    hwFont,
    hwGauge,
    hwGroup,
    hwImage,
    hwLabel,
    hwLayout,
    hwLight,
    hwMesh,
    hwOrient,
    hwPolygon,
    hwPolyline,
    hwPolymarker,
    hwQuads,
    hwRing,
    hwRowCol,
    hwSphere,
    hwSpinner,
    hwStrip,
    hwSurfRev,
    hwSurface,
    hwSweep,
    hwText,
    hwTexture,
    hwTimer,
    hwToggle,
    hwTorus,
    hwTriangles;
