/* audio capture */

// Reads audio from a portaudio audio device
// If the GPIO pin is pushed, the audio is codec2-encoded and send as a
// UDP stream to the gmskmodem


//////////////////// CONFIGURATION: CHANGE BELOW 
// set portaudio device
// If set to -1, use "default device" value returned by portaudio
// Pa_GetDefaultInputDevice() function
#define PORTAUDIODEVICE -1

//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING



#define RATE 8000
#define PERIOD 160

// portaudio
#include <portaudio.h>

// posix threads and inter-process control
#include <pthread.h>
#include <signal.h>

// termio to disable echo on local console
#include <termios.h>

// codec2 libraries
#include <codec2.h>

// for memcpy
#include <string.h>

// for int16_t and others
#include <stdint.h>

// for malloc, exit
#include <stdlib.h> 

// for (f)printf
#include <stdio.h>

// for errno
#include <errno.h>

#define TRUE 1

// global data structure//
typedef struct {
	int transmit; // are we transmitting ???
	int pa_devicenr; // only used to display information
} globaldatastr;

// global data itself //
globaldatastr global;

// defines and structures for codec2-encapsulation
#include "c2encap.h"

// functions defined below
void * funct_keypress (void *) ;


int main (int argc, char ** argv) {
int buffersize;
int stereo;
int numBytes;

// state of keypress
int state, oldstate;

// portaudio
int portaudiodevice;
PaStreamParameters inputParameters;
PaStream * stream;
PaError pa_ret;
const PaDeviceInfo *devinfo;


// vars for codec2
void *codec2;
unsigned char *c2_buff;
int mode, nsam, nbit, nbyte;
int16_t * audiobuffer;


// structure for c2encap data
c2encap c2_voice;
c2encap c2_begin, c2_end;

// "audio in" posix thread
pthread_t thr_keypress;

// init data
stereo=1;
global.transmit=0;

// if 1st argument exists, use it as capture device
if (argc >= 2) {
	portaudiodevice = atoi(argv[1]);
} else {
	// no argument given; use default
	portaudiodevice = PORTAUDIODEVICE;
}; //



// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;

// configure portaudio global data
if (stereo) {
	numBytes = 1280;
} else {
	numBytes = 640; 
}; // end if

// create memory for audiobuffer
audiobuffer = malloc(numBytes); // allow memory for buffer 0
if (!audiobuffer) {
	// memory could not be allocated
	fprintf(stderr,"Error: could not allocate memory for portaudio buffer 0!\n");
	exit(-1);
}; // end if

fprintf(stderr,"INITIALISING PORTAUDIO ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
fprintf(stderr,".... DONE\n");

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (portaudiodevice == -1) {
	// portaudio device = -1 -> use portaudio "get default input device"
	inputParameters.device = Pa_GetDefaultInputDevice();

	if (inputParameters.device == paNoDevice) {
		fprintf(stderr,"Error: no portaudio default input device!\n");
		exit(-1);
	}; // end if
} else {
	inputParameters.device = portaudiodevice;
}; // end else - if

global.pa_devicenr=inputParameters.device;

devinfo = Pa_GetDeviceInfo (inputParameters.device);
fprintf(stderr,"Audio device = %d (%s %s)\n",global.pa_devicenr,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);



// first try mono
inputParameters.channelCount = 1;

inputParameters.sampleFormat = paInt16;
inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
inputParameters.hostApiSpecificStreamInfo = NULL;


pa_ret = Pa_IsFormatSupported(&inputParameters,NULL, (double) RATE);
if (pa_ret == paFormatIsSupported) {
	stereo=0;

	fprintf(stderr,"8 KHz sampling rate supported in mono! \n");
} else {
	// try again in stereo
	inputParameters.channelCount = 2;

	pa_ret = Pa_IsFormatSupported(&inputParameters, NULL, (double) RATE);
	if (pa_ret == paFormatIsSupported) {
		stereo=1;
		fprintf(stderr,"8 KHz sampling rate supported in stereo! \n");
	} else {
		fprintf(stderr,"Error: 8 Khz sampling not supported in mono or stereo! Exiting. \n");
		exit(-1);
	}; // end if
}; // end 



// open PortAudio stream
// do not start stream yet, will be done further down
pa_ret = Pa_OpenStream (
	&stream,
	&inputParameters,
	NULL, // output Parameters, not used here 
	8000, // sample rate
	320, // frames per buffer: 40 ms @ 8000 samples/sec
	paClipOff, // we won't output out of range samples,
					// so don't bother clipping them
	NULL, // no callback function, syncronous read
	&global
);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_OpenStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if


// init codec2
mode = CODEC2_MODE_1400;
codec2 = codec2_create (mode);
nsam = codec2_samples_per_frame(codec2);
nbit = codec2_bits_per_frame(codec2);

nbyte = (nbit + 7) >> 3; // ">>3" is same as "/8"

if (nbyte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nbyte);
}; // end if

if (nsam != 320) {
	fprintf(stderr,"Error: number of samples for codec2 frames should be 320. We got %d \n",nsam);
}; // end if

c2_buff = (unsigned char *)&c2_voice.c2data.c2data_data7;


// allocate audiobuffer
if (stereo) {
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


// start thread to read detect keypress (used to switch transmitting)
pthread_create (&thr_keypress, NULL, funct_keypress, (void *) &global);


// Start stream
pa_ret=Pa_StartStream(stream);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_StartStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if


// init some vars;
oldstate=0;

while (( pa_ret = Pa_IsStreamActive (stream)) == 1) {
// get audio

	pa_ret = Pa_ReadStream(stream, audiobuffer, 320);

	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_ReadStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

	// get state from subthread
	state=global.transmit;

	if (state) {
		// State = 1: write audio

		// first check old state, if we go from oldstate=0 to state=1, this is
		// the beginning of a new stream; so send start packe
		if (oldstate == 0) {
			// start "start" marker
			fwrite((void *) &c2_begin,C2ENCAP_SIZE_MARK,1,stdout);
			fflush(stdout);
			fprintf(stderr,"B");
		}

		// if stereo, only use left channel
		if (stereo) {
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
		fprintf(stderr,"T");
	} else {
		// state = 0, do not send
		// however, if we go from "oldstate = 1 - > state = 0", this is
		// the end of a stream

		if (oldstate) {
			// send "end" marker
			fwrite((void *)&c2_end,C2ENCAP_SIZE_MARK,1,stdout);
			fflush(stdout);
			fprintf(stderr,"E");
		}; // end if 
	}; // end else - if

	oldstate=state;

	
}; // end while
// dropped out of endless loop. Should not happen

if (pa_ret < 0) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_isStreamActive: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

fprintf(stderr,"Error: audiocap dropped out of audiocapturing loop. Should not happen!\n");

pa_ret=Pa_CloseStream(stream);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// Done!!!

Pa_Terminate();

exit(0);


}; // end main applicion

// FUNCTIONS

// function "keypress"
// detects <enter> to switch "on" or "off" status
// started as a subthread
void * funct_keypress (void * globalin) {
globaldatastr * p_global;
p_global=(globaldatastr *) globalin;

// local vars
struct termios tio;
int stop;


// toggle "transmit" flag based on enter begin pushed

// first disable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag &= (~ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);


fprintf(stderr,"AUDIO CAPTURE, ENCODE AND OUTPUT\n");
fprintf(stderr,"audio device = %d\n",p_global->pa_devicenr);
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
		if (p_global->transmit) {
			// ON to OFF

			fprintf(stderr,"audio is OFF! \n");
			p_global->transmit=0;

		} else {
			// OFF to ON
			fprintf(stderr,"audio is ON! \n");
			p_global->transmit=1;
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

// exit application
global.transmit=-1;
exit(-1);

}; // end function funct_audioin
