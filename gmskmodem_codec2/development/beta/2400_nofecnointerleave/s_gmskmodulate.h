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

// filterret only used for int64
#if _USEFLOAT == 0
#if _INTMATH == 64
int64_t filterret;
#endif
#endif
int16_t ret16;

int16_t *p=NULL;

// some error checking
if (! audioret) {
	return;
}; // end if

// flip bit if audioinvert for sending
// audioinvert: (0=no), (1=receive), 2 = transmit, 3 = both
if (g_global.audioinvert & 0x02) {
	bit ^= 0x01;
}; // end if

p=audioret;

// send 20 audio-samples to firfilter per bit
if (bit) {
// bit = 1 -> send -16384 to firfilter
	for (loop=0; loop<20; loop++) {
#if _USEFLOAT == 1
		ret16=(int)(firfilter_modulate(-16384));
#else
#if _INTMATH == 64
		// 16 bit audio
		filterret=firfilter_modulate(-16384);
		ret16=process_return(filterret);
#else
#if _INTMATH == 3212
		// 12 bit audio
		ret16=(firfilter_modulate(-1024)>> 16);
#else
		// mathint 32_10
		// 10 bit audio
		ret16=(firfilter_modulate(-256)>> 16);
#endif
#endif
#endif

		// store data
		*p=ret16;
		p++;
	}; // end for
} else {
// bit = 0 -> send +16384 to firfilter
	for (loop=0; loop<20; loop++) {
#if _USEFLOAT == 1
		ret16=(int)(firfilter_modulate(16384));
#else
#if _INTMATH == 64
		// 16 bit audio
		filterret=firfilter_modulate(16384);
		ret16=process_return(filterret);
#else 
#if _INTMATH == 3212
		// 12 bit audio
		ret16=(firfilter_modulate(1024)>> 16);
#else
		// mathint 32_10
		// 10bit audio
		ret16=(firfilter_modulate(256)>> 16);
#endif
#endif
#endif

		// store data
		*p=ret16;
		p++;
	}; // end for
}; // end if

return;

}; // end function modulate1bit


void * funct_s_modulateandbuffer(void * globaldatain) {
//local vars
int loop_bits;
int nextbuffer;

// global data
s_globaldatastr *s_pglobal;
s_pglobal=(s_globaldatastr *) globaldatain;

// audio returns
// we receive 20 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
int16_t audioret[20];

// tempory data
signed char thisdata;
signed char thismask;

// init vars
s_pglobal->waiting_bits=-1;

nextbuffer=s_pglobal->ptr_b_read +1;
if (nextbuffer >= BUFFERSIZE_BITS) {
        nextbuffer=0;
}; // end if


while (FOREVER) {
        // if there something in the bits buffer to process?
        if (s_pglobal->ptr_b_fillup == nextbuffer) {
                // nothing to process, wait for 100 µS 
                s_pglobal->waiting_bits=1;
                usleep(10000);
                continue;
        }; // end if
//fprintf(stderr,"start bits %X \n",nextbuffer);
        

        // reset "waiting" flag
        s_pglobal->waiting_bits=0;


        // yes, we've got something to process
        thisdata=s_pglobal->buffer_bits[nextbuffer];
        thismask=s_pglobal->buffer_bits_mask[nextbuffer];

        if (s_pglobal->octetorderinvert[nextbuffer]) {
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
                                bufferfill_audio(audioret, 20, 1, s_pglobal);
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
                                bufferfill_audio(audioret, 20, 1, s_pglobal);
                        }; // end if

                        // left shift c
                        thisdata >>= 1;
                        thismask >>= 1;
                }; // end for
        }; // end else - if

        // go to next audio-sample
        s_pglobal->ptr_b_read=nextbuffer;

        // find next buffer
        nextbuffer++;
        if (nextbuffer >= BUFFERSIZE_BITS) {
                nextbuffer=0;
        }; // end if
}; // end for

// re-init vars
s_pglobal->waiting_bits=-1;

}; // end function modulate and output




int modulateandout_bits(int * buffer, int size, FILE * fileout, char * return_message) {
	//local vars
	int loop_bit;

	// audio returns
	// we receive 20 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
	int16_t audioret[20];

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
		fwrite(audioret,20*sizeof(uint16_t),1,fileout);
	}; // end for

	return(0);
}; // end function modulate and output


int modulateandout_octets(unsigned char * buffer, int size, FILE * fileout, char * return_message, int reverse) {
	//local vars
	int loop_bits;
	int loop_octets;
	char c;

	// audio returns
	// we receive 20 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
	int16_t audioret[20];

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
				fwrite(audioret,20*sizeof(uint16_t),1,fileout);
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
				fwrite(audioret,20*sizeof(uint16_t),1,fileout);

			}; // end for

		}; // end else - if

		// go to next audio-sample
		p++;

	}; // end for

return(0);
}; // end function modulate and output



