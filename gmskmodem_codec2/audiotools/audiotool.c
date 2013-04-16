/* audio capture and playback */

// received C2encap frames from a UDP socket
// decodes and plays it out

// captures audio, if key is pressed, encode and send out as c2encap frame
// in UDP socket if PTT is switched on (keyin or GPIO)

// version 20121222: initial release
// version 20130404: added half duplex support
// version 20130415: minor bugfixes

/*
 *      Copyright (C) 2013 by Kristoff Bonne, ON1ARF
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */




//////////////////// CONFIGURATION: CHANGE BELOW 
#define MINBUFF 5
#define NUMBUFF 30

// change to 1 for strict c2encap streaming protocol checking
#define C2ENCAP_STRICT 0

//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING

#define FOREVER 1


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

// for usleep
#include <unistd.h>

// for networking, getaddrinfo and related
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// defines and structures for codec2-encapsulation
#include <c2encap.h>


// for open and read
#include <sys/stat.h>
#include <fcntl.h>

// global data structure//
typedef struct {
	// devicename + "exact" are global for receive and transmit
	char * devicename; 
	int exact;

	// network data for playback (UDP receive)
	int p_udpport;

	// network data for capture (UDP send)	
	char * c_ipaddr;
	int c_udpport;

	// ringbuffer for capture
	int c_ptr_write;
	int c_ptr_read; 
	int16_t *c_ringbuffer[NUMBUFF];


	// ringbuffer for playback
	int p_ptr_write;
	int p_ptr_read;
	int16_t *p_ringbuffer[NUMBUFF];

	// portaudio parameters for input and output
	PaStreamParameters outputParameters, inputParameters;
	int halfduplex;

	// CLI parameters
	int ipv4only;
	int ipv6only;
	int force_stereo_capt;
	int force_stereo_play;

	// counting number of time the callback functions is called
	int callbackcount;

	// network_send thread is ready (init done)
	int networksendready;

	// PTT
	int transmit;
	int ptt; // how is user ptt read? -2 = not set, -1 = keyboard , >= 0 = gpio port
	int pttinvert;

	// other stuff
	int verbose;
} globaldatastr;

// global data itself //
globaldatastr global;


// all functions

// 8k/48k audio conversion
#include "at_audio_convert_filter.h"
#include "at_audio_convert.h"

// audio callback, contains both callback for capture and send
#include "at_audio_callback.h"
// portaudio device management
#include "at_audio_devs.h"

// network receive and send
#include "at_net_receive.h"
#include "at_net_send.h"

// PTT switching: GPIO or KEYin
#include "at_ptt_gpio.h"
#include "at_ptt_keyin.h"



// set "c2encap strict" define to 0 if not defined
#ifndef C2ENCAP_STRICT
	#define C2ENCAP_STRICT 0
#endif

/////////////////////////////
// MAIN APPLICATION
//////////////
int main (int argc, char ** argv) {
// counting number of time the callback function is called per second
int callback_errorcount, callback_thiscount, callback_lastcount;

// other vars
int numBytes;
int loop;

PaStream * stream;
PaError pa_ret;

// "audio out" posix thread
pthread_t thr_net_receive, thr_net_send, thr_ptt;

// portaudio callback function, depends if full or half duplex
int (callback_funct(const void *, void *, long unsigned, const struct PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void*));


// init data
global.inputParameters.channelCount=-1;
global.outputParameters.channelCount=-1;

global.callbackcount=0;
 
global.c_ptr_write=1;
global.c_ptr_read=0;

global.p_ptr_write=1;
global.p_ptr_read=0;


// init some vars
global.ipv4only=0;
global.ipv6only=0;
global.force_stereo_capt=0;
global.force_stereo_play=0;
global.halfduplex=0;
global.c_ipaddr=NULL;
global.c_udpport=-1;
global.p_udpport=-1;
global.devicename=NULL;
global.exact=0;
global.ptt=-2; // -2 = not-set
global.pttinvert=0;
global.verbose=0;

for (loop=1; loop < argc; loop++) {
	// part 1: options
	if (strcmp(argv[loop],"-v") == 0) {
		global.verbose++;
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-4") == 0) {
		global.ipv4only=1;
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-6") == 0) {
		global.ipv6only=1;
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-forcestereo") == 0) {
		if (loop+1 < argc) {
			loop++;
			if ((strcmp(argv[loop],"c") == 0) || (strcmp(argv[loop],"C") == 0)) {
				global.force_stereo_play=0; global.force_stereo_capt=1;
			} else if ((strcmp(argv[loop],"p") == 0) || (strcmp(argv[loop],"P") == 0)) {
				global.force_stereo_play=1; global.force_stereo_capt=0;
			} else if ((strcmp(argv[loop],"b") == 0) || (strcmp(argv[loop],"B") == 0)) {
				global.force_stereo_play=1; global.force_stereo_capt=1;
			}; // end elsif - elsif - if
		}; // end if
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-halfduplex") == 0) {
		global.halfduplex=1;
	} else if (strcmp(argv[loop],"-pttinvert") == 0) {
		global.pttinvert=1;
	} else if (strcmp(argv[loop],"-key") == 0) {
		// user PTT is read via pressing enter
		if (global.ptt >= 0) {
			// gpio value is already set to a "gpio" (>= 0) value
			fprintf(stderr,"Error: PTT-reading \"key\" and \"gpio\" are mutually exclusive\n");
			exit(-1);
		}; // end if
		global.ptt=-1; // ptt=-1 -> PTT switching done via keyboard
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-gpio") == 0) {
		// user PTT is read via GPIO
		if (global.ptt == -1) {
			// gpio value is already set to a "key" (-1) value
			fprintf(stderr,"Error: PTT-reading \"gpio\" and \"key\" are mutually exclusive\n");
			exit(-1);
		}; // end if

		if (loop+1 < argc) {
			loop++;
			global.ptt=atoi(argv[loop]); // ptt=gpio port number
		}; // end if
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-adevice") == 0) {
		if (loop+1 < argc) {
			loop++;
			global.devicename=argv[loop];
		}; // end if
		continue;
	} else if (strcmp(argv[loop],"-exact") == 0) {
		global.exact=1;
		continue;
	} else if (strcmp(argv[loop],"-udp") == 0) {
		if (loop+3 < argc) {
			loop++; global.c_ipaddr=argv[loop]; 
			loop++; global.c_udpport=atoi(argv[loop]);
			loop++; global.p_udpport=atoi(argv[loop]);
		}; // end if
		continue;
	}; // end elsif - ... - if (options)
}; // end for

// We need at least 2 arguments: destination-ip destination-udpport listening-udpport
if (argc < 2) {
	fprintf(stderr,"Error: at least 3 arguments needed. \n");
	fprintf(stderr,"Usage: %s [ options ] [-key | -gpio <gpioport>] -adevice <audiodevice> [-exact] -udp <destination ip-address> <destination udp port> <local udp port> \n",argv[0]);
	fprintf(stderr,"Note: use device \"\" to get list of devices.\n");
	fprintf(stderr,"Options: \n");
	fprintf(stderr,"           -4: ipv4 hostlookup only\n");
	fprintf(stderr,"           -6: ipv6 hostlookup only\n");
	fprintf(stderr,"\n");
	fprintf(stderr," -forcestereo: Force stereo audio ('c' = capture, 'p' = playback, 'b' = both)\n");
	fprintf(stderr," -halfduplex : Audio playback/capture is half duplex\n");
	fprintf(stderr,"\n");
	fprintf(stderr," -pttinvert  : ptt invert (only used for gpio user-PTT input)\n");
	fprintf(stderr,"\n");
	fprintf(stderr," -v          : increase verboselevel\n");
	fprintf(stderr,"\n");
	exit(-1);
}; // end if


// check network ports
if ((global.c_udpport < 0) || (global.c_udpport > 65535)) {
	fprintf(stderr,"Error: Listening UDPport number must be between 0 and 65535! \n");
	exit(-1);
}; // end if

if ((global.p_udpport < 0) || (global.p_udpport > 65535)) {
	fprintf(stderr,"Error: Remote UDPport number must be between 0 and 65535! \n");
	exit(-1);
}; // end if


if ((global.ipv4only) && (global.ipv6only)) {
	fprintf(stderr,"Error: ipv4only and ipv6only are mutually exclusive! \n");
	exit(-1);
}; // end if


// if PTT keyin and gpio is not set, 
if (global.ptt == -2){
	fprintf(stderr,"Error: no user PTT switching selected.\n");
	exit(-1);
}; // end if

/// INIT AUDIO
printf("INITIALISING PORTAUDIO (this can take some time, please ignore any errors below) ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
printf(".... DONE\n");
fflush(stdout);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (global.devicename == NULL) {
	checkaudiodevice_default(&global);
} else {
	// devicename given, try to find/list device with matching name
	checkaudiodevice_find(&global);
}; // end if



// set other parameters of Parameters structure
global.outputParameters.suggestedLatency = Pa_GetDeviceInfo(global.outputParameters.device)->defaultHighOutputLatency;
global.inputParameters.suggestedLatency = Pa_GetDeviceInfo(global.inputParameters.device)->defaultHighInputLatency;


// allocate memory for ringbuffers
// PART 1: audio capture
if (global.inputParameters.channelCount == 1) {
	numBytes=3840; // 40 ms @ 48000 Ksamples/sec, 2 octets / sample, MONO
} else {
	numBytes=7680; // 40 ms @ 48000 Ksamples/sec, 2 octets / sample, STEREO
}; // end if

for (loop=0; loop < NUMBUFF; loop++) {
	// create memory for audiobuffer
	global.c_ringbuffer[loop] = malloc(numBytes); // allow memory for buffer 0
	if (!global.c_ringbuffer[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for ringbuffer %d for capture!\n",loop);
		exit(-1);
	}; // end if
}; // end for


// PART 2: audio playback
if (global.outputParameters.channelCount == 1) {
	numBytes=3840; // 40 ms @ 48000 Ksamples/sec, 2 octets / sample, MONO
} else {
	numBytes=7680; // 40 ms @ 48000 Ksamples/sec, 2 octets / sample, STEREO
}; // end if

for (loop=0; loop < NUMBUFF; loop++) {
	// create memory for audiobuffer
	global.p_ringbuffer[loop] = malloc(numBytes); // allow memory for buffer 0
	if (!global.p_ringbuffer[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for ringbuffer %d for playback!\n",loop);
		exit(-1);
	}; // end if
	memset(global.p_ringbuffer[loop],0,numBytes);
}; // end for



// start thread network receiver
pthread_create (&thr_net_receive, NULL, funct_net_receive, (void *) &global);


// start thread network sender
global.networksendready=0;
pthread_create (&thr_net_send, NULL, funct_net_send, (void *) &global);

// the "network sender" thread needs to do some initialisation which can
// take some time (e.g. DNS lookups).
// so wait here for that thread to finish initialisation

// variable "loop" is used to detect how long the network_send initialisation takes
loop=0;
while (!global.networksendready) {
	loop++;

	// wait 100 ms
	usleep(10000);

	// quit if network_send initialisation takes more then 20 seconds
	if (loop > 200) {
		fprintf(stderr,"Error: network_send inialisation takes more then 20 seconds. Should not happen. Exiting!\n");
		exit(-1);
	}; // end if
}; // end while


// start user-PTT reading thread
if (global.ptt == -1) {
	// PTT = -1: user-ptt is read from keyboard
	pthread_create (&thr_ptt, NULL, funct_ptt_keyin, (void *) &global);
} else {
	// PTT >= 0: user-ptt is read via gpio
	pthread_create (&thr_ptt, NULL, funct_ptt_gpio, (void *) &global);
}; // end else - if


// init vars
callback_lastcount=0; 
callback_errorcount=0;


while (FOREVER) {
	// open PortAudio stream
	PaStreamParameters * pa_in, * pa_out;	


	if (global.halfduplex==0) {
		pa_in=&global.inputParameters;
		pa_out=&global.outputParameters;
	} else if (global.transmit==0) {
		// sending, make stream output
		pa_in=NULL;
		pa_out=&global.outputParameters;
	} else {
		pa_in=&global.inputParameters;
		pa_out=NULL;
	}; // end if

	// open stream
	if (global.halfduplex) {
		pa_ret = Pa_OpenStream (
			&stream,
			pa_in,
			pa_out,
			48000, // sample rate
			1920, // frames per buffer: (48000 samples / sec @ 40 ms)
			paClipOff, // we won't output out of range samples,
							// so don't bother clipping them
			funct_callback_hd, // half duplex
			&global // parameters passed to callback function
		);
	} else {
		pa_ret = Pa_OpenStream (
			&stream,
			pa_in,
			pa_out,
			48000, // sample rate
			1920, // frames per buffer: (48000 samples / sec @ 40 ms)
			paClipOff, // we won't output out of range samples,
							// so don't bother clipping them
			funct_callback_fd, // full duplex
			&global // parameters passed to callback function
		);
	}; // end - else - if

	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_OpenStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

	// Start stream
	pa_ret=Pa_StartStream(stream);

	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_StartStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if


	while (( pa_ret = Pa_IsStreamActive (stream)) == 1) {
		// endless sleep, wake up and check status every 0.2 seconds
		Pa_Sleep(200);

		callback_thiscount=global.callbackcount;

		// we should get 5 audio-frames per second (40 ms / frame). If we receive less then 2 or more then 8, there is clearly something wrong
		if ((callback_thiscount-callback_lastcount < 2) || (callback_thiscount-callback_lastcount > 8)) {
			callback_errorcount++;

			// only print out error message as of second event
			// (to avoid messages during PTT switches)
			if (callback_errorcount >= 2) {
				fprintf(stderr,"Warning: callback function called %d times last second. Should have been 5 times!\n",callback_thiscount-callback_lastcount);
			}; // end if

			// break out if error during more then 20 seconds
			if (callback_errorcount > 20) {
				fprintf(stderr,"Error: callback function called to few for more then 20 seconds. Exiting!\n");
				break;
			}; // end if
		} else {
			// reset error counter
			callback_errorcount=0;
		}; // end if

		callback_lastcount=callback_thiscount;
	}; // end while

	// break out if to many errors
	if (callback_errorcount > 20) {
		break;
	}; // end if


	// did "Pa_isStreamActive" give errors?
	if (pa_ret < 0) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_isStreamActive: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

	// close stream
	pa_ret = Pa_CloseStream (stream);
	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

}; // end while (endless loop)

// dropped out of endless loop. Should not happen


pa_ret=Pa_CloseStream(stream);
if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// Done!!!

Pa_Terminate();


// if "key" PTT switching, re-enable echo
if (global.ptt == -1) {
	// reenable echo on local console

	struct termios tio;
	// get termiod struct for stdin
	tcgetattr(0,&tio);
	// disable echo
	tio.c_lflag |= (ECHO);
	// write change to console
	tcsetattr(0,TCSANOW,&tio);
}

exit(-1);
}; // end function funct_audioin


