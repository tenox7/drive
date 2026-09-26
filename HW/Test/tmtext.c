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
#include <math.h>
#include "hw.h"
#include "hw_internal.h"
#include "TexFont.h"

hwFloat
    data[] = {
    -1.0, -1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 0.0,
     1.0, -1.0, 0.0,  0.0, 0.0, -1.0,  1.0, 0.0,
     1.0,  1.0, 0.0,  0.0, 0.0, -1.0,  1.0, 1.0,
    -1.0,  1.0, 0.0,  0.0, 0.0, -1.0,  0.0, 1.0,
    };

int currState = 0;

void callback( hwDrawable draw, hwWinEvent *event );

float scale = 0.10f;

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;
    TexFont
        *txf;
    hwObject
        quad, cam, tm;
    hwFloat
        bg[3] = {0.f, 0.f, 1.f},
        mat[4][4],
        offs = 0;
    FILE
        *f;
    int
        done = 0;

    f = fopen("default.txf", "rb");
    txf = txfLoadFont((TxfReadFunc)fread, f);
    if (f) fclose(f);
    if (!txf) exit(1);

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );

    if( !disp ) exit(1);
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF|HW_VIS_DEPTH, 0 ) )     exit( 1 );
    draw = disp->createWindow( disp, "Test", 50, 20, 800, 800, HW_WIN_INPUT );
    if( !draw ) exit(1);

    /* This needs to happen before most HW calls */
    disp->makeCurrent( disp, draw );

    cam = hwCamera->create( hwCamera );
    if( !cam )          exit( 1 );
    HW_MODIFY_2F( cam, hwStrPlanes, 0.4, 2.4 );
    HW_MODIFY_1I( cam, hwStrField, 90 );
    HW_MODIFY_1B( cam, hwStrPerspective, HW_TRUE );

    disp->backgroundColor(disp, bg);
    disp->update(disp, HW_UPDATE_ALL);

    /* Draw a polygon textured with the font texture. Lets see what it
    ** looks like! */

    tm = hwTexture->create( hwTexture );
    HW_MODIFY_1I( tm, hwStrRows, txf->tex_height );
    HW_MODIFY_1I( tm, hwStrColumns, txf->tex_width );
    HW_MODIFY_1I( tm, hwStrComponents, 2 );
    HW_MODIFY_1I( tm, hwStrType, HW_IMG_UBYTE );
    HW_MODIFY_1I( tm, hwStrApply, HW_TM_MODULATE );
    tm->modify( tm, hwStrImage,
                HW_MAKE_TYPE(HW_TYPE_BYTE, (2*txf->tex_width*txf->tex_height)),
                txf->teximage);
    HW_MODIFY_1I( tm, hwStrFilter, HW_TM_TRILINEAR );
    //HW_MODIFY_1I( tm, hwStrScale, 1 );


    quad = hwQuads->create( hwQuads );
    if( !quad ) exit( 1 );
    HW_MODIFY_3F( quad, hwStrColor, 1.0, 1.0, 1.0 );
    HW_MODIFY_1I( quad, hwStrOptFlags, HW_OPT_USE_DL );
    HW_MODIFY_1B( quad, hwStrHasNormals, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrHasST0, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrTwoSided, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrBright, HW_TRUE );
    HW_MODIFY_1B( quad, hwStrBlend, HW_TRUE );
    //HW_MODIFY_1B( quad, hwStrWireframe, Wireframe );
    //HW_MODIFY_1B( quad, hwStrSampVert, sampVert );
    quad->modify( quad, hwStrData, HW_MAKE_TYPE(HW_TYPE_FLOAT,4*8), data );
    quad->modify( quad, hwStrTexture, HW_TYPE_OBJECT, tm ); 
    disp->inputHandler( disp, callback );

    while (!done) {
        switch (currState) {
        case 0 :
            cam->draw( cam );
            quad->draw( quad );
            break;
        case 1 :
            txfSetPosition(-.9f, 0.f);
            txfSetScale(scale);
            txfBeginFontRendering(disp, txf, TXF_COORD_MODE_NDC);
                txfRenderString(disp, txf, "This Is A Test!", 15);
            txfEndFontRendering(disp);
            break;
        case 2 :
            txfSetPosition(-.9f, 0.f);
            txfSetScale(scale);
            txfBeginFontRendering(disp, txf, TXF_COORD_MODE_NDC);
                txfRenderFancyString(disp, txf,
                        "This Is A "
                        "\033T\377\377\000\377\000\000"
                        "Fancy"
                        "\033M\377\377\377"
                        " Test!",
                        34);
            txfEndFontRendering(disp);
            break;
        default :
            done = 1;
            break;
        }
        disp->update( disp, HW_UPDATE_ALL );
    }
}

void callback( hwDrawable draw, hwWinEvent *event )
{
    if( event->type == HW_INPUT_KEYBOARD ) {
        switch (event->keyboard.key) {
        case ' ' :
            currState++;
            break;
        case '-' :
            scale -= 0.01f;
            if (scale < 0.01f) scale = 0.01f;
            break;
        case '+' :
            scale += 0.01f;
            if (scale > 2.f) scale = 2.f;
            break;
        }
    }
}

