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

#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "hw_internal.h"

#ifdef ANDROID_NDK
//#  define ANDROID_LOG(a) __android_log_print a
#  define ANDROID_LOG(a)
#else
#  define ANDROID_LOG(a)
#endif

hwInt32
    hwDefaultOptFlags = HW_OPT_DEFAULT;

char
    *hwDefaultDataFilePath = NULL;

#ifdef WIN32
DWORD __hwTlsSlot;
#else
pthread_key_t __hwTlsSlot;
#endif

/* Declare the global mutex */
HW_DECLARE_GLOBAL_MUTEX;

hwInt32 hwInit( hwInt32 argc, char **argv )
{
    ANDROID_LOG((ANDROID_LOG_INFO,"HB_HW", "hwInit"));

    /* Initialize the global lock */
    HW_INITIALIZE_GLOBAL_MUTEX();

    /* Initialize TLS slot */
#ifdef WIN32
    __hwTlsSlot = TlsAlloc();
    if (__hwTlsSlot == TLS_OUT_OF_INDEXES) {
        return 0;
    }
#else
    if (pthread_key_create(&__hwTlsSlot, NULL) != 0) {
        return 0;
    }
#endif

    if( !__hwIntInitVtx() )     return 0;
    if( !__hwIntInitSurf() )    return 0;
    if( !__hwIntInitOrient() )  return 0;

    return 1;
}

/* Copied from the drand48 code in Common/hwUtils.c */
#define RAND48_SEED_0   (0x330e)
#define RAND48_SEED_1   (0xabcd)
#define RAND48_SEED_2   (0x1234)
#define RAND48_MULT_0   (0xe66d)
#define RAND48_MULT_1   (0xdeec)
#define RAND48_MULT_2   (0x0005)
#define RAND48_ADD      (0x000b)

void __hwInternalInitDisplay( hwDisplay disp, hwDisplay shareDisp )
{
    struct __hwDisplayInternal
        *intDisp;

    intDisp = (struct __hwDisplayInternal *)disp;

    hwDefaultSurf(&intDisp->scratchSurf);
    intDisp->nameSpace = malloc(sizeof(struct __hwNameSpace));
    memset(intDisp->nameSpace, 0, sizeof(struct __hwNameSpace));
    // TBD: Sharing

    intDisp->rand48_seed[0] = RAND48_SEED_0;
    intDisp->rand48_seed[1] = RAND48_SEED_1;
    intDisp->rand48_seed[2] = RAND48_SEED_2;
    intDisp->rand48_mult[0] = RAND48_MULT_0;
    intDisp->rand48_mult[1] = RAND48_MULT_1;
    intDisp->rand48_mult[2] = RAND48_MULT_2;
}

void __hwInternalMakeCurrent( hwDisplay disp )
{
    HW_SET_CURR_DISP(disp);
}

hwDisplay hwGetCurrentDisplay( void )
{
    return (hwDisplay) HW_GET_CURR_DISP();
}

void __hwIntDestroyList( hwInt32 dl )
{
    HW_USE_CURR_DISP;
    __hwDisp->destroyList( __hwDisp, dl );
}

void __hwIntDestroyGuiList( hwInt32 dl )
{
    HW_USE_CURR_DISP;
    __hwDisp->destroyGuiList( __hwDisp, dl );
}

/* Various implementations of the time function */

#if defined(WIN32) && !defined(CYGWIN)
    #include <sys/timeb.h>
    double hwGetSysTime(void)
    {
        struct _timeb val;
        _ftime(&val);
        return ((double)val.time) + ((double)val.millitm) / 1000.0;
    }
#else
    // Works on OSX, Linux, and Android
    #include <time.h>
    #include <sys/time.h>
    double hwGetSysTime(void)
    {
        struct timeval val;
        gettimeofday(&val, (struct timezone *)NULL);
        return ((double)val.tv_sec) + ((double)val.tv_usec) / 1000000.0;
    }
#endif
