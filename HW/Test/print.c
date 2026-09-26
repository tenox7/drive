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
#include <stdlib.h>
#include <math.h>
#include "hw.h"

int
    backface;
hwInt32
    animMask = 0xFFFFFFFF,
    invisibleInclude,
    invisibleExclude;
float
    scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0,
    biasX = 0.0, biasY = 0.0, biasZ = 0.0,
    minX = 1e38, minY = 1e38, minZ = 1e38,
    maxX = -1e38, maxY = -1e38, maxZ = -1e38;

static void printMesh
(
    hwDisplay disp,
    hwFloat *data,
    hwInt32 dataFlags,
    hwInt32 n,
    hwInt32 m
)
{
    int
        i, j, vn,
        normalOffset = 0;
    hwFloat
        *next;

    vn = hwCalcWPV( dataFlags );

    next = data + vn*m;

    for( i = 1; i < n; i++ ) {
        printf( "b GL_TRIANGLE_STRIP\n" );
        for( j = 0; j < m; j++ ) {
            if( normalOffset ) {
                printf( "n %g %g %g\n",
                            data[normalOffset  ],
                            data[normalOffset+1],
                            data[normalOffset+2] );
            }
            if( data[0] < minX ) minX = data[0];
            if( data[1] < minY ) minY = data[1];
            if( data[2] < minZ ) minZ = data[2];
            if( data[0] > maxX ) maxX = data[0];
            if( data[1] > maxY ) maxY = data[1];
            if( data[2] > maxZ ) maxZ = data[2];
            printf( "v %g %g %g\n",
                        biasX + scaleX*data[0],
                        biasY + scaleY*data[1],
                        biasZ + scaleZ*data[2] );
            data += vn;

            if( normalOffset ) {
                printf( "n %g %g %g\n",
                            next[normalOffset  ],
                            next[normalOffset+1],
                            next[normalOffset+2] );
            }
            if( next[0] < minX ) minX = next[0];
            if( next[1] < minY ) minY = next[1];
            if( next[2] < minZ ) minZ = next[2];
            if( next[0] > maxX ) maxX = next[0];
            if( next[1] > maxY ) maxY = next[1];
            if( next[2] > maxZ ) maxZ = next[2];
            printf( "v %g %g %g\n",
                        biasX + scaleX * next[0],
                        biasY + scaleY * next[1],
                        biasZ + scaleZ * next[2] );
            next += vn;
        }
        printf( "e\n" );
    }
}

static void printPoly
(
    hwDisplay disp,
    hwFloat *data,
    hwInt32 dataFlags,
    hwInt32 n
)
{
    int
        i, vn,
        normalOffset = 0;

    vn = hwCalcWPV( dataFlags );

    printf( "b GL_POLYGON\n" );
    for( i = 0; i < n; i++ ) {
        if( normalOffset ) {
            printf( "n %g %g %g\n",
                        data[normalOffset  ],
                        data[normalOffset+1],
                        data[normalOffset+2] );
        }
        if( data[0] < minX ) minX = data[0];
        if( data[1] < minY ) minY = data[1];
        if( data[2] < minZ ) minZ = data[2];
        if( data[0] > maxX ) maxX = data[0];
        if( data[1] > maxY ) maxY = data[1];
        if( data[2] > maxZ ) maxZ = data[2];
        printf( "v %g %g %g\n",
                                biasX + scaleX * data[0],
                                biasY + scaleY * data[1],
                                biasZ + scaleZ * data[2] );
        data += vn;
    }
    printf( "e\n" );
}

static void printPushMat( hwDisplay disp, hwFloat mat[4][4] )
{
    printf( "# Push matrix\n" );
}

static void printPopMat( hwDisplay disp )
{
    printf( "# Pop matrix\n" );
}

static void printSurfAttrs( hwDisplay disp, hwSurfaceType *surf )
{
    int
        newBackface;

    if( !(surf->flags & HW_SURF_UNCOLORED) ) {
        printf( "c %g %g %g\n",
                surf->color[0], surf->color[1], surf->color[2] );
    }
    newBackface = surf->flags & HW_SURF_BACKFACE;
    if( newBackface != backface ) {
        printf( "sb %d\n", newBackface );
        backface = newBackface;
    }
}

int printBoundsVisible
(
    hwDisplay disp, hwSurfaceType *surf, hwFloat BBox[6], hwInt32 complexity
)
{
    unsigned long
        srcMask, dstMask;

    if( surf->flags & HW_SURF_INVISIBLE )       return 0;

    if( animMask ) {
        /* Check for Hoverball-style animation - all of the bits
         * in the animMask MUST be present in the surface mask
         */
        srcMask = (unsigned long)animMask;
        dstMask = (unsigned long)surf->visibility;
        if( (srcMask & dstMask) != srcMask ) {
            return 0;
        }
    }
    else {
        /* Check for DRIVE-style animation:
         *      "Graphical primitives will not be drawn whenever the current
         *      name set includes any of the names in the invisibility
         *      filter's inclusion set but doesn't include any of the names
         *      in the invisibility filter's exclusion set."
         */
        dstMask = surf->visibility;
        if( invisibleInclude & dstMask ) {
            if( !(invisibleExclude & dstMask) ) {
                return 0;
            }
        }
    }

    return 1;
}

void printSetVisibility( hwDisplay disp, hwInt32 mask )
{
    animMask = mask;
}

void printSetInvisibility( hwDisplay disp, hwInt32 include, hwInt32 exclude )
{
    animMask = 0;
    invisibleInclude = include;
    invisibleExclude = exclude;
}

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwObject
        *objects;
    int
        i, numObjects;
    hwInt32
        invisibility = 0, excl = 0,
        visibility = 0xFFFFFFFF;
    char
        *filename = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-vis" ) == 0 ) {
            sscanf( argv[i+1], "%x", &visibility );
            i++;
        }
        else if( strcmp( argv[i], "-invis" ) == 0 ) {
            sscanf( argv[i+1], "%x", &invisibility );
            sscanf( argv[i+2], "%x", &excl);
            visibility = 0;
            i += 2;
        }
        else if( strcmp( argv[i], "-scale" ) == 0 ) {
            sscanf( argv[i+1], "%f", &scaleX );
            sscanf( argv[i+2], "%f", &scaleY );
            sscanf( argv[i+3], "%f", &scaleZ );
            i += 3;
        }
        else if( strcmp( argv[i], "-bias" ) == 0 ) {
            sscanf( argv[i+1], "%f", &biasX );
            sscanf( argv[i+2], "%f", &biasY );
            sscanf( argv[i+3], "%f", &biasZ );
            i += 3;
        }
        else if( *argv[i] != '-' ) {
            filename = argv[i];
        }
    }
    if( !filename )     exit( 1 );

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );

    if( !disp->chooseVisual( disp, HW_VIS_DEPTH, 0 ) )  
        exit( 1 );

    draw = disp->createWindow( disp, "Test", 100, 100, 512, 512, 0 );
    if( !draw ) exit( 1 );

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    numObjects = hwParseFile( filename, &objects );
    if( !numObjects )   exit( 1 );

    disp->setVisibility = printSetVisibility;
    disp->setInvisibility = printSetInvisibility;
    disp->boundsVisible = printBoundsVisible;

    if( visibility ) {
        disp->setVisibility( disp, visibility );
    }
    else {
        disp->setInvisibility( disp, invisibility, excl );
    }

    for( i = 0; i < numObjects; i++ ) {
        objects[i]->draw( objects[i] );
    }

    disp->surfAttrs = printSurfAttrs;
    disp->pushMatrix = printPushMat;
    disp->popMatrix = printPopMat;
    disp->drawMesh = printMesh;
    disp->drawPolygon = printPoly;

    printf( "# BBox: (%g,%g,%g) - (%g,%g,%g)\n",
                minX, minY, minZ, maxX, maxY, maxZ );

    for( i = 0; i < numObjects; i++ ) {
        objects[i]->draw( objects[i] );
    }
}
