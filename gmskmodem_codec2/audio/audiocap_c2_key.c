/* audio in */


//#define RATE 48000
//#define PERIOD 960

#define RATE 8000
#define PERIOD 160

#include <alsa/asoundlib.h>

// posix threads and inter-process control
#include <pthread.h>
#include <signal.h>

// termio to disable echo on local console
#include <termios.h>

// codec2 libraries
#include <codec2.h>

// for memcpy
#include <string.h>

#define TRUE 1

// global data structure//
typedef struct {
	snd_pcm_t *alsahandle;
	int stereo;
	int thisframes;
	int16_t * audiobuffer;
	int transmit; // are we transmitting ???
} globaldatastr;

// global data itself //
globaldatastr global;

// defines and structures for codec2-encapsulation
#include "c2encap.h"

// functions defined below
void * funct_audioin (void *) ;


int main (int argc, char ** argv) {
int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;

char defaultcapturedevice[]="hw:CARD=Intel,DEV=0";
char *capturedevice;
int numchannels;
unsigned int val;
int dir;
int stop;
unsigned int thisrate;

// structure for raw codec2 data
c2encap c2_begin, c2_end;

pthread_t thr_audioin;

// terminal IO control
struct termios tio;

// init data
global.stereo=1;
global.transmit=0;

// if 1st argument exists, use it as capture device
if (argc >= 2) {
	capturedevice = argv[1];
} else {
	// no argument given; use default
	capturedevice = defaultcapturedevice;
}; //

// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

// open PCM device for capture 
ret=snd_pcm_open(&global.alsahandle, capturedevice, SND_PCM_STREAM_CAPTURE, 0);

if (ret < 0) {
	fprintf(stderr,"Error: unable to open alsa capture device: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if


// Allocate a hardware parameters object
snd_pcm_hw_params_alloca(&params);

// fill in default values
ret=snd_pcm_hw_params_any(global.alsahandle, params);
if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting default values: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if

// set the desired parameters
ret=snd_pcm_hw_params_set_access(global.alsahandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting access interleaved: %s \n",snd_strerror(ret));
	exit(-1);
}; // end if

// signed 16-bit little endian
ret=snd_pcm_hw_params_set_format(global.alsahandle, params, SND_PCM_FORMAT_S16_LE);

if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting format 16 bit signed: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if

// number of channels 

if (global.stereo) {
	numchannels=2;
} else {
	numchannels=1;
}; // end if

ret=snd_pcm_hw_params_set_channels(global.alsahandle, params, numchannels);

if (ret < 0) {
	if (global.stereo) {
		fprintf(stderr,"ALSA capture: Error setting number of channels to %d: %s\n",numchannels, snd_strerror(ret));
		exit(-1);
	} else {
		// mono failed, try again in stereo;
		ret=snd_pcm_hw_params_set_channels(global.alsahandle, params, 2);

		if (ret < 0) {
			fprintf(stderr,"ALSA capture: Error setting number of channels. Tried both mono and sterero %s\n",snd_strerror(ret));
			exit(-1);
		} else {
			fprintf(stderr,"ALSA capture: setting mono capture failed. Switched to stereo. Audio is exptected on left channel.\n");
			global.stereo=1;
		}; // end else - if

	}; // end else - if
}; // end of

/* 48 Khz sampling rate */
val=RATE;
snd_pcm_hw_params_set_rate_near(global.alsahandle, params, &val, &dir);

// get actual rate
snd_pcm_hw_params_get_rate(params, &thisrate, &dir);

if (thisrate != RATE) {
	fprintf(stderr,"ALSA-CAPTURE: warning samplerate could not be set: wanted %d, got %d \n",RATE,(int) thisrate);
}; // end if

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = PERIOD;
snd_pcm_hw_params_set_period_size_near(global.alsahandle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);

global.thisframes = frames;


if (frames != PERIOD) {
	fprintf(stderr,"ALSA-CAPTURE: warningperiode could not be set: wanted %d, got %d \n",PERIOD,(int) global.thisframes);
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(global.alsahandle, params);
if (ret < 0) {
	fprintf(stderr,"ALSA-CAPTURE: unable to set hw parameters: %s\n", snd_strerror(ret));
	return(-1);
}


// start thread to read audio data

pthread_create (&thr_audioin, NULL, funct_audioin, (void *) &global);


// toggle "transmit" flag based on enter begin pushed

// first disable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag &= (~ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);


fprintf(stderr,"AUDIO CAPTURE, ENCODE AND OUTPUT\n");
fprintf(stderr,"Alsa device: %s\n",capturedevice);
fprintf(stderr,"press <enter> to switch on or off\n");
fprintf(stderr,"press \"x\" to stop\n");
fprintf(stderr,"\n");

fprintf(stderr,"audio is OFF! \n");

stop=0;
while (!stop) {
	int c;

	c=getchar();

	// is it lf?
	if (c == 0x0a) {
		if (global.transmit) {
			// ON to OFF

			fprintf(stderr,"audio is OFF! \n");
			global.transmit=0;

			// send "end" marker
			fwrite((void *)&c2_end,C2ENCAP_SIZE_MARK,1,stdout);
			fflush(stdout);
		} else {
			// send "start" marker
			fwrite((void *)&c2_begin,C2ENCAP_SIZE_MARK,1,stdout);
			fflush(stdout);

			// OFF to ON
			fprintf(stderr,"audio is ON! \n");
			global.transmit=1;
		}; // end if

	} else if ((c == 'x') || (c == 'X')) {
		fprintf(stderr,"STOP! \n");
		stop=1;
	}; // end else - if

}; // end while

// reenable ECHO

// reenable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag |= (ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);

// terminate program
exit(0);
}; // end main program

//////////////////////////////
//////////////////////////////
//  function audioin

void * funct_audioin (void * globalin) {
globaldatastr * pglobal;
int framesleft;
int16_t *p;
int framesreceived;
int buffersize;

// vars for codec2
void *codec2;
unsigned char *c2_buff;
int mode, nsam, nbit, nbyte;
int16_t * audiobuffer;

// structure for raw codec2 data
c2encap c2_voice;


pglobal = (globaldatastr *) globalin;

// init c2encap structures
memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;


// if stereo, actual buffersize send is half (only left channel)
//if (pglobal->stereo) {
//	thisbuffersize=pglobal->buffersize >> 1;
//} else {
//	thisbuffersize=pglobal->buffersize;
//}; // end else - if

// init codec2
mode = CODEC2_MODE_1400;
codec2 = codec2_create (mode);
nsam = codec2_samples_per_frame(codec2);
nbit = codec2_bits_per_frame(codec2);

nbyte = (nbit + 7) >> 3; // ">>3" is same as "/8"

if (nbyte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nbyte);
}; // end if

c2_buff = (unsigned char *)&c2_voice.c2data.c2data_data7;



// allocate audiobuffer
if (global.stereo) {
	buffersize=nsam<<2; // = * 4 (stereo and 16 bit/sample)
} else {
	// mono
	buffersize=nsam<<1; // = * 2 (16 bit/sample)
}; // end else - if
audiobuffer=malloc(buffersize);

if (!audiobuffer) {
	fprintf(stderr,"Error: malloc audiobuffer: %s",strerror(errno));
	exit(-1);
}; // end if


while (TRUE) {
// get audio
	framesleft=nsam;

	p=audiobuffer;

	while (framesleft > 0) {
		// grab audio data
		if (framesleft > pglobal->thisframes) {
			// capture is limited to "number of frames" given by alsa
			framesreceived = snd_pcm_readi(pglobal->alsahandle, p, pglobal->thisframes);
		} else {
			framesreceived = snd_pcm_readi(pglobal->alsahandle, p, framesleft);
		}; // end else - if


		if (framesreceived == -EAGAIN) {
			// receiving EAGAIN mains no data
			usleep(250);
		} else {
			if (framesreceived < framesleft) {
				if (pglobal->stereo) {
					// stereo: move up 2 "int16_t" slots per sample
					p += (framesreceived<<1);
				} else {
					p += framesreceived;
				}; // end else - if
			}; // end if
			framesleft -= framesreceived;
		}; // end if

	// keep on reading audio untill sufficient data read
	}; // end while


	// do we send out data ???

	if (pglobal->transmit) {
		// if stereo, only use left channel
		if (pglobal->stereo) {
			int loop;

			int16_t *p1, *p2;

			// start at 2th element (in stereo format); which is 3th (in mono format)
			p1=&audiobuffer[1];
			p2=&audiobuffer[2];
	
			for (loop=1; loop < nsam; loop++) {
				*p1=*p2;
				p1++; p2 += 2;
			}; // end for
		}; // end if

		// do codec2 encoding
		codec2_encode(codec2, c2_buff, audiobuffer);

		fwrite((void *)&c2_voice,C2ENCAP_SIZE_VOICE1400,1,stdout);
		fflush(stdout);
	}; // end if
}; // end while

}; // end function audioin
