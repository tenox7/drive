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


/*	Provide generic routines for driving RS232 ports.  This doesn't
	remove the requirement to understand termios(7), it just provides
	a consistent wrapper for all the programs. */

/*	Listen to a joystick coming through the Gameport Pro interface. 
	This version prints the coordinates and switch positions in a
	more legible format. */

#if !defined(__linux__) && !defined(WIN32) && !defined(MAC)

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include        <termios.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<sys/time.h>
#include        <sys/modem.h>
#include	<sys/shm.h>
#include	"global.h"
#include	"drive.h"

static int
    rs232_setup( char *, tcflag_t, tcflag_t, tcflag_t, tcflag_t, int, int ),
    rs232_await_input( int, int ),
    rs232_set_DTR( int ),
    rs232_clear_DTR( int ),
    rs232_set_RTS( int ),
    rs232_clear_RTS( int );

#define		TRIGGER1 0x10
#define		THUMB1	0x20
#define		TRIGGER2 0x40
#define		THUMB2	0x80

static int
    gp_fd;
static char
    *devfile;

void calibrate_joystick(void);

#define	KEY	(('J'<<24)|('S'<<16)|('T'<<8)|'K')

int init_joystick_device( char *Device )
{
    int
	i, count, bad = -1;
    int
	ID;
    struct Joystick
	*Addr;

    ID = shmget( KEY, 4096, IPC_CREAT|0666 );
    if( ID < 0 ) {
	perror( "test1: shmget" );
	exit( 1 );
    }
    Addr = (struct Joystick *)shmat( ID, 0, 0 );
    if( Addr == (struct Joystick *)-1 ) {
	perror( "test1: shmat" );
	(void)shmctl( ID, IPC_RMID, 0 );
	exit( 1 );
    }


    cstate.joystick = Addr;
    devfile = Device;
    strcpy( cstate.joystick->devfile, Device);
    
    gp_fd = rs232_setup(devfile,
	    ICRNL | IGNPAR,        			/* iflags */
	    0,        					/* oflags */
	    CSTOPB|B9600|CS8|CREAD|PARODD|CLOCAL|HUPCL,	/* cflags */
	    0,                				/* lflags */
	    6, 0 );					/* vmin, vtime */

    if( gp_fd < 0 ) {
	fprintf( stderr, "Can't set up %s", devfile );
	perror( " " );
	return 0;
    }

    if( rs232_clear_DTR( gp_fd ) == -1 ) {
	fprintf( stderr, "Can't clear DTR on %s", devfile );
	perror( " " );
	return 0;
    }

    if( rs232_clear_RTS( gp_fd ) == -1 ) {
	fprintf(stderr, "Can't clear RTS on %s", devfile);
	perror( " " );
	return 0;
    }

    sleep( 1 );	/* Why? */

    if( rs232_set_DTR( gp_fd ) == -1 ) {
	fprintf( stderr, "Can't set DTR on %s", devfile );
	perror( " " );
	return 0;
    }

    if( rs232_set_RTS( gp_fd ) == -1 ) {
	fprintf( stderr, "Can't set RTS on %s", devfile );
	perror(" ");
	return 0;
    }

    sleep( 1 );	/* Again, why? */

    cstate.joystick->gp_fd = gp_fd;

    /* Fork a process to read the input from the joystick */
    Addr->Valid = 0;
    switch( fork() ) {
    case 0 :
	/* Child */
	if( execl( "decode", "decode", (char *)0 ) < 0 )  {
	    perror( "decode: exec" );
	    exit( 1 );
	}
	break;
    case -1 :
	/* Error */
	perror( "decode: fork" );
	(void)shmctl( ID, IPC_RMID, 0 );
	exit( 1 );
	break;
    default :
	/* Parent */
	while( !Addr->Valid ) sleep(1);  /* Wait for the child to start */
	break;
    }

    calibrate_joystick();

    return 1;
}

/* All joysticks have a variable range of values that they return.  
 * Ideally, they return between 0 and 255, but in reality they miss those
 * Number by quite a bit sometime.  To try to allow for this, we will 
 * start by assuming certain minimums and maximums.  If the values returned
 * exceed these values, we can adjust them.  At any rate, these values will
 * be used to determine and X and Y range between -1 and 1, with 0 being
 * the center position of the stick.
 */

static float
    XMIN = 40.0,
    XMAX = 200.0,
    YMIN = 40.0,
    YMAX = 200.0,
    DEADZONE = 4.0,
    XCENTER   = 128.0,
    YCENTER   = 128.0;

#define XMIN_RANGE  (XCENTER -XMIN)
#define YMIN_RANGE  (YCENTER -YMIN)

int get_joystick_values( float *x, float *y, float *x1, float *y1, int *ix, int *iy, int *buttons )
{
    int
	i, count, bad = -1;
    unsigned char
	buffer[100];
    float
	xpos, ypos;
    int
	mask;
    int
	nfds;
    static struct timeval
	timeout = {0,100};

    *x1 = *y1 = 0.;

    *ix =         cstate.joystick->x1;
    *iy =         cstate.joystick->x2;
    xpos = (float)cstate.joystick->x1;
    ypos = (float)cstate.joystick->x2;  /* For the Thrustmaster wheel/brake */
    *buttons =    cstate.joystick->buttons & 0xf0;

    if( (xpos > (XCENTER - DEADZONE)) && (xpos < (XCENTER + DEADZONE)) ) {
	*x = 0.0;
    }
    else if( xpos < XCENTER ) {
	*x =  - (XMIN_RANGE - (xpos - XMIN)) / (XMIN_RANGE);
    }
    else {
	*x =   (xpos - XCENTER) / (XMAX -XCENTER);
    }

    if( (ypos > (YCENTER - DEADZONE)) && (ypos < (YCENTER + DEADZONE)) ) {
	*y = 0.0;
    }
    else if( ypos < YCENTER ) {
	*y =  - (YMIN_RANGE - (ypos - YMIN)) / (YMIN_RANGE);
	*y *= 1.3; /* Make it easier to accelerate */
    }
    else {
	*y =   (ypos - YCENTER) / (YMAX -YCENTER);
    }

    if( *x > 1.0 ) *x = 1.0;
    if( *y > 1.0 ) *y = 1.0;
    if( *x < -1.0 ) *x = -1.0;
    if( *y < -1.0 ) *y = -1.0;

    /*
    printf("X = %x  %f,  Y = %x %f \n",*ix,*x,*iy,*y);
    */
    return 1;
}

void calibrate_joystick( void )
{
    int buttons;
    int ix, iy;
    int maxx, maxy, minx,miny;
    float x,y;

    maxx = 0; maxy = 0;
    minx = 0xff; miny = 0xff;

    buttons = 0;
    cstate.joystick->buttons = 0;
    printf("  Center Wheel and Brakes, then press a button:  ");
    fflush(stdout);
    while(buttons == 0 )
	if ( ! get_joystick_values(&x,&y,&buttons, &ix, &iy) ) buttons = 0;
    XCENTER = ix;
    YCENTER = iy;
    printf("\n     -------- Center Value:  %x  %x\n",ix,iy);
    fflush(stdout);

    buttons = 0;
    cstate.joystick->buttons = 0;
    printf("\n  Turn the Wheel all the way left, then right.");
    printf("\n  Press and release both pedals, one at a time.");
    printf("\n  Then, press a button:   ");
    fflush(stdout);
    while(buttons == 0 )
    {
	get_joystick_values(&x,&y,&buttons, &ix, &iy); 
	if( ix > maxx ) maxx = ix;
	if( iy > maxy ) maxy = iy;
	if( ix < minx ) minx = ix;
	if( iy < miny ) miny = iy;
    }
    XMIN = minx + 3;
    YMIN = miny + 5;
    XMAX = maxx - 3;
    YMAX = maxy - 5;
    printf("\n     -------- Minimum Values:  %x  %x\n",minx,miny);
    printf("\n     -------- Maximum Values:  %x  %x\n",maxx,maxy);
    fflush(stdout);

}

void joystick_close( void )
{
    rs232_clear_DTR( gp_fd );
    rs232_clear_RTS( gp_fd );
    close( gp_fd );
}


/*	Provide generic routines for driving RS232 ports.  This doesn't
    remove the requirement to understand termios(7), it just provides
    a consistent wrapper for all the programs. */

/*	Return -1 on failure because "0" could conceivably be a valid
    file descriptor */

static int rs232_setup
(
    char	*devfile,
    tcflag_t	iflags,
    tcflag_t	oflags,
    tcflag_t	cflags,
    tcflag_t	lflags,
    int		vmin,
    int		vtime
)
{
    int
	fd;
    struct termios
	signio;

    fd = open( devfile, O_RDWR | O_NOCTTY );
    
    if( fd == -1) return -1;

    signio.c_iflag = iflags;
    signio.c_oflag = oflags;
    signio.c_cflag = cflags;
    signio.c_lflag = lflags;

    if( !(iflags & ICANON) ) {
	signio.c_cc[VMIN] = (cc_t)vmin;		/* must read vmin */
	signio.c_cc[VTIME] = (cc_t)(vtime * 10); /* timeout in secs */
    }

    if( tcflush( fd, TCIOFLUSH ) == -1 ) return -1;
    
    if( tcsetattr( fd, TCSANOW, &signio ) == -1 ) return -1;
    
    return fd;
}

/* Use select to wait for input available for a certain time.  Return false
on timeout.  Assumes fd < 31. */

static int rs232_await_input( int fd, int milliseconds )
{
    int
	read_mask, write_mask, exception_mask, retval;
    struct timeval
	ticktock;

    read_mask = 1 << fd;
    write_mask = 0;
    exception_mask = read_mask;

    ticktock.tv_sec = milliseconds / 1000;
    ticktock.tv_usec = milliseconds % 1000;

    retval = select(fd+1, &read_mask, &write_mask, &exception_mask, &ticktock);

    /* If > 0, data is available; == 0, timeout; < 0, error */
    return retval;
}


static int rs232_set_DTR( int fd )
{
    mflag
	modem_flags;
    int result;

    while (((result = ioctl( fd, MCGETA, &modem_flags)) == -1 ) 
	    && (errno == EINTR));
    if (result == -1) return 0;

    modem_flags |= MDTR;
    while (((result = ioctl( fd, MCSETA, &modem_flags)) == -1 )
	    && (errno == EINTR));
    if (result == -1) return 0;

    rs232_await_input( fd, 100 );	/* Just want the wait */
    return 1;
}

static int rs232_clear_DTR( int fd )
{
    mflag
	modem_flags;
    int result;

    while (((result = ioctl( fd, MCGETA, &modem_flags)) == -1)
	    && (errno == EINTR));
    if (result == -1) return 0;

    modem_flags &= (~MDTR);
    while (((result = ioctl( fd, MCSETA, &modem_flags)) == -1)
	    && (errno == EINTR));
    if (result == -1) return 0;

    rs232_await_input( fd, 100 );	/* Just want the wait */
    return 1;
}

static int rs232_set_RTS( int fd )
{
    mflag
	modem_flags;
    int result;

    while (((result = ioctl( fd, MCGETA, &modem_flags )) == -1 )
	    && (errno == EINTR));
    if (result == -1) return 0;

    modem_flags |= MRTS;
    while (((result = ioctl( fd, MCSETA, &modem_flags )) == -1 )
	    && (errno == EINTR));
    if (result == -1) return 0;

    rs232_await_input( fd, 100 );	/* Just want the wait */
    return 1;
}

static int rs232_clear_RTS( int fd )
{
    mflag
	modem_flags;
    int result;

    while (((result = ioctl( fd, MCGETA, &modem_flags )) == -1 )
	    && (errno == EINTR));
    if (result == -1) return 0;

    modem_flags &= (~MRTS);
    while (((result = ioctl( fd, MCSETA, &modem_flags )) == -1 )
	    && (errno == EINTR));
    if (result == -1) return 0;

    rs232_await_input( fd, 100 );	/* Just want the wait */
    return 1;
}


#else


int init_joystick_device(char *Device)
{
    return(-1);
}

int get_joystick_values(float *x, float *y, int *buttons, int *ix, int *iy)
{
    return(-1);
}

void calibrate_joystick(void)
{
}

void joystick_close(void)
{
}

#endif /* !__linux__ else */
