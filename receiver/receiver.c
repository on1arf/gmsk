/* receiver.c */


// A program to capture/read, demodulate, a gmsk stream from an audio-port
// (ALSA) or file, extracts the D-STAR stream and saves it to a .dvtool file

// The low-level DSP and radio header decoding functions are largly
// taken from the pcrepeatercontroller project written by
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
#include "parsecliopts.h"
#include "initalsa.h"
#include "initinfile.h"


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

char retmsg[160];
int retval;


int octetpersample;
int numbuffer;
int loop;
int size;

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

// parse CLI options, store info in global data
// returns 0 if all OK; -1 on error, 1 on warning
retval=parsecliopts(argc,argv,retmsg);

if (retval < 0) {
// -1: error
	fprintf(stderr,"%s",retmsg);
	exit(-1);
} else if (retval > 0) {
// +1: warning
	fprintf(stderr,"%s",retmsg);
}; // end elsif - if


// INIT ALSA or FILE (depending on CLI options)
if (global.fileorcapture == 0) {
	retval=init_alsa(retmsg);
} else if (global.fileorcapture == 1) {
	retval=init_infile(retmsg);
} else {
	// not capture or file input
	fprintf(stderr,"Error: invalid value for file-or-capture. Shouldn't happen!\n");
	exit(-1);
}; // end else - elsif - if
	

if (retval < 0) {
// -1: error
	fprintf(stderr,"%s",retmsg);
	exit(-1);
} else if (retval > 0) {
// +1: warning
	fprintf(stderr,"%s",retmsg);
}; // end elsif - if


/////////////////////
// CREATE BUFFERS to transfer audio from "capture" to "process"
// functions

if (global.stereo) {
	octetpersample=4;
	numbuffer=128;
} else {
	octetpersample=2;
	numbuffer=256;
}; // end else - if



if (global.fileorcapture == 0) {
	// if ALSA, get frame size from information returned by alsa drivers
	size = global.frames * octetpersample;
} else {
	// file, size is fixed: 960 samples
	size=960 * octetpersample;
}; // end if

// create buffers
for (loop=0;loop<numbuffer;loop++) {
	global.buffer[loop] = (char *) malloc(size);

	if (global.buffer[loop] == NULL) {
		fprintf(stderr,"Error: could not allocate memory for buffer %d\n",loop);
		exit(-1);
	}; // end if

	// init fileend
	global.fileend[loop]=0;
}; // end for

// init buffer pointers
global.pntr_capture=0;
global.pntr_process=numbuffer-1;



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

// start timed function timer capture, every 20 ms, offset is 1 second to
// avoid "starvation" of the main thread by the capture thread (is started
// every 20 ms and takes just a little bit then 20 ms to execute)
its.it_value.tv_sec = 1;
its.it_value.tv_nsec = 0;
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


