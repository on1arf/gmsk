/* capture.h */

// function func_capture:
// This function captures 960 samples (20 ms of audio, samples at 48000 Khz)
// of audio from an ALSA device and stores it in a buffer

// This function is started every 20 ms by a timed interrupt
// communication with the other threads is done via global variables


// version 20111107: initial release
// version 20111109: add input from file

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
int nextbuffer;
char *p;

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

if (global.fileorcapture) {
	// READ FROM FILE

	// exit if file completely read
	if (feof(global.filein)) {
		return;
	}; // end if

	int stop=0;
	int eof=0;

	// reading from file goes much faster then processing data, so check if
	// we can store it BEFORE reading from file

	
	// do we have some place to store the audio?
	nextbuffer = (global.pntr_capture +1) & 0xFF;

	while ((nextbuffer != global.pntr_process) && !(stop)) {
		int32_t average;
		int16_t *pointer;

		p=global.buffer[global.pntr_capture];


		// OK, we have place to store it

		rc=fread(p, sizeof(int16_t), 960, global.filein);

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
		global.buffersize[global.pntr_capture]=rc;

		if (eof) {
			global.fileend[global.pntr_capture]=1;
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
			pointer= (int16_t *)p;

			for (loop=0;loop<rc;loop++) {
			average += abs(*pointer);
				pointer += sizeof(int16_t);
			}; // end for

			// bitshift right 10 = divide by 1024 (is faster then division)
			average >>= 10;

			// store average
			global.audioaverage[global.pntr_capture]=average;
		} else {
			global.audioaverage[global.pntr_capture]=0;
		}; // end if

		// set pointer forward
		global.pntr_capture=nextbuffer;

		// do we still have more place to store the next audio frame?
		nextbuffer = (global.pntr_capture +1) & 0xFF;
	}; // end whille

// DONE

// mark thread as stopped
global.audioread=0;

return;
};



// CAPTURE FROM ALSA AUDIO DEVICE

p=global.buffer[global.pntr_capture];

// read audio from ALSA device
rc = snd_pcm_readi(global.handle, p, global.frames);

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
nextbuffer = (global.pntr_capture +1) & 0xFF;


if (global.pntr_process == nextbuffer) {
	// Oeps, playback is using the next buffer
	// give error and do NOT increase pointer
	putc('F',stderr);
} else {
	int32_t average;
	int16_t *pointer;

	global.buffersize[global.pntr_capture]=rc;

	// calculate average of absolute value
	// this will given an indication of the amplitude of the
	// received signal
	// we only do this when we have 960 samples (as we are supposted to have)
	// as we do not divide by 960 but by 1024 (shift right 10) as this is
	// faster then division

	if (rc == 960) {
		int loop;

		average=0;
		pointer= (int16_t *)p;

		for (loop=0;loop<rc;loop++) {
		average += abs(*pointer);
			pointer += sizeof(int16_t);
		}; // end for

		// bitshift right 10 = divide by 1024 (is faster then division)
		average >>= 10;

		// store average
		global.audioaverage[global.pntr_capture]=average;
	} else {
		global.audioaverage[global.pntr_capture]=0;
	}; // end if


	// set pointer forward
	global.pntr_capture=nextbuffer;

}; // end else - if

// mark thread as stopped
global.audioread=0;

}; // end function

