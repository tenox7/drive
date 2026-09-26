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
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/audio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "sound.h"

char configFile[80]="sound.cnf";
int debug = FALSE;

void main(argc, argv)
int argc;
char *argv[];
{
    int c;
    struct playList List;
    extern char *optarg;
    extern int optind, optopt;

    int ID;
    struct Sound *AudShmStruct;
    int read_mask, write_mask, exception_mask, retval;

    /*///////////////////////////////////////////////////////////////////
    // check any comman line arguments
    ///////////////////////////////////////////////////////////////////*/
    while((c=getopt(argc, argv, "c:d")) != -1)
       switch(c) {
	  case 'c':	/* change default config file */
	     strcpy(configFile, optarg);
	     break;
	  case 'd':	/* enable debug printing */
	     debug = TRUE;
	     break;
          default:	/* dunno */
	     fprintf(stderr, "Usage: %s [-c config_file]\n", argv[0]);
	     exit(1);
       }

    /*///////////////////////////////////////////////////////////////////
    // initialize shared memory stuff
    ///////////////////////////////////////////////////////////////////*/
    ID = shmget( SKEY, 4096, 0666 );
    if( ID < 0 ) {
	perror( "audaemon: shmget" );
	exit(1);
    }
    AudShmStruct = (struct Sound *)shmat( ID, 0, 0 );
    if( AudShmStruct == (struct Sound *)-1 ) {
	perror( "test2: shmat" );
	exit( 1 );
    }

    if (read_sound_config_file(configFile, &List)) {
	fprintf(stderr, "%s:  unable to read config file %s\n", argv[0], configFile);
	exit(1);
    }

    if (load_sound_files(&List)) {
	fprintf(stderr, "%s:  unable to load sound files\n", argv[0]);
	exit(1);
    }

    aud_play_loop(&List, AudShmStruct);

    exit(0);
}

/*//////////////////////////////////////////////////////////////////////////////
// read_sound_config_file:
//////////////////////////////////////////////////////////////////////////////*/
int read_sound_config_file(fileName, List)
char *fileName;
struct playList *List;
{
    FILE *cfp;
    char input[80], s1[80], s2[80];
    int lineno, i;

    if ((cfp=fopen(fileName, "r")) == NULL)
       return(1);

    /* check file header */
    fscanf(cfp, "%s", input);
    if (strcmp(input, "audaemon_config")) {
        fprintf(stderr, "Invalid confi file type: %s ", input);
        return(1);
    }

    /* read file */
    i = 1; /* sound 0 is silent */
    for (lineno=2; fgets(input, 80, cfp) != NULL; lineno++) {
        if (input[0] != '#' && input[0] != '\n') { /* ignore comments */
	    if (sscanf(input, "%s %s", s1, s2) == 2) {
		if(!(s1[0] == '#' || s1[0] == ';')) {
		   strcpy(List->sound[i].fileName, s1);
		   List->sound[i].fileLen = (int) atoi(s2);
		   i++;
		}
	    } else {
	       fprintf(stderr, "error in line %d of config file: %s", lineno, input);
	       return(1);
	    }
        }
    }
    List->numbSounds = i;

    return(0);
}

/*//////////////////////////////////////////////////////////////////////////////
// load_sound_files
//////////////////////////////////////////////////////////////////////////////*/
int load_sound_files(List)
struct playList *List;
{
    int i;
    FILE *fp;
    struct sound_struct *ssp;

    ssp = (struct sound_struct *)&(List->sound[0]);
    /* sound 0 is reserved for silence */
    ssp->fileLen = 0;
    ssp->dataPlayLen = DBUFSZ;
    /* allocate memory for sound */
    if ((ssp->data = (char *)calloc(ssp->dataPlayLen, sizeof(char))) == NULL) {
	fprintf(stderr, "Unable to allocate memory for sound!\n");
	return(1);
    }
    /* build silent buffer */
    for (i=0; i<DBUFSZ; i++) ssp->data[i] = 0;

    /* load the rest of the sound from the sound files listed in the config file */
    for (i=1; i<List->numbSounds; i++) {
	ssp = (struct sound_struct *)&(List->sound[i]);

	/* open sound file */
        if ((fp = fopen(ssp->fileName, "rb")) == NULL) {
	    fprintf(stderr, "unable to open file: %s  :", List->sound[i].fileName);
	    return(1);
        }

	/* allocate memory for sound */
	ssp->dataPlayLen = ssp->fileLen + ssp->fileLen%DBUFSZ; /* allocate multiples of DBUFSZ */
        if ((ssp->data = (char *)calloc(ssp->dataPlayLen, sizeof(char))) == NULL) {
	    fprintf(stderr, "Unable to allocate memory for sound!\n");
	    return(1);
        }

	/* read sound */
	fread(ssp->data, sizeof(char), ssp->fileLen, fp);

	/* close sound file */
	fclose(fp);
    }

    return(0);
}


#ifndef M_PI
#	define	M_PI	3.141592653589
#endif


#define MASK(f)	(1<<(f))
#define WAIT(x,y)
#define SHORT_WAIT(x)

#define DEFAULT_SOUND 1
#define SAMPLE_SIZE (1024 * 8)

extern int debug;

char audioPath[80]="/dev/audio";
char *silence;
char verrm[80];
int dev_fd;			/* audio device */

#define NUM_RPM   100

char Buffer1[ 1024 * 8 ];
char Buffer2[ 1024 * 8 ];

short *orig;
int    orig_size;

short *ping;
short *pong;
int   pingstart;

struct audio_test_buffer tbuffer;
struct registers audio_regs;	/* for keeping track of register values */
struct audio_regio regio;	/* for kernel IO */
struct audio_describe aud_desc;


void close_audio_dev();
int play_sound();
int open_audio_dev();
void reset_audio();
void init_audio();
void write_control();
void enter_test_mode();
int Resample( short *Src, short *Dst, int Count, float Factor );
int CopySample
(
    short *Src, int SrcSize,		/* Source buffer to copy */
    short *Ping, short *Pong,		/* Dest. buffer to copy to */
    int PingStart, int BufSize,		/* Where to start, how big buffs are */
    float Factor			/* Resampling factor <= 1.0 */
);

/*/////////////////////////////////////////////////////////////////////////
//  aud_play_loop:
/////////////////////////////////////////////////////////////////////////*/
aud_play_loop(List, AudShmStruct)
struct playList *List;
struct Sound *AudShmStruct;
{
    struct audio_select_thresholds threshold;
    int writemask, writefds;
    int no_fds, rc, i, quit = FALSE;
    int waiting;
    pid_t parent_pid;
    int   last_sound = 0;
    int   same_count = 0;

#if 0
	waiting = 1;
	while( waiting )
	{
	    waiting = 1;
	}
#endif

    /* Get the process id of my parent id -- terminate if parent dies */
    parent_pid = getppid();

    generate_sine_sound( );

    Resample( (short *)List->sound[1].data, orig, 8192/2, 8.0 );
    orig_size = AdjustSample( orig, 1024/2);
    ping = (short *)Buffer1;
    pong = (short *)Buffer2;
    pingstart = 0;

    if (debug) printf("command %d\n", AudShmStruct->command);
    while (!quit)
	switch (AudShmStruct->command) {
	    case OPEN_AUDIO:
		/*////////////////////////////////////////////
		// open and initialize audio device
		////////////////////////////////////////////*/
		if (debug) printf("open audio\n");
		open_audio_dev(audioPath);
		init_audio(START);
		AudShmStruct->command = NOOP_AUDIO;
		silence = List->sound[0].data;
		break;

	    case RESET_AUDIO:
		/*////////////////////////////////////////////
		// reset audio device and begin audio DMA
		////////////////////////////////////////////*/

		if (debug) printf("reset audio\n");

		/* reset audio device */
		reset_audio();

		/* exit driver test mode */
		if (exit_test_mode())
		   return(FAIL);

		/* set the audio i/o buffer size */
		if (ioctl(dev_fd, AUDIO_SET_TXBUFSIZE, AUD_BUFSIZE)<0) {
		    sprintf(verrm, "mdma: AUDIO_SET_TXBUFSIZE ioctl returned errno %d\n", errno);
		    perror(verrm);
		    exit_test_mode();
		    return(FAIL);
		}
		if (ioctl(dev_fd, AUDIO_SET_RXBUFSIZE, AUD_BUFSIZE)<0) {
		    sprintf(verrm, "mdma: AUDIO_SET_RXBUFSIZE ioctl returned errno %d\n", errno);
		    perror(verrm);
		    exit_test_mode();
		    return(FAIL);
		}

		/* set up select masks */
		writemask = 1<<dev_fd;
		no_fds = dev_fd+1;
	       
		/* set up select thresholds */
		threshold.read_threshold = DBUFSZ;
		threshold.write_threshold = DBUFSZ;
		if (ioctl(dev_fd, AUDIO_SET_SEL_THRESHOLD, &threshold) < 0) {
		    sprintf(verrm,"mdma: AUDIO_SET_SEL_THRESHOLD ioctl returned errno %d\n", errno);
		    perror(verrm);
		    exit_test_mode();
		    return(FAIL);
		}

		/* enter PAUSED state */
		if (ioctl(dev_fd, AUDIO_PAUSE, AUDIO_RECEIVE|AUDIO_TRANSMIT) < 0) {
		    sprintf(verrm,"mdma: AUDIO_PAUSE ioctl returned errno %d\n",errno);
		    perror(verrm);
		    exit_test_mode();
		    return(FAIL);
		}

		/* fill the driver's play buffer with silence */
		if ((rc = write(dev_fd, silence, DBUFSZ)) < 0) {
		     sprintf(verrm, "mdma: write returned rc %d errno %d\n", rc, errno);
		     perror(verrm);
		     exit_test_mode();
		     enter_test_mode();
		}

		/* Begin DMA */
		if (ioctl(dev_fd, AUDIO_RESUME, AUDIO_RECEIVE|AUDIO_TRANSMIT) < 0) {
		    sprintf(verrm, "mdma: AUDIO_RESUME ioctl returned errno %d\n", errno);
		    perror(verrm);
		    exit_test_mode();
		    return(FAIL);
		}

		AudShmStruct->command = NOOP_AUDIO;
		break;

	    case QUIT_AUDIO:	
		/*////////////////////////////////////////////
		// close audio device
		////////////////////////////////////////////*/
		if (debug) printf("quit audio\n");
		close_audio_dev();
		quit = TRUE;
		break;

	    case PLAY_AUDIO:
		/*////////////////////////////////////////////
		// play sound seg. specified in AudShmStruct
		////////////////////////////////////////////*/
		if (debug) printf("play audio\n");
		play_sound(List, AudShmStruct);
		if( AudShmStruct->sound != last_sound ) {
		    last_sound = AudShmStruct->sound;
		    same_count = 0;
		}
		else {
		    same_count++;
		}
		if( same_count >= 20 ) {
		    /* If the sound hasn't changed in 5 seconds, check to
		    ** see if our parent is still alive 
		    */
		    same_count = 0;
		    if( getppid() == 1 ) {
			close_audio_dev();
			quit = TRUE;
		    }
		}
		break;

	    default:
		break;

	}
}

/*/////////////////////////////////////////////////////////////////////////
// play_sound:
/////////////////////////////////////////////////////////////////////////*/
int play_sound(List, AudShmStruct)
struct playList *List;
struct Sound *AudShmStruct;
{
    int i, rc; 
    struct audio_select_thresholds threshold;
    int writemask, writefds, readfds;
    int no_fds;
    float pct,factor;

    static int done = TRUE, lastSoundPlayed;
    static int old_i = -1;
    static int p_buffers;
    static char *pptr;
    static buffers = 0;


    if( AudShmStruct->sound == 1 ) {  /* Engine Sound! */
	if(AudShmStruct->rpm <= 1010) AudShmStruct->rpm = 1010; /*KLUDGE*/

	pct = (AudShmStruct->rpm - 1000.0) / 7000.0;
	factor = 1.0 / 8.0 + pct * ( 7.0 / 8.0);

	pingstart = 
	    CopySample(orig, orig_size ,ping, pong, pingstart, 4*1024, factor);
	pptr = (char *)ping;
	ping = pong;
	pong = (short *)pptr;
	p_buffers = 0;
	buffers = 4;
	if(debug) printf("Engine Noise...\n");

    } else if (done || AudShmStruct->newsound ) {
	done = FALSE;
	AudShmStruct->newsound = 0;
	p_buffers = 0;
	pptr = List->sound[AudShmStruct->sound].data;
	buffers = ((List->sound[AudShmStruct->sound].dataPlayLen-1)/DBUFSZ) ;
	if (debug) printf("sound=%d playLen=%d buffers=%d\n", 
	    AudShmStruct->sound, List->sound[AudShmStruct->sound].dataPlayLen,
	    buffers);
    }

    /* set up select masks */
    writemask = 1<<dev_fd;
    no_fds = dev_fd+1;

    rc = write(dev_fd, pptr, DBUFSZ);
    if (rc < 0) {
	sprintf(verrm, "mdma: write returned rc %d errno %d while in write\n", rc, errno);
	perror(verrm);
	exit_test_mode();
	enter_test_mode();
    }
    if (debug) printf("p_buffer=%d\n", p_buffers);

    writefds = writemask;
    if (select(no_fds, 0, &writefds, 0, 0) < 0) {
	sprintf(verrm, "mdma: select returned errno %d\n", errno);
	perror(verrm);
	exit_test_mode();
	return(FAIL);
    }

    pptr += DBUFSZ;
    p_buffers++;

    if ((done = !(p_buffers < buffers))) {
	/*
	printf("Sound %d Done \n",AudShmStruct->sound );
	*/
	if( AudShmStruct->sound != SOUND_SILENCE )
	    AudShmStruct->sound = DEFAULT_SOUND;
    }

} /* end play_sound */

/*/////////////////////////////////////////////////////////////////////////
// close audio device
/////////////////////////////////////////////////////////////////////////*/
void close_audio_dev()
{
    int rc;

    /* exit test mode */

    if ((rc = ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer)) < 0) {
	sprintf(verrm, "close_audio_dev: ioctl error %d\n", errno);
        if (debug) perror(verrm);
    }
    
    close(dev_fd);
}

/*/////////////////////////////////////////////////////////////////////////
// open audio device
/////////////////////////////////////////////////////////////////////////*/
int open_audio_dev(path)
char *path;
{
    int rc;
    int i;
    int test_mode;

    /*
    set_up_alarm(INVOKE_SIG);
    */

    if ((dev_fd = open(path, O_RDWR)) == -1) {
	fprintf(stderr, "open_dev: can't open %s, errno %d\n", path, errno);
	exit(1);
    }

    /* get the audio_describe structure */
    if ((rc = ioctl(dev_fd, AUDIO_DESCRIBE, &aud_desc)) < 0) {
	sprintf(verrm, "open_dev: AUDIO_DESCRIBE returns %d errno %d\n", rc, errno);
	perror(verrm);
    };
	
    /* go into test mode to allow access to Audio registers */
    for (i=0, test_mode=0; i<50 && !test_mode; i++) {
        if ((rc = ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer)) < 0) {
            if (errno != EBUSY) {
	         sprintf(verrm, "dma: TEST_MODE didn't work; errno %d (try %d)\n", errno, i);
	         perror(verrm);
	    }
        }
    
        /* 
         * try a TEST_RW_REGISTER; if it works we 
         * successfully got into test mode 
 	*/
    
	regio.audio_register = AUDIO_REG_ID;
	regio.writeflag = 0;
	rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
	if (rc < 0) {
	   sprintf(verrm, "dma: TEST_RW_REG after TEST_MODE failed; errno %d\n", errno);
	   perror(verrm);
	   WAIT(5,0);
	}
	else {
	   test_mode = 1;
	}
    }

    if (!test_mode) {
        sprintf(verrm, "can't get back into test mode!\n");
	perror(verrm);
	close_audio_dev();
	exit(1);
    }

#ifdef foo
    /* this is the old open routine */
    if ((rc = ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer)) < 0) {
	if (rc == -1 && errno == 1) 
	    sprintf(verrm, "open_dev: must be root to use AUDIO_TEST_MODE.\n");
	else
	    sprintf(verrm, "open_dev: ioctl error %d\n", errno);
	perror(verrm);
	close_audio_dev();
	exit(1);
    }
    for (i=0; i<MAX_TEST_BUFS; i++) {
        printf("test buffers addresses: %d - 0x%lx\n",i,tbuffer.test_buffer_addr[i]);
        fprintf(stderr,"test buffers addresses: %d - 0x%lx\n",i,tbuffer.test_buffer_addr[i]);
    }
#endif

    return(PASS);
}

/*/////////////////////////////////////////////////////////////////////////
// Write register
/////////////////////////////////////////////////////////////////////////*/
unsigned int write_reg(reg, data)
int reg;
unsigned int data;
{
    static short rc;

    regio.audio_register = reg;
    regio.writeflag = 1;
    regio.value = data;
    rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);

    if (rc < 0) {
	sprintf(verrm, "write_reg: (%c) ioctl error %d\n", reg, errno);
	perror(verrm);
	return(FAIL);
    }

    return(PASS);
}

/*/////////////////////////////////////////////////////////////////////////
// Read register
/////////////////////////////////////////////////////////////////////////*/
unsigned int read_reg(reg)
int reg;
{
    static int rc;
    unsigned int ret;

    regio.audio_register = reg;
    regio.writeflag = 0;
    rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
    ret = regio.value;
    
    if (rc < 0) {
	sprintf(verrm, "read_reg: (%d) ioctl error %d\n", reg, errno);
	perror(verrm);
    }
    return (ret);
}

/*/////////////////////////////////////////////////////////////////////////
// Verifiy register value
/////////////////////////////////////////////////////////////////////////*/
unsigned int verify_reg(reg, data)
int reg;
unsigned int data;
{
    int rc;
    unsigned long ret;

    regio.audio_register = reg;
    regio.writeflag = 0;
    rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
    ret = regio.value;
    
    if (rc < 0) {
	sprintf(verrm, "verify_reg: ioctl error %d\n", errno);
	perror(verrm);
    }
    if (ret != data) {
	sprintf(verrm, "verify_reg: %d should be %d, not %d\n", reg, data, ret);
	perror(verrm);
    }
}

/*/////////////////////////////////////////////////////////////////////////
// reset_audio:  check the ID register, reset the codec 
/////////////////////////////////////////////////////////////////////////*/
void reset_audio()
{
    unsigned int Id;

    /* check for a toggling codec clock */

    /* no access to diag register yet:

    int clkout, reads;
    reads = 0;
    clkout = read_reg(REG_DIAG);
    while (clkout == read_reg(REG_DIAG)) {
	if (reads++ > 50) {
	    sprintf(verrm,"reset_audio: clkout won't toggle.\n");
	    perror(verrm);
	}
    }
    */

    /* check the ID register */
    Id = read_reg(REG_ID);
    if ((Id & 0xfffe0fff) != 0x00140000 ) {
        sprintf(verrm, "reset_audio: incorrenct ID 0x%x\n", Id);
        perror(verrm);
	close_audio_dev();
	exit(1);
    }

    /* write the reset bit */
    /* we can't do this blindly due to hardware bug -- only do a reset
       if the control bit is set. */

    if (read_reg(REG_CNTL) & CNTL_STATUS) {
	fprintf(stderr, "asserting RESET 3\n");
        write_reg(REG_RESET, 1);
        WAIT(0,50000);
    }

    write_reg(REG_RESET, 0);

    /* clear the OVRANGE reg if necessary */
    if (read_reg(REG_OVRANGE)) 
	write_reg(REG_OVRANGE, 0);

}

/*/////////////////////////////////////////////////////////////////////////
// init_audio:  set some defaults for audio
/////////////////////////////////////////////////////////////////////////*/
void init_audio(init_type)
int init_type;
{

    switch (init_type) {

    case START:
        /* put 0xf in PIO register, per ERS */

        write_reg(REG_PIO, 0xf);

        /* set up values for control and gain registers */

        audio_regs.IS = 0;             /* line input */
        audio_regs.LS = 0;             /* no loopback */
        audio_regs.DF = 0;             /* 16-bit linear */
        audio_regs.XSSR = 9;           /* 16 kHz */
        audio_regs.HE = 1;             /* headset on */
        audio_regs.LE = 1;             /* line out on */
        audio_regs.SE = 0;             /* speaker on */
        audio_regs.ST = 1;             /* stereo on */
        audio_regs.MA = 15;            /* monitor off */
        audio_regs.LI = 0;             /* zero input gain */
        audio_regs.RI = 0;             /* zero input gain */
        audio_regs.LO = 0x2;           /* 30 dB output attenuation */
        audio_regs.RO = 0x2;           /* 30 dB output attenuation */
        write_control(1);
	write_reg(REG_OVRANGE, 0);
	break;

    case TEST:
        /* set up values for control and gain registers */

        audio_regs.IS = 0;             /* line input */
        audio_regs.DF = 0;             /* 16-bit linear */
        audio_regs.XSSR = 14;          /* 48 kHz */
#ifdef R5513
        audio_regs.XSSR = 16;          /* 5513 Hz */
#endif
#ifdef R8000
        audio_regs.XSSR = 8;          /* 8000 Hz */
#endif
#ifdef R11025
        audio_regs.XSSR = 17;          /* 11025 Hz */
#endif
        audio_regs.HE = 1;             /* headset on */
        audio_regs.LE = 0;             /* line out off */
        audio_regs.SE = 0;             /* speaker off */
        audio_regs.ST = 1;             /* stereo on */
        audio_regs.MA = 15;            /* monitor off */
        audio_regs.LI = 0;             /* zero input gain */
        audio_regs.RI = 0;             /* zero input gain */
        audio_regs.LO = 0;           /* 0 dB output attenuation */
        audio_regs.RO = 0;           /* 0 dB output attenuation */
        write_control(1);
	break;
    }
}

/*/////////////////////////////////////////////////////////////////////////
// write_gain:   write the gain control register
/////////////////////////////////////////////////////////////////////////*/
void write_gain()
{
    unsigned int word;

    word = 0;
    word |= audio_regs.HE << 27;
    word |= audio_regs.LE << 26;
    word |= audio_regs.SE << 25;
    word |= audio_regs.IS << 24;
    word |= audio_regs.MA << 20;
    word |= audio_regs.LI << 16;
    word |= audio_regs.RI << 12;
    word |= audio_regs.LO << 6;
    word |= audio_regs.RO;
    write_reg(REG_GAINCTL, word);
}


/*/////////////////////////////////////////////////////////////////////////
// write_driver_control:   write the control and gain registers using normal driver
/////////////////////////////////////////////////////////////////////////*/
int write_driver_control(wait)
int wait;
{

     /*  
	 Can't use AUDIO_RAW_SET_PARAMS for fear of the driver not
         handling it correctly.
     */

     struct cs4215conf conf;
     int rc;

     conf.control =  audio_regs.LS << 8;
     conf.control |= audio_regs.DF << 6;
     conf.control |= audio_regs.ST << 5;
     conf.control |= audio_regs.XSSR;

     conf.gainctl =  audio_regs.HE << 27;
     conf.gainctl |= audio_regs.LE << 26;
     conf.gainctl |= audio_regs.SE << 25;
     conf.gainctl |= audio_regs.IS << 24;
     conf.gainctl |= audio_regs.MA << 20;
     conf.gainctl |= audio_regs.LI << 16;
     conf.gainctl |= audio_regs.RI << 12;
     conf.gainctl |= audio_regs.LO << 6;
     conf.gainctl |= audio_regs.RO;

     rc = ioctl(dev_fd, AUDIO_RAW_SET_PARAMS, &conf);
     if (rc < 0) {
	 sprintf(verrm, "write_driver_control: ioctl error %d\n", errno);
	 perror(verrm);
	 return(FAIL);
     }

    /* wait 512 frames for calibration to complete */
    /*
    if (wait) WAIT(0,512000000/samp_rates[audio_regs.XSSR-8]);
    */

    return(PASS);
}

int nonsticks = 0;

/*/////////////////////////////////////////////////////////////////////////
// write_control:   write the control register
/////////////////////////////////////////////////////////////////////////*/
void write_control(wait)
int wait;
{
    unsigned int word;
    unsigned int LO_save, RO_save;
    unsigned int HE_save, SE_save, LE_save;
    unsigned int dstatus;
    register int i, j, rc;
    struct audio_regio       reg_io;
    register int queries = 0;
    register int control;

    /* mute the outputs */

    LO_save = audio_regs.LO;
    RO_save = audio_regs.RO;
    audio_regs.LO = 0x3F;
    audio_regs.RO = 0x3F;

    /* turn off the outputs */

    HE_save = audio_regs.HE;
    SE_save = audio_regs.SE;
    LE_save = audio_regs.LE;
    audio_regs.HE = 0;
    audio_regs.SE = 0;
    audio_regs.LE = 0;

    write_gain();

    /* if recording or playing, wait for it to stop */

    for (i=0; i<100; i++) {
        dstatus = read_reg(REG_DSTATUS);
	if ((dstatus & (DSTAT_RC|DSTAT_PC)) == 0)
	    break;
	WAIT(0,20000);
    }
    if (i==100) {
	/* display_register_state(); */
	sprintf(verrm, "write_control: Audio DMA won't stop.  (PC=0x%x, RC=0x%x\n", (dstatus&DSTAT_PC)<<8, dstatus&DSTAT_RC);
	perror(verrm);
	close_audio_dev();
	exit(1);
    }

    word = 0;
    word += audio_regs.LS << 8;
    word += audio_regs.DF << 6;
    word += audio_regs.ST << 5;
    word += audio_regs.XSSR;
    write_reg(REG_CNTL, word);
    WAIT(0,1000);

    /* turn on the outputs */
    audio_regs.HE = HE_save;
    audio_regs.SE = SE_save;
    audio_regs.LE = LE_save;

    /* unmute */

    audio_regs.LO = LO_save;
    audio_regs.RO = RO_save;
    write_gain();


    /* now do all the kludges to make sure the control bit didn't stick */


    /* read the control register twice jupiter may have the wrong data the first time */

    control = read_reg(REG_CNTL) & CNTL_STATUS;
    SHORT_WAIT(100);
    control = read_reg(REG_CNTL) & CNTL_STATUS;

    /* keep trying (max 100 tries) until control bit is clear */
    while ((control != 0) && (queries < 100)) {
	queries++;
	control = read_reg(REG_CNTL) & CNTL_STATUS;
	SHORT_WAIT(1000);
    }

    /* count the number of successive times it doesn't stick */
    if (!control) {
	nonsticks++;
    }

    if (control) {  /* stuck! */
	fprintf(stderr, "non-sticks before sticking: %d\n", nonsticks);
	nonsticks=0;

	/* assert reset */
        reg_io.audio_register = AUDIO_REG_RESET;
        reg_io.writeflag = 1;
        reg_io.value = 1;
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &reg_io);
        if (rc < 0) {
            sprintf(verrm, "write_control: ioctl error %d\n", errno);
            perror(verrm);
            return;
        }

	/* wait 50 msec */
        WAIT(0, 50000);

	/* de-assert reset */
        reg_io.value = 0;
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &reg_io);
        if (rc < 0) {
            sprintf(verrm, "write_control: ioctl error %d\n", errno);
            perror(verrm);
        }

	/* write the control register again */
        regio.audio_register = AUDIO_REG_CNTL;
        regio.writeflag = 1;
        regio.value = audio_regs.LS<<8 |
		      audio_regs.DF<<6 |
		      audio_regs.ST<<5 |
		      audio_regs.XSSR;
    
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
        if (rc < 0) {
	    sprintf(verrm, "write_control: ioctl error %d\n", errno);
	    perror(verrm);
	    return;
        }

	queries = 0;
        while ((control != 0) && (queries < 100)) {
	    queries++;
	    control = read_reg(REG_CNTL) & CNTL_STATUS;
	    SHORT_WAIT(1000);
        }
	if (control) 
	    sprintf(verrm,"C bit won't clear.\n");
	else 
	    sprintf(verrm,"C bit cleared after sticking.\n");
    }
    
    for (j = 0; ((j < 1000) && (read_reg(REG_DIAG) == 0)); j++) ;

    if (j == 1000) { 
	/* jupiter and codec are waiting on each other to drive the
	   clock -- write the control reg and assert reset */

	sprintf(verrm, "audio clock not running; asserting reset.\n");
	perror(verrm);
        regio.audio_register = AUDIO_REG_CNTL;
        regio.writeflag = 1;
        regio.value = audio_regs.LS<<8 |
		      audio_regs.DF<<6 |
		      audio_regs.ST<<5 |
		      audio_regs.XSSR;
    
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
        if (rc < 0) {
	    sprintf(verrm, "write_control: ioctl error %d\n", errno);
	    perror(verrm);
	    return;
        }

	/* assert reset */
        reg_io.audio_register = AUDIO_REG_RESET;
        reg_io.writeflag = 1;
        reg_io.value = 1;
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &reg_io);
        if (rc < 0) {
            sprintf(verrm, "write_control: ioctl error %d\n", errno);
            perror(verrm);
            return;
        }

	/* wait 50 msec */
        WAIT(0, 50000);

	/* de-assert reset */
        reg_io.value = 0;
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &reg_io);
        if (rc < 0) {
            sprintf(verrm, "write_control: ioctl error %d\n", errno);
            perror(verrm);
        }

	/* write the control reg again */
        regio.audio_register = AUDIO_REG_CNTL;
        regio.writeflag = 1;
        regio.value = audio_regs.LS<<8 |
		      audio_regs.DF<<6 |
		      audio_regs.ST<<5 |
		      audio_regs.XSSR;
    
        rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
        if (rc < 0) {
	    sprintf(verrm, "write_control: ioctl error %d\n", errno);
	    perror(verrm);
	    return;
        }

	
    }

    /* wait 512 frames for calibration to complete */
    /*
    if (wait) WAIT(0,(long)(512000000/samp_rates[audio_regs.XSSR-8]));
    */

    /* clear overrange register */

    write_reg(REG_OVRANGE, 0);

    /*
    signal(SIGINT, icatch);
    */
    return;
}

/*/////////////////////////////////////////////////////////////////////////
// enter_test_mode:
/////////////////////////////////////////////////////////////////////////*/
void enter_test_mode()
{
    int i, j, rc, test_mode=0;
    long wait_time;
    struct audio_status audiostat;

#ifdef NEWDRIVER
    int mode_from_driver;
    if (ioctl(dev_fd, AUDIO_TEST_MODE_STATUS, &mode_from_driver) < 0) {
        sprintf(verrm, "begin enter_test_mode: error from AUDIO_TEST_MODE_STATUS %d \n",errno);
	perror(verrm);
    }
    else if (dma_debug >= 1) 
            printf("begin enter_test_mode: AUDIO_TEST_MODE_STATUS is %s.\n", mode_from_driver?"ON":"OFF");
#endif

    signal(SIGINT, SIG_IGN);
    if ((rc = ioctl(dev_fd, AUDIO_DRAIN, 0)) < 0) {
	sprintf(verrm, "enter_test_mode: AUDIO_DRAIN errno %d\n", errno);
	perror(verrm);
    }
    /*
    signal(SIGINT, icatch);
    */

    if ((rc = ioctl(dev_fd, AUDIO_RESET, RESET_TX_BUF)) < 0) {
	sprintf(verrm, "enter_test_mode: AUDIO_RESET errno %d\n", errno);
	perror(verrm);
    }

    if ((rc = ioctl(dev_fd, AUDIO_RESET, RESET_RX_BUF)) < 0) {
	sprintf(verrm, "enter_test_mode: AUDIO_RESET errno %d\n", errno);
	perror(verrm);
    }

    ioctl(dev_fd, AUDIO_GET_STATUS, &audiostat);
    if ((rc = ioctl(dev_fd, AUDIO_RESET, RESET_RX_OVF)) < 0) {
	sprintf(verrm, "enter_test_mode: AUDIO_RESET errno %d\n", errno);
	perror(verrm);
    }

    if ((rc = ioctl(dev_fd, AUDIO_RESET, RESET_TX_UNF)) < 0) {
	sprintf(verrm, "enter_test_mode: AUDIO_RESET errno %d\n", errno);
	perror(verrm);
    }
	
    rc = ioctl(dev_fd, AUDIO_RESET, RESET_RX_BUF|RESET_TX_BUF|
			     RESET_RX_OVF|RESET_TX_UNF);
    if (rc < 0) {
	sprintf(verrm, "enter_test_mode: RESET_ALL returned %d errno %d\n", rc, errno);
	perror(verrm);
    }

    /* wait for 3 DBUFSZ buffers to complete */

    /*
    wait_time = (long) ((3*DBUFSZ/PAGESZ)*1000000.0*SAMPLES_PER_PAGE/SAMPLES_PER_SEC);
    WAIT(wait_time/1000000, wait_time%1000000);
    */

    for (i=0; i<50 && !test_mode; i++) {
	if ((rc = ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer)) < 0) {
	    if (rc == -1 && errno == 1) {
		sprintf(verrm, "enter_test_mode: must be root to use AUDIO_TEST_MODE.  \n");
	    }
	    else if (errno == EBUSY) {
		WAIT(0, 5000);
	    }
	    else {
		for (j=0; j<5; j++) {
		    WAIT(0, 1000);
		    rc = ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer);
		    if (rc>=0) break;
		}
		if (j>4) 
		    sprintf(verrm, "enter_test_mode: TEST_MODE didn't work; errno %d (try %d)\n", errno, i);
	    }
	    perror(verrm);
	}

	/* 
	 * try a TEST_RW_REGISTER; if it works we 
	 * successfully got into test mode 
	 */

	regio.audio_register = AUDIO_REG_ID;
	regio.writeflag = 0;
	rc = ioctl(dev_fd, AUDIO_TEST_RW_REGISTER, &regio);
	if (rc < 0) {
	    sprintf(verrm, "enter_test_mode: TEST_RW_REG after TEST_MODE failed; errno %d\n", errno);
	    /*
	    Vwarn(0);
	    */
	    WAIT(0,100000);
	}
	else {
	    test_mode = 1;
	}
    }

    if (!test_mode) {
	sprintf(verrm, "can't get into test mode!\n");
	perror(verrm);
	close_audio_dev();
	exit(1);
    }

#ifdef NEWDRIVER
    if (ioctl(dev_fd, AUDIO_TEST_MODE_STATUS, &mode_from_driver) < 0) {
        sprintf(verrm, "leave enter_test_mode: error from AUDIO_TEST_MODE_STATUS %d \n",errno);
	perror(verrm);
    }
    else if (dma_debug >= 1) 
            printf("leave enter_test_mode: AUDIO_TEST_MODE_STATUS is %s.\n", mode_from_driver?"ON":"OFF");
#endif
}

/*/////////////////////////////////////////////////////////////////////////
// exit_test_mode:
/////////////////////////////////////////////////////////////////////////*/
int exit_test_mode()
{
    int tries=0, done=0, ret_val=0;

#ifdef NEWDRIVER
    int mode_from_driver;
    if (ioctl(dev_fd, AUDIO_TEST_MODE_STATUS, &mode_from_driver) < 0) {
        sprintf(verrm, "begin exit_test_mode: error from AUDIO_TEST_MODE_STATUS %d \n", errno);
        perror(verrm);
    }
    else if (dma_debug >= 1) 
            printf("begin exit_test_mode: AUDIO_TEST_MODE_STATUS is %s.\n", mode_from_driver?"ON":"OFF");
#endif

    while (!done) { /* wait upto one minute */
        if (ioctl(dev_fd, AUDIO_TEST_MODE, &tbuffer) < 0) {
	    if (errno == 1) {
	        sprintf(verrm, "exit_test_mode: must be root to use AUDIO_TEST_MODE.\n");
		perror(verrm);
		done = 1;
                ret_val = 1;
	    } else {
	        sprintf(verrm, "exit_test_mode: ioctl error %d\n", errno);
	        perror(verrm);
	        WAIT(1, 0);
		if (tries++ > 60) done = 1;
	    }
        }
	else done = 1;
    }
    if (tries>60) {
       sprintf(verrm, "exit_test_mode: too many consecutive failures.\nUnable to allocate system memory!  Exiting test!\n");
       perror(verrm);
       exit(1);
    }

#ifdef NEWDRIVER
    if (ioctl(dev_fd, AUDIO_TEST_MODE_STATUS, &mode_from_driver) < 0) {
        sprintf(verrm, "leave exit_test_mode: error from AUDIO_TEST_MODE_STATUS %d \n", errno);
        perror(verrm);
    }
    else {
        if (dma_debug >= 1) 
            printf("leave exit_test_mode: AUDIO_TEST_MODE_STATUS is %s.\n", mode_from_driver?"ON":"OFF");
    }
#endif

    return (ret_val);
}

#define PERIOD 1024

generate_sine_sound( )
{
    int i;
    float factor;

    orig = (short *)malloc( PERIOD );
    for( i = 0; i < PERIOD/2; i++ ) {
    /*
	factor = (i / (float)(PERIOD/2)) * 64.0 * M_PI;
    */
	factor = (i / (float)(PERIOD/2)) * 16.0 * M_PI;
	orig[i] = sin( factor ) * 32767.0;
    }
    AdjustSample( orig, PERIOD/2 );
}


/* Copy a new sample into ping/pong, resampling as we go */
int CopySample
(
    short *Src, int SrcSize,		/* Source buffer to copy */
    short *Ping, short *Pong,		/* Dest. buffer to copy to */
    int PingStart, int BufSize,		/* Where to start, how big buffs are */
    float Factor			/* Resampling factor <= 1.0 */
)
{
    int
	i, n,
	p0, p1;
    float
	v0, v1, v,
	t, f, Last;

    /* Check for valid factors */
    if( (Factor <= 0.0) || (Factor > 1.0) )	return -1;

    /* Copy into Ping */
    n = PingStart;
    t = 0.0;
    Last = SrcSize;
    while( n < BufSize ) {
	/* Get bracketing points */
	p0 = ((int)t) % SrcSize;
	p1 = (p0 + 1) % SrcSize;

	/* Get fractional offset */
	f = t - (float)p0;
	v0 = (float)Src[p0]; v1 = (float)Src[p1];

	/* Interpolate and store */
	v = (1.0 - f)*v0 + f*v1;
	Ping[n] = (int)(v + 0.5);

	/* Increment to next point */
	n++;
	t += Factor;
	while( t > Last ) t -= Last;
    }

    /* Copy into Pong */
    n = 0;
    while( (t < Last) && (n < BufSize) ) {
	/* Get bracketing points */
	p0 = ((int)t) % SrcSize;
	p1 = (p0 + 1) % SrcSize;

	/* Get fractional offset */
	f = t - (float)p0;
	v0 = (float)Src[p0]; v1 = (float)Src[p1];

	/* Interpolate and store */
	v = (1.0 - f)*v0 + f*v1;
	Pong[n] = (int)(v + 0.5);

	/* Increment to next point */
	n++;
	t += Factor;
    }

    /* Return number of words we copied into Pong */
    return n;
}

/* Try to adjust a sample so it doesn't "pop" when it cycles */
int AdjustSample( short *Sample, int Count )
{
    int
	i, First,
	Last;

    /* Find first 'zero' sample */
    for( First = 0; First < Count; First++ ) {
	if( (Sample[First] > -256) && (Sample[First] < 256) ) {
	    break;
	}
    }
    if( First >= Count ) {
	return -1;
    }

    Last = Count;
    while( 1 ) {
	/* Find last 'zero' sample */
	for( Last = Last-1; Last > First; Last-- ) {
	    if( (Sample[Last] > -256) && (Sample[Last] < 256) ) {
		break;
	    }
	}
	if( Last <= First ) {
	    return -1;
	}
	/* See if it matches */
	if( (Sample[First+1] ^ Sample[Last-1]) < 0 ) {
	    /* Opposite signs */
	    break;
	}
    }

    /* Adjust the buffer and return the total number of samples */
    for( i = First; i <= Last; i++ ) {
	Sample[i-First] = Sample[i];
    }
    return Last-First+1;
}


/* Code to resample a sound sample to a different frequency.
 * It returns
 * 0 if it could not resample, 1 otherwise.  It will use
 * averaging as an approximation to decimation.
 */

int Resample( short *Src, short *Dst, int Count, float Factor )
{
    int
	i, j, 		/* Counter over destination points */
	n,		/* Decimation factor */
	p0, p1;		/* Previous and next source point */
    float
	v0, v1,		/* Previous and next sample values */
	Res,		/* Interpolated value */
	t, f;		/* Current time factor, current time fraction */
    short
	*Tmp = 0;

    if( Factor <= 0.0 ) {
	/* Barf-o-rama: can't scale a sound *that* small! */
	return 0;
    }

    /* Figure out decimation factor */
    n = (int)( Factor);

    if( n > 1 ) {
	/* We should decimate the source; we need a temporary buffer */
	Tmp = malloc( Count * sizeof(short) );
	if( !Tmp )	return 0;

	/* Average smooth (decimate) */
	for( i = 0; i < Count; i++ ) {
	    /* Get range of values to average */
	    p0 = (i + Count - n/2) % Count;
	    p1 = (p0 + n) % Count;

	    /* Sum up that range */
	    Res = 0.0;
	    for( j = p0; j != p1; j = (j+1)%Count ) {
		Res += Src[j];
	    }

	    /* Calculate result and put it back */
	    Tmp[i] = (Res / n) + 0.5;
	}

	/* Kludge the Src pointer to point to the smoothed buffer */
	Src = Tmp;
    }

    /* For each destination sample, do */
    for( i = 0, t = 0.0; i < Count; i++, t += Factor ) {
	/* Figure out source samples that surround this value */
	p0 = ((int)t) % Count;
	p1 = (p0 + 1) % Count;
	v0 = Src[p0]; v1 = Src[p1];

	/* Get fractional value and interpolate */
	f = t - (int)t;
	Res = (1.0 - f)*v0 + f*v1;

	/* Stuff it in the destination */
	Dst[i] = Res + 0.5;
    }

    /* Free our temporary buffer if necessary */
    if( Tmp ) free( Tmp );

    /* Success! */
    return 1;
}
