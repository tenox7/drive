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


/* Routines to read the NT joystick device */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static UINT wNumDevs, wDeviceID; 

int init_joystick_device( char *Device )
{
    /* See if there is a joystick attached */
    JOYINFO joyinfo; 
    BOOL bDev1Attached, bDev2Attached; 
 
    if((wNumDevs = joyGetNumDevs()) == 0) {
	return 0;
    }

    bDev1Attached = joyGetPos(JOYSTICKID1,&joyinfo) != JOYERR_UNPLUGGED; 
    bDev2Attached = (wNumDevs > 1) && 
                    (joyGetPos(JOYSTICKID2,&joyinfo) != JOYERR_UNPLUGGED);

    if (bDev1Attached || bDev2Attached) {  // decide which joystick to use 
        wDeviceID = bDev1Attached ? JOYSTICKID1 : JOYSTICKID2; 
    } else {
	return 0;
    }

    return 1;
}


int get_joystick_values( float *x, float *y, float *x1, float *y1,
                         int *hatX, int *hatY, int *buttons)
{
    JOYINFO joyinfo;
    JOYINFOEX joyinfoex;
    float xx, yy,z,r,u,v ;
    int success;

    if (wNumDevs == 0) {
        return 0;
    }

    joyinfoex.dwFlags = JOY_RETURNALL;
    joyinfoex.dwSize = sizeof(JOYINFOEX);

    success =  joyGetPosEx(wDeviceID, &joyinfoex);
    if (success != 0) {
	return 0;
    }

    *x = (joyinfoex.dwXpos / 32767.0) -1.0;
    *y = (joyinfoex.dwYpos / 32767.0) - 1.0;
    *x1 = (joyinfoex.dwZpos / 32767.0) -1.0;
    *y1 = (joyinfoex.dwRpos / 32767.0) - 1.0;
    *buttons = joyinfoex.dwButtons;

    *hatX = *hatY = 0;

    switch( joyinfoex.dwPOV) {
    case     0 : *hatX =  0; *hatY = -1; break;
    case  4500 : *hatX =  1; *hatY = -1; break;
    case  9000 : *hatX =  1; *hatY =  0; break;
    case 13500 : *hatX =  1; *hatY =  1; break;
    case 18000 : *hatX =  0; *hatY =  1; break;
    case 22500 : *hatX = -1; *hatY =  1; break;
    case 27000 : *hatX = -1; *hatY =  0; break;
    case 31500 : *hatX = -1; *hatY = -1; break;
    }

    return 1;
}



void joystick_close( void )
{
}


#ifdef TEST
int main(int argc, char **argv)
{
    float x, y, x1, y1, oldx = 0, oldy = 0, oldx1 = 0, oldy1 = 0;
    int hx, hy, buttons, oldhx = 0, oldhy = 0, oldbuttons = 0;

    if (!init_joystick_device("")) {
        printf("Joystick init failed\n");
        exit(1);
    }

    while (1) {
        if (!get_joystick_values(&x, &y, &x1, &y1, &hx, &hy, &buttons)) {
            exit(1);
        }
        if ((x != oldx) || (y != oldy) || (x1 != oldx1) || (y1 != oldy1) ||
            (hx != oldhx) || (hy != oldhy) || (buttons != oldbuttons)) {
            printf("(%g,%g), (%g,%g), (%d,%d), %08x\n",
                    x, y, x1, y1, hx, hy, buttons);
        }
        oldx = x; oldy = y;
        oldx1 = x1; oldy1 = y1;
        oldhx = hx; oldhy = hy;
        oldbuttons = buttons;
    }
}
#endif
