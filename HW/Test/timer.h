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

#ifdef WIN32 /* [ */

#include <sys/timeb.h>

double
    __start_time, __stop_time;

void start_timer( void )
{
    struct _timeb
        tm;

    _ftime( &tm );
    __start_time = ((double)tm.time) + ((double)tm.millitm)/1000.0;
}

void stop_timer( void )
{
    struct _timeb
        tm;

    _ftime( &tm );
    __stop_time = ((double)tm.time) + ((double)tm.millitm)/1000.0;
}

float get_elapsed_time()
{
    float
        elapsed;

    elapsed = __stop_time - __start_time;
    if( elapsed < 0.0 ) elapsed = 0.0;
    return( elapsed );
}

void print_elapsed_time()
{
    printf("Elapsed Time: %f\n", get_elapsed_time() );
}

#else /* ] [ */

#include <sys/time.h>

struct timeval __start_time;
struct timeval __stop_time;
struct timezone tz;


void start_timer()
{
    gettimeofday(&__start_time, &tz);
}

void stop_timer()
{
    gettimeofday(&__stop_time, &tz);
}

float get_elapsed_time()
{
    float elapsed;

    elapsed = ((float) (__stop_time.tv_sec - __start_time.tv_sec)
            + (float) (__stop_time.tv_usec - __start_time.tv_usec)/1000000.0);

    return( elapsed );
}

void print_elapsed_time()
{
    printf("Elapsed Time: %f\n", get_elapsed_time() );
}

#endif /* ] */
