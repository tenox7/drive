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
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/param.h>
#endif

#include "filenames.h"

/* Global data */
char dash_filename[MAXPATHLEN];
char gear_filename[7][MAXPATHLEN];
char compass_filename[MAXPATHLEN];
char title_filename[MAXPATHLEN];
char visctrls_filename[MAXPATHLEN];
char server_programname[MAXPATHLEN];
char help_filename[MAXPATHLEN];
char config_filename[MAXPATHLEN];
char scene_dir[MAXPATHLEN];
char construct_dir[MAXPATHLEN];
char object_block_filename[MAXPATHLEN];
char drivedir[MAXPATHLEN+1];

#define ACCESS_EXECUTE	01
#define ACCESS_READ	04
#define ACCESS_ERROR	(-1)

void construct_filenames(
    char *drivepath)
{
    char savedir[100*MAXPATHLEN+1],*cptr,*cptr2,c;
    char server_configdir[MAXPATHLEN+1];
    int got_it;

    /* A reasonable default */
    strcpy(drivedir,DEFAULT_DIRECTORY);

    if ((cptr = getenv("DRIVE_DIRECTORY")) != NULL) {
	strcpy(drivedir,cptr);
    }
    else if (*drivepath == '/') {
	/* absolute pathname */
	strcpy(drivedir,drivepath);
	/* strip the "/drive" part */
	cptr = strrchr(drivedir,'/'); *cptr = '\0';
	if (*drivedir == '\0') strcpy(drivedir,"/");
    }
    else if (*drivepath == '.') {
	strcpy(drivedir,drivepath);
	if ((cptr = strrchr(drivedir,'/')) == NULL) {
	    /* Path is local directory */
	    getcwd(drivedir,sizeof(drivedir));
	}
	else {
	    /* The path may be full of ..'s and .'s */
	    /* strip the "/drive" part */
	    *cptr = '\0';
	    /* An easy way to resolve the path is change there and back. */
	    getcwd(savedir,sizeof(savedir));
	    if (chdir(drivedir) == 0) {
		getcwd(drivedir,sizeof(drivedir));
		chdir(savedir);
	    }
	    else {
		/* Hosed */
		strcpy(drivedir,DEFAULT_DIRECTORY);
	    }
	}
    }
    else {
#ifdef WIN32
        strcpy(drivedir,DEFAULT_DIRECTORY);
#else
	/* Follow the directories in the PATH */
	if (getenv("PATH") != NULL) {
	    strcpy(savedir,getenv("PATH"));
	    cptr = savedir;
	    got_it = 0;
	    do {
		cptr2 = cptr;
		while ((*cptr2 != ':') && (*cptr2 != '\0')) ++cptr2;
		c = *cptr2;
		*cptr2 = '\0';
		sprintf(drivedir,"%s/drive",cptr);
		if (access(drivedir,ACCESS_EXECUTE) == 0) {
		    /* Got it! */
		    strcpy(drivedir,cptr);
		    got_it = 1;
		    break;
		}
		sprintf(drivedir,"%s/PEXdrive",cptr);
		if (access(drivedir,ACCESS_EXECUTE) == 0) {
		    /* Got it! */
		    strcpy(drivedir,cptr);
		    got_it = 1;
		    break;
		}
		cptr = cptr2+1;
	    } while (c == ':');
	    if (!got_it) {
		/* Hosed */
		/* First, try the DEFAULT_DIRECTORY */
		sprintf(drivedir,"%s/PEXdrive",cptr);
		if (access(drivedir,ACCESS_EXECUTE) == 0) {
		    /* Got it! */
		    strcpy(drivedir,DEFAULT_DIRECTORY);
		}
		else
		{
		    /* Try the CONTRIB_DIRECTORY path */
		    sprintf(drivedir,"%s/PEXdrive",CONTRIB_DIRECTORY);
		    if (access(drivedir,ACCESS_EXECUTE) == 0) {
			/* Got it! */
			strcpy(drivedir,CONTRIB_DIRECTORY);
		    }
		    else
		    {
			/* Really hosed -- Now what? */
			strcpy(drivedir,DEFAULT_DIRECTORY);
		    }
		}
	    }
	}
#endif
    }

#ifdef __APPLE__
    /* Inside a macOS .app the executable sits in Contents/MacOS and the
     * game files in Contents/Resources.
     */
    {
	static const char macos[] = "/Contents/MacOS";
	static const char rsrc[]  = "/Contents/Resources";
	int n = strlen(drivedir) - (sizeof(macos) - 1);

	if ((n > 0) && (strcmp(drivedir + n, macos) == 0)
		&& (n + sizeof(rsrc) <= sizeof(drivedir))) {
	    strcpy(drivedir + n, rsrc);
	}
    }
#endif

#if 0 /*def V4_FILE_SYS* [ */
    /* On the V4 File System, the server config file can (and usually does)
     * reside in a different directory than the rest of the files.
     */
    strcpy(server_configdir,DEFAULT_SERVER_CONFIG_DIRECTORY);

    /* Override with the environment variable if set. */
    if ((cptr = getenv("DRIVE_SERVER_CONFIG_DIRECTORY")) != NULL) {
	strcpy(server_configdir,cptr);
    }
    /* See if it's there. */
    sprintf(config_filename,"%s/%s",server_configdir,SERVER_CONFIG);
    if (access(config_filename,ACCESS_READ) == ACCESS_ERROR) {
	/* Nope -- see if we can find it in the same directory as
	 * all the other drive files.
	 */
	sprintf(config_filename,"%s/%s",drivedir,SERVER_CONFIG);
	if (access(config_filename,ACCESS_READ) == 0) {
	    /* Got it!  Use this one. */
	    strcpy(server_configdir,drivedir);
	}
    }
#else /* ][  must be V3 File System */
    /* On the V3 File System, the server config file must reside
     * in the same directory as the others.
     */
    strcpy(server_configdir,drivedir);
#endif /* ] V4_FILE_SYS else */

    sprintf(dash_filename,   "%s/%s",drivedir,DASH_FILE);
    sprintf(gear_filename[0],"%s/%s",drivedir,GEAR_R_FILE);
    sprintf(gear_filename[1],"%s/%s",drivedir,GEAR_N_FILE);
    sprintf(gear_filename[2],"%s/%s",drivedir,GEAR_1_FILE);
    sprintf(gear_filename[3],"%s/%s",drivedir,GEAR_2_FILE);
    sprintf(gear_filename[4],"%s/%s",drivedir,GEAR_3_FILE);
    sprintf(gear_filename[5],"%s/%s",drivedir,GEAR_4_FILE);
    sprintf(gear_filename[6],"%s/%s",drivedir,GEAR_5_FILE);
    sprintf(compass_filename,"%s/%s",drivedir,COMPASS_FILE);
    sprintf(title_filename,"%s/%s",drivedir,TITLE_FILE);
    sprintf(visctrls_filename,"%s/%s",drivedir,VISCTRLS_FILE);
    sprintf(server_programname,"%s/%s",drivedir,SERVER_PROGRAM);
    sprintf(help_filename,"%s/%s",drivedir,HELP_FILE);
    sprintf(scene_dir,"%s/%s",drivedir,SCENE_DIRECTORY);
    sprintf(construct_dir,"%s/%s",drivedir,CONSTRUCT_DIRECTORY);
    sprintf(object_block_filename,"%s/%s",drivedir,BLOCKS_FILE);

    /* Config files */
    sprintf(config_filename,"%s/%s",server_configdir,SERVER_CONFIG);
}
