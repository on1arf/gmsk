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


// Release information:
// version 20111213: initial release


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

int modulateandout_bits(int * buffer, int size, FILE * fileout, char * return_message) {
	//local vars
	int loop_bit;

	// audio returns
	// we receive 10 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
	int16_t audioret[10];

	int * p;

	// return audio buffer

	// init vars
	p=buffer;

	for (loop_bit=0;loop_bit<size;loop_bit++) {
		if (*p) {
			modulate1bit(1,audioret);
		} else {
			modulate1bit(0,audioret);
		}; // end if

		// go to next audio-sample
		p++;

		// write audio samples to file
		fwrite(audioret,10*sizeof(uint16_t),1,fileout);
	}; // end for

	return(0);
}; // end function modulate and output


int modulateandout_octets(unsigned char * buffer, int size, FILE * fileout, char * return_message, int reverse) {
	//local vars
	int loop_bits;
	int loop_octets;
	char c;

	// audio returns
	// we receive 10 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
	int16_t audioret[10];

	unsigned char * p;

	// return audio buffer

	// init vars
	p=buffer;

	for (loop_octets=0;loop_octets<size;loop_octets++) {

		c=*p & 0xff;


		if (reverse) {
			// note, we read bits from left to right as that is the way they are transmitted
			for (loop_bits=7;loop_bits >= 0; loop_bits--) {

				// look at left most bit
				if (c & 0x80) {
					modulate1bit(1,audioret);
				} else {
					modulate1bit(0,audioret);
				}; // end if

				// left shift c
				c <<= 1;

				// write audio samples to file
				fwrite(audioret,10*sizeof(uint16_t),1,fileout);
			}; // end for
		} else {
			// NO REVERSE
			for (loop_bits=0;loop_bits <= 7; loop_bits++) {

				// look at right most bit
				if (c & 0x01) {
					modulate1bit(1,audioret);
				} else {
					modulate1bit(0,audioret);
				}; // end if

				// left shift c
				c >>= 1;

				// write audio samples to file
				fwrite(audioret,10*sizeof(uint16_t),1,fileout);

			}; // end for

		}; // end else - if

		// go to next audio-sample
		p++;

	}; // end for

return(0);
}; // end function modulate and output



