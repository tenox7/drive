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
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/param.h>
#endif

#include "global.h"
#include "drive_server.h"
#include "filenames.h"



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
#define FIELD_NEG_BITFIELD	11

#define CONFIGFILENAME ".server_config"


static void parse_server_config(
    char *fname,
    FILE *fileptr)
{
    typedef struct {
	char	*header;
	int	header_strlen;
	int	type;
	void	*addr;
	unsigned int value;
    } FIELD;
    static FIELD field[] = {
	{ "Normal Mode",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_NORMAL },
	{ "Race Mode",		0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_RACE },
	{ "Demo Interrupt Mode",0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_DEMO_INTERRUPT },
	{ "Demo Mode",		0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_DEMO },
	{ "Night Driving",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_NIGHT_DRIVING },
	{ "Spacecraft Races",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_ALLOW_SPACE_RACES },
	{ "Reread Scenefiles",	0,	FIELD_BOOL,
	    &server_reread_scenefiles, 0 },

	{ "Practice Seconds:",	0,	FIELD_INT,
	    &default_course.practice_seconds, 0},
	{ "Pre-Race Seconds:",	0,	FIELD_INT,
	    &default_course.pre_race_seconds, 0 },
	{ "Race Seconds:",	0,	FIELD_INT, 
	    &default_course.race_seconds, 0 },
	{ "Post-Race Seconds:",	0,	FIELD_INT, 
	    &default_course.post_race_seconds, 0 },
	{ "Start SceneX:",	0,	FIELD_INT, 
	    &default_course.start_scene_x, 0 },
	{ "Start SceneZ:",	0,	FIELD_INT, 
	    &default_course.start_scene_z, 0 },
	{ "Start Angle:",	0,	FIELD_FLOAT, 
	    &default_course.start_angle, 0 },
	{ "Start Position:",	0,	FIELD_3FLOAT,
	    default_course.start_position, 0},
	{ "Start Positions:",	0,	FIELD_INT, 
	    &default_course.num_positions, 0},
	{ "Turbo Mode:",	0,	FIELD_INT, 
	    &default_course.turbo_mode, 0},
	{ "Turbo Boosts:",	0,	FIELD_INT, 
	    &default_course.turbo_boosts, 0},
	{ "Guns Mode:",		0,	FIELD_INT, 
	    &default_course.guns_mode, 0},
	{ "Ammo:",		0,	FIELD_INT, 
	    &default_course.ammo, 0},
	{ "Allow Spacecraft",	0,	FIELD_BITFIELD,
	    &allowable_vehicles, OBJECTCLASS_SPACESHIP },
	{ "Disallow Tanks",	0,	FIELD_NEG_BITFIELD,
	    &allowable_vehicles, OBJECTCLASS_TANK },
	{ "Disallow Motorcycles",	0,	FIELD_NEG_BITFIELD,
	    &allowable_vehicles, OBJECTCLASS_MOTORCYCLE },
	{ "Disallow UFOs",	0,	FIELD_NEG_BITFIELD,
	    &allowable_vehicles, OBJECTCLASS_UFO },
	{ "Explosions Disabled",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_EXPLOSION_DISABLED },
	{ "Explosions Visual",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_EXPLOSION_VISUAL },
	{ "Explosions Force",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_EXPLOSION_FORCE },
	{ "Explosions Restart",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_EXPLOSION_RESTART },
	{ "Checkpoint Reload",	0,	FIELD_BITFIELD,
	    &server_mode, SERVER_MODE_CHKPT_RELOAD },
    };
#   define NUMFIELDS (sizeof(field)/sizeof(FIELD))
    int i,j;
    char str[4096],*cptr,*cptr2;
    float *fptr;



    /* Initialize header strlens, if not already done. */
    if (field[0].header_strlen == 0) {
	for (i=0; i<NUMFIELDS; ++i) {
	    field[i].header_strlen = strlen(field[i].header);
	}
    }

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
			    if( strncmp(field[i].header,"Explosion",9) == 0)
			    {
				/* We need to undo what was, and set what is */
				*intptr  &= ~SERVER_EXPLOSION_MODES;
				*intptr |= field[i].value;
			    }
			    else  /* normal bitfield */
			    {
				*intptr |= field[i].value;
			    }
		        }
			break;
		    case FIELD_NEG_BITFIELD:
		        {
			    unsigned int *intptr;

			    intptr = (unsigned int *)(field[i].addr);
			    *intptr &= ~(field[i].value);
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
}


void read_config_file(
    void)
{
    FILE *fptr;


    if (access(config_filename,04) != 0) return;

    if ((fptr = fopen(config_filename,"r")) == NULL) {
	return;
    }

    parse_server_config(config_filename,fptr);

    fclose(fptr);
}
