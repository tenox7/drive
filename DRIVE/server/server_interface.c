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


#if defined(MAC) || defined(WIN32)

#include "global.h"
#include "drive.h"
#include "filenames.h"
#include "gauge.h"
#include "drive_server.h"

/*** PROGRAM GLOBALS ***/
int server_state,
    loop_timer,
    ignore_input;
COURSE *course;		/* head of linked list */
COURSE *current_course;
time_value server_virtual_time;
boolean_type server_virtual_time_frozen;

static boolean_type automatic_mode;
static boolean_type user_changed_virtual_time = FALSE;
static boolean_type iface_init;

#include "textgad.h"
#include "text_gui.h"

void outputString( const char *str )
{
    if (iface_init) {
        tgAddString(str, glist, GAD_MESSAGE, GAD_MESSAGE_SIZE);
    } else {
        fprintf(stderr, "%s\n", str);
    }
}

static void reformat_seconds(
    char *str,
    int seconds)
{
    int h,m,s;

    /* Convert to standard format */
    s = seconds;
    h = s/3600; s -= h*3600;  if (h > 99) h = 99;
    m = s/60;   s -= m*60;
    if (h > 0) {
	sprintf(str,"%2d:%02d:%02d",h,m,s);
    }
    else {
	sprintf(str,"   %2d:%02d",m,s);
    }
}

void state_clock_tick(
    void)
{
    static char str[256];

    if (!iface_init) return;

    if (((server_state == PRE_RACE_STATE)
		|| (server_state == POST_RACE_STATE))
	    && !automatic_mode) {
	sprintf(str,"%s"," WAITING");
    }
    else if (loop_timer == CLOCK_FROZEN) {
	/* No countdown to show -- the state clock is stopped. */
	sprintf(str,"%s","UNLIMITED");
    }
    else {
	reformat_seconds(str,loop_timer/LOOP_PER_SECOND);
    }
    glist[GAD_TIME].pval = str;
    tgUpdate(glist, GAD_TIME);
    tgFlush(glist);
}

static void update_state_window(
    int current_state)
{
    char *statestr;

    if (!iface_init) return;

    switch (current_state) {
	case PRACTICE_STATE:
	    statestr = "Practicing";
	    break;

	case PRE_RACE_STATE:
	    statestr = " Pre-Race ";
	    break;

	case RACE_STATE:
	    statestr = "  Racing  ";
	    break;

	case POST_RACE_STATE:
	    statestr = " Post-Race";
	    break;
    }

    glist[GAD_STATE].pval = statestr;
    tgUpdate(glist, GAD_STATE);
    tgFlush(glist);
}

void next_server_state(
    void)
{
    int next_state;

    /* Decide which state to go to */
    switch (server_state) {
	case PRACTICE_STATE:
	    if (current_course->pre_race_seconds > 0)
		next_state = PRE_RACE_STATE;
	    else if (current_course->race_seconds > 0)
		next_state = RACE_STATE;
	    else next_state = POST_RACE_STATE;
	    break;

	case PRE_RACE_STATE:
	    if (current_course->race_seconds > 0)
		next_state = RACE_STATE;
	    else if (current_course->post_race_seconds > 0)
		next_state = POST_RACE_STATE;
	    else next_state = PRACTICE_STATE;
	    break;

	case RACE_STATE:
	    update_leader_board(STANDINGS_FINAL);
	    if (current_course->post_race_seconds > 0)
		next_state = POST_RACE_STATE;
	    else if (current_course->practice_seconds > 0)
		next_state = PRACTICE_STATE;
	    else next_state = PRE_RACE_STATE;
	    break;

	case POST_RACE_STATE:
	    if (current_course->practice_seconds > 0)
		next_state = PRACTICE_STATE;
	    else if (current_course->pre_race_seconds > 0)
		next_state = PRE_RACE_STATE;
	    else next_state = RACE_STATE;
	    break;
    }

    /* Do everything necessary for the new state */
    server_state = next_state;
    switch (next_state) {
	case PRACTICE_STATE:
	    loop_timer = current_course->practice_seconds * LOOP_PER_SECOND;
	    break;

	case PRE_RACE_STATE:
	    if (automatic_mode)
		loop_timer = current_course->pre_race_seconds * LOOP_PER_SECOND;
	    else
		loop_timer = CLOCK_FROZEN;
	    reset_all_clients();
	    /* Make sure none of the scenefiles have changed. */
	    check_all_defined_scenefiles();
	    break;

	case RACE_STATE:
	    loop_timer = current_course->race_seconds * LOOP_PER_SECOND;
	    break;

	case POST_RACE_STATE:
	    if (automatic_mode)
		loop_timer = current_course->post_race_seconds * LOOP_PER_SECOND;
	    else
		loop_timer = CLOCK_FROZEN;
	    reset_all_clients();
	    /* Make sure none of the scenefiles have changed. */
	    check_all_defined_scenefiles();
	    break;
    }

    update_state_window(server_state);
    update_all_clients_state(server_state);
    ignore_input = (server_state == PRE_RACE_STATE)
	|| (server_state == POST_RACE_STATE);
    state_clock_tick();
}

/* Park the server in Practice with the state clock stopped, so clients can
 * drive indefinitely without anyone working the console.
 */
void set_practice_mode(
    void)
{
    automatic_mode = FALSE;
    glist[GAD_AUTO_CONTROL].ival = 0;
    server_state = PRACTICE_STATE;
    loop_timer = CLOCK_FROZEN;
    ignore_input = FALSE;

    if (iface_init) tgUpdate(glist, GAD_AUTO_CONTROL);
    update_state_window(server_state);
    update_all_clients_state(server_state);
    state_clock_tick();
}

void loop_timer_done(
    void)
{
    switch (server_state) {
	case PRACTICE_STATE:
	    next_server_state();
	    break;

	case RACE_STATE:
	    if (server_mode & SERVER_MODE_TIMEOUT_FINISH)
		next_server_state();
	    break;

	case PRE_RACE_STATE:
	case POST_RACE_STATE:
	    if (automatic_mode) next_server_state();
	    else loop_timer = CLOCK_FROZEN;
	    break;
    }
}

static void updateTOD(time_value t)
{
    int hrs, mins;

    hrs = (int)server_virtual_time;
    mins = (int)(60.0 * (server_virtual_time - hrs));

    glist[GAD_TOD_SLIDER].ival = hrs;
    sprintf(glist[GAD_TOD_BOX].pval, "%02d:%02d", hrs, mins);

    if (!iface_init) return;

    tgUpdate(glist, GAD_TOD_SLIDER);
    tgUpdate(glist, GAD_TOD_BOX);
    tgFlush(glist);
}

void set_virtual_time_slider(
    boolean_type override,
    time_value newtime,
    boolean_type freeze_it)
{
    /* Don't mess with it if the user has manually changed it. */
    if ((!override) && user_changed_virtual_time) return;

    if (override) server_virtual_time_frozen = freeze_it;
    server_virtual_time = newtime;

    updateTOD(newtime);
}

void virtual_clock_tick(
    void)
{
    if (!server_virtual_time_frozen) {
	if ((server_virtual_time += (1.0/60.0)) >= 24.0) {
	    server_virtual_time -= 24.0;
	}

        updateTOD(server_virtual_time);
	/* No need to tell the clients -- they'll keep running starting
	 * from the last time you told them about.
	 */
    }
}

static void courseCallback(char *newCourse)
{
    COURSE *crsptr;
    char *str;

    crsptr = course;
    while (crsptr != NULL) {
	if (strcmp(crsptr->coursename,newCourse) == 0) break;
	crsptr = crsptr->next;
    }
    if (crsptr == NULL) return;

    /* else stuff the values into the right places */
    current_course = crsptr;

    /* Times */
    str = glist[GAD_PRACTICE].pval;
    reformat_seconds(str,current_course->practice_seconds);
    tgUpdate(glist, GAD_PRACTICE);

    str = glist[GAD_PRE_RACE].pval;
    reformat_seconds(str,current_course->pre_race_seconds);
    tgUpdate(glist, GAD_PRE_RACE);

    str = glist[GAD_RACE].pval;
    reformat_seconds(str,current_course->race_seconds);
    tgUpdate(glist, GAD_RACE);

    str = glist[GAD_POST_RACE].pval;
    reformat_seconds(str,current_course->post_race_seconds);
    tgUpdate(glist, GAD_POST_RACE);

    /* Turbo mode */
    switch (current_course->turbo_mode) {
    case TURBO_NONE:
        glist[GAD_TURBO].ival = 0;
        break;
    case TURBO_LIMITED:
        glist[GAD_TURBO].ival = 1;
        break;
    case TURBO_UNLIMITED:
        glist[GAD_TURBO].ival = 2;
        break;
    }
    tgUpdate(glist, GAD_TURBO);

    /* Turbo boosts */
    sprintf(glist[GAD_BOOSTS].pval, "%d", current_course->turbo_boosts);
    tgUpdate(glist, GAD_BOOSTS);


    /* Guns mode */
    switch (current_course->guns_mode) {
    case GUNS_NONE:
        glist[GAD_GUNS].ival = 0;
        break;
    case GUNS_LIMITED:
        glist[GAD_GUNS].ival = 1;
        break;
    case GUNS_UNLIMITED:
        glist[GAD_GUNS].ival = 2;
        break;
    }
    tgUpdate(glist, GAD_GUNS);

    /* Ammo */
    sprintf(glist[GAD_AMMO].pval, "%d", current_course->ammo);
    tgUpdate(glist, GAD_AMMO);
    tgFlush(glist);
}

static int stringToSeconds(const char *str)
{
    int h, m, s;

    if (sscanf(str,"%d:%d:%d",&h,&m,&s) == 3) {
	return h*3600 + m*60 + s;
    }
    else if (sscanf(str,"%d:%d",&m,&s) == 2) {
	return m*60 + s;
    }
    else if (sscanf(str,"%d",&s) == 1) {
	return s;
    }
    return 0;
}

void callback(TextGadget *glist, int which)
{
    int i, seconds;

    switch (which) {
    case GAD_NEXT_STATE :
        next_server_state();
        break;
    case GAD_AUTO_CONTROL :
	automatic_mode = glist[which].ival;
	if (automatic_mode && current_course) {
	    /* Un-freeze the clock if we are sitting in a waiting state */
	    if (server_state == PRE_RACE_STATE)
		loop_timer = current_course->pre_race_seconds * LOOP_PER_SECOND;
	    else if (server_state == POST_RACE_STATE)
		loop_timer = current_course->post_race_seconds * LOOP_PER_SECOND;
	    state_clock_tick();
	}
	break;
    case GAD_COURSE :
        i = glist[which].ival;
        courseCallback(courses[i]);
        break;
    case GAD_START :
        server_mode &= ~SERVER_START_MODES;
        switch (glist[which].ival) {
        case 0 :
            server_mode |= SERVER_MODE_SIMULTANEOUS_START;
            break;
        case 1 :
            server_mode |= SERVER_MODE_STARTLINE_START;
            break;
        }
        break;
    case GAD_FINISH :
        server_mode &= ~SERVER_FINISH_MODES;
        switch (glist[which].ival) {
        case 0 :
            server_mode |= SERVER_MODE_TIMEOUT_FINISH;
            break;
        case 1 :
            server_mode |= SERVER_MODE_FINISHLINE_FINISH;
            break;
        case 2 :
            server_mode |= SERVER_MODE_TIMEOUT_FINISH |
                           SERVER_MODE_FINISHLINE_FINISH;
            break;
        }
        break;
    case GAD_WRECK :
        server_mode &= ~SERVER_WRECK_MODES;
        switch (glist[which].ival) {
        case 0 :
            server_mode |= SERVER_MODE_WRECK_NO_ACTION;
            break;
        case 1 :
            server_mode |= SERVER_MODE_WRECK_UPRIGHT;
            break;
        case 2 :
            server_mode |= SERVER_MODE_WRECK_RESTART;
            break;
        }
        break;
    case GAD_EXPLOSION :
        server_mode &= ~SERVER_EXPLOSION_MODES;
        switch (glist[which].ival) {
        case 0 :
            server_mode |= SERVER_MODE_EXPLOSION_DISABLED;
            break;
        case 1 :
            server_mode |= SERVER_MODE_EXPLOSION_VISUAL;
            break;
        case 2 :
            server_mode |= SERVER_MODE_EXPLOSION_FORCE;
            break;
        case 3 :
            server_mode |= SERVER_MODE_EXPLOSION_RESTART;
            break;
        }
        break;
    case GAD_TURBO :
        switch (glist[which].ival) {
        case 0 :
            current_course->turbo_mode = TURBO_NONE;
            break;
        case 1 :
            current_course->turbo_mode = TURBO_LIMITED;
            break;
        case 2 :
            current_course->turbo_mode = TURBO_UNLIMITED;
            break;
        }
        break;
    case GAD_BOOSTS :
        current_course->turbo_boosts = atoi(glist[which].pval);
        break;
    case GAD_GUNS :
        switch (glist[which].ival) {
        case 0 :
            current_course->guns_mode = GUNS_NONE;
            break;
        case 1 :
            current_course->guns_mode = GUNS_LIMITED;
            break;
        case 2 :
            current_course->guns_mode = GUNS_UNLIMITED;
            break;
        }
        break;
    case GAD_AMMO :
        current_course->ammo = atoi(glist[which].pval);
        break;
    case GAD_PRACTICE :
    case GAD_PRE_RACE :
    case GAD_RACE :
    case GAD_POST_RACE :
        seconds = stringToSeconds(glist[which].pval);
        switch (which) {
        case GAD_PRACTICE :
            current_course->practice_seconds = seconds;
            break;
        case GAD_PRE_RACE :
            current_course->pre_race_seconds = seconds;
            break;
        case GAD_RACE :
            current_course->race_seconds = seconds;
            break;
        case GAD_POST_RACE :
            current_course->post_race_seconds = seconds;
            break;
        }
        reformat_seconds(glist[which].pval, seconds);
        tgUpdate(glist, which);
        tgFlush(glist);
        break;
    case GAD_TOD_SLIDER :
        user_changed_virtual_time = TRUE;

        server_virtual_time = (time_value) glist[which].ival;
        if (server_virtual_time == 24.0) server_virtual_time = 0.0;

        change_virtual_time(server_virtual_time_frozen, server_virtual_time);
        updateTOD(server_virtual_time);
        break;
    case GAD_TOD_BOX :
        user_changed_virtual_time = TRUE;

        seconds = stringToSeconds(glist[which].pval);
        if (seconds > 24*60) seconds = 24*60-1;
        server_virtual_time = (time_value) seconds / 60.0;

        change_virtual_time(server_virtual_time_frozen, server_virtual_time);
        updateTOD(server_virtual_time);
        break;
    case GAD_TOD_FREEZE :
        server_virtual_time_frozen = glist[which].ival;
        break;
    case GAD_QUIT :
        shutdown_server();
        break;
    }
}

void process_X_events(
    void)
{
    while (tgCheckInput(glist, callback)) {
        /* NOTHING */
    }
}

static int cmpStr(const void *a, const void *b)
{
    const char **sa, **sb;
    sa = a;
    sb = b;
    return strcmp(*sa, *sb);
}

void open_server_interface(
    int argc, char *argv[])
{
    COURSE *crsptr;
    int i;

    /* Initialize the "courses" array */
    if (course->next) {
        i = 0;
        crsptr = course;
        while (crsptr) {
            courses[i++] = crsptr->coursename;
            crsptr = crsptr->next;
        }
        courses[i] = NULL;
        qsort(courses, i, sizeof(char *), cmpStr);
    }

    tgCompile(glist);

    /* DRIVE_NO_CONSOLE: no operator console at all.  Every tg* call then
     * does nothing and messages go to stderr, so the server can run out of
     * sight with no terminal of its own.
     */
    if (getenv("DRIVE_NO_CONSOLE") == NULL) {
        if (!tgInit()) {
            exit(1);
        }
        tgPaintAll(glist);
        iface_init = 1;
    }

    /* Make the display agree with reality: fill in the course times and
     * pick up the initial state of the Automatic Race Control checkbox.
     */
    automatic_mode = glist[GAD_AUTO_CONTROL].ival;
    courseCallback(current_course->coursename);
    update_state_window(server_state);
    state_clock_tick();
}

time_t time_value_to_secs(
    time_value vt)
{
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <langinfo.h>
#include <locale.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/Text.h>
#include <Xm/ToggleBG.h>

#include "global.h"
#include "drive.h"
#include "filenames.h"
#include "gauge.h"
#include "drive_server.h"

#define CLASS			"DriveServer"
#define PROGRAM			"driveServer"
#define MAX_ARG			32
#define GUTTER			10	/* space between objects */
#define LABELHEIGHT		30
#define TEXT_STRLEN		8
#define BIGFONT			"fgb-13"
#define BIG_INT			0x7fffffff
#define RIGHT_COLUMN_POSITION	65	/* % of the way over, left to right */
#define QUIT_RIGHT_POSITION	(RIGHT_COLUMN_POSITION-5)
#define VIRTUAL_TIME_GRANULARITY	60

static char *turbo_modes[] = {"Off", "Limited", "Unlimited" };
#define NUM_TURBO_MODES (sizeof(turbo_modes)/sizeof(char *))
static char *guns_modes[] = {"Off", "Limited Ammo", "Unlimited Ammo" };
#define NUM_GUNS_MODES (sizeof(guns_modes)/sizeof(char *))

static char *wreck_actions[] = {"None", "Upright", "Restart" };
#define NUM_WRECK_ACTIONS (sizeof(wreck_actions)/sizeof(char *))
static char *explosion_actions[] = {"Disabled", "Visual Only",
    "Random Force", "Restart" };
#define NUM_EXPLOSION_ACTIONS (sizeof(explosion_actions)/sizeof(char *))
static char *start_actions[] = {"Race Starts", "Start Line Crossed" };
#define NUM_START_ACTIONS (sizeof(start_actions)/sizeof(char *))
static char *finish_actions[] = {"Race Time Over", "Finish Line Crossed",
    "Either" };
#define NUM_FINISH_ACTIONS (sizeof(finish_actions)/sizeof(char *))


/*** PROGRAM GLOBALS ***/
int server_state,
    loop_timer,
    ignore_input;
COURSE *course;		/* head of linked list */
COURSE *current_course;
time_value server_virtual_time;
boolean_type server_virtual_time_frozen;


/*** MODULE GLOBALS ***/
static XtAppContext app_context;
static Display *display;
static Widget stateW,timeW;
static Boolean automatic_mode;
static Widget practiceW, preRaceW, raceW, postRaceW,
    wreckW, explosionW, startW, finishW,
    turboModeW, turboW, turboBtnW[NUM_TURBO_MODES],
    gunsModeW, ammoW, gunsBtnW[NUM_TURBO_MODES],
    virtualTimeW = NULL, vtLabelW = NULL;
static boolean_type user_changed_virtual_time = FALSE;


time_t time_value_to_secs(
    time_value vt)
{
    time_t t;
    struct tm *lt;
    float current_time,delta;

    t = time(NULL);
    /* Convert to local time */
    lt = localtime(&t);
    if (lt == NULL)
	return t;
    current_time =
	  (float) lt->tm_hour
	+ (float) lt->tm_min / 60.0
	+ (float) lt->tm_sec / 3600.0;
    delta = vt - current_time;
    if (delta < 0.0) delta += 24*3600;

    return((time_t) ((int) t + (int) (delta*3600.0 + 0.5)));
}


static void reformat_seconds(
    char *str,
    int seconds)
{
    int h,m,s;

    /* Convert to standard format */
    s = seconds;
    h = s/3600; s -= h*3600;  if (h > 99) h = 99;
    m = s/60;   s -= m*60;
    if (h > 0) {
	sprintf(str,"%2d:%02d:%02d",h,m,s);
    }
    else {
	sprintf(str,"   %2d:%02d",m,s);
    }
}


/* Returns a pointer to a reused static area */
static char *time_of_day_string(
    time_value virtual_time)
{
    char *nlfmt,*cptr1,*cptr2,format[256],str[256];
    static char result[256];
    time_t secs;
    struct tm *local;

    /* setlocale should have already been called */	

    nlfmt = nl_langinfo(T_FMT_AMPM);
    /* Strip out the ":%S" */
    cptr1 = nlfmt;
    cptr2 = format;
    while (*cptr1 != '\0') {
        if ((*cptr1 == ':')
                && (*(cptr1+1) == '%')
                && (*(cptr1+2) == 'S')) {
            cptr1 += 3;
        }
        else {
            *cptr2++ = *cptr1++;
        }
    }
    *cptr2 = '\0';

    secs = time_value_to_secs(virtual_time);
    local = localtime(&secs);
    strftime(str,sizeof(str),format,local);
    sprintf(result,"Time of Day:  %s",str);
    return(result);
}


static void set_label(
    Widget w,
    char *label)
{
    Arg arg[MAX_ARG];
    Cardinal ac;
    XmString xmstr;

    ac = 0;
    xmstr = XmStringCreateLtoR(label,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr);  ++ac;
    XtSetValues(w,arg,ac);
    XmStringFree(xmstr);
}


void state_clock_tick(
    void)
{
    char str[256];

    if (((server_state == PRE_RACE_STATE)
		|| (server_state == POST_RACE_STATE))
	    && !automatic_mode) {
	sprintf(str,"%*s",TEXT_STRLEN,"WAITING");
    }
    else {
	reformat_seconds(str,loop_timer/LOOP_PER_SECOND);
    }
    set_label(timeW,str);
}


static void update_state_window(
    int current_state)
{
    char *statestr;

    switch (current_state) {
	case PRACTICE_STATE:
	    statestr = "Practicing";
	    break;

	case PRE_RACE_STATE:
	    statestr = " Pre-Race ";
	    break;

	case RACE_STATE:
	    statestr = "  Racing  ";
	    break;

	case POST_RACE_STATE:
	    statestr = " Post-Race";
	    break;
    }

    set_label(stateW,statestr);
}


void next_server_state(
    void)
{
    int next_state;

    /* Decide which state to go to */
    switch (server_state) {
	case PRACTICE_STATE:
	    if (current_course->pre_race_seconds > 0)
		next_state = PRE_RACE_STATE;
	    else if (current_course->race_seconds > 0)
		next_state = RACE_STATE;
	    else next_state = POST_RACE_STATE;
	    break;

	case PRE_RACE_STATE:
	    if (current_course->race_seconds > 0)
		next_state = RACE_STATE;
	    else if (current_course->post_race_seconds > 0)
		next_state = POST_RACE_STATE;
	    else next_state = PRACTICE_STATE;
	    break;

	case RACE_STATE:
	    update_leader_board(STANDINGS_FINAL);
	    if (current_course->post_race_seconds > 0)
		next_state = POST_RACE_STATE;
	    else if (current_course->practice_seconds > 0)
		next_state = PRACTICE_STATE;
	    else next_state = PRE_RACE_STATE;
	    break;

	case POST_RACE_STATE:
	    if (current_course->practice_seconds > 0)
		next_state = PRACTICE_STATE;
	    else if (current_course->pre_race_seconds > 0)
		next_state = PRE_RACE_STATE;
	    else next_state = RACE_STATE;
	    break;
    }

    /* Do everything necessary for the new state */
    server_state = next_state;
    switch (next_state) {
	case PRACTICE_STATE:
	    loop_timer = current_course->practice_seconds * LOOP_PER_SECOND;
	    break;

	case PRE_RACE_STATE:
	    if (automatic_mode)
		loop_timer = current_course->pre_race_seconds * LOOP_PER_SECOND;
	    else
		loop_timer = CLOCK_FROZEN;
	    reset_all_clients();
	    /* Make sure none of the scenefiles have changed. */
	    check_all_defined_scenefiles();
	    break;

	case RACE_STATE:
	    loop_timer = current_course->race_seconds * LOOP_PER_SECOND;
	    break;

	case POST_RACE_STATE:
	    if (automatic_mode)
		loop_timer = current_course->post_race_seconds * LOOP_PER_SECOND;
	    else
		loop_timer = CLOCK_FROZEN;
	    reset_all_clients();
	    /* Make sure none of the scenefiles have changed. */
	    check_all_defined_scenefiles();
	    break;
    }

    update_state_window(server_state);
    update_all_clients_state(server_state);
    ignore_input = (server_state == PRE_RACE_STATE)
	|| (server_state == POST_RACE_STATE);
    state_clock_tick();
}


void loop_timer_done(
    void)
{
    switch (server_state) {
	case PRACTICE_STATE:
	    next_server_state();
	    break;

	case RACE_STATE:
	    if (server_mode & SERVER_MODE_TIMEOUT_FINISH)
		next_server_state();
	    break;

	case PRE_RACE_STATE:
	case POST_RACE_STATE:
	    if (automatic_mode) next_server_state();
	    else loop_timer = CLOCK_FROZEN;
	    break;
    }
}


static void automaticCallback(
    Widget whichW,
    caddr_t data,
    XmToggleButtonCallbackStruct *cb)
{
    automatic_mode = cb->set;
    if (automatic_mode) {
	if (server_state == PRE_RACE_STATE)
	    loop_timer = current_course->pre_race_seconds * LOOP_PER_SECOND;
	else if (server_state == POST_RACE_STATE)
	    loop_timer = current_course->post_race_seconds * LOOP_PER_SECOND;
	state_clock_tick();
    }
}

static void quitCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{
    shutdown_server();
}


static void courseCallback(
    Widget whichW,
    char *newCourse,
    XmAnyCallbackStruct *cb)
{
    COURSE *crsptr;
    char str[256];
    Cardinal ac;
    Arg arg[MAX_ARG];

    crsptr = course;
    while (crsptr != NULL) {
	if (strcmp(crsptr->coursename,newCourse) == 0) break;
	crsptr = crsptr->next;
    }
    if (crsptr == NULL) return;

    /* else stuff the values into the right places */
    current_course = crsptr;

    /* Times */
    reformat_seconds(str,current_course->practice_seconds);
    XmTextSetString(practiceW,str);

    reformat_seconds(str,current_course->pre_race_seconds);
    XmTextSetString(preRaceW,str);

    reformat_seconds(str,current_course->race_seconds);
    XmTextSetString(raceW,str);

    reformat_seconds(str,current_course->post_race_seconds);
    XmTextSetString(postRaceW,str);
	
    /* Turbo mode */
    ac = 0;
    switch (current_course->turbo_mode) {
	case TURBO_NONE:
	    XtSetArg(arg[ac],XmNmenuHistory,turboBtnW[0]); ++ac;
	    break;
	case TURBO_LIMITED:
	    XtSetArg(arg[ac],XmNmenuHistory,turboBtnW[1]); ++ac;
	    break;
	case TURBO_UNLIMITED:
	    XtSetArg(arg[ac],XmNmenuHistory,turboBtnW[2]); ++ac;
	    break;
    }
    XtSetValues(turboModeW,arg,ac);

    /* Turbo boosts */
    sprintf(str,"%d",current_course->turbo_boosts);
    XmTextSetString(turboW,str);


    /* Guns mode */
    ac = 0;
    switch (current_course->guns_mode) {
	case GUNS_NONE:
	    XtSetArg(arg[ac],XmNmenuHistory,gunsBtnW[0]); ++ac;
	    break;
	case GUNS_LIMITED:
	    XtSetArg(arg[ac],XmNmenuHistory,gunsBtnW[1]); ++ac;
	    break;
	case GUNS_UNLIMITED:
	    XtSetArg(arg[ac],XmNmenuHistory,gunsBtnW[2]); ++ac;
	    break;
    }
    XtSetValues(gunsModeW,arg,ac);

    /* Ammo */
    sprintf(str,"%d",current_course->ammo);
    XmTextSetString(ammoW,str);
}


static void timeChangeCallback(
    Widget whichW,
    int whichTime,
    XmTextVerifyCallbackStruct *cb)
{
    char *newtext;
    char str[256];
    int h,m,s,seconds;

    /* Get the text */
    newtext = XmTextGetString(whichW);
    if (sscanf(newtext,"%d:%d:%d",&h,&m,&s) == 3) {
	seconds = h*3600 + m*60 + s;
    }
    else if (sscanf(newtext,"%d:%d",&m,&s) == 2) {
	seconds = m*60 + s;
    }
    else if (sscanf(newtext,"%d",&s) == 1) {
	seconds = s;
    }

    /* Convert to standard format */
    reformat_seconds(str,seconds);
    XmTextSetString(whichW,str);

    /* Update the number */
    switch (whichTime) {
	case PRACTICE_STATE:
	    current_course->practice_seconds = seconds;
	    break;

	case PRE_RACE_STATE:
	    current_course->pre_race_seconds = seconds;
	    break;

	case RACE_STATE:
	    current_course->race_seconds = seconds;
	    break;

	case POST_RACE_STATE:
	    current_course->post_race_seconds = seconds;
	    break;
    }

    XtFree(newtext);
}


static void turboModeCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    if (strcmp(newmode,turbo_modes[0]) == 0)
	current_course->turbo_mode = TURBO_NONE;
    else if (strcmp(newmode,turbo_modes[1]) == 0)
	current_course->turbo_mode = TURBO_LIMITED;
    else 
	current_course->turbo_mode = TURBO_UNLIMITED;
}


static void turboBoostsCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{
    char *newtext;
    char str[256];

    /* Get the text */
    newtext = XmTextGetString(whichW);
    current_course->turbo_boosts = atoi(newtext);
    sprintf(str,"%d",current_course->turbo_boosts);
    XmTextSetString(whichW,str);

    XtFree(newtext);
}


static void gunsModeCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    if (strcmp(newmode,guns_modes[0]) == 0)
	current_course->guns_mode = GUNS_NONE;
    else if (strcmp(newmode,guns_modes[1]) == 0)
	current_course->guns_mode = GUNS_LIMITED;
    else 
	current_course->guns_mode = GUNS_UNLIMITED;
}


static void ammoCallback(
    Widget whichW,
    caddr_t data,
    XmAnyCallbackStruct *cb)
{
    char *newtext;
    char str[256];

    /* Get the text */
    newtext = XmTextGetString(whichW);
    current_course->ammo = atoi(newtext);
    sprintf(str,"%d",current_course->ammo);
    XmTextSetString(whichW,str);

    XtFree(newtext);
}


static void wreckCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    /* Turn them off */
    server_mode &= ~SERVER_WRECK_MODES;

    if (strcmp(newmode,wreck_actions[0]) == 0)
	server_mode |= SERVER_MODE_WRECK_NO_ACTION;
    else if (strcmp(newmode,wreck_actions[1]) == 0)
	server_mode |= SERVER_MODE_WRECK_UPRIGHT;
    else if (strcmp(newmode,wreck_actions[2]) == 0)
	server_mode |= SERVER_MODE_WRECK_RESTART;
}


static void explosionCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    /* Turn them off */
    server_mode &= ~SERVER_EXPLOSION_MODES;

    if (strcmp(newmode,explosion_actions[0]) == 0)
	server_mode |= SERVER_MODE_EXPLOSION_DISABLED;
    else if (strcmp(newmode,explosion_actions[1]) == 0)
	server_mode |= SERVER_MODE_EXPLOSION_VISUAL;
    else if (strcmp(newmode,explosion_actions[2]) == 0)
	server_mode |= SERVER_MODE_EXPLOSION_FORCE;
    else if (strcmp(newmode,explosion_actions[3]) == 0)
	server_mode |= SERVER_MODE_EXPLOSION_RESTART;
}


static void startCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    /* Turn them off */
    server_mode &= ~SERVER_START_MODES;

    if (strcmp(newmode,start_actions[0]) == 0)
	server_mode |= SERVER_MODE_SIMULTANEOUS_START;
    else
	server_mode |= SERVER_MODE_STARTLINE_START;
}


static void finishCallback(
    Widget whichW,
    char *newmode,
    XmAnyCallbackStruct *cb)
{
    /* Turn them off */
    server_mode &= ~SERVER_FINISH_MODES;

    if (strcmp(newmode,finish_actions[0]) == 0)
	server_mode |= SERVER_MODE_TIMEOUT_FINISH;
    else if (strcmp(newmode,finish_actions[1]) == 0)
	server_mode |= SERVER_MODE_FINISHLINE_FINISH;
    else
	server_mode |= SERVER_MODE_TIMEOUT_FINISH|SERVER_MODE_FINISHLINE_FINISH;
}


static void virtualTimeChangeCallback(
    Widget whichW,
    char *newmode,
    XmScaleCallbackStruct *cb)
{
    int val;

    user_changed_virtual_time = TRUE;
    XmScaleGetValue(virtualTimeW,&val);
    server_virtual_time = (time_value) ((float) val / VIRTUAL_TIME_GRANULARITY);
    if (server_virtual_time == 24.0) server_virtual_time = 0.0;


    set_label(vtLabelW,time_of_day_string(server_virtual_time));
    change_virtual_time(server_virtual_time_frozen,server_virtual_time);
}


static void freezeTimeCallback(
    Widget whichW,
    caddr_t data,
    XmToggleButtonCallbackStruct *cb)
{
    server_virtual_time_frozen = (boolean_type) (cb->set);
}


/* external entrypoint */
void set_virtual_time_slider(
    boolean_type override,
    time_value newtime,
    boolean_type freeze_it)
{
    /* Don't mess with it if the user has manually changed it. */
    if ((!override) && user_changed_virtual_time) return;

    if (override) server_virtual_time_frozen = freeze_it;
    server_virtual_time = newtime;

    if (virtualTimeW != NULL) {
	XmScaleSetValue(virtualTimeW,
	    (int)(server_virtual_time*VIRTUAL_TIME_GRANULARITY));
    }
    if (vtLabelW != NULL) {
	set_label(vtLabelW,time_of_day_string(server_virtual_time));
    }
}


/* external entrypoint */
/* The server is letting this module know that a minute has passed. */
void virtual_clock_tick(
    void)
{
    if (!server_virtual_time_frozen) {
	if ((server_virtual_time += (1.0/60.0)) >= 24.0) {
	    server_virtual_time -= 24.0;
	}
	if (virtualTimeW != NULL) {
	    XmScaleSetValue(virtualTimeW,
		(int)(server_virtual_time*VIRTUAL_TIME_GRANULARITY));
	}
	if (vtLabelW != NULL) {
	    set_label(vtLabelW,time_of_day_string(server_virtual_time));
	}
	/* No need to tell the clients -- they'll keep running starting
	 * from the last time you told them about.
	 */
    }
}


static XmFontList createFontList(
    char *option,
    char *default_font)
{
    XFontStruct *xfs;
    char *fontname;

    if ((option == NULL) || (*option == '\0')) {
	fontname = default_font;
    }
    else if ((fontname = XGetDefault(display,PROGRAM,option)) == NULL) {
	fontname = default_font;
    }

    if ((xfs = XLoadQueryFont(display,fontname)) == NULL) {
        /* Load a default */
        if ((xfs = XLoadQueryFont(display,"fixed")) == NULL) {
            fprintf(stderr,"Cannot load font %s!!!\n",fontname);
            exit(1);
        }
    }
	
    return(XmFontListCreate(xfs,XmSTRING_DEFAULT_CHARSET));
}


static Widget createPulldown(
    Widget parentW,
    char *label,
    int numbtns, char *btn_labels[],
    XtCallbackProc callback,
    char *current_setting,
    Widget *buttons,
    Arg arg_option[MAX_ARG], Cardinal ac_option)
{
    Widget pulldownW,*subW,optionW,defW;
    int w;
    Cardinal ac;
    Arg arg[MAX_ARG];
    XmString xmstr;


    subW = (Widget *) fastmalloc(sizeof(Widget)*numbtns);

    ac = 0;
    pulldownW = XmCreatePulldownMenu(parentW,"pulldown",arg,ac);

    defW = NULL;
    for (w=0; w<numbtns; ++w) {
	ac = 0;
	xmstr = XmStringCreateLtoR(btn_labels[w],XmSTRING_DEFAULT_CHARSET);
	XtSetArg(arg[ac],XmNlabelString,xmstr);  ++ac;
	subW[w] = XmCreatePushButtonGadget(pulldownW,"names",arg,ac);
	XmStringFree(xmstr);
	if (callback != NULL) {
	    XtAddCallback(subW[w],XmNactivateCallback,
		callback,btn_labels[w]);
	}
	if (strcmp(btn_labels[w],current_setting) == 0) defW = subW[w];
    }
    XtManageChildren(subW,w);

    if (defW == NULL) defW = subW[0];

    /* Create options menu an attach pulldown */
    xmstr = XmStringCreateLtoR(label,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg_option[ac_option],XmNlabelString,xmstr);  ++ac_option;
    XtSetArg(arg_option[ac_option],XmNsubMenuId,pulldownW);  ++ac_option;
    XtSetArg(arg_option[ac_option],XmNmenuHistory,defW); ++ac_option;
    XtSetArg(arg_option[ac_option],XmNmarginWidth,0); ++ac_option;
    optionW = XmCreateOptionMenu(parentW,"option_menu",arg_option,ac_option);
    XmStringFree(xmstr);

    /* Eliminate pixmap */
    ac = 0;
    XtSetArg(arg[ac],XmNcascadePixmap,XmUNSPECIFIED_PIXMAP); ++ac;
    XtSetValues(XmOptionButtonGadget(optionW),arg,ac);

    if (buttons != NULL) {
	memcpy(buttons,subW,sizeof(Widget)*numbtns);
    }
    fastfree((void *) subW,sizeof(Widget)*numbtns);

    return(optionW);
}



static Widget createEditWidget(
    Widget parentW,
    Widget topW, Widget leftOppositeW,
    char *label, char *startValue,
    XtCallbackProc callback,
    caddr_t callback_data)
{
    XmString xmstr;
    Arg arg[MAX_ARG];
    Cardinal ac;
    Widget labelW,editW;

    /* Time label */
    ac = 0;
    xmstr = XmStringCreateLtoR(label,XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,topW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,leftOppositeW); ++ac;
    XtSetArg(arg[ac],XmNheight,LABELHEIGHT); ++ac;
    labelW = XmCreateLabelGadget(parentW,"label",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(labelW);

    /* Time field */
    ac = 0;
    XtSetArg(arg[ac],XmNvalue,startValue); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,topW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,labelW); ++ac;
    XtSetArg(arg[ac],XmNmaxLength,TEXT_STRLEN); ++ac;
    XtSetArg(arg[ac],XmNcolumns,TEXT_STRLEN); ++ac;
    XtSetArg(arg[ac],XmNcursorPosition,TEXT_STRLEN); ++ac;
    XtSetArg(arg[ac],XmNheight,LABELHEIGHT); ++ac;
    editW = XmCreateText(parentW,"edit",arg,ac);
    XtManageChild(editW);
    XtAddCallback(editW,XmNlosingFocusCallback,
	callback, callback_data);

    return(editW);
}


void process_X_events(
    void)
{
    XEvent event;

    while (XtAppPending(app_context)) {
	XtAppNextEvent(app_context,&event);
	XtDispatchEvent(&event);
    }
}


void outputString( const char *str )
{
    fprintf(stderr, "%s\n", str);
}

void open_server_interface(
    int argc, char *argv[])
{
    Widget app_shellW, formW, csLabelW, tlLabelW, nsBtnW,
	autoBtnW, quitBtnW, courseW, freezeTimeW;
    XmString xmstr;
    Arg arg[MAX_ARG];
    Cardinal ac;
    char *racecourses[1024];
    int num_racecourses;
    COURSE *crsptr;
    XmFontList bigFL;
    char str[256],*cptr;
    char *dflt;


    /* Set up so we can print time values to the user's LANG preference. */
    setlocale(LC_TIME,"");

    loop_timer = 0;

    server_state = POST_RACE_STATE;

#if XtSpecificationRelease == 4
    app_shellW = XtAppInitialize(&app_context,CLASS,
	NULL,0,
	(Cardinal *) &argc,argv,
	NULL,NULL,0);
#else
    app_shellW = XtAppInitialize(&app_context,CLASS,
	NULL,0,
	&argc,argv,
	NULL,NULL,0);
#endif /* X11R4 else */

    if (app_shellW == NULL) {
	fprintf(stderr,"Could not open Motif1.2 X11R5 Windows on display!!\n");
	exit(1);
    }

    display = XtDisplay(app_shellW);

    bigFL = createFontList("bigfont",BIGFONT);

    /*** The main form ***/
    ac = 0;
    XtSetArg(arg[ac],XmNallowOverlap, False); ++ac;
    XtSetArg(arg[ac],XmNrubberPositioning, True); ++ac;
    formW = XmCreateForm(app_shellW,"main",arg,ac);
    XtManageChild(formW);

    /******************************* TOP STUFF ********************************/
    /* "Current state" label */
    ac = 0;
    xmstr = XmStringCreateLtoR("Current State: ",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNleftOffset,GUTTER); ++ac;
    csLabelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(csLabelW);

    /* Current state display */
    ac = 0;
    xmstr = XmStringCreateLtoR(" Post-Race",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,csLabelW); ++ac;
    XtSetArg(arg[ac],XmNfontList,bigFL); ++ac;
    XtSetArg(arg[ac],XmNforeground,BlackPixel(display,DefaultScreen(display)));
	++ac;
    XtSetArg(arg[ac],XmNbackground,WhitePixel(display,DefaultScreen(display)));
	++ac;
    stateW = XmCreateLabel(formW,"label",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(stateW);

    /* Time left display */
    ac = 0;
    xmstr = XmStringCreateLtoR("   00:00",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightOffset,GUTTER); ++ac;
    timeW = XmCreateLabelGadget(formW,"label",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(timeW);

    /* "Time left" label */
    ac = 0;
    xmstr = XmStringCreateLtoR("Time Left: ",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_END); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNrightWidget,timeW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,stateW); ++ac;
    XtSetArg(arg[ac],XmNleftOffset,GUTTER*2); ++ac;
    tlLabelW = XmCreateLabelGadget(formW,"label",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(tlLabelW);

    /**************************** LEFT COLUMN *********************************/
    /* Next state button */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,tlLabelW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER*2); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_POSITION); ++ac;
    XtSetArg(arg[ac],XmNrightPosition,45); ++ac;
    XtSetArg(arg[ac],XmNfontList,bigFL); ++ac;
    XtSetArg(arg[ac],XmNheight,LABELHEIGHT*3/2); ++ac;
    xmstr = XmStringCreateLtoR("NEXT STATE",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    XtSetArg(arg[ac],XmNshadowThickness,4); ++ac;
    nsBtnW = XmCreatePushButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(nsBtnW,XmNactivateCallback,
	(XtCallbackProc) next_server_state, NULL);
    XmStringFree(xmstr);
    XtManageChild(nsBtnW);

    /* Autorun toggle button */
    automatic_mode = False;
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,nsBtnW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER*2); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    xmstr = XmStringCreateLtoR("Automatic Race Control",
	XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    autoBtnW = XmCreateToggleButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(autoBtnW,XmNvalueChangedCallback,
	(XtCallbackProc) automaticCallback, NULL);
    XmStringFree(xmstr);
    XtManageChild(autoBtnW);

    /* Race start selector */
    if (server_mode & SERVER_MODE_SIMULTANEOUS_START)
	dflt = start_actions[0];
    else
	dflt = start_actions[1];
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,autoBtnW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    startW = createPulldown(formW,"Start stopwatch when: ",
	NUM_START_ACTIONS, start_actions,
	(XtCallbackProc) startCallback, dflt, NULL, arg,ac);
    XtManageChild(startW);

    /* Race finish selector */
    if ((server_mode & SERVER_FINISH_MODES) == SERVER_FINISH_MODES)
	dflt = finish_actions[2];
    else if (server_mode & SERVER_MODE_TIMEOUT_FINISH)
	dflt = finish_actions[0];
    else
	dflt = finish_actions[1];
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,startW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    finishW = createPulldown(formW,"End race when: ",
	NUM_FINISH_ACTIONS, finish_actions,
	(XtCallbackProc) finishCallback, dflt, NULL, arg,ac);
    XtManageChild(finishW);

    /* Wreck action selector */
    if (server_mode & SERVER_MODE_WRECK_UPRIGHT)
	dflt = wreck_actions[1];
    else if (server_mode & SERVER_MODE_WRECK_RESTART)
	dflt = wreck_actions[2];
    else
	dflt = wreck_actions[0];
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,finishW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    wreckW = createPulldown(formW,"Wreck Action: ",
	NUM_WRECK_ACTIONS, wreck_actions,
	(XtCallbackProc) wreckCallback, dflt, NULL, arg,ac);
    XtManageChild(wreckW);

    /* Explosion action selector */
    if (server_mode & SERVER_MODE_EXPLOSION_VISUAL)
	dflt = explosion_actions[1];
    if (server_mode & SERVER_MODE_EXPLOSION_FORCE)
	dflt = explosion_actions[2];
    else if (server_mode & SERVER_MODE_EXPLOSION_RESTART)
	dflt = explosion_actions[3];
    else
	dflt = explosion_actions[0];
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,wreckW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    explosionW = createPulldown(formW,"Explosion Action: ",
	NUM_EXPLOSION_ACTIONS, explosion_actions,
	(XtCallbackProc) explosionCallback, dflt, NULL, arg,ac);
    XtManageChild(explosionW);

    /* Time-of-day selector */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,explosionW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNdecimalPoints,1); ++ac;
    XtSetArg(arg[ac],XmNhighlightOnEnter,False); ++ac;
    XtSetArg(arg[ac],XmNhighlightThickness,0); ++ac;
    XtSetArg(arg[ac],XmNminimum,0); ++ac;
    XtSetArg(arg[ac],XmNmaximum,24*VIRTUAL_TIME_GRANULARITY-1); ++ac;
    XtSetArg(arg[ac],XmNorientation,XmHORIZONTAL); ++ac;
    XtSetArg(arg[ac],XmNprocessingDirection,XmMAX_ON_RIGHT); ++ac;
    XtSetArg(arg[ac],XmNscaleHeight,20); ++ac;
    XtSetArg(arg[ac],XmNscaleWidth,300); ++ac;
    XtSetArg(arg[ac],XmNshowValue,False); ++ac;
    XtSetArg(arg[ac],XmNtraversalOn,False); ++ac;
    XtSetArg(arg[ac],XmNvalue,
	(int) (server_virtual_time*VIRTUAL_TIME_GRANULARITY)); ++ac;
    virtualTimeW = XmCreateScale(formW,"scale",arg,ac);
    XtAddCallback(virtualTimeW,XmNvalueChangedCallback,
	(XtCallbackProc) virtualTimeChangeCallback, NULL);
    XtManageChild(virtualTimeW);

    /* And text output */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,virtualTimeW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNrightWidget,virtualTimeW); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    xmstr = XmStringCreateLtoR(time_of_day_string(server_virtual_time),
	XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    vtLabelW = XmCreateLabelGadget(formW,"scale",arg,ac);
    XmStringFree(xmstr);
    XtManageChild(vtLabelW);

    /* Time Freeze toggle button */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,vtLabelW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    xmstr = XmStringCreateLtoR("Freeze Time-of-Day",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    XtSetArg(arg[ac],XmNset,server_virtual_time_frozen); ++ac;
    freezeTimeW = XmCreateToggleButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(freezeTimeW,XmNvalueChangedCallback,
	(XtCallbackProc) freezeTimeCallback, NULL);
    XmStringFree(xmstr);
    XtManageChild(freezeTimeW);

    /* Quit button */
    ac = 0;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER*2); ++ac;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,freezeTimeW); ++ac;
    XtSetArg(arg[ac],XmNbottomAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_FORM); ++ac;
    XtSetArg(arg[ac],XmNrightAttachment,XmATTACH_POSITION); ++ac;
    XtSetArg(arg[ac],XmNrightPosition,QUIT_RIGHT_POSITION); ++ac;
    xmstr = XmStringCreateLtoR("QUIT",XmSTRING_DEFAULT_CHARSET);
    XtSetArg(arg[ac],XmNlabelString,xmstr); ++ac;
    XtSetArg(arg[ac],XmNalignment,XmALIGNMENT_CENTER); ++ac;
    quitBtnW = XmCreatePushButtonGadget(formW,"btn",arg,ac);
    XtAddCallback(quitBtnW,XmNactivateCallback,
	(XtCallbackProc) quitCallback, NULL);
    XmStringFree(xmstr);
    XtManageChild(quitBtnW);

    /**************************** RIGHT COLUMN ********************************/
    /* Race course selector */
    num_racecourses = 0;
    crsptr = course;
    while (crsptr != NULL) {
	racecourses[num_racecourses++] = crsptr->coursename;
	crsptr = crsptr->next;
    }

    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,timeW); ++ac;
    XtSetArg(arg[ac],XmNtopOffset,GUTTER*2); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_POSITION); ++ac;
    XtSetArg(arg[ac],XmNleftPosition,RIGHT_COLUMN_POSITION); ++ac;
    courseW = createPulldown(formW,"Course: ",
	num_racecourses, racecourses,
	(XtCallbackProc) courseCallback,
	current_course->coursename, NULL,
	arg,ac);
    XtManageChild(courseW);

    /* Times */
    reformat_seconds(str,current_course->practice_seconds);
    practiceW = createEditWidget(formW,courseW,courseW,
	"Practice Time:  ",str,
	(XtCallbackProc) timeChangeCallback,
	(caddr_t) PRACTICE_STATE);

    reformat_seconds(str,current_course->pre_race_seconds);
    preRaceW = createEditWidget(formW,practiceW,courseW,
	"Pre-Race Time:  ",str,
	(XtCallbackProc) timeChangeCallback,
	(caddr_t) PRE_RACE_STATE);

    reformat_seconds(str,current_course->race_seconds);
    raceW = createEditWidget(formW,preRaceW,courseW,
	"Race Time:      ",str,
	(XtCallbackProc) timeChangeCallback,
	(caddr_t) RACE_STATE);

    reformat_seconds(str,current_course->post_race_seconds);
    postRaceW = createEditWidget(formW,raceW,courseW,
	"Post-Race Time: ",str,
	(XtCallbackProc) timeChangeCallback,
	(caddr_t) POST_RACE_STATE);
	
    /* Turbo mode */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,postRaceW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,courseW); ++ac;
    if (current_course->turbo_mode == TURBO_NONE)
	cptr = turbo_modes[0];
    else if (current_course->turbo_mode == TURBO_LIMITED)
	cptr = turbo_modes[1];
    else if (current_course->turbo_mode == TURBO_UNLIMITED)
	cptr = turbo_modes[2];
    turboModeW = createPulldown(formW,"Turbo Mode: ",
	NUM_TURBO_MODES, turbo_modes,
	(XtCallbackProc) turboModeCallback,
	cptr, turboBtnW, arg,ac);
    XtManageChild(turboModeW);

    /* Turbo boosts */
    sprintf(str,"%d",current_course->turbo_boosts);
    turboW = createEditWidget(formW,turboModeW,courseW,
	"Turbo Boosts:   ",str,
	(XtCallbackProc) turboBoostsCallback,NULL);

    /* Guns mode */
    ac = 0;
    XtSetArg(arg[ac],XmNtopAttachment,XmATTACH_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNtopWidget,turboW); ++ac;
    XtSetArg(arg[ac],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); ++ac;
    XtSetArg(arg[ac],XmNleftWidget,courseW); ++ac;
    if (current_course->guns_mode == GUNS_NONE)
	cptr = guns_modes[0];
    else if (current_course->guns_mode == GUNS_LIMITED)
	cptr = guns_modes[1];
    else if (current_course->guns_mode == GUNS_UNLIMITED)
	cptr = guns_modes[2];
    gunsModeW = createPulldown(formW,"Guns Mode: ",
	NUM_GUNS_MODES, guns_modes,
	(XtCallbackProc) gunsModeCallback,
	cptr, gunsBtnW, arg,ac);
    XtManageChild(gunsModeW);

    /* Ammo */
    sprintf(str,"%d",current_course->ammo);
    ammoW = createEditWidget(formW,gunsModeW,courseW,
	"Ammo:   ",str,
	(XtCallbackProc) ammoCallback,NULL);

    /************************************************************************/

    state_clock_tick();

    XtRealizeWidget(app_shellW);
    XFlush(display);
    process_X_events();

    XmFontListFree(bigFL);
}


#ifdef TEST_CODE
int state,loop_timer,ignore_input;
void update_all_clients_state(int state) {}
void shutdown_server(void) {}

main(
    int argc, char *argv[])
{
    open_server_interface(argc,argv);

    while (1) process_X_events();
}
#endif /* TEST_CODE */

#endif /* !WIN32 */
