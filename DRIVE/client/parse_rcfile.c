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
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <sys/param.h>
#endif

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

#include "global.h"
#include "drive.h"
#include "camera.h"

#define FIELD_INT	0
#define FIELD_2INT	1
#define FIELD_3INT	2
#define FIELD_FLOAT	3
#define FIELD_2FLOAT	4
#define FIELD_3FLOAT	5
#define FIELD_MATRIX	6
#define FIELD_STRING	7
#define FIELD_BOOL	8
#define FIELD_CONSTANT	9
#define FIELD_BITFIELD	10

#ifdef WIN32
#define RCFILENAME "driverc.txt"
#else
#define RCFILENAME ".driverc"
#endif


static void parse_rcfile(
    char *fname,
    FILE *fileptr)
{
    static char vehicle_selected[256] = "";
    typedef struct {
	char	*header;
	int	header_strlen;
	int	type;
	void	*addr;
	unsigned int value;
    } FIELD;
    static char colorname[256] = "";
    static char static_hostname[256] = "";
    static FIELD field[] = {
	{ "Autostart",		0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_AUTOSTART_MODE },
	{ "Vehicle:",		0,	FIELD_STRING,
	      vehicle_selected, 0},
	{ "Color:",		0,	FIELD_3FLOAT,
	      cstate.upd.color, 0	},
	{ "Colorname:",		0,	FIELD_STRING,
	      colorname,	0	},
	{ "Confine Cursor",	0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_CONFINE_CURSOR_MODE},
	{ "Confine Small",	0,	FIELD_BITFIELD,	
	      &(cstate.mode), CLIENT_CONSTRAIN_CURSOR_MODE },
	{ "Server:",		0,	FIELD_STRING,
	      static_hostname, 0 },
	{ "Automatic Camera",	0,	FIELD_CONSTANT,
	      &(cstate.camera.mode), CAM_MODE_AUTOCAM },
	{ "Overhead Camera",	0,	FIELD_CONSTANT,
	      &(cstate.camera.mode), CAM_MODE_SKYCAM },
	{ "Automatic Watch Mode", 0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_AUTOWATCH_MODE },
	{ "Watch Mode",		0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_WATCH_MODE },
	{ "Camera Up Angle:",	0,	FIELD_INT,
	      &(cstate.camera.skycam_up_angle), 0},
	{ "Camera Around Angle:",0,	FIELD_INT,
	      &(cstate.camera.skycam_around_angle), 0},
	{ "Camera Distance:",	0,	FIELD_FLOAT,
	      &(cstate.camera.skycam_distance), 0},
	{ "Automatic Watch Frames:",0,	FIELD_INT,
	      &(cstate.watch_frames), 0},
	{ "Siggraph Mode",	0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_ROBUST_MODE},
	{ "Small Window",	0,	FIELD_BITFIELD,
	      &(cstate.mode), CLIENT_SMALLWINDOW_MODE },
	{ "Scene X:",	0,	FIELD_INT,  &(cstate.start_pos.scene_x), 0 },
	{ "Scene Z:",	0,	FIELD_INT,  &(cstate.start_pos.scene_z), 0 },
	{ "Position:", 	0, 	FIELD_3FLOAT, &(cstate.start_pos.pos[0]), 0 },
	{ "X:",		0, 	FIELD_FLOAT,  &(cstate.start_pos.pos[0]), 0 },
	{ "Y:",		0, 	FIELD_FLOAT,  &(cstate.start_pos.pos[1]), 0 },
	{ "Z:",		0, 	FIELD_FLOAT,  &(cstate.start_pos.pos[2]), 0 },
	{ "Angle:", 	0, 	FIELD_FLOAT,  &(cstate.start_pos.angle), 0 },
	{ "Nickname:",	0,	FIELD_STRING, &(cstate.nickname[0]), 0},
	{ "Use Joystick",	0,	FIELD_BOOL, &(cstate.use_joystick), 1},
	{ "Use Sound",	0,	FIELD_BOOL, &(cstate.use_sound), 1},
	{ "Sound Config:",	0,	FIELD_STRING, 
		&(cstate.sound_config[0]), 0},
	{ "Radar",	0,	FIELD_BITFIELD,
		&(cstate.mode), CLIENT_INITIAL_RADAR },
	{ "Dual Radar",	0,	FIELD_BITFIELD,
		&(cstate.mode), CLIENT_DUAL_RADAR_MODE },
	{ "Heads Up Display",	0,	FIELD_BITFIELD,
		&(cstate.mode), CLIENT_HUD_MODE },
	{ "Standings",	0,	FIELD_BITFIELD,
		&(cstate.mode), CLIENT_INITIAL_STANDINGS },
    };
#   define NUMFIELDS (sizeof(field)/sizeof(FIELD))
    int i,j;
    char str[4096],*cptr,*cptr2;
    float *fptr;


    hostname = static_hostname;

    /* Initialize header strlens, if not already done. */
    if (field[0].header_strlen == 0) {
	for (i=0; i<NUMFIELDS; ++i) {
	    field[i].header_strlen = strlen(field[i].header);
	}
    }

    /* Initialize colors */
    cstate.colorname = NULL;
    hsv_to_rgb(FLOATRAND(1.0),1.0,0.5+FLOATRAND(0.5),
	&cstate.upd.color[0], &cstate.upd.color[1], &cstate.upd.color[2]);

    while (fgets(str,sizeof(str),fileptr) != NULL) {
	/* Ignore comments */
	if (str[0] == '#') continue;

	/* Skip blank stuff */
	cptr = str;
	while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;

	/* Ignore blank lines */
	if ((*cptr == '\n') || (*cptr == '\0')) continue;

	for (i=0; i<NUMFIELDS; ++i) {
	    if (strncmp(cptr,field[i].header,field[i].header_strlen) == 0) {
		/* Got a match! */
		switch (field[i].type) {
		    case FIELD_BOOL:
			*((int *) field[i].addr) = TRUE;
			break;

		    case FIELD_INT:
			if (sscanf(cptr+field[i].header_strlen,"%d",
				    (int *) field[i].addr)
				< 1) {
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_2INT:
			if (sscanf(cptr+field[i].header_strlen,"%d %d",
				    (int *) field[i].addr,
				    (int *) field[i].addr + 1)
				< 2) { 
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_3INT:
			if (sscanf(cptr+field[i].header_strlen,"%d %d %d",
				    (int *) field[i].addr,
				    (int *) field[i].addr + 1,
				    (int *) field[i].addr + 2)
				< 3) { 
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f",
				    (float *) field[i].addr)
				< 1) {
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_2FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f %f",
				    (float *) field[i].addr,
				    (float *) field[i].addr + 1)
				< 2) {
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_3FLOAT:
			if (sscanf(cptr+field[i].header_strlen,"%f %f %f",
				    (float *) field[i].addr,
				    (float *) field[i].addr + 1,
				    (float *) field[i].addr + 2)
				< 3) {
			    /* error */
			    i = NUMFIELDS;
			}
			break;

		    case FIELD_MATRIX:
			fptr = (float *) field[i].addr;
			if (sscanf(cptr+field[i].header_strlen,"%f %f %f %f",
				    fptr,
				    fptr + 1,
				    fptr + 2,
				    fptr + 3)
				< 4) {
			    /* error */
			    i = NUMFIELDS;
			}
			fptr += 4;
			for (j = 0; j<3; ++j) {
			    if (fgets(str,sizeof(str),fileptr) == NULL) {
				/* error */
				i = NUMFIELDS;
				break;
			    }
			    if (sscanf(str,"%f %f %f %f",
					fptr,
					fptr + 1,
					fptr + 2,
					fptr + 3)
				    < 4) {
				/* error */
				i = NUMFIELDS;
				break;
			    }
			    fptr += 4;
			}
			break;

		    case FIELD_CONSTANT:
		        {
			    unsigned int *intptr;

			    intptr = (unsigned int *)(field[i].addr);
			    *intptr = field[i].value;
		        }
			break;

		    case FIELD_BITFIELD:
		        {
			    unsigned int *intptr;

			    intptr = (unsigned int *)(field[i].addr);
			    *intptr |= field[i].value;
		        }
			break;

		    case FIELD_STRING:
			cptr += field[i].header_strlen;
			/* skip blank space */
			while ((*cptr == ' ') || (*cptr == '\t')) ++cptr;
			cptr2 = cptr + (strlen(cptr)-1); /* last letter */
			if (*cptr2 == '\n') *cptr2 = '\0';
			strcpy((char *) field[i].addr,cptr);
			break;
		}
		break;  /* out of string search loop. */
	    }
	}

	if (i >= NUMFIELDS) {
	    fprintf(stderr,"%s rc file:  bad syntax:  %s.\n",
		fname,str);
	}
    }

    if (vehicle_selected[0] != '\0')
	cstate.car_name = vehicle_selected;

    if (colorname[0] != '\0') {
	cstate.colorname = colorname;
    }
}


void read_rcfile(
    void)
{
    char path[MAXPATHLEN];
    FILE *fptr;

#ifndef WIN32
    if (getenv("HOME") != NULL) {
	sprintf(path,"%s/%s",getenv("HOME"),RCFILENAME);
    }
    else {
	sprintf(path,"/users/%s/%s",getlogin(),RCFILENAME);
    }
#else
    sprintf(path,"%s",RCFILENAME);
#endif

    if (access(path,04) != 0) return;

    if ((fptr = fopen(path,"r")) == NULL) {
	fprintf(stderr,"Could not read rc file: %s\n",path);
	fprintf(stderr,"Proceeding with default values.\n");
	return;
    }

    parse_rcfile(path,fptr);

    fclose(fptr);
}
