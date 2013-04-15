/* gmskmodem */


// GMSKmodem, API / PORTAUDIO / ONLY-UDP version


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


// Release information
// version 20130326 initial release
// version 20130404: added halfduplex support



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


// needed for S_IRUSR, S_IWUSR, S_IRGRP and S_IWGRP
#include <sys/stat.h>

// for timezone and timeval (needed for random)
#include <sys/time.h>

// POSIX threads + inter-process control
#include <pthread.h>
#include <signal.h>

// posix interrupt timers
#include <time.h>

// needed for usleep
#include <unistd.h>


// for memset
#include <strings.h>

// for Error Number
#include <errno.h>

// IP networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>


// ioctl for CS driving serial port (PTT)
#include <sys/ioctl.h>

// for serial port out (PTT)
#include <termios.h>

// portaudio libraries
#include <portaudio.h>

// for open (file)
#include "fcntl.h"

// c2gmsk API
#include <c2gmsk.h>

// codec2 encapsulation
#include <c2encap.h>

// global data structure
#include "global.h"


// Portaudio functions:
#include "paudio_devs.h" // finding and opening devices
#include "paudio_callback.h" // callback function

// parse CLI options
#include "parsecliopts.h"

// support functions for receiver and sender
#include "audioin_demod_netout.h"


// convert 8k to 48 k (used for idfiles)
#include "at_audio_convert_filter.h"
#include "at_audio_convert.h"


// suport functions for sender
#include "buffer.h"
#include "netin_mod_audioout.h"
#include "ptt.h"



/////////////////////////////////////////
/////////////////////////////////////////
//  MAIN APPLICATION STARTS HERE !!!  ///
/////////////////////////////////////////
/////////////////////////////////////////

int main (int argc, char ** argv) {

struct globaldata global;

char retmsg[PARSECLIRETMSGSIZE];
int retval;

int loop;

// local vars for portaudio
PaError pa_ret;
PaStream * stream;

// local vars to check on number of callback calls per second
int callback_thiscount, callback_lastcount, callback_errorcount;

// local vars for c2gmsk API
struct c2gmsk_msgchain * chain=NULL,  ** pchain; // pointer to pointer of message chain
struct c2gmsk_param param;
struct c2gmsk_msg * msg;
int c2gmsk_msg_data[2];
int supported;
int tod;
int datasize;

// threads
pthread_t thr_sender, thr_receiver, thr_ptt;



///////////////////////////////////
// APPLICATION STARTS HERE ////////
///////////////////////////////////


// INIT ALL VARS vars

// RECEIVER
global.dumpstream=0;
global.dumpaverage=0;
global.udpout_port=0;
global.udpout_host=NULL;
global.ipv4only=0;
global.ipv6only=0;
global.forcestereo_r=0;
global.audioinvert_s=0; // set when receiving stream


// SENDER
global.pttlockfile=NULL;
global.pttcsdevice=NULL;
global.ptttxdevice=NULL;
global.ptt_invert=0;
global.ptt_gpiopin=-1;
global.silencebegin=DEFAULT_SILENCEBEGIN;
global.silenceend=DEFAULT_SILENCEEND;
global.udpin_port=0;
global.idfile=NULL;
global.idfreq=0;
global.forcestereo_s=0;
global.audioinvert_r=0; // set by cli
global.networksendready=0;
global.receivefromnet=0;
global.transmitptt=0; // set to trigger ptt

// BOTH FOR RECEIVER AND SENDER
global.verboselevel=0;
global.adevice=0;
global.exact=0;
global.c2gmsksessid=NULL;
global.halfduplex=0;

// pointers in ringbuffer
global.pntr_r_write=1;
global.pntr_r_read=0;
global.pntr_s_write=1;
global.pntr_s_read=0;



// init messagechain
pchain=&chain;

/////////////////////////////
// Command line parameters parsing
//

// parse CLI options, store info in global data
// returns 0 if all OK; -1 on error, 1 on warning
retval=parsecliopts(argc,argv,retmsg,&global);

if (retval < 0) {
// -1: error
	fprintf(stderr,"%s",retmsg);
	exit(-1);
} else if (retval > 0) {
// +1: warning
	fprintf(stderr,"%s",retmsg);
}; // end elsif - if


// allocate memory for ringbuffers
// part 1: audio capture

for (loop=0; loop < NUMBUFF; loop++) {
	// create memory for audiobuffer
	global.ringbuffer_s[loop] = malloc(3840); // 40 ms @ 48000 samples / sec, 2 octets / samples, mono
	if (!global.ringbuffer_s[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for ringbuffer %d for capture!\n",loop);
		exit(-1);
	}; // end if

	global.ringbuffer_r[loop] = malloc(3840); // 40 ms @ 48000 samples / sec, 2 octets / samples, mono
	if (!global.ringbuffer_r[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for ringbuffer %d for playback!\n",loop);
		exit(-1);
	}; // end if

	// clear all the buffers
	memset(global.ringbuffer_r[loop],0,3840);
	memset(global.ringbuffer_r[loop],0,3840);
}; // end for



// initialise c2gmsk API

// create parameters structure
retval=c2gmsk_param_init(&param);
if (retval != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: Could not initialise parameter structures! \n");
	exit(-1);
}; // end if

// fill in parameters
param.expected_apiversion=0;
param.d_disableaudiolevelcheck=1;
// bitrate 4800, version 0
param.m_version=0;
param.m_bitrate=C2GMSK_MODEMBITRATE_4800;

// create gmsk session
global.c2gmsksessid=c2gmsk_sess_new(&param,&retval, pchain);
if (retval != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",retval);
	exit(-1);
}; // end if

// c2gmsk_sess_new returns list of supported versions/modes
chain=*pchain;

// loop through all message and check if the API supports version 1 / 4800 bps
supported=0;


tod=C2GMSK_MSG_CAPABILITIES;
while ((msg = c2gmsk_msgchain_search (C2GMSK_SEARCH_POSCURRENT, chain, &tod, &datasize, NULL))) {
	retval=c2gmsk_msgdecode_numeric (msg, c2gmsk_msg_data);

	// "retval" returns number of numeric elements:
		// C2GMSK_MSG_CAPABILITIES contain at least 2 data elements
	if (retval == 2) {
	// OK, 2 elements received
	// data[0] = versionid
	// data[1] = bitrate

		if ((c2gmsk_msg_data[1] == C2GMSK_MODEMBITRATE_4800) && (c2gmsk_msg_data[0] == 0)) {
			supported=1;
		}; // end else - if
	}; 
}; // end while

if (!supported) {
	fprintf(stderr,"Error: c2gmsk mode 0 @ 4800 bps not supported by the c2gmsk API.\n");
	exit(-1);	
}; // end if



// initialising portaudio + find correct PA device
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

// devicename given, try to find/list device with matching name
checkaudiodevice_find(&global);

// at this point, we know we have a valid portaudio device
// set remaining parameters of Portaudio structure
global.outputParameters.suggestedLatency = Pa_GetDeviceInfo(global.outputParameters.device)->defaultLowOutputLatency;
global.inputParameters.suggestedLatency = Pa_GetDeviceInfo(global.inputParameters.device)->defaultLowInputLatency;


// SENDER INPUT
// start thread to receive network UDP packet, do gmsk modulation and put audio on queue
pthread_create(&thr_sender, NULL, funct_netin_modulate_audioout, (void *) &global); 

// start "PTT" thread.
if (global.pttcsdevice) {
	pthread_create(&thr_ptt, NULL, funct_pttcs, (void *) &global);
} else if (global.ptttxdevice) {
	pthread_create(&thr_ptt, NULL, funct_ptttx, (void *) &global);
} else if (global.pttlockfile) {
	pthread_create(&thr_ptt, NULL, funct_pttlockfile, (void *) &global);
} else if (global.ptt_gpiopin >= 0) {
	pthread_create(&thr_ptt, NULL, funct_pttgpio, (void *) &global);
}; // end elsif - elsif - if


// RECEIVER INPUT
// start thread GMSK demodulator
global.networksendready=0;
pthread_create (&thr_receiver, NULL, funct_audioin_demod_proc_netwsend, (void *) &global);

// the "network sender" thread needs some time for initialisation so wait here for that
// thread to finish initialisation

// variable "loop" is used to detect how long the network_send initialisation takes
loop=0;
while (!global.networksendready) {
	loop++;

	// wait 100 ms
	usleep(100000);

	// quit if network_send initialisation takes more then 20 seconds
	if (loop > 200) {
		fprintf(stderr,"Error: network_send inialisation takes more then 20 seconds. Should not happen. Exiting!\n");
		exit(-1);
	}; // end if
}; // end while


/// ENDLESS LOOP
// init some vars
callback_errorcount=0;
callback_lastcount=0; 

while (FOREVER) {
	PaStreamParameters * pa_in, *pa_out;


	if (global.halfduplex == 0) {
		// full duplex
		pa_in=&global.inputParameters;
		pa_out=&global.outputParameters;
	} else if (global.receivefromnet) {
		// transmitting -> half duplex playback
		pa_in=NULL;
		pa_out=&global.outputParameters;
	} else {
		// not transmitting -> half duplex capture
		pa_in=&global.inputParameters;
		pa_out=NULL;
	}; // end if

	// open PortAudio stream
	if (global.halfduplex) {
		pa_ret = Pa_OpenStream (
			&stream,
			pa_in,
			pa_out,
			48000, // sample rate
			1920, // frames per buffer: (48000 samples / sec @ 40 ms)
			paClipOff, // we won't output out of range samples,
							// so don't bother clipping them
			funct_callback_hd, // callback function HALF DUPLEX
			&global // parameters passed to callback function
		);
	} else {
		// full duplex
		pa_ret = Pa_OpenStream (
			&stream,
			pa_in,
			pa_out,
			48000, // sample rate
			1920, // frames per buffer: (48000 samples / sec @ 40 ms)
			paClipOff, // we won't output out of range samples,
							// so don't bother clipping them
			funct_callback_fd, // callback function FULL duplex
			&global // parameters passed to callback function
		);
	}; // end else - if

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
		int callback_diff;

		// endless sleep, wake up and check status every 0.2 second
		Pa_Sleep(200);

		callback_thiscount=global.callbackcount;
		callback_diff=callback_thiscount-callback_lastcount;


		// we should get 5 audio-frames per second (40 ms / frame). If we receive less then 2 or more then 8, there is clearly something wrong
		if ((callback_diff < 2) || (callback_diff > 8)) {
			callback_errorcount++;
			fprintf(stderr,"Warning: callback function called %d times last second. Should have been 5 times!\n",callback_diff);

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
	}; // end while (stream active)


	// stop on error IsStreamActive
	if (pa_ret < 0) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_isStreamActive: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

	// break out of endless loop on to many errors
	if (callback_errorcount > 20) {
		break;
	}; // end if

	// close stream
	pa_ret=Pa_CloseStream(stream);
	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if
	
}; // endless loop

// dropped out of endless loop. Should not happen


if (callback_errorcount <= 20) {
	fprintf(stderr,"Error: audiocap dropped out of endless loop for unknown reason. Should not happen!\n");
}; // end if


pa_ret=Pa_CloseStream(stream);
if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// Done!!!
Pa_Terminate();
exit(-1);

}; // end main program


