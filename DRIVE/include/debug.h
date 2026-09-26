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


/* 
 * debug.h - stuff to help in debugging
 *
 */

#ifndef _DEBUG_INCLUDED
#define _DEBUG_INCLUDED

#ifdef TEST_CODE
# define DPRINTF1(p1) \
    { if (debug) printf(p1); }
# define DPRINTF2(p1,p2) \
    { if (debug) printf((p1),(p2)); }
# define DPRINTF3(p1,p2,p3) \
    { if (debug) printf((p1),(p2),(p3)); }
# define DPRINTF4(p1,p2,p3,p4) \
    { if (debug) printf((p1),(p2),(p3),(p4)); }
# define DPRINTF5(p1,p2,p3,p4,p5) \
    { if (debug) printf((p1),(p2),(p3),(p4),(p5)); }
# define DPRINTF6(p1,p2,p3,p4,p5,p6) \
    { if (debug) printf((p1),(p2),(p3),(p4),(p5),(p6)); }
# define DPRINTF7(p1,p2,p3,p4,p5,p6,p7) \
    { if (debug) printf((p1),(p2),(p3),(p4),(p5),(p6),(p7)); }
# define DPRINTF8(p1,p2,p3,p4,p5,p6,p7,p8) \
    { if (debug) printf((p1),(p2),(p3),(p4),(p5),(p6),(p7),(p8)); }
# define DPRINTF9(p1,p2,p3,p4,p5,p6,p7,p8,p9) \
    { if (debug) printf((p1),(p2),(p3),(p4),(p5),(p6),(p7),(p8),(p9)); }
# define DFLUSH \
    { if (debug) { XSync(cstate.display,True); fflush(stdout); fflush(stderr);}}
#else /* not test code */
# define DPRINTF1(p1)
# define DPRINTF2(p1,p2) 
# define DPRINTF3(p1,p2,p3)
# define DPRINTF4(p1,p2,p3,p4)
# define DPRINTF5(p1,p2,p3,p4,p5)
# define DPRINTF6(p1,p2,p3,p4,p5,p6)
# define DPRINTF7(p1,p2,p3,p4,p5,p6,p7)
# define DPRINTF8(p1,p2,p3,p4,p5,p6,p7,p8)
# define DPRINTF9(p1,p2,p3,p4,p5,p6,p7,p8,p9)
# define DFLUSH
#endif /* TEST_CODE else */

#endif /* _DRIVE_INCLUDED */
