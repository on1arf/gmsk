/* receiver-savetodvtool.c */


// A program to capture, demodulate, a gmsk stream from an audio-port
// (ALSA), extracts the D-STAR stream and saves it to a .dvtool file

// The low-level DSP and radio header decoding functions are largly
// based on code from the pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


// This program is provided both as a functional program and as an
// educational tool on how to receive, demodulate and process
// gmsk streams, especially when used for D-STAR

/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API


// for timezone and timeval (needed for random)
#include <sys/time.h>

// POSIX threads + inter-process control
#include <pthread.h>
#include <signal.h>

// posix interrupt timers
#include <time.h>

// needed for usleep
#include <unistd.h>

// ALSA sound
#include <alsa/asoundlib.h>

// for memset
#include <strings.h>

// defines for timed interrupts
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

// global data
#include "global.h"

// functions
#include "dspstuff.h"
#include "descramble.h"
#include "fcs.h"

// structures for DVtool files
#include "dvtool.h"

// support functions
#include "printbit.h"
#include "countdiff.h"


// functions used by main program
#include "capture.h"
#include "processaudio.h"

/////////////////////////////////////////
/////////////////////////////////////////
//  MAIN APPLICATION STARTS HERE !!!  ///
/////////////////////////////////////////
/////////////////////////////////////////

int main (int argc, char ** argv) {

int ret;

char * capturedevice;


int loop;
int dir;
int size;

snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;

int period=960; // 48000 samples/second, 20 ms = 960 samples
unsigned int rate=48000;
unsigned int val;

// vars for timed interrupts
struct sigaction sa;
struct sigevent sev;
timer_t timerid;
struct itimerspec its;


// threads
pthread_t thr_processaudio;


/////////////////////////////
// Command line parameters parsing
//

if (argc <= 2) {
	fprintf(stderr,"Error: missing parameters: need alsa device name and output filename.\n");
	exit(-1);
}; // end if

capturedevice=argv[1];

global.fnameout=argv[2];


///////////////////////////////
// init ALSA device

/* Open PCM device for recording (capture). */
ret = snd_pcm_open(&global.handle, capturedevice, SND_PCM_STREAM_CAPTURE, 0);
if (ret < 0) {
	fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(ret));
	exit(1);
}

/* Allocate a hardware parameters object. */
snd_pcm_hw_params_alloca(&params);

/* Fill it in with default values. */
snd_pcm_hw_params_any(global.handle, params);

/* Set the desired hardware parameters. */

/* Interleaved mode */
snd_pcm_hw_params_set_access(global.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

/* Signed 16-bit little-endian format */
snd_pcm_hw_params_set_format(global.handle, params, SND_PCM_FORMAT_S16_LE);

/* One channel (mono) */
snd_pcm_hw_params_set_channels(global.handle, params, 1);

/* 48 Khz sampling rate */
val=rate;
snd_pcm_hw_params_set_rate_near(global.handle, params, &val, &dir);

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = period;
snd_pcm_hw_params_set_period_size_near(global.handle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);
global.frames = frames;

if (frames != period) {
	printf("periode could not be set: wanted %d, got %d \n",period,(int) global.frames);
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(global.handle, params);
if (ret < 0) {
	fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(ret));
	exit(1);
}


/////////////////////
// CREATE BUFFERS to transfer audio from "capture" to "process"
// functions

size = global.frames * 2; /* 2 bytes/sample, 1 channel */

// create 8 buffers
for (loop=0;loop<=255;loop++) {
	global.buffer[loop] = (char *) malloc(size);

	if (global.buffer[loop] == NULL) {
		fprintf(stderr,"Error: could not allocate memory for buffer %d\n",loop);
		exit(-1);
	}; // end if
}; // end for

// init buffer pointers
global.pntr_capture=0;
global.pntr_process=255;



// This application is designed as a multithreaded program:
// * Main thread (main): initiates programs, starts subthreads and goes
// to sleep
// * Capture thread (funct_capture): started every 20 ms from a system
// "timed interrupt": reads audio from ALSA audio-device and stores
// in buffer
// * Processing thread (funct_process): process audio-information, as
// received by the capture thread
// This runs as a seperate thread next to the main application to shield
// it of from being constantly interrupted by the "capture"
// timed interrupt.


//////////////////////
// Starting capture function (timed interrupt)

// Set up interrupt handler for signal SIG, pointing to
// unction capture
sa.sa_flags=0;
sa.sa_handler = funct_capture;
sigemptyset(&sa.sa_mask);

ret=sigaction(SIG,&sa,NULL);
if (ret < 0) {
	fprintf(stderr,"error in sigaction: function capture!\n");
	exit(-1);
}; // end if

/* Create the timer for capture */
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo = SIG;
sev.sigev_value.sival_ptr = &timerid;

ret=timer_create(CLOCKID, &sev, &timerid);
if (ret < 0) {
	fprintf(stderr,"error in timer_create timer: function capture!\n");
	exit(-1);
}; // end if


// Configured timed interrupt, that will trigger a "SIG" interrupt
// every 20 ms

// start timed function timer capture, every 20 ms, no offset (1 ns)
its.it_value.tv_sec = 0;
its.it_value.tv_nsec = 1;
its.it_interval.tv_sec = 0;
its.it_interval.tv_nsec = 20000000; // 20 ms = 20 million nanoseconds

ret=timer_settime(timerid, 0, &its, NULL);
if (ret < 0) {
	fprintf(stderr,"error in timer_settime timer: function capture!\n");
	exit(-1);
}; // end if


// POSIX thread "processaudio"

// start endless loop to process information received from it

// processing of received audio is done on seperate thread
// this is to avoid it begin interrupted by the "capture" application
// start thread to process received audio
pthread_create(&thr_processaudio, NULL, funct_processaudio, (void *) &global);



// OK, we are done. Both subthreads are started. We can now retire.

// Because the main application is being interrupted by the 
// capture thread, every 20 ms.
// it cannot do much else usefull except sleep :-)

while (1) {
	sleep(500);
}; // end if

}; // end main program


