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


/*
 * This is not the most perfect random number generator in the world, since
 * it has a period of M1 and, especially, since M1 isn't prime.  However,
 * it's about 8x as fast as drand48() and twice as fast as rand(), and
 * it does a pretty good job because of the shuffling.
 *
 * The first method is a simple multiple-key linear congruential generator.
 * The fifty_fifty routine based on "primitive polynomials modulo 2".
 * See "Numerical Recipes in C" for more details.
 *
 *
 */

#include <stdlib.h>

extern double drand48();

#define M1	(1<<21)
#define A1	4093
#define C1	374009
#define TSIZE	97
static unsigned long tnum[TSIZE] = {
    16783, 26323, 27370, 6897, 20800,
    20142, 8405, 32100, 877, 13923,
    28243, 19192, 15040, 24375, 31631,
    24850, 26625, 20269, 1536, 29803,

    12064, 16164, 19026, 4748, 20738,
    20141, 8469, 16135, 26124, 6574,
    14888, 10363, 13680, 3000, 8932,
    12613, 13626, 3367, 776, 18438,

    32537, 26111, 15463, 19417, 17902,
    29313, 22867, 30801, 6166, 1424,
    30152, 5429, 31352, 5319, 3928,
    26107, 10317, 2647, 26586, 5381,

    946, 22689, 26322, 5789, 751,
    11159, 9631, 16306, 9861, 21459,
    4711, 3152, 17401, 18292, 21955,
    20961, 12643, 21692, 26409, 22821,

    7605, 25194, 31773, 7155, 8652,
    23000, 15895, 8877, 22072, 5845,
    29491, 15748, 31180, 31040, 1435,
    5489, 29924
};
static float last = 0.294911574831180310401435;

float floatrand(
    void)
{
    int k;

    k = last * TSIZE;
    last = tnum[k] / ((float) M1);
    tnum[k] = (tnum[k] * A1 + C1) % M1;
    return(last);
}



#define IBA	(1<<3)
#define IBB	(1<<5)
#define IBC	(1<<29)
#define IBMASK	(1+IBC+IBB)
static unsigned int iseed = 0x31ac3512;

unsigned int fifty_fifty(
    void)
{
    if (iseed & IBC) {
	iseed = ((iseed ^ IBMASK) << 1) | 1;
	return (1);
    }
    else {
	iseed <<= 1;
	return (0);
    }
}



void seedrand(
    unsigned int seed)
{
    int i;

#if defined(_HPUX_SOURCE) || defined(LINUX_SOURCE)
    iseed = seed;
    srand48(seed);
    last = (float) drand48();
    for (i=0; i<TSIZE; ++i) {
	tnum[i] = (unsigned int) (drand48() * ((float) M1));
    }
#endif
}
