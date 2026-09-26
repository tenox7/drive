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
 * daylight.c - Code to change background and depth cue based on
 *              time of day.
 */

#include "hw.h"
#include "global.h"
#include "drive.h"

#define DAYLIGHT_DEPTH_CUE_NIGHT	(0.1)
#define DAYLIGHT_DEPTH_CUE_DAY		(1.0)
#define DAYLIGHT_AMBIENT_NIGHT		(0.2)	/* Was 0.7 */
#define DAYLIGHT_AMBIENT_DAY		(0.5)
#define DAYLIGHT_SUN_NIGHT		(0.0)
#define DAYLIGHT_SUN_DAY		(0.8)

/* Modify this value to change how often the background color is
 * updated.  The value is in seconds.
 */
#define DAYLIGHT_UPDATE_INTERVAL	(60.0)

/* These values control how long sunrise and sunset transitions
 * take.  The value is in hours.
 */
#define DAYLIGHT_SUNRISE_DURATION	(1.00)
#define DAYLIGHT_SUNSET_DURATION	(1.00)

/*****************************************************************
 * daylight_background_color
 * daylight_depth_cue_color
 * daylight_ambient_color
 * daylight_sun_color
 * daylight_depth_cue_range
 */
static float daylight_background_color[3];
static float daylight_depth_cue_color[3];
static float daylight_ambient_color[3];
static float daylight_sun_color[3];
static float daylight_depth_cue_range[2];

/*****************************************************************
 * day_begin_time
 * night_begin_time
 * sunrise_begin_time
 * sunset_begin_time
 *
 *	Global variables that are set to the time in hours (0-24)
 *	for color change events.
 */
static double day_begin_time;
static double night_begin_time;
static double sunrise_begin_time;
static double sunset_begin_time;

/*****************************************************************
 * sunrise_colors
 * sunset_colors
 *
 *	Tables for changes in sky color.  Sunrise table gives 
 *	transition from dark to daylight.  Sunset table gives
 *	transition from daylight to dark.
 *
 *	The last color in the sunrise table needs to be the 
 *	same as the first color in the sunset table.  The first
 *	color in the sunrise table needs to be the same as the
 *	last color in the sunset table.
 */
#define N_SUNRISE_COLORS	3
#define N_SUNSET_COLORS		3
#define DAY_COLOR		{SKY_RED, SKY_GREEN, SKY_BLUE}
#define NIGHT_COLOR		{0.000, 0.000, 0.000}
static float sunrise_colors[N_SUNRISE_COLORS][3] = {
	NIGHT_COLOR,
	{0.35, 0.35, 0.35},
	DAY_COLOR
};
static float sunset_colors[N_SUNSET_COLORS][3] = {
	DAY_COLOR,
	{0.80, 0.00, 0.40},
	NIGHT_COLOR
};


/**** hpfcla:net.sources / nsc-pdc!rgb / 10:24 am  May 16, 1985
 *
 * Changed constants to Fort Collins, Colorado.  (ajs, 850520)
 * Made other minor output format improvements also (at later times).
 * Someday I will wholly rewrite this program...
 *
 * Right now, strangely enough, longitude is POSITIVE for west.
 * 
 *        All output is to standard io.  
 *
 *	 Compile with cc -O -o sun sun.c -lm
 *	 Non 4.2 systems may have to change <sys/time.h> to <time.h> below.
 *	(yes, done)
 *
 *	 Note that the latitude, longitude, time zone correction and
 *	 time zone string are all defaulted in the global variable section.
 *
 *	 Most of the code in this program is adapted from algorithms
 *	 presented in "Practical Astronomy With Your Calculator" by
 *	 Peter Duffet-Smith.
 *
 *	 The GST and ALT-AZIMUTH algorithms are from Sky and Telescope,
 *	 June, 1984 by Roger W. Sinnott
 *
 *	 Author Robert Bond - Beaverton Oregon.
 *	
 */


#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

char *usage[] = {
    "usage: %s [-pfv] [-d mm/dd/yyyy] [-t h:m:s] [-a lat] [-o lon] [-e elev] [-z tz]",
    "-p print sun's current position",
    "-f print time of day (fractional hours) and solar UV factor only",
    "-v print extra debugging output",
    "-d date for which to figure (default is current system date)",
    "-t time of day (default is current system time); useful with -p",
    "-a latitude for which to figure (default:  Fort Collins, Colorado)",
    "-o longitude for which to figure",
    "   lat/long format:  [+/-]dd[d]mm[ss|.f][suffix-char]",
    "-e elevation in feet, only useful with -f",
    "-z time zone (default = 7, MST)",
    CPNULL,
};

#define JDE	 2444238.5	/* Julian date of EPOCH */

double	GetLoc();

int	debug = 0;

#define	GETLAT	0		/* flags for GetLoc() */
#define	GETLON	1
#define	BADLOC	1000

/**********************************************************************
 *
 */
#define ACOS_DEG(k)	RADIANS_TO_DEGREES(acos(k))
#define ASIN_DEG(k)	RADIANS_TO_DEGREES(asin(k))
#define ATAN_DEG(k)	RADIANS_TO_DEGREES(atan(k))
#define  SIN_DEG(k)	sin(DEGREES_TO_RADIANS(k))
#define  COS_DEG(k)	cos(DEGREES_TO_RADIANS(k))
#define  TAN_DEG(k)	tan(DEGREES_TO_RADIANS(k))


/**********************************************************************
 *
 */
static double atan_q_deg(
    double y,
    double x)
{
    double rv;
    
    if (y == 0)
        rv = 0;
    else if (x == 0)
        rv = y>0 ? 90.0 : -90.0;
    else rv = ATAN_DEG(y/x);
    
    if (x<0) return rv+180.0;
    if (y<0) return rv+360.0;
    return(rv);
}


/**********************************************************************
 *
 */
static double adj360(
    double deg)
{
    while (deg < 0.0) 
	deg += 360.0;
    while (deg > 360.0)
	deg -= 360.0;
    return(deg);
}


/**********************************************************************
 *
 */
static double adj24(
    double hrs)
{
    while (hrs < 0.0) 
	hrs += 24.0;
    while (hrs > 24.0)
	hrs -= 24.0;
    return(hrs);
}


/**********************************************************************
 *
 */
static void eq_to_altaz(
    double r, 
    double d, 
    double t,
    double lat,
    double lon, 
    double *alt,
    double *az)
{
    double p = 3.14159265;
    double r1 = p / 180.0;
    double b = lat * r1;
    double l = (360 - lon) * r1;
    double t5, s1, c1, c2, s2, a, h;
    
    if (debug)
	printf("Given R. A. = %f, DECL. = %f, gmt = %f \n", r, d, t);
    
    r = r * 15.0 * r1;
    d = d * r1;
    t = t * 15.0 * r1;
    t5 = t - r + l;
    s1 = sin(b) * sin(d) + cos(b) * cos(d) * cos(t5);
    c1 = 1 - s1 * s1;
    if (c1 > 0) {
	c1 = sqrt(c1);
	h = atan(s1 / c1);
    } else {
	h = (s1 / fabs(s1)) * (p / 2.0);
    }
    c2 = cos(b) * sin(d) - sin(b) * cos(d) * cos(t5);
    s2 = -cos(d) * sin(t5);
    if (c2 == 0) 
	a = (s2/fabs(s2)) * (p/2);
    else {
	a = atan(s2/c2);
	if (c2 < 0)
	    a=a+p;
    }
    if (a<0)
        a=a+2*p;
    *alt = h / r1;
    *az = a / r1;
    
    if (debug)
	printf("alt = %f, az = %f \n",*alt,*az);
}


/**********************************************************************
 *
 */
static double gmst(
    double j,
    double f)
{
    double d, j0, t, t1, t2, s;
    
    d = j - 2451545.0;
    t = d / 36525.0;
    t1 = floor(t);
    j0 = t1 * 36525.0 + 2451545.0;
    t2 = (j - j0 + 0.5)/36525.0;
    s = 24110.54841 + 184.812866 * t1; 
    s += 8640184.812866 * t2;
    s += 0.093104 * t * t;
    s -= 0.0000062 * t * t * t;
    s /= 86400.0;
    s -= floor(s);
    s = 24 * (s + (f - 0.5) * 1.002737909);
    if (s < 0)
	s += 24.0;
    if (s > 24.0)
	s -= 24.0;
    
    if (debug)
	printf("For jd = %f, f = %f, gst = %f \n", j, f, s);
    
    return(s);
}


/**********************************************************************
 * julian_date - Computes Julian date from Gregorian.
 *
 * Input:
 *	m	Month.
 *	d	Day.
 *	y	Year.
 */    
static double julian_date(
    int m,
    int d,
    int y)
{
    long a, b;
    double jd;
    
    if (m == 1 || m == 2) {
	--y;
	m += 12;
    }
    if (y < 1583) {
	printf("Can't handle dates before 1583\n");
	exit(1);
    }
    a = y/100;
    b = 2 - a + a/4;
    b += (int)((double)y * 365.25);
    b += (int)(30.6001 * ((double)m + 1.0));
    jd = (double)d + (double)b + 1720994.5;
    
    if (debug) 
	printf( "Julian date for %d/%d/%d is %f\n", m, d, y, jd );
    
    return jd;
}


/**********************************************************************
 * rise_set
 *
 *	alpha	Right ascension.
 *	delta	Declination.
 *	lat	Latitude.
 *	lstr	Local sidereal time of rise.
 *	lsts	Local sidereal time of set.
 *      ar	Azimuth of rise.
 *	as	Azimuth of set.
 */    
static void rise_set(
   double alpha,
   double delta,
   double lat,
   double *lstr,
   double *lsts,
   double *ar,
   double *as)
{
    double tar;
    double h;
    
    tar = SIN_DEG(delta)/COS_DEG(lat);
    if (tar < -1.0 || tar > 1.0) {
	printf("The object is circumpolar\n");
	exit (1);
    }
    *ar = ACOS_DEG(tar);
    *as = 360.0 - *ar;
    
    h = ACOS_DEG(-TAN_DEG(lat) * TAN_DEG(delta)) / 15.0;
    *lstr = 24.0 + alpha - h;
    if (*lstr > 24.0)
	*lstr -= 24.0;
    *lsts = alpha + h;
    if (*lsts > 24.0)
	*lsts -= 24.0;
    
    if (debug) {
	printf("For ra, decl. of %f, %f: \n", alpha, delta);
	printf("lstr = %f, lsts = %f, \n", *lstr, *lsts);
	printf("ar =   %f, as =   %f \n", *ar, *as);
    }
}


/**********************************************************************
 * solar_longitude - Computes solar longitude given Julian time since 
 *                   Epoch.
 * 
 * Input:
 *	ed	Julian time since Epoch.
 *
 */
static double solar_longitude(
    double ed)
{
    double n, m, e, ect, errt, v;
    
    n = 360.0 * ed / 365.2422;
    n = adj360(n);
    m = n + 278.83354 - 282.596403;
    m = adj360(m);
    m = DEGREES_TO_RADIANS(m);
    e = m; ect = 0.016718;
    while ((errt = e - ect * sin(e) - m) > 0.0000001) 
        e = e - errt / (1 - ect * cos(e));
    v = 2 * atan(1.0168601 * tan(e/2));
    v = adj360(v * 180.0 / M_PI + 282.596403);
    
    if (debug)
	printf("Solar Longitude for %f days is %f \n", ed, v); 
    
    return(v);
}


/**********************************************************************
 * lon_to_eq - Compute Right ascension, declination from solar
 *             longitude.
 * 
 * Input:
 *	lambda	Solar longitude.
 *
 * Output:
 *	alpha	Right ascension.
 *	delta	Declination.
 */
static void lon_to_eq(
    double lambda,
    double *alpha,
    double *delta)
{
    double tlam, epsilon;
    
    tlam = DEGREES_TO_RADIANS(lambda);
    epsilon = DEGREES_TO_RADIANS((double)23.441884);

    *alpha = atan_q_deg((sin(tlam))*cos(epsilon),cos(tlam)) / 15.0;
    *delta = ASIN_DEG(sin(epsilon)*sin(tlam));
    
    if (debug)
	printf("Right ascension, declination for lon %f is %f, %f \n",
	       lambda, *alpha, *delta);
}

/**********************************************************************
 * hms_to_h
 * 
 * 	Convert time in hours, minutes, and seconds to a decimal-
 * 	fraction form in hours.
 */
static double hms_to_h(
    int hours,
    int minutes,
    int seconds)
{
    double decimal_hours;

    decimal_hours = hours + minutes / 60.0 + seconds / 3600.0;
    
    if (debug)
	printf("For time %d:%d:%d decimal hours are: %f\n",
	       hours, minutes, seconds, decimal_hours);
    
    return decimal_hours;
}


/**********************************************************************
 * h_to_hms
 * 
 * 	Convert time in decimal-fraction hours to hours, minutes, and
 *	seconds.
 */
static void h_to_hms(
    double decimal_hours,
    int *hours,
    int *minutes,
    int *seconds)
{
    double decimal_minutes;
    double decimal_seconds;
    
    *hours = (int)decimal_hours;

    decimal_minutes = (decimal_hours - (double)(*hours)) * 60.0;
    *minutes = (int)decimal_minutes;

    decimal_seconds = (decimal_minutes - (double)(*minutes)) * 60.0 + 0.5;
    *seconds = (int)decimal_seconds;

    if (decimal_seconds > 30.0)
	(*minutes)++;

    if (*minutes == 60)
    {
	*minutes = 0;
	(*hours)++;
    }

    if (debug)
	printf("For decimal hours %f time is: %d:%d:%d\n",
	       decimal_hours, *hours, *minutes, *seconds );
}


/**********************************************************************
 * lst_to_hms
 * 
 * 	Convert local sidereal time to local time.
 */
static void lst_to_hm(
    double lst,
    double jd,
    double lon,
    int yr,
    int tz,
    int *h,
    int *m)
{
    double ed, gst, jzjd, t, r, b, t0, gmt;
    int tmps;
    
    if ((gst = lst + (lon / 15.0)) > 24.0)
	gst -= 24.0;
    
    jzjd = julian_date( 1, 0, yr + 1900 );
    ed	 = jd - jzjd;
    t	 = (jzjd - 2415020.0) / 36525.0;
    r	 = 6.6460656 + (2400.05126 * t) + (2.58E-05 * t * t);
    b	 = 24 - (r - (24 * yr));
    t0   = (ed * 0.0657098) - b;
    
    if (t0 < 0.0)
	t0 += 24;
    
    if ((gmt = (gst - t0)) < 0)
	gmt += 24.0;
    
    gmt = (gmt * 0.99727) - tz;
    
    if (gmt < 0)
	gmt += 24.0;
    
    if (debug)
	printf ("lst = %g, t0 = %g, gmt = %g\n", lst, t0, gmt);
    
    /* Convert hours to hours, minutes, seconds. */
    h_to_hms(gmt, h, m, &tmps);
}


/**********************************************************************
 * 
 * 
 *
 *
 */
static void daylight_compute_times(
    struct tm *now,
    int timezone,	     
    double longitude,
    double latitude,
    double *sunrise,
    double *sunset,
    double *altitude,
    double *azimuth)
{
    double jd;			/* Julian date. */
    double lambda1, lambda2;	/* Solar longitudes. */
    double alpha1, alpha2;	/* Right ascensions. */
    double delta1, delta2;	/* Declinations. */
    double st1r, st2r;		/* Sidereal times of rise. */
    double st1s, st2s;		/* Sidereal times of set. */
    double a1r, a2r, a1s, a2s;	/* Azimuths of rise and set. */
    double m1, hsm, ratio, trise,tset, ar,as, delta, tri, x, y;
    double da, dt, alpha, dh, gst;
    int h, m, tmps;

    /* Compute Julian date. */
    jd = julian_date( now->tm_mon + 1, now->tm_mday, now->tm_year + 1900 );

    /* Compute solar longitude for today and tomorrow. */
    lambda1 = solar_longitude( jd - JDE );
    lambda2 = solar_longitude( jd - JDE + 1.0 );
    
    /* Compute right ascension, declination from solar longitudes. */
    lon_to_eq( lambda1, &alpha1, &delta1 );
    lon_to_eq( lambda2, &alpha2, &delta2 );
    
    /* Compute local sidereal time and azimuth of rise and set. */
    rise_set( alpha1, delta1, latitude, &st1r, &st1s, &a1r, &a1s );
    rise_set( alpha2, delta2, latitude, &st2r, &st2s, &a2r, &a2s );
    
    /* Find local sidereal time of first midnight. */
    m1 = adj24( gmst( jd - 0.5, 0.5 + (timezone / 24.0)) - (longitude / 15.0));
    if (debug)
	printf( "Local sidereal time of midnight is %f \n", m1 );
    
    hsm = adj24(st1r - m1);
    if (debug)
	printf( "About %f hours from midnight to dawn \n", hsm );
    
    ratio = hsm / 24.07;
    if (debug)
	printf( "%f is how far dawn is into the day \n", ratio );
    
    if (fabs(st2r - st1r) > 1.0)
    {
	st2r += 24.0;
	if (debug)
	    printf("st2r corrected from %f to %f \n", st2r-24.0, st2r);
    }
    
    trise = adj24((1.0 - ratio) * st1r + ratio * st2r);
    
    hsm = adj24(st1s - m1);
    
    if (debug)
	printf ("about %f hours from midnight to sunset \n", hsm);
    
    ratio = hsm / 24.07;
    
    if (debug)
	printf("%f is how far sunset is into the day \n", ratio);
    
    if (fabs(st2s - st1s) > 1.0) {
	st2s += 24.0;
	if (debug)
	    printf("st2s corrected from %f to %f \n", st2s-24.0, st2s);
    }
    
    tset = adj24((1.0 - ratio) * st1s + ratio * st2s);
    
    if (debug)
	printf("Uncorrected rise = %f, set = %f \n", trise, tset);
    
    ar = a1r * 360.0 / (360.0 + a1r - a2r);
    as = a1s * 360.0 / (360.0 + a1s - a2s);
    
    delta = (delta1 + delta2) / 2.0;
    tri = ACOS_DEG(SIN_DEG(latitude)/COS_DEG(delta));
    
    x = 0.835608;		/* correction for refraction, parallax, ? */
    y = ASIN_DEG(SIN_DEG(x)/SIN_DEG(tri));
    da = ASIN_DEG(TAN_DEG(x)/TAN_DEG(tri));
    dt = 240.0 * y / COS_DEG(delta) / 3600;
    
    if (debug)
	printf("Corrections: dt = %f, da = %f \n", dt, da);

    lst_to_hm(trise - dt, jd, longitude, now->tm_year, timezone, &h, &m);
    *sunrise = hms_to_h( h, m, 0 );
    if (debug)
	printf( "Sunrise: %2d:%02d (%f)   ", h, m, *sunrise );

    /* Convert hours to hours, minutes, seconds. */
    h_to_hms(ar - da, &h, &m, &tmps);
    
    /*
     * st1r and st2r are fractions of sidereal days, so the rate of
     * change is their difference less the fraction of a day which is
     * the difference between a sidereal and solar day.
     */
    if (debug)
	printf ("Azimuth: %3d %02d'  rate of change: %+.1f min/day\n",
	    h, m, (st2r - st1r - 0.06552) * 60);
    
    lst_to_hm (tset + dt, jd, longitude, now->tm_year, timezone, &h, &m);
    *sunset = hms_to_h( h, m, 0 );
    if (debug)
	printf( "Sunset:  %2d:%02d (%f)   ", h, m, *sunset );

    /* Convert hours to hours, minutes, seconds. */
    h_to_hms (as + da, &h, &m, &tmps);
    
    if (debug)
	printf ("Azimuth: %3d %02d'  rate of change: %+.1f min/day\n",
	    h, m, (st2s - st1s - 0.06552) * 60);
    
    if (alpha1 < alpha2)
	alpha = (alpha1 + alpha2) / 2.0;
    else
	alpha = (alpha1 + 24.0 + alpha2) / 2.0;
    
    if (alpha > 24.0)
	alpha -= 24.0;
    
    dh = (hms_to_h(now->tm_hour, now->tm_min, now->tm_sec) + timezone) / 24.0;
    if (dh > 0.5)
    {
	dh -= 0.5;
	jd += 0.5;
    }
    else
    {
	dh += 0.5;
	jd -= 0.5;
    }
    
    gst = gmst(jd, dh);
    
    eq_to_altaz( alpha, delta, gst, latitude, longitude, altitude, azimuth );
    
    if (debug)
	printf ("The sun is at:   ");
    /* Convert azimuth degrees to degrees, minutes, seconds. */
    h_to_hms(*azimuth, &h, &m, &tmps);
    if (debug)
	printf ("Azimuth: %3d %02d' (%f)", h, m, *azimuth);

    /* Convert altitude degrees to degrees, minutes, seconds. */
    h_to_hms( *altitude, &h, &m, &tmps );
    if (debug)
	printf ( "Altitude: %3d %02d' (%f)\n", h, m, *altitude );

}

/*****************************************************************
 * daylight_interpolate_colors
 *
 *	Set background and depth cue color from specified table.
 *
 * The parameter t must be in the range [0,1].
 * Minimum table size is 2.
 */
static void daylight_interpolate_colors(
    float t,					
    float table[3][3],
    int table_size)
{
    int rgb;			/* RGB index. */
    int index;			/* Index into the table. */
    float delta;		/* Parametric distance between table entries. */
    float scale, value;
    
    /* Find how much distance we have between table entries. */
    delta = 1.0 / (table_size - 1.0);

    /* Find appropriate index in table for this value of t. */
    for (index = 0; index<table_size-1; index++)
    {
	value = delta + (index * delta);
	if ( t < value )
	{
	    /* Find how far between entries we are. */
	    scale = (t - (value - delta)) / delta;
	    break;
	}
    }

    /* Compute background color from table. */
    for (rgb=0; rgb<3; rgb++)
    {
	value = INTERPOLATE( scale, table[index][rgb], table[index+1][rgb] );
	daylight_background_color[rgb] = value;
	daylight_depth_cue_color[rgb] = value;
    }
}	

/*****************************************************************
 * daylight_compute_colors
 * 
 * 	Computes colors for background, depth cue, and ambient
 * 	based on precomputed values of:
 *
 *		day_begin_time
 *		night_begin_time
 *		sunrise_begin_time
 *		sunset_begin_time
 */
static void daylight_compute_colors(
    daylight_type *d,
    double now)			
{
    float delta, scale, value;
    int i;

    if (0 && (now >= sunrise_begin_time) && (now < day_begin_time))
    {
	/* Sun is rising. */
	if (debug) printf("Sun is rising...\n");

	/* Find a number between 0 and 1 that describes how far
	 * into the sunrise we are.
	 */
	delta = day_begin_time - sunrise_begin_time;
	scale = (now - sunrise_begin_time) / delta;

	/* Compute background color from sunrise table. */
	daylight_interpolate_colors( scale, sunrise_colors, N_SUNRISE_COLORS );
	
	/* Compute depth cue range. */
	daylight_depth_cue_range[0] = 0.0;
	daylight_depth_cue_range[1] =
	    INTERPOLATE( scale, DAYLIGHT_DEPTH_CUE_NIGHT,
			DAYLIGHT_DEPTH_CUE_DAY );

	/* Set up ambient light. */
	value = INTERPOLATE( scale, DAYLIGHT_AMBIENT_NIGHT,
			    DAYLIGHT_AMBIENT_DAY );
	daylight_ambient_color[0] = value;
	daylight_ambient_color[1] = value;
	daylight_ambient_color[2] = value;

	/* Set up sun color. */
	value = INTERPOLATE( scale, DAYLIGHT_SUN_NIGHT,
			    DAYLIGHT_SUN_DAY );
	daylight_sun_color[0] = value;
	daylight_sun_color[1] = value;
	daylight_sun_color[2] = value;

        /* Turn on sunlight. */
	d->mask |= 0x2;
    }
    else if (1 || (now >= day_begin_time) && (now < sunset_begin_time))
    {
	/* Sun is up. */
	if (debug) printf("Sun is up...\n");

	/* Use background color from time just before sunset. */
	for (i=0; i<3; i++)
	{
	    daylight_background_color[i] = sunset_colors[0][i];
	    daylight_depth_cue_color[i] = sunset_colors[0][i];
	}
	
	/* Set up depth cue range for maximum sight distance. */
	daylight_depth_cue_range[0] = 0.0;
	daylight_depth_cue_range[1] = DAYLIGHT_DEPTH_CUE_DAY;

	/* Set up ambient light. */
	daylight_ambient_color[0] = DAYLIGHT_AMBIENT_DAY;
	daylight_ambient_color[1] = DAYLIGHT_AMBIENT_DAY;
	daylight_ambient_color[2] = DAYLIGHT_AMBIENT_DAY;

	/* Set up sun color. */
	daylight_sun_color[0] = DAYLIGHT_SUN_DAY;
	daylight_sun_color[1] = DAYLIGHT_SUN_DAY;
	daylight_sun_color[2] = DAYLIGHT_SUN_DAY;

	/* Turn on sunlight. */
	d->mask |= 0x2;
    }
    else if ((now >= sunset_begin_time) && (now < night_begin_time))
    {
	/* Sun is setting. */
	if (debug) printf("Sun is setting...\n");

	/* Find a number between 0 and 1 that describes how far
	 * into the sunset we are.
	 */
	delta = night_begin_time - sunset_begin_time;
	scale = (now - sunset_begin_time) / delta;

	/* Compute background color from sunrise table. */
	daylight_interpolate_colors( scale, sunset_colors, N_SUNSET_COLORS );
	
	/* Compute depth cue range. */
	daylight_depth_cue_range[0] = 0.0;
	daylight_depth_cue_range[1] =
	    INTERPOLATE( scale, DAYLIGHT_DEPTH_CUE_DAY,
			DAYLIGHT_DEPTH_CUE_NIGHT );

	/* Set up ambient light. */
	value = INTERPOLATE( scale, DAYLIGHT_AMBIENT_DAY,
			    DAYLIGHT_AMBIENT_NIGHT );
	daylight_ambient_color[0] = value;
	daylight_ambient_color[1] = value;
	daylight_ambient_color[2] = value;

	/* Set up sun color. */
	value = INTERPOLATE( scale, DAYLIGHT_SUN_DAY,
			    DAYLIGHT_SUN_NIGHT );
	daylight_sun_color[0] = value;
	daylight_sun_color[1] = value;
	daylight_sun_color[2] = value;

	/* Sunlight should be on. */
	d->mask |= 0x2;
    }
    else /* ((now >= night_begin_time) && (now < sunrise_begin_time)) */
    {
	/* Sun is down. */
	if (debug) printf("Sun is down...\n");

	/* Use background color from time just before sunrise. */
	for (i=0; i<3; i++)
	{
	    daylight_background_color[i] = sunrise_colors[0][i];
	    daylight_depth_cue_color[i] = sunrise_colors[0][i];
	}
	
	/* Set up depth cue range for minimum sight distance. */
	daylight_depth_cue_range[0] = 0.0;
	daylight_depth_cue_range[1] = DAYLIGHT_DEPTH_CUE_NIGHT;

	/* Set up ambient light. */
	daylight_ambient_color[0] = DAYLIGHT_AMBIENT_NIGHT;
	daylight_ambient_color[1] = DAYLIGHT_AMBIENT_NIGHT;
	daylight_ambient_color[2] = DAYLIGHT_AMBIENT_NIGHT;

	/* Set up sun color. */
	daylight_sun_color[0] = DAYLIGHT_SUN_NIGHT;
	daylight_sun_color[1] = DAYLIGHT_SUN_NIGHT;
	daylight_sun_color[2] = DAYLIGHT_SUN_NIGHT;

	/* Turn off sunlight. */
	d->mask &= (~0x2);
    }
}


/*****************************************************************
 * daylight_initialize
 * 
 * 	Initialize daylight structure.  Default is low beam,
 * 	daylights off.
 */
void daylight_initialize(
    daylight_type *d)
{
    time_t now_seconds;		/* "Current" time in seconds. */
    struct tm *now_tm;
    double sunrise, sunset;
    double altitude, azimuth;

    if ( d->use_server_time )
    {
	/* Using time sent from server. */

	if ( d->static_time )
	{
	    /* Time stands still (at the time sent from the server.) */
	    
	}
	else
	{
	    /* Time marches forward using time sent from server as the
	     * reference. 
	     */
	    /* use sun and server time to compute...*/
	}
    }
    else
    {
	/* Use actual time of day. */
	now_seconds = time((time_t *) 0);  
    }

    /* Find sunrise and sunset times. */
    now_tm = localtime( &now_seconds );  
    daylight_compute_times( now_tm, 7, 105.0119, 40.5253,
			   &sunrise, &sunset, &altitude, &azimuth );
    
    sunrise_begin_time = sunrise - (0.5 * DAYLIGHT_SUNRISE_DURATION);
    day_begin_time =  sunrise + (0.5 * DAYLIGHT_SUNRISE_DURATION);
    sunset_begin_time = sunset - (0.5 * DAYLIGHT_SUNSET_DURATION);
    night_begin_time = sunset + (0.5 * DAYLIGHT_SUNSET_DURATION);
}


/*****************************************************************
 * daylight_update
 * 
 * 	Update background color and depth cue based on time of day
 * 	parameters.
 */
void daylight_update(
    int fildes,
    daylight_type *d,
    boolean_type force_update)
{
    static time_t last_update_time;
    time_t now_seconds;		/* "Current" time in seconds. */
    double now_hours;		/* "Current" time in hours. */
    double delta;
    struct tm *tm;
    extern hwDisplay disp;

    memcpy(&(cstate.daylight),d,sizeof(daylight_type));
    /* Copy background color to structure for other modules' use */
    cstate.daylight.back_clr[0] = daylight_background_color[0];
    cstate.daylight.back_clr[1] = daylight_background_color[1];
    cstate.daylight.back_clr[2] = daylight_background_color[2];

    if ( d->use_server_time )
    {
	/* Using time reference sent from server. */

	if ( d->static_time )
	{
	    /* Time stands still.  Use the time the server gave. */
	    now_seconds = d->reference_time;
	}
	else
	{
	    /* Time marches on using time from server as reference. */
	    time_t sec_1970;
	    
	    /* Find the amount of time that has passed since the server
	     * sent us the reference time.  Add this to the reference
	     * time to get the current server time.
	     */
	    sec_1970 = time((time_t *) 0);  
	    delta = difftime( sec_1970, d->initial_time );
	    now_seconds = d->reference_time + delta;
	}
    }
    else
    {
	/* Use actual time of day. */
	now_seconds = time((time_t *) 0);  
    }

    /* Make sure that we don't do updates at every frame! */
    if (!force_update)
    {
	/*
	depth_cue_range( fildes,
		    0.8, 1.0,
		    1.0, 0.0 ); */
	/* Don't need to do non-forced updates when static. */
	if (d->static_time)
	    return;

	/* Only update if sufficient time has passed. */
	delta = difftime( now_seconds, last_update_time );
	if (delta < DAYLIGHT_UPDATE_INTERVAL)
	    return;
    }

    /* Update colors based on the computed time. */
    tm = localtime( &now_seconds );  
    now_hours = hms_to_h(tm->tm_hour, tm->tm_min, tm->tm_sec);
    daylight_compute_colors( d, now_hours );

    /* Set color values. */
    disp->backgroundColor( disp,
			daylight_background_color );
    disp->ambientLight( disp,
		1.0, daylight_ambient_color );
    /* Colour only.  cam_update() owns the range, and recomputes it from
     * the clip planes every frame, so setting it here would clobber it.
     */
    disp->doFog( disp,
			driveFogAmount > 0.0, daylight_depth_cue_color );
    /*
    depth_cue_range( fildes,
		    daylight_depth_cue_range[0],
		    daylight_depth_cue_range[1],
		    1.0, 0.0 ); */

    /* Define light for sun.  In the future, if we decide to
     * actually change the position of the sun dynamically, this
     * should be moved to daylight_update().
     */
    {
	static float sunDir[3] = {-SUN_DIRECTION_X,
					-SUN_DIRECTION_Y,
					-SUN_DIRECTION_Z};
	disp->directionalLight( disp,
			daylight_sun_color,
			sunDir );
    }
    disp->enableLighting( disp, 1 );

    /* Copy background color to structure for other modules' use */
    cstate.daylight.back_clr[0] = daylight_background_color[0];
    cstate.daylight.back_clr[1] = daylight_background_color[1];
    cstate.daylight.back_clr[2] = daylight_background_color[2];

    /* Record update time. */
    last_update_time = now_seconds;
}
