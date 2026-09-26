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
#include <math.h>
#include "hw.h"

void usage( char *progname )
{
    fprintf( stderr, "Usage: %s [-binsrc] [-bindst outfile] infile\n",
                        progname );
}

main( int argc, char **argv )
{
    hwObject
        *objects;
    char
        *filename = 0,
        *outfile = 0;
    hwInt32
        numObjects;
    int
        i, srcBin = 0, dstBin = 0;
    FILE
        *f;
    hwDisplay
        disp;

    for( i = 1; i < argc; i++ ) {
        if( argv[i][0] == '-' ) {
            if( strcmp( argv[i], "-binsrc" ) == 0 ) {
                srcBin = 1;
            }
            else if( strcmp( argv[i], "-bindst" ) == 0 ) {
                dstBin = 1;
                outfile = argv[++i];
            }
            else {
                usage( argv[0] );
            }
        }
        else if( !filename ) {
            filename = argv[i];
        }
        else {
            usage( argv[0] );
        }
    }
    if( !filename ) {
        usage( argv[0] );
    }

    if (!hwInit(argc, argv)) exit(1);
    disp = hwNullDisplay->create( hwNullDisplay, NULL, NULL );
    disp->makeCurrent(disp, NULL);

    if( srcBin ) {
        f = fopen( filename, "rb" );
        if( !f ) {
            fprintf( stderr, "Cannot open file %s\n", filename );
            exit( 1 );
        }
        numObjects = hwReadBinary( (hwObjRW)fread, f, 0, &objects );
        (void)fclose( f );
    }
    else {
        numObjects = hwParseFile( filename, &objects );
    }

    if( dstBin ) {
        f = fopen( outfile, "wb" );
        if( !f ) {
            fprintf( stderr, "Cannot open file %s\n", outfile );
            exit( 1 );
        }

        (void)hwBeginBinary( (hwObjRW)fwrite, f, HW_FILE_WRITE_HDR, &numObjects );
        for( i = 0; i < numObjects; i++ ) {
            (void)hwWriteBinary( objects[i], (hwObjRW)fwrite, f );
        }
        (void)hwEndBinary( (hwObjRW)fwrite, f );

        (void)fclose( f );
    }
    else {
        for( i = 0; i < numObjects; i++ ) {
            (void)hwWriteAscii( objects[i], stdout );
        }
    }
}
