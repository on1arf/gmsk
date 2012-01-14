/* sender.c */


// A program to read a .dvtool file from file, create the radio-channel
// bitpattern, modulate and play out the GMSK/D-STAR stream to a ALSA
// audio-device

// The low-level DSP and radio header decoding functions are largly
// taken from the pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


// This program is provided both as a functional program and as an
// educational tool on how to create gmsk streams, especially when used
// for D-STAR

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


// Release information:
// version 20111213: initial release
// version 20120105: play out to alsa-devices, PTT file-lock, raw files, read from stdin


// include files

// for stdio
#include <stdio.h>

// for exit
#include <stdlib.h>

// standard integer types
#include "stdint.h"

// for memset and memmove
#include "string.h"

// for hton
#include "arpa/inet.h"

// for usleep
#include <unistd.h>

// ALSA audio
#include <alsa/asoundlib.h>

// POSIX threads
#include <pthread.h>
#include <signal.h>

// posix interrupt timers
#include <time.h>

// defines for timed interrupts
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN


////////// FILE INCLUDES
#include "global.h"
#include "parsecliopts.h"
#include "dvtool.h"
#include "fcs.h"
#include "rewriteheader.h"
#include "buffer.h"
#include "dstarheader.h"
#include "dspstuff.h"
#include "gmskmodulate.h"
#include "readdvtool.h"
#include "writefile.h"
#include "input.h"
#include "initalsa.h"
#include "alsaout.h"

/////////////////////////////////////////
/////////////////////////////////////////
//  MAIN APPLICATION STARTS HERE !!!  ///
/////////////////////////////////////////
/////////////////////////////////////////

int main (int argc, char ** argv) {


// local vars
int retval;
char retmsg[RETMSGSIZE];


// threads
pthread_t thr_writefile;
pthread_t thr_input;
pthread_t thr_gmskmodulate;


// timed interrupts
struct sigaction sa;
struct sigevent sev;
timer_t timerid;
struct itimerspec its;




////////////
// init vars
global.invert=0;

global.ptr_a_read=0;
global.ptr_a_fillup=1;

global.ptr_b_read=0;
global.ptr_b_fillup=1;

global.lockfd=-1;

/////////////////////////////
// Command line parameters parsing
//

// parse CLI options, store info in global data
// returns 0 if all OK; -1 on error, 1 on warning
retval=parsecliopts(argc,argv,retmsg);

if (retval) {
	// retval != 0: error or warning
	fprintf(stderr,"%s",retmsg);

	// retval < 0: error -> exit
	if (retval <0 ) {
		exit(-1);
	};
}; // end if



// open (create if needed) PTTlockfile.
// The sender application will set up a POSIX lock on that file
// when the PTT needs to be pushed
// That way, an external application can monitor this and
// do the actual hardware invention to switch the PTT

if (global.lockfile) {
	global.lockfd=open(global.lockfile, O_WRONLY|O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	if (global.lockfd == -1) {
		fprintf(stderr,"Error: Lockfile %s could not be opened or created!\n",global.lockfile);
		exit(-1);
	}; // end if

}; // end if


// start threads
// start thread to read input. This runs as a seperate thread so
// not to be interrupted by the "alsa out" interrupt driven
// interrupt which can "starve" the main thread 
pthread_create(&thr_input, NULL, funct_input, (void *) &global);
pthread_create(&thr_gmskmodulate, NULL, funct_modulateandbuffer, (void *) &global);


// start file out or alsa out
if (global.fileoralsa == 0) {
	pthread_create(&thr_writefile, NULL, funct_writefile, (void *) &global);
} else if (global.fileoralsa == 1) {
	// Set up interrupt handler for signal SIG, pointing to
	// function 'alsaout'
	sa.sa_flags=0;
	sa.sa_handler = funct_alsaout;
	sigemptyset(&sa.sa_mask);

	retval=sigaction(SIG,&sa,NULL);
	if (retval < 0) {
		fprintf(stderr,"error in sigaction: function alsaout!\n");
		exit(-1);
	}; // end if

	/* Create the timer */
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;

	retval=timer_create(CLOCKID, &sev, &timerid);
	if (retval < 0) {
		fprintf(stderr,"error in timer_create timer: function alsaout!\n");
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

	retval=timer_settime(timerid, 0, &its, NULL);

	if (retval < 0) {
		fprintf(stderr,"error in timer_settime timer: function capture!\n");
		exit(-1);
	}; // end if

} else {
	// error
	fprintf(stderr,"Error: Output should be either FILE or ALSA!\n");
	exit(-1);
}; // end elsif - if


// endless loop
while (FOREVER) {
	sleep(1000);
}; // end while

}; // end main application
