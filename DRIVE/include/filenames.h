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


#ifndef _FILENAMES_H_INCLUDED
#define _FILENAMES_H_INCLUDED 1

#ifndef WIN32
#include <sys/param.h>
#else
#define MAXPATHLEN 1024
#endif

#ifdef WIN32
#define DEFAULT_DIRECTORY		"."
#else
#define DEFAULT_DIRECTORY		"/usr/demos/graphics/drive"
#endif
/* Server Config resides in the same directory. */
#define CONTRIB_DIRECTORY		"/usr/contrib/drive"

#define DASH_FILE		"pixmaps/dboard.raw"
#define GEAR_R_FILE		"pixmaps/dashR.raw"
#define GEAR_N_FILE		"pixmaps/dashN.raw"
#define GEAR_1_FILE		"pixmaps/dash1.raw"
#define GEAR_2_FILE		"pixmaps/dash2.raw"
#define GEAR_3_FILE		"pixmaps/dash3.raw"
#define GEAR_4_FILE		"pixmaps/dash4.raw"
#define GEAR_5_FILE		"pixmaps/dash5.raw"
#define COMPASS_FILE		"pixmaps/compass.raw"
#define TITLE_FILE		"pixmaps/title.raw"
#define VISCTRLS_FILE		"pixmaps/visctrls.raw"
#define SERVER_PROGRAM		"drive_server"
#define HELP_FILE		"drive_help"
#define SERVER_CONFIG		".server_config"
#define SCENE_DIRECTORY		"scenes/"
#define CONSTRUCT_DIRECTORY	"constructs/"
#define BLOCKS_FILE		"drive_blocks"

/*************************** GLOBAL DATA **************************************/
extern char dash_filename[MAXPATHLEN];
extern char gear_filename[][MAXPATHLEN];
extern char compass_filename[MAXPATHLEN];
extern char title_filename[MAXPATHLEN];
extern char visctrls_filename[MAXPATHLEN];
extern char server_programname[MAXPATHLEN];
extern char help_filename[MAXPATHLEN];
extern char config_filename[MAXPATHLEN];
extern char scene_dir[MAXPATHLEN];
extern char construct_dir[MAXPATHLEN];
extern char object_block_filename[MAXPATHLEN];
extern char drivedir[MAXPATHLEN+1];

/************************* FUNCTION PROTOTYPES ********************************/
extern void construct_filenames(
    char *drivepath);
#endif /* _FILENAMES_H_INCLUDED  */
