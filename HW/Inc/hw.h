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

/* Get OS definitions, header files, etc. that HW needs */
#include "hw_os.h"

/* Get typedefs, #defines, etc. */
#include "hw_types.h"

/* Get definition of display class */
#include "hw_display.h"

/* Get function declarations */
#include "hw_protos.h"

/* Get strings */
#include "hw_strings.h"

extern hwInt32
    hwDefaultOptFlags;  /* Default optimization level */

extern char 
    *hwDefaultDataFilePath;  /* Default location of texture, other Data files */

/* Built-in classes */
extern hwObject
    hwData,             /* Generic array of data */
    hwCamera,           /* Eye position, orientation */
    hwLight,            /* A single light */
    hwSpinner,          /* An animation object */
    hwTimer,            /* Control the passage of time */
    hwFile,             /* A reference to a file containing other objects */
    hwEnviron,          /* Environmental settings (depth cue, etc.) */
    hwImage,            /* A raster image, for textures, etc. */
    hwTexture,          /* A texture map, for subsequent use */
    hwSphere,           /* Sphere with rad, lat[01], lon[01] */
    hwCone,             /* Cone with rad0, rad1, height, lon[01] */
    hwRing,             /* Ring with rad0, rad1, lon[01] */
    hwTorus,            /* Torus with rad0, rad1, lat[01], lon[01] */
    hwDisc,             /* Disc with rad, lon[01] */
    hwGroup,            /* Group of primitives */
    hwBox,              /* Box with [xyz]min and [xyz]max */
    hwSurfRev,          /* Surface of revolution */
    hwSweep,            /* Sweep a shape about a path */
    hwText,             /* A text string with extrusion */
    hwText2D,           /* A 2D bitmap-font text string */
    hwMesh,             /* A N x M array of vertices */
    hwPolygon,          /* A convex polygon */
    hwPolyline,         /* A connected list of line segments */
    hwPolymarker,       /* A set of dots */
    hwTriangles,        /* A pile o' triangles */
    hwQuads,            /* A pile o' quads */
    hwStrip,            /* A Triangle Strip */
    hwSurface,          /* Conveniently packaged surface attributes */
    hwOrient;           /* Conveniently packaged orientation values */

/* GUI classes */
extern hwObject
    hwFont,             /* Texture font object */
    hwButton,           /* Command push button */
    hwGauge,            /* Circular analog gauge */
    hwJoystick,         /* Joystick mapper */
    hwLabel,            /* Text label */
    hwLayout,           /* Arbitrary gadget layout */
    hwRowCol,           /* Row/col layout widget */
    hwToggle;           /* Toggle button */
