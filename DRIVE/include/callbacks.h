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

#ifndef _DRIVE_CALLBACKS_INCLUDED
#define _DRIVE_CALLBACKS_INCLUDED 1

/*** From drive.c ***/
extern void client_grab_cursor();
extern void client_release_cursor();
extern void client_is_ready();
extern void client_quit();
extern void client_restart();
extern Widget create_sb_widget1( /* Widget toplevel */ );
extern Widget create_sb_widget2( /* Widget toplevel */ );
extern void hsv_to_rgb( /* h,s,v */ );
extern int  redraw_checkpoint( /* Widget widget, int repaint */ );
extern void redraw_controls( /*  Widget w */ );
extern void redraw_strip( /* Widget widget, int repaint */ );
extern void unmap_title_window();

/*** From server_interface.c ***/
extern void practice_mode_button_pushed();
extern void quit_button_pushed();
extern void race_mode_button_pushed();
extern void reset_button_pushed();

#endif /* _DRIVE_CALLBACKS_INCLUDED */
