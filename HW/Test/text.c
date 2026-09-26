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
#include "hw.h"
#include "hw_internal.h"

/* Draw an unlit, red sphere on the display */

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    hwFloat
        lightColor[3] = {1,1,1},
        lightDir[3] = {1,1,0},
        ambColor[3] = {1,1,1};

    hwSurfaceType
        front = {
            {1,0,0,},   /* Color */
            0,          /* Transp */
            {1,1,1},    /* SpecColor */
            0.1,        /* Shininess, */
            1.0,        /* PrimSize */
            -1,         /* Visibility */
            HW_SURF_TWOSIDED,           /* Flags */
                0,      /* Internal Flags */
                0,      /* Num Textures */
            {NULL}              /* Texture */
        },
        back = {
            {0,0,1,},   /* Color */
            0,          /* Transp */
            {1,1,1},    /* SpecColor */
            0.1,        /* Shininess, */
            1.0,        /* PrimSize */
            -1,         /* Visibility */
            HW_SURF_TWOSIDED,           /* Flags */
                0,      /* Internal Flags */
                0,      /* Num Textures */
            0           /* Texture */
        },
        side = {
            {1,0,1,},   /* Color */
            0,          /* Transp */
            {1,1,1},    /* SpecColor */
            0.1,        /* Shininess, */
            4.0,        /* PrimSize */
            -1,         /* Visibility */
            HW_SURF_WIREFRAME,          /* Flags */
                0,      /* Internal Flags */
                0,      /* Num Textures */
            0           /* Texture */
        };
    hwCamStruct
        cam = {
            1,  /* Perspective */
            0,  /* Mirror */
            {0,0,-5}, /* Pos */
            {0,0,1}, /* Dir */
            {0,1,0}, /* Up */
            {0,0},   /* Jitter */
            {0,0},   /* Skew */
            30.0, 1.0, /* Field, aspect */
            {1,10}    /* Planes */
        };
    hwFloat
        Mat[4][4],
        ang = 0.0;
    hwInt32
        texID;
    hwImageStruct
        *img;    /* For the texture */
    hwOrientType
        texOrient = {
            {1,1,1},    /* Scale */
            {0,0,0},    /* Rotate */
            {0,0,0},    /* Pos */
        };
    int
        i, wireframe = 0;

    for (i = 1; i < argc; i++) {
        if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = 1;
        }
    }
        hwDefaultOptFlags = HW_OPT_USE_DL;

    if( wireframe ) {
        front.flags = HW_SURF_TWOSIDED;
        back.flags = HW_SURF_TWOSIDED;
        side.flags = HW_SURF_WIREFRAME;
    }
    else {
        front.flags = HW_SURF_BLEND;
        back.flags = 0;
        side.flags = 0;
    }

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) exit( 1 );
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 0, 0, 512, 512, 0 );
    if( !draw ) exit( 1 );

    disp->makeCurrent( disp, draw );

    img = __hwIntReadJPG("Texture/Earth.jpg");
    if (img) {
        texID = disp->createTexture(disp, img,
                                1,              /* Filter */
                                HW_TM_MODULATE, /* Apply */
                                HW_TM_REPEAT,   /* Bound */
                                HW_TM_EXPLICIT, /* Coord mode */
                                &texOrient,     /* Orient */
                                0 );            /* InternalFormat */
        front.numTextures = 1;
        front.textureIDs[0] = texID;
        back.numTextures = 1;
        back.textureIDs[0] = texID;
        side.numTextures = 1;
        side.textureIDs[0] = texID;
    }

    __hwIntTextAttrs( disp, &front, &side, &back );
    __hwIntTextHeight( disp, 1.0 );
    __hwIntTextAlign( disp, HW_TEXT_ALIGN_CENTER,
                            HW_TEXT_ALIGN_CENTER,
                            HW_TEXT_ALIGN_CENTER );
    __hwIntTextOrient( disp, 0.0, 1.0, 0.0,  1.0, 1.0, 1.0 );
    __hwIntTextExtrude( disp, 0.0, 0.0, 0.5 );

    disp->ambientLight( disp, 0.2, ambColor );
    disp->enableLighting( disp, HW_TRUE );
    while( 1 ) {
        disp->setCamera( disp, &cam );
        disp->directionalLight( disp, lightColor, lightDir );

        hwRotateY( Mat, ang );

        disp->pushMatrix( disp, Mat );
        __hwIntText3d( disp, 0.0, 0.0, 0.0, "Test" );
        disp->popMatrix( disp );

        disp->update( disp, HW_UPDATE_ALL );
        ang += 0.5;
    }
}
