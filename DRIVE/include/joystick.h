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


/* Joystick structure:
** This structure is used to communicate (via shared memory) joystick position
** values between the joystick daemon program and the drive client.
*/

#if !defined(_JOYSTICK_H_INCLUDED)
#define _JOYSTICK_H_INCLUDED 1

struct Joystick {
    int 
	Valid,
	new,
	gp_fd;
    char  
	devfile[100];
    unsigned char
	buttons,
	x1,
	y1,
	x2,
	y2;

};

#ifdef WIN32
#  define		TRIGGER1 0x01
#  define		THUMB1	 0x02
#  define		TRIGGER2 0x04
#  define		THUMB2	 0x08
#else
#  define		TRIGGER1 0x10
#  define		THUMB1	0x20
#  define		TRIGGER2 0x40
#  define		THUMB2	0x80
#endif


extern int init_joystick_device(
    char *Device);
extern int get_joystick_values(
    float *x, float *y,
    float *x1, float *y1,
    int *hx, int *hy,
    int *buttons);
extern void calibrate_joystick(void);
extern void joystick_close(void);

#endif /* !_JOYSTICK_H_INCLUDED */
