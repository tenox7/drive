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
#include <time.h>
#include <fcntl.h>

typedef unsigned int KEYPRESSES;

typedef struct
{
    float x_val;
    float y_val;
    int gear_wanted;
    unsigned int key_mask;		/* Modifier keys from keysym.h */
    KEYPRESSES keypresses;		/* Other keys passed to server */
} pointer_msg_type;


main(
    int argc,
    char *argv[])
{
    int infile;
    struct timeval tm;
    pointer_msg_type msg;
    int count;

    if (argc < 2) {
	infile = 0;
    }
    else if ((infile = open(argv[1],O_RDONLY)) == -1) {
	fprintf(stderr,"Cannot open file %s\n",argv[1]);
	perror("open");
	exit(-10);
    }

    count = 0;
    while ((read(infile,&tm,sizeof(struct timeval))
		== sizeof(struct timeval))
	    && (read(infile,&msg,sizeof(pointer_msg_type))
		== sizeof(pointer_msg_type))) {
	printf("%6.6f: %6.6f %6.6f %2.2d 0x%08x 0x%08x\n",
		(float) tm.tv_sec + (float) tm.tv_usec/1000000.0,
		msg.x_val, msg.y_val,
		msg.gear_wanted,
		msg.key_mask,
		msg.keypresses);
	++count;
    }

    close(infile);
    printf("\nCount: %d\n",count);
}

