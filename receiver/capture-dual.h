/* capture.h */

// function func_capture:
// This function captures 960 samples (20 ms of audio, samples at 48000 Khz)
// of audio from an ALSA device and stores it in a buffer

// This function is started every 20 ms by a timed interrupt
// communication with the other threads is done via global variables


// version 20111107: initial release
// version 20111109: add input from file
// version 20111112: add stereo

// version 20111113: DUAL version (process same stream with integers and floats)

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


static void funct_capture (int sig) {
int errorcount=0;
int rc;
int nextbuffer_int, nextbuffer_float;
char *p_int, *p_float;

static int init=0;

static int octetpersample;
static int channel;
static int buffermask;

// now defined here (as shared by code for int and float)
// used to be defined inside "if" statement below
int32_t average=-1;

if (!(init)) {
	init=1;

	if (global.stereo) {
		octetpersample=4;
		channel=2;
		buffermask=0x7f; // 128 buffers : 0x00 to 0x7f
	} else {
		octetpersample=2;
		channel=1;
		buffermask=0xff; // 256 buffers : 0x00 to 0xff
	}; // end else - if
}; // end if init

// Function_capture is started automatically every 20 ms; so there
// is a risk that the earlier instance is still running
// break of if that is the case

if (global.audioread) {

	// only print warning when capturing from audiodevice
	if (global.fileorcapture == 0) {
		putc('R',stderr);
		return;
	}; // end if
}; // end if

// make thread as running
global.audioread=1;


///////////////////
//// READ FROM FILE 
if (global.fileorcapture) {

	// exit if file completely read
	if (feof(global.filein)) {
		return;
	}; // end if

	int stop=0;
	int eof=0;

	// reading from file goes much faster then processing data, so check if
	// we can store it BEFORE reading from file

	
	// do we have some place to store the audio?

	// we will wait until we can store BOTH the integer and the
	// floating point data
	nextbuffer_int = (global.pntr_int_capture +1) & buffermask;
	nextbuffer_float = (global.pntr_float_capture +1) & buffermask;

	while (((nextbuffer_int != global.pntr_int_process) && (nextbuffer_float != global.pntr_float_process)) && !(stop)) {
		int32_t average;
		int16_t *pointer;

		p_int=global.int_buffer[global.pntr_int_capture];
		p_float=global.float_buffer[global.pntr_float_capture];


		// OK, we have place to store it

		// read data and store in "int" buffer
		rc=fread(p_int, octetpersample, 960, global.filein);
		memcpy(p_float,p_int,960*octetpersample);

		if (rc < 960) {
			// less then 960 samples read.

			// EOF?
			if (feof(global.filein)) {
				fprintf(stderr, "EOF: read %d frames\n", rc);
				stop=1;
				eof=1;
			} else {
				fprintf(stderr, "UNEXPECTED short read: read %d frames\n", rc);
				stop=1;
			}; // end else - if
		}; // end  if

		// store data
		global.int_buffersize[global.pntr_int_capture]=rc;
		global.float_buffersize[global.pntr_float_capture]=rc;

		if (eof) {
			global.int_fileend[global.pntr_int_capture]=1;
			global.float_fileend[global.pntr_float_capture]=1;
		}; // end if
		

		// calculate average of absolute value
		// this will given an indication of the amplitude of the
		// received signal
		// we only do this when we have 960 samples (as we are supposted to have)
		// as we do not divide by 960 but by 1024 (shift right 10) as this is
		// faster then division

		if (rc == 960) {
			int loop;

			average=0;
			pointer= (int16_t *)p_int;

			for (loop=0;loop<rc;loop++) {
				average += abs(*pointer);
				pointer += channel;

			}; // end for

			// bitshift right 10 = divide by 1024 (is faster then division)
			average >>= 10;

			// store average
			global.int_audioaverage[global.pntr_int_capture]=average;
			global.float_audioaverage[global.pntr_float_capture]=average;
		} else {
			global.int_audioaverage[global.pntr_int_capture]=0;
			global.float_audioaverage[global.pntr_float_capture]=0;
		}; // end if

		// set pointer forward
		global.pntr_int_capture=nextbuffer_int;
		global.pntr_float_capture=nextbuffer_float;

		// do we still have more place to store the next audio frame?
		nextbuffer_int = (global.pntr_int_capture +1) & buffermask;
		nextbuffer_float = (global.pntr_float_capture +1) & buffermask;
	}; // end whille

// DONE

// mark thread as stopped
global.audioread=0;

return;
};
/// END OF part "READ FROM FILE"



///////////////////
// CAPTURE FROM ALSA AUDIO DEVICE

p_int=global.int_buffer[global.pntr_int_capture];
p_float=global.float_buffer[global.pntr_float_capture];

// read audio from ALSA device
rc = snd_pcm_readi(global.handle, p_int, global.frames);
// copy data to float buffer
memcpy(p_float,p_int,global.frames*octetpersample);


if (rc == -EPIPE) {
	/* EPIPE means overrun */
	fprintf(stderr, "overrun occurred CAPTURE %d\n",errorcount);
	errorcount++;
	snd_pcm_prepare(global.handle);
	return;
} else if (rc < 0) {
	fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
	return;
} else if (rc != (int)global.frames) {
	fprintf(stderr, "short read, read %d frames\n", rc);
};

// do we have some place to store the audio?
nextbuffer_int = (global.pntr_int_capture +1) & buffermask;
nextbuffer_float = (global.pntr_float_capture +1) & buffermask;

average=-1;

if (global.pntr_int_process == nextbuffer_int) {
	// Oeps, playback is using the next buffer
	// give error and do NOT increase pointer
	putc('F',stderr); putc('I',stderr);
} else {
	int16_t *pointer;

	global.int_buffersize[global.pntr_int_capture]=rc;

	// calculate average of absolute value
	// this will given an indication of the amplitude of the
	// received signal
	// we only do this when we have 960 samples (as we are supposted to have)
	// as we do not divide by 960 but by 1024 (shift right 10) as this is
	// faster then division

	if (rc == 960) {
		int loop;

		average=0;
		pointer= (int16_t *)p_int;

		for (loop=0;loop<rc;loop++) {
		average += abs(*pointer);
		pointer += channel;
		}; // end for

		// bitshift right 10 = divide by 1024 (is faster then division)
		average >>= 10;

		// store average
		global.int_audioaverage[global.pntr_int_capture]=average;
	} else {
		global.int_audioaverage[global.pntr_int_capture]=0;
	}; // end if


	// set pointer forward
	global.pntr_int_capture=nextbuffer_int;

}; // end else - if


// do the same thing for FLOATS
// Note, now, we only calculate the average, if not yet done above

if (global.pntr_float_process == nextbuffer_float) {
	// Oeps, playback is using the next buffer
	// give error and do NOT increase pointer
	putc('F',stderr); putc('F',stderr);
} else {
	int16_t *pointer;

	global.float_buffersize[global.pntr_float_capture]=rc;

	// calculate average of absolute value
	// this will given an indication of the amplitude of the
	// received signal
	// we only do this when we have 960 samples (as we are supposted to have)
	// as we do not divide by 960 but by 1024 (shift right 10) as this is
	// faster then division


	if (rc == 960) {
		int loop;

		// calculate average only if not calculated above
		if (average == -1) {
			average=0;
			pointer= (int16_t *)p_float;

			for (loop=0;loop<rc;loop++) {
				average += abs(*pointer);
				pointer += channel;
			}; // end for

			// bitshift right 10 = divide by 1024 (is faster then division)
			average >>= 10;
		}; // end if (calculate average)

		// store average
		global.float_audioaverage[global.pntr_float_capture]=average;
	} else {
		global.float_audioaverage[global.pntr_float_capture]=0;
	}; // end if


	// set pointer forward
	global.pntr_float_capture=nextbuffer_float;

}; // end else - if

// mark thread as stopped
global.audioread=0;

}; // end function

