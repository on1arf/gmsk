/* modem.c */


// A program to capture/read, demodulate, a gmsk stream from an audio-port
// (ALSA) or file, extracts codec2 stream and saves it to a
// .c2 file.
// At the same time, read a c2 binary file and stream it out


// The low-level DSP and radio header decoding functions are largly
// taken from the pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


/*
 *      Copyright (C) 2012 by Kristoff Bonne, ON1ARF
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
// version 20120322 initial release


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

// set _USEFLOAT to 0 if not set
#ifndef _USEFLOAT
	#define _USEFLOAT 0
#endif

// set _INTMATH to 64 if not set
#ifndef _INTMATH
	#define _INTMATH 64
#endif


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


#ifdef _USEALSA
	// ALSA sound
	#include <alsa/asoundlib.h>
#endif

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

// global data
#include "global.h"


// DSP functions for receiver and sender
#include "dspstuff.h"


// support functions for receiver and sender
#include "parsecliopts.h"
#ifdef _USEALSA
	#include "initalsa.h"
#endif

// codec2 functions
#include "c2_scramble.h"
#include "c2_fec13.h"
#include "c2_interleave.h"


// support functions for receiver
#include "r_printbit.h"
#include "r_countdiff.h"
#include "r_initinfile.h"
#include "r_output_dal.h"

// functions used by main program for receiver
#include "r_filein.h"
#ifdef _USEALSA
	#include "r_capture.h"
#endif

// codec2 encapsulation
#include "c2encap.h"

#include "r_processaudio_codec2.h"

// suport functions for sender
#include "s_buffer.h"
#include "s_gmskmodulate.h"
#include "s_writefile.h"
#include "s_input_sal.h"
#include "s_input_codec2.h"
#ifdef _USEALSA
	#include "s_alsaout.h"
	#include "s_ptt.h"
#endif

/////////////////////////////////////////
/////////////////////////////////////////
//  MAIN APPLICATION STARTS HERE !!!  ///
/////////////////////////////////////////
/////////////////////////////////////////

int main (int argc, char ** argv) {

char retmsg[PARSECLIRETMSGSIZE];
int retval;


int octetpersample;
int numbuffer;
int loop;
int size;


// threads
pthread_t thr_r_input, thr_r_processaudio, thr_s_writefile, thr_s_input, thr_s_gmskmodulate;

#ifdef _USEALSA
	pthread_t thr_s_ptt, thr_s_alsaout;
#endif


///////////////////////////////////
// APPLICATION STARTS HERE ////////
///////////////////////////////////

// fill in combined data structure
c_global.p_r_global=&r_global;
c_global.p_s_global=&s_global;
c_global.p_g_global=&g_global;


// init vars
s_global.ptr_a_read=0;
s_global.ptr_a_fillup=1;

s_global.ptr_b_read=0;
s_global.ptr_b_fillup=1;

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


// SENDER
if (!s_global.disable) {
	// SENDER INPUT

	// start threads
	// start thread to read input. This runs as a seperate thread so
	// not to be interrupted by the "alsa out" interrupt driven
	// interrupt which can "starve" the main thread 

	pthread_create(&thr_s_input, NULL, funct_input_codec2, (void *) &c_global); 

	// start "gmsk and buffer" thread
	pthread_create(&thr_s_gmskmodulate, NULL, funct_s_modulateandbuffer, (void *) &s_global);


	#ifdef _USEALSA
		// start "PTT" thread.
		if (s_global.pttcsdevice) {
			pthread_create(&thr_s_ptt, NULL, funct_pttcs, (void *) &c_global);
		} else if (s_global.ptttxdevice) {
			pthread_create(&thr_s_ptt, NULL, funct_ptttx, (void *) &c_global);
		} else if (s_global.pttlockfile) {
			pthread_create(&thr_s_ptt, NULL, funct_pttlockfile, (void *) &c_global);
		}; // end elsif - elsif - if
	#endif

	// SENDER OUTPUT

	#ifdef _USEALSA
		// start file out or alsa out
		if (s_global.fileoralsa == 0) {
			pthread_create(&thr_s_writefile, NULL, funct_writefile, (void *) &s_global);
		} else if (s_global.fileoralsa == 1) {
			pthread_create(&thr_s_alsaout, NULL, funct_alsaout, (void *) &c_global);
		} else {
			// error
			fprintf(stderr,"Error: Output should be either FILE or ALSA!\n");
			exit(-1);
		}; // end elsif - if
	#else 
	// NO alsa -> only file output possible
		pthread_create(&thr_s_writefile, NULL, funct_writefile, (void *) &s_global);
	#endif

}; // end if (sender not disabled)


// RECEIVER
if (!r_global.disable) {
	// RECEIVER INPUT

	#ifdef _USEALSA 
		// INIT ALSA or FILE (depending on CLI options)
		if (r_global.fileorcapture == 0) {
			retval=init_alsa_capture(retmsg);
		} else if (r_global.fileorcapture == 1) {
			retval=init_infile(retmsg);
		} else {
			// not capture or file input
			fprintf(stderr,"Error: invalid value for file-or-capture. Shouldn't happen!\n");
			exit(-1);
		}; // end else - elsif - if
	#else
		// NO ALSA -> only file input possible
		retval=init_infile(retmsg);
	#endif
	
	// return message?
	if (retval < 0) {
	// -1: error
		fprintf(stderr,"%s",retmsg);
		exit(-1);
	} else if (retval > 0) {
	// +1: warning
		fprintf(stderr,"%s",retmsg);
	}; // end elsif - if

	// CREATE BUFFERS to transfer audio from "capture" to "process"
	// functions
	if (r_global.stereo) {
		octetpersample=4;
		numbuffer=128;
	} else {
		octetpersample=2;
		numbuffer=256;
	}; // end else - if


	#ifdef _USEALSA
		if (r_global.fileorcapture == 0) {
			// if ALSA, get frame size from information returned by alsa drivers
			size = r_global.frames * octetpersample;
		} else {
			// file, size is fixed: 960 samples
			size=960 * octetpersample;
		}; // end if
	#else
		// file, size is fixed: 960 samples
		size=960 * octetpersample;
	#endif

	// create buffers
	for (loop=0;loop<numbuffer;loop++) {
		r_global.buffer[loop] = (char *) malloc(size);

		if (r_global.buffer[loop] == NULL) {
			fprintf(stderr,"Error: could not allocate memory for buffer %d\n",loop);
			exit(-1);
		}; // end if

		// init fileend
		r_global.fileend[loop]=0;
	}; // end for

	// init buffer pointers
	r_global.pntr_capture=0;
	r_global.pntr_process=numbuffer-1;


	// RECEIVER INPUT

	// This application is designed as a multithreaded program:
	// * Main thread (main): initiates programs, starts subthreads and goes
	// to sleep
	// * Capture thread (funct_capture): reads audio from ALSA audio-device
	// and stores in buffer
	// * Processing thread (funct_process): process audio-information, as
	// received by the capture thread
	// This runs as a seperate thread next to the main application to shield
	// it of from being constantly interrupted by the "capture"
	// timed interrupt.

	// POSIX thread "capture" or "filein"
	#ifdef _USEALSA
		if (r_global.fileorcapture) {
			pthread_create(&thr_r_input, NULL, funct_r_filein, (void *) &c_global);
		} else {
			pthread_create(&thr_r_input, NULL, funct_r_capture, (void *) &c_global);
		}; // end else - if
	#else
		// NO ALSA -> only file in 
		pthread_create(&thr_r_input, NULL, funct_r_filein, (void *) &c_global);
	#endif

	// POSIX thread "processaudio"
	// start thread to process received audio
	pthread_create(&thr_r_processaudio, NULL, funct_r_processaudio_codec2, (void *) &c_global);

	// OK, done
}; // end if (receiver not disabled)


// now go to sleep
while (FOREVER) {
	sleep(100);

}; // end if

}; // end main program


