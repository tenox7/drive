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

#ifdef __cplusplus
 extern "C" {
#endif

/* Initialize Hoverware - once per process */
extern hwInt32
    hwInit( hwInt32 argc, char **argv );

/* Create a display.  Note that you still have to call
 * disp->makeCurrent with the resulting hwDisplay for
 * much of anything useful to happen
 */
extern hwDisplay hwCreateDisplay
    (
        hwDisplay dispFunc, hwDisplay sharedDisp,
        OS_DISPLAY_TYPE display
    );

/* Get the current display for this thread */
extern hwDisplay
    hwGetCurrentDisplay( void );

/* Get the most recent error code, and zero it out */
extern hwInt32
    hwGetError( void );

/* Register a constant for use in parsing.  Constants are global.  */
extern void
    hwRegisterConstant( const char *, hwInt32, void * );

/* Register a class for use in parsing.  Classes are global.  */
extern void
    hwRegisterClass( const hwObject );

/* Register an object for use in parsing. Objects are LOCAL to
 * the current hwDisplay - you must have done a disp->makeCurrent()
 * in the current thread for this to work!
 */
extern void
    hwRegisterObject( const hwObject );

/* Register a callback for use with various classes.  Callbacks are global. */
extern void
    hwRegisterCallback( const char *, hwCallback );

/* Unregister all register objects */
extern void
    hwUnregisterObjects( void );

/* Find a constant in the registry */
extern hwInt32
    hwFindConstant( const char *, void ** );

/* Find a class in the registry */
extern hwObject
    hwFindClass( const char * );

/* Find an object in the registry */
extern hwObject
    hwFindObject( const char * );

/* Find a callback in the registry */
extern hwCallback
    hwFindCallback( const char * );

/************ File I/O routines ************/
/* Parse a file, returning all of the top-level objects found in it */
extern hwInt32
    hwParseFile( const char *, hwObject ** );

/* Parse a string array, returning all of the top-level objects found in it.
 * The array should be NULL-terminated.
 */
extern hwInt32
    hwParseArray( char **, hwObject ** );

/* Write an object (in ASCII) to a file */
extern hwInt32
    hwWriteAscii( const hwObject, FILE * );

/* Initialize a binary file for read/write */
extern hwInt32
    hwBeginBinary( hwObjRW oRdWr, void *f, hwInt32 flags, hwInt32 *numObj );

/* Finish up reading/writing a binary file */
extern hwInt32
    hwEndBinary( hwObjRW oRdWr, void *f );

/* Write an object to a binary file */
extern hwInt32
    hwWriteBinary( const hwObject obj, hwObjRW oWrite, void *f );

/* Read objects from a binary file */
hwInt32
    hwReadBinary( hwObjRW oRead, void *f, hwInt32 numObjs, hwObject **objs );

/********* Matrix utilities ***********/
extern void
    hwIdentity( hwFloat Mat[4][4] ),
    hwMakeMatrix(const hwOrientType *, hwFloat Mat[4][4]),
    hwScale( hwFloat Mat[4][4], hwFloat sx, hwFloat sy, hwFloat sz ),
    hwRotateX( hwFloat Mat[4][4], hwFloat Ang ),
    hwRotateY( hwFloat Mat[4][4], hwFloat Ang ),
    hwRotateZ( hwFloat Mat[4][4], hwFloat Ang ),
    hwRotateAxis( hwFloat Mat[4][4], hwFloat Ang, const hwFloat XYZ[3] ),
    hwMatMult( hwFloat Res[4][4], hwFloat A[4][4], hwFloat B[4][4] ),
    hwTransform( hwFloat Mat[4][4], hwFloat Vect[3] ),
    hwTransform4d( hwFloat dst[4], const hwFloat src[4], hwFloat Mat[4][4] ),
    hwTransformPersp( hwFloat dst[3], const hwFloat src[3], hwFloat Mat[4][4] );
extern hwInt32
    hwInvertMat( hwFloat Src[4][4], hwFloat Dst[4][4] );

/* Random numbers Note that each hwDisplay context has its own
 * random number seed.
 */
extern void
    hwSrand48(long seed);
extern double
    hwDrand48(void);

/* Hash table utilities.  Note that these ARE NOT thread-safe. */
void
    *hwCreateHash( int size );
void
    hwEmptyHash( void *table );
int
    hwInsert( const char *name, int value, void *table );
int
    hwLookup( const char *name, void *table );
int
    hwFindSym( char name[256], hwInt32 value, void *table );
void
    hwDestroyHash( void *table );

/* Data flag utilities */
extern hwInt32
    hwGetVertexOffsets( hwInt32 flags, hwVertexOffsets *offs ),
    hwCalcWPV( hwInt32 flags ),
    hwTextureFlags( const hwSurfaceType *surf );

/* Surface attr texture ID cooking utility */
extern void
   hwSurfAttrs(hwSurfaceType *surf);

/* Initialize an hwSurface to default values */
extern void
    hwDefaultSurf( hwSurfaceType * );

/* Initialize an hwOrient to default values */
extern void
    hwDefaultOrient( hwOrientType * );

/* Normal / bump map utilities */
extern void
    hwMakeNormalMap8( hwUint8 *dst, hwUint8 *src, hwInt32 w, hwInt32 h,
                      hwInt32 clampW, hwInt32 clampH,
                      hwFloat bumpScale ),
    hwMakeNormalMap16( hwUint8 *dst, hwUint16 *src, hwInt32 w, hwInt32 h,
                       hwInt32 clampW, hwInt32 clampH,
                       hwFloat bumpScale );

/* Time stamp utility - system time, in fractional seconds */
extern double hwGetSysTime(void);

/* File I/O routines which can access the APK */
extern void *hwFopen(const char *, const char *);
extern void hwFclose(void *);
extern int hwFread(void *, int, int, void *);
extern int hwFwrite(void *, int, int, void *);
extern int hwFgetc(void *);
extern int hwFungetc(int, void *);
extern int hwFscanf(void *, const char *, void *);
extern char *hwFgets(char *, int, void *);
extern int hwFseek(void *, long, int);
extern void hwFrewind(void *);
extern long hwFtell(void *);

#ifdef __cplusplus
 }
#endif
