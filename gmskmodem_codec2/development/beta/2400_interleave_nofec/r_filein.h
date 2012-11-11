/* filein.h */

// function func_capture:
// This function captures 960 samples (20 ms of audio, samples at 48000 Khz)
// of audio from an ALSA device and stores it in a buffer

// This function runs as a independant thread
// communication with the other threads is done via global variables


// version 20111107: initial release
// version 20111109: add input from file
// version 20111112: add stereo
// version 20120209: rewrite as thread instead of timed interrupt
// version 20120612: split file-input into seperate file

/*
 *      Copyright (C) 2011-2012 by Kristoff Bonne, ON1ARF
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


void * funct_r_filein (void * c_globaldatain) {

// vars
int loop;
int nextbuffer;
char *p;

int octetpersample;
int channel;
int buffermask;

int stop;


// vars for file
int eof;
int32_t average;
int16_t *pointer;
int octetsleft;
int octetsread;



// import global data
c_globaldatastr * p_c_global;
r_globaldatastr * p_r_global;
g_globaldatastr * p_g_global;

p_c_global=(c_globaldatastr *) c_globaldatain;

p_r_global=p_c_global->p_r_global;
p_g_global=p_c_global->p_g_global;


// init
if (p_r_global->stereo) {
	octetpersample=4;
	channel=2;
	buffermask=0x7f; // 128 buffers : 0x00 to 0x7f
} else {
	octetpersample=2;
	channel=1;
	buffermask=0xff; // 256 buffers : 0x00 to 0xff
}; // end else - if

///////////////////
//// READ FROM FILE 


// check on "fileorcapture" flag

// only do this check when ALSA enabled
#ifdef _USEALSA
if (!p_r_global->fileorcapture) {
	fprintf(stderr,"Error: r_filein called when input set to CAPTURE)! \n");
	exit(-1);
}; // end if
#endif

stop=0;
eof=0;
 
while (! feof(p_r_global->filein)) {

	// reading from file goes much faster then processing data, so check if
	// we can store it BEFORE reading from file

	
	// do we have some place to store the audio?
	nextbuffer = (p_r_global->pntr_capture +1) & buffermask;

	while ((nextbuffer != p_r_global->pntr_process) && !(stop)) {
		octetsleft=960;

		p=p_r_global->buffer[p_r_global->pntr_capture];

		// OK, we have place to store it

		// note: file counters are octets (for ALSA, counters are frames)
		while ((octetsleft > 0) && (!(stop))) {
			octetsread=fread(p, octetpersample, octetsleft, p_r_global->filein);

			octetsleft -= octetsread;
			p += octetsread;

			if (octetsleft < 0) {
				// very strange, we received more octets then we asked for
				fprintf(stderr,"WEIRD ERROR: ALSA capture received more octets then requested! Exiting!!!\n");
				exit(-1);
			} else if (octetsleft > 0) {
				// we did not receive enough data, perhaps file end?
				if (feof(p_r_global->filein)) {
					eof=1;
					stop=1;
				} else {
					if (p_g_global->verboselevel >= 2) {
						fprintf(stderr, "UNEXPECTED short read: read %d octets\n", octetsread);
					}; // end if
				}; // end else - if
			}; // end else - if
		}; // end while

		// store data
		p_r_global->buffersize[p_r_global->pntr_capture]=960-octetsleft;

		if (eof) {
			p_r_global->fileend[p_r_global->pntr_capture]=1;
		}; // end if

		// copy "sending" flag to 0 (not relevent)
		p_r_global->sending[p_r_global->pntr_capture]=0;
		

		// calculate average of absolute value
		// this will given an indication of the amplitude of the
		// received signal
		// we only do this when we have 960 samples (as we are supposted to have)
		// as we do not divide by 960 but by 1024 (shift right 10) as this is
		// faster then division

		if (octetsleft == 0) {
			average=0;
			// set pointer to beginning of sample
			pointer= (int16_t *)p_r_global->buffer[p_r_global->pntr_capture];

			for (loop=0;loop<960;loop++) {
				average += abs(*pointer);
				pointer += channel;

		}; // end for

			// bitshift right 10 = divide by 1024 (is faster then division)
			average >>= 10;

			// store average
			p_r_global->audioaverage[p_r_global->pntr_capture]=average;
		} else {
			p_r_global->audioaverage[p_r_global->pntr_capture]=0;
		}; // end if

		// move pointer forward
		p_r_global->pntr_capture=nextbuffer;

		// do we still have more place to store the next audio frame?
		nextbuffer = (p_r_global->pntr_capture +1) & buffermask;
	}; // end while


}; // end loop read complete file

return(NULL);
};
/// END OF part "READ FROM FILE"


