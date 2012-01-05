/* gmskmodulate.h */

// This code is part of the "gmsksender" application

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


// This is a function that runs as a continues freerunning thread that
// takes bits from the "bit" buffer, gmsk modulates them and stores
// them on the "audio" buffer


// Release information:
// version 20111213: initial release
// version 20120105: No change



// function defined below
int16_t process_return (int64_t);
void modulate1bit (int, int16_t *);


// MODULATE AND BUFFER
// MAIN FUNCTION

// this function is a freerunning and never ending loop
// it scans the "bit buffer" for data to encode, and -after modulating- queues it on the 
// audio buffer 

void * funct_modulateandbuffer(void * globaldatain) {
//local vars
int loop_bits;
int nextbuffer;

// global data
globaldatastr *pglobal;
pglobal=(globaldatastr *) globaldatain;

// audio returns
// we receive 10 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
int16_t audioret[10];


// tempory data
signed char thisdata;
signed char thismask;

// init vars
pglobal->waiting_bits=-1;

nextbuffer=pglobal->ptr_b_read +1;
if (nextbuffer >= BUFFERSIZE_BITS) {
	nextbuffer=0;
}; // end if


// endless loop
while (FOREVER) {
/////////fprintf(stderr,"nextbuffer: %d, b_fillup = %d \n",nextbuffer,pglobal->ptr_b_fillup);
	// if there something in the bits buffer to process?
	if (pglobal->ptr_b_fillup == nextbuffer) {
		// nothing to process, wait for 100 µS 
		pglobal->waiting_bits=1;
		usleep(10000);
/////////fprintf(stderr,"SLEEPING! \n");
		continue;
	}; // end if
	

	// reset "waiting" flag
	pglobal->waiting_bits=0;


	// yes, we've got something to process
	thisdata=pglobal->buffer_bits[nextbuffer];
	thismask=pglobal->buffer_bits_mask[nextbuffer];

	if (pglobal->octetorderinvert[nextbuffer]) {
		// REVERSE BITORDER
		// read bits from left to right.
		for (loop_bits=7;loop_bits >= 0; loop_bits--) {

			// look at left most bit
			if (thismask & 0x80) {
			// check mask if do we need to process this data
				if (thisdata & 0x80) {
					modulate1bit(1,audioret);
				} else {
					modulate1bit(0,audioret);
				}; // end if

				// write audio samples to audio buffer
				bufferfill_audio(audioret, 10, 1, pglobal);
			}; // end if

			// left shift data and mask
			thisdata <<= 1;
			thismask <<= 1;
		}; // end for
	} else {
		// NO REVERSE
		for (loop_bits=0;loop_bits <= 7; loop_bits++) {

			// look at right most bit
			if (thismask & 0x01) {
			// check mask if we need to process this data
				if (thisdata & 0x01) {
					modulate1bit(1,audioret);
				} else {
					modulate1bit(0,audioret);
				}; // end if

				// write audio samples to audio buffer
				bufferfill_audio(audioret, 10, 1, pglobal);
			}; // end if

			// left shift c
			thisdata >>= 1;
			thismask >>= 1;
		}; // end for

	}; // end else - if

	// go to next audio-sample
	pglobal->ptr_b_read=nextbuffer;

	// find next buffer
	nextbuffer++;
	if (nextbuffer >= BUFFERSIZE_BITS) {
		nextbuffer=0;
	}; // end if
}; // end for

// re-init vars
pglobal->waiting_bits=-1;

}; // end function modulate and output



////////////////////////////
//// Function process_return 
int16_t process_return (int64_t filterret) {
// convert 48 bit result back to 16 bits
int bit30;
int16_t filterret16;

// remember 30th bit, is value just below unity
bit30=filterret & 0x40000000;

if (filterret >= 0) {
	// positive
	filterret16=((filterret & 0x3fff80000000LL)>>31);

	// add one for rounding if bit 30 was set
	if (bit30) {
		filterret16++;
	};
} else {
	// negative
	filterret16=-((-filterret & 0x3fff80000000LL)>>31);

	// subtract one for rounding if bit 30 was not set
	if (!(bit30)) {
		filterret16--;
	}; // end if
};


return(filterret16);

}; // end function process_return

////////////////////////////
//// Function modulate1bit
void modulate1bit (int bit, int16_t * audioret) {
// local vars
int loop;

int64_t filterret;
int16_t ret16;

int16_t *p=NULL;

// some error checking
if (! audioret) {
	return;
}; // end if
	

p=audioret;

// send 10 audio-samples to firfilter per bit
if (bit) {
// bit = 1 -> send -16384 to firfilter
	for (loop=0; loop<10; loop++) {
		filterret=firfilter_modulate(-16384);
		ret16=process_return(filterret);

		// store data
		*p=ret16;
		p++;
	}; // end for
} else {
// bit = 0 -> send +16384 to firfilter
	for (loop=0; loop<10; loop++) {
		filterret=firfilter_modulate(16384);
		ret16=process_return(filterret);

		// store data
		*p=ret16;
		p++;
	}; // end for
}; // end if

return;

}; // end function modulate1bit
