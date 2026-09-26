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


/* sound.c */
#if defined(HAVE_CONFIG_H)
# include "config.h"
#elif defined(_HPUX_SOURCE)
# define USE_RTA 1
# define HAVE_SYS_AUDIO_H
# undef  HAVE_SYS_SOUNDCARD_H
#endif


#ifdef USE_RTA /* [ */

#include <sys/types.h>
#if defined(HAVE_SYS_AUDIO_H)
# include <sys/audio.h>
#elif defined(HAVE_SYS_SOUNDCARD_H)
# include <sys/soundcard.h>
#else
# error "Cannot compile with RTA if no audio header file"
#endif
#include <sys/shm.h>

#include "global.h"
#include "camera.h"
#include "drive.h"
#include <rtaudio.h>

#pragma OPT_LEVEL 0


void stop_sound( int type);

int Mapping[100][2];	/*  Mapping for sounds */

static int sound_debug=1;

RtaConnection
    Handle;

RtaTag
    Props[64];

unsigned char 
    Noise[1024];

typedef struct _DriveSound {
   int sound_num;	/* RTA returned number for this sound */
   int sound_handle;	/* RTA returned handle of instance (for stop_sound) */
} DriveSound;

DriveSound DriveSounds[20];

void AudioErrorHandler( RtaConnection Handle, RtaErrorType ErrNum )
{
    cstate.use_sound = 0;
}

int init_sound( char *SoundsFile )
{
    FILE 
	*fp;
    char
	s1[80],
	input[80],
	Str[1024];
    RTT_U32
	NumSamples;
    RtaTagList
	SampProps = 0;
    void
	*Samples;
    RtaSound
	Sound;
    int
	indx, lineno, n,i,val;

    RtaInstallHandler( AudioErrorHandler );

    Handle = RtaOpenAudio( 0 );
    if( !Handle ) {
	printf("Could not initialize Real Time Audio! \n");
	cstate.use_sound = 0;
	return(-1);
    }

    cstate.use_sound = 1;

    /* Now, read in the audaemon.cnf file, and define the sounds */

    fp = fopen(SoundsFile, "r");
    if( !fp) {
	cstate.use_sound = 0;
	fclose(fp);
	return(-1);
    }

    fscanf(fp, "%s", input);
    if (strcmp(input, "audaemon_config")) {
        fprintf(stderr, "Invalid config file type: %s ", input);
	cstate.use_sound = 0;
	fclose(fp);
        return(-1);
    }

    /* First, suspend the rtad so we define our sounds quickly */
    n=0;
    n = RtaAddIntToList(RTA_TAG_MODE, RTA_MODE_SUSPEND, Props, n, 64);
    RtaPreferences( Handle, Props);

    lineno = 1;
    indx = SOUND_ENGINE;
    while( fgets(input, 80, fp) != NULL) {
	lineno++;
        if (input[0] != '#' && input[0] != '\n') { /* ignore comments */
	    if (sscanf(input, "%s", s1) == 1) {
		if(!(s1[0] == '#' || s1[0] == ';')) {
		   /* Define this sound */
		   NumSamples = RtuReadAuFile( s1, &SampProps, &Samples );
		    if( NumSamples ) {
			Sound = RtaDefineSound( Handle,
				SampProps, NumSamples, Samples );
			if( Sound != -1 ) {
			  DriveSounds[indx].sound_handle = 0;
			  DriveSounds[indx].sound_num = Sound;
			  indx++;
			  if(sound_debug) 
			    (void)printf("Use index %d for this sound\n",Sound);

			    if( SampProps)
				free( SampProps );
			}
			else {
			    (void)printf( "ERROR DEFINING SOUND\n" );
			}
		    }
		}
	    } else {
	       fprintf(stderr, "error in line %d of config file: %s", lineno, input);
	       return(1);
	    }
        }
    }


    /* Re-enable the rtad  */
    n=0;
    n = RtaAddIntToList(RTA_TAG_MODE, RTA_MODE_RESUME, Props, n, 64);
    RtaPreferences( Handle, Props);

    stop_sound(0);  /* Hack to make sure background sound works */

    /* Initial static buffer */
    for(i=0; i<1024; i++ ) {
	Noise[i] = rand() % 255;
    }

#if 0
    /* Start up voice activated recording */
    n=0;
    n = RtaAddIntToList(RTA_TAG_VOICE_ACTIVATED, 1, Props, n, 64);

    RtaBeginRecording(Handle, Props, NULL);
#endif

    /* Start up the background engine sound */
    n=0;
    n = RtaAddIntToList(RTA_TAG_SOUND, SOUND_ENGINE-1, Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_LVOL, (65535), Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_RVOL, (65535), Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_PRIORITY, 1, Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_FREQUENCY, (int)6000, Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_BACKGROUND_SOUND, 1, Props, n, 64);

    DriveSounds[SOUND_ENGINE].sound_handle = RtaPlaySound(Handle, Props);

}


void play_sound( int type )
{
    int n;

    n=0;
    n = RtaAddIntToList(RTA_TAG_SOUND, DriveSounds[type].sound_num,Props,n,64);
    n = RtaAddIntToList(RTA_TAG_LVOL, (65535), Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_RVOL, (65535), Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_PRIORITY, 1, Props, n, 64);

    DriveSounds[type].sound_handle = RtaPlaySound(Handle, Props);
}

void stop_sound( int type)
{
    int n;

    if( type == 0 ) {
	n = RtaAddIntToList(RTA_TAG_SOUND, 0, Props, n, 64);
	RtaStopSound(Handle, Props);
    }
    else {
	if(DriveSounds[type].sound_handle == 0 ) return;

	n=0;
	n = RtaAddIntToList(RTA_TAG_SOUND, DriveSounds[type].sound_handle, 
		Props, n, 64);
	RtaStopSound(Handle, Props);
	DriveSounds[type].sound_handle = 0;
    }

}

void update_rpm( float rpm)
{
    float freq;
    int n;


    /* Frequency is normally at 16khz.  Say normal engine rpm is 1000.
    ** To get the right sound, take 6k + (rpm - 1000) * 32000 / 7000
    */

    freq = 6000 + (rpm - 1000.0) * 32000 / 7000;
    n=0;
    n = RtaAddIntToList(RTA_TAG_HANDLE, DriveSounds[SOUND_ENGINE].sound_handle,
	    Props, n, 64);
    n = RtaAddIntToList(RTA_TAG_FREQUENCY, (int)(freq), Props, n, 64);
    n = RtaAddIntToList( RTA_TAG_PROP_MASK, RTA_PROP_FREQUENCY, Props, n, 16);
    RtaModifySound(Handle, Props);
}


void terminate_audio( void )
{
}



#else /* ] [ */

/* Just stub everything out */

void AudioErrorHandler( )
{
}

int init_sound( char *SoundsFile )
{
    return(0);
}


void play_sound( int type )
{
}

void stop_sound( int type)
{
}

void update_rpm( float rpm)
{
}

void terminate_audio( void )
{
}

#endif /* ] */
