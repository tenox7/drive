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

static float joystickAxisValues[6];

void callback( hwDrawable draw, hwWinEvent *event )
{
    hwInt32 i, n, *ptr;
    hwFloat v;

    switch( event->type ) {
    case HW_INPUT_EXPOSE :
        n = event->expose.numClipRects;
        ptr = event->expose.clipRectList;
        printf( "Expose: %d rects:\n", n );
        while( n > 0 ) {
            printf("%dx%d @ (%d,%d)\n", ptr[2], ptr[3], ptr[0], ptr[1] );
            ptr += 4;
            n--;
        }
        break;
    case HW_INPUT_CONFIG :
        printf( "Config : %dx%d @ %d,%d\n",
                        event->config.width, event->config.height,
                        event->config.posx, event->config.posy );
        break;
    case HW_INPUT_DESTROY :
        printf( "Win destroy\n" );
        exit( 1 );
        break;
    case HW_INPUT_FOCUS :
        printf( "Focus: %d\n", event->focus.hasFocus );
        break;
    case HW_INPUT_KEYBOARD :
        if ((event->keyboard.key >= 32) && (event->keyboard.key < 128)) {
            /* ASCII key */
            printf( "Keyboard: \'%c\'\n", event->keyboard.key );
        } else {
            /* Other key */
            printf( "Keyboard: %08x(%04x)\n",
                    event->keyboard.key, event->keyboard.mods );
        }
        break;
    case HW_INPUT_POINTER :
        printf( "Pointer: %d,%d (%d + %d)\n",
                        event->pointer.x,
                        event->pointer.y,
                        event->pointer.buttonState,
                        event->pointer.kbdMods );
        break;
    case HW_INPUT_BUTTON_PRESS :
        printf( "Button press %d: %d,%d (%d)\n",
                        event->button.button,
                        event->button.x,
                        event->button.y,
                        event->button.kbdMods );
        break;
    case HW_INPUT_BUTTON_RELEASE :
        printf( "Button release %d: %d,%d (%d)\n",
                        event->button.button,
                        event->button.x,
                        event->button.y,
                        event->button.kbdMods );
        break;
    case HW_INPUT_JOY_PRESS :
        printf( "Joystick press %d\n", event->joystickButton.value );
        break;
    case HW_INPUT_JOY_RELEASE :
        printf( "Joystick release %d\n", event->joystickButton.value );
        break;
    case HW_INPUT_JOY_AXIS :
        n = event->joystickAxis.axis;
        v = round(5.*event->joystickAxis.value) / 5.;
        if ((n < 0) || (n > 5)) break;
        if (joystickAxisValues[n] != v) {
            joystickAxisValues[n] = v;
            printf( "Joystick axis %d %g\n", n, v);
        }
        break;
    }
}

main( int argc, char **argv )
{
    hwDisplay
        disp;
    hwDrawable
        draw;

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !disp ) {
        printf("hwInit failed\n");
        exit( 1 );
    }
    if( !disp->chooseVisual( disp, HW_VIS_DBUFF, 0 ) )  {
        printf("chooseVisual failed\n");
        exit( 1 );
    }
    draw = disp->createWindow( disp, "Test", 100, 100, 256, 256,
                        HW_WIN_INPUT | HW_WIN_JOYSTICK );
    if( !draw ) {
        printf("createWindow failed\n");
        exit( 1 );
    }
    disp->makeCurrent( disp, draw );
    disp->inputHandler( disp, callback );
#if 0
    disp->draw2Dtext( disp,
                txfLoadStaticFont(16),
                0x00FF0000,
                HW_TEXT_ALIGN_CENTER, HW_TEXT_ALIGN_CENTER,
                16, 128, 128, "Hit return" );
#endif
    while( 1 ) {
        disp->update( disp, HW_UPDATE_ALL );    /* Incl. swap */
    }
}
