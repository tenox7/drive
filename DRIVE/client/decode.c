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


/*	Listen to a joystick coming through the Gameport Pro interface. 
	This version prints the coordinates and switch positions in a
	more legible format. */

/*
#include	<stdio.h>
#include	<errno.h>
#include        <termios.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<ctype.h>
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include    <termios.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<sys/time.h>
#include        <sys/modem.h>
#include	<sys/shm.h>

#include    "joystick.h"

#define	KEY	(('J'<<24)|('S'<<16)|('T'<<8)|'K')

extern char *optarg;
extern int optind, opterr;

#define		TRIGGER1 0x10
#define		THUMB1	0x20
#define		TRIGGER2 0x40
#define		THUMB2	0x80

main(argc, argv)
int argc;
char **argv;
{

    fd_set	server_mask;
    struct timeval ticktock;

    int i, count, bad = -1;
    int timeout_count = 0;
    unsigned char buffer[100];
    int
	ID;
    struct Joystick
	*Addr;

    ticktock.tv_sec = 5;
    ticktock.tv_usec = 0;

    ID = shmget( KEY, 4096, 0666 );
    if( ID < 0 ) {
	perror( "decode: shmget" );
	exit( 1 );
    }
    Addr = (struct Joystick *)shmat( ID, 0, 0 );
    if( Addr == (struct Joystick *)-1 ) {
	perror( "decode: shmat" );
	exit( 1 );
    }
    Addr->Valid = 1 ;

	printf("Ready!\n\n");
	i = 0;
	while (1) {
	    if( Addr->Valid == -1)
	    {
		/* This is the clue from the drive client that it is
		** terminating; we need to terminate as well! 
		*/
		printf("DRIVE client has exited; joystick daemon exiting.\n");
		break;
	    }

	    if (tcflush(Addr->gp_fd, TCIFLUSH) == -1) {
                fprintf(stderr, "Flush of joystick device failed");
                perror(" ");
            }
	    FD_ZERO(&server_mask );
	    FD_SET(Addr->gp_fd,&server_mask);
	    /* Block until something is there */
	    if (select(FD_SETSIZE, (int *) &server_mask,
		    (int *)0, (int *)0, &ticktock) < 0) {
		perror( "Client select" );
		exit( -1 );
	    }

	    /* Check socket for pending messages. */
	    if (FD_ISSET(Addr->gp_fd, &server_mask)) {

	      /* Go ahead and read the data */
	      count = read(Addr->gp_fd, buffer, 6);

	      /* Reset the timeout_count to 0 */
	      timeout_count = 0;

	      /* Make the drive client tell us it is still there */
	      if( Addr->Valid != -1 )
		  Addr->Valid = 1;

	      if (count == 6) {

		if( buffer[0] != 0 ) {
		    printf("Joystick Read:  First byte != 0 !!! \n");
		}
		else {
		    Addr->buttons = buffer[0+1];
		    Addr->x1 = buffer[1+1];
		    Addr->y1 = buffer[2+1];
		    Addr->x2 = buffer[3+1];
		    Addr->y2 = buffer[4+1];
		    Addr->new = 1;
		}

#ifdef DAEMON_PRINT
 		printf("J1: %02X %02X %s %s\n", (unsigned int)buffer[2], (unsigned int)buffer[3],
			buffer[1] & TRIGGER1 ? "TRIGGER" : "",
			buffer[1] & THUMB1 ? "THUMB" : "");
 		printf("J2: %02X %02X %s %s\n\n", (unsigned int)buffer[4], (unsigned int)buffer[5],
			buffer[1] & TRIGGER2 ? "TRIGGER" : "",
			buffer[1] & THUMB2 ? "THUMB" : "");
		
		i++;
		if (bad >= 0) {
	    		if (buffer[4] != buffer[5]) bad++;
			printf("%d bad out of %d\n\n", bad, i);
		}
#endif 
	      } else {
		printf("Got %d bytes\n\n", count);
		exit(1);	/* SHOULD NEVER OCCUR!!! */
	      }
	   }
	   else {
	       /* We get in here after a 5 second timeout */
	       /* printf("Not getting any input! \n"); */
	       if( (Addr->Valid != 3) ){
		   /* We have been waiting all this time and the client
		   ** has not been updated. 
		   */
		   /* printf("Joystick Daemon:  tick.... \n"); */
		   timeout_count++;
	       }
	       if( timeout_count > 3 )
	       {
		   if( getppid() == 1 )  
		   {
		       /* Parent Process has died, exit .. */
		       printf(" Joystick Input daemon exiting... \n");
		       break;
		   }
	       }
	   }
	}
		
	exit(0);
}


