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


/* sound.h */

#if !defined(_SOUND_H_INCLUDED)
#define _SOUND_H_INCLUDED 1

#define MAX_SOUNDS 	20
#define DBUFSZ          8192
#define AUD_BUFSIZE     16384

#define PASS	0
#define FAIL	1

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

typedef enum command { NOOP_AUDIO, OPEN_AUDIO, RESET_AUDIO, 
		       QUIT_AUDIO, PLAY_AUDIO } type_sound_command;

#define START 		0
#define TEST 		1
#define SILENCE 	0

#define SKEY	(('S'<<24)|('O'<<16)|('N'<<8)|'D')

typedef struct sound_struct {
    char 	fileName[80];
    int	 	fileLen;
    char	*data;
    int		dataPlayLen;
} type_sound_struct;

typedef struct playList {
    int			numbSounds;
    struct sound_struct	sound[MAX_SOUNDS];
} type_sound_playList;

typedef struct Sound {
    volatile enum command 	command;
    volatile int 		sound;
    volatile int			newsound;
    volatile float		rpm;
} type_Sound;

/* //// F U N C T I O N  P R O T O T Y P E S ///////////////////////// */
int read_sound_config_file();
int load_sound_files();


/* //// O L D   D E F I N E S //////////////////////////////////////// */

/*
 *  Register name definitions
 *  NOTE:  this definitions must follow the same order as the
 *         reg_name stucture in registers.c
 */
#define REG_ID          AUDIO_REG_ID       /* Id register */
#define REG_RESET       AUDIO_REG_RESET    /* reset */
#define REG_CNTL        AUDIO_REG_CNTL     /* codec control */
#define REG_GAINCTL     AUDIO_REG_GAINCTL  /* codec gain control */
#define REG_PNXTADD     AUDIO_REG_PNXTADD  /* playback next address */
#define REG_PCURADD     AUDIO_REG_PCURADD  /* playback current address */
#define REG_RNXTADD     AUDIO_REG_RNXTADD  /* record next address */
#define REG_RCURADD     AUDIO_REG_RCURADD  /* record current address */
#define REG_DSTATUS     AUDIO_REG_DSTATUS  /* DMA and interrupt status */
#define REG_OVRANGE     AUDIO_REG_OVRANGE  /* over-range indicator */
#define REG_PIO         AUDIO_REG_PIO      /* pio register */
#define REG_DIAG        AUDIO_REG_DIAG     /* diagnostic:  chi bit clock */

#define NUM_OF_REGS     24      /* number of total registers above */

/* undefine these, as they are defined in the BE as of 10.20ic3 */
#ifdef HE
#undef HE
#endif
#ifdef SE
#undef SE
#endif

struct registers {
        unsigned int ID;                       /* ID       register */
        unsigned int RESET;                    /* RESET    register */
        unsigned int C;                        /* CNTL     register */
        unsigned int LS;                       /* CNTL     register */
        unsigned int DF;                       /* CNTL     register */
        unsigned int ST;                       /* CNTL     register */
        unsigned int XSSR;                     /* CNTL     register */
        unsigned int PNXTADD;                  /* PNXTADD  register */
        unsigned int PCURADD;                  /* PCURADD  register */
        unsigned int RNXTADD;                  /* PCURADD  register */
        unsigned int RCURADD;                  /* PCURADD  register */
        unsigned int IE;                       /* DSTATUS  register */
        unsigned int PN, PC, RN, RC;           /* DSTATUS  register */
        unsigned int HE, LE, SE;               /* GAINCTL  register */
        unsigned int IS;                       /* GAINCTL  register */
        unsigned int MA;                       /* GAINCTL  register */
        unsigned int LI, RI;                   /* GAINCTL  register */
        unsigned int LO, RO;                   /* GAINCTL  register */
        unsigned int OV;                       /* OV       register */
        unsigned int PI, PO;                   /* PIO      register */
        unsigned int CO;                       /* DIAG     register */
};


#define   SOUND_SILENCE		0
#define   SOUND_ENGINE		1
#define   SOUND_SHOOT		2
#define   SOUND_COLLISION	3
#define   SOUND_EXPLOSION	4
#define   SOUND_SQUEAL		5
#define   SOUND_SCREECH		6
#define   SOUND_IGNITION	7
#define   SOUND_MYHORN		8
#define   SOUND_HORN		9
#define   SOUND_KLAXON		10
#define   SOUND_PEAL  		11

#endif /* !_SOUND_H_INCLUDED */
