/* INPUT AND PROCESS: RAW format */


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

// Release information:
// 20120105: Initial release

// This is split of in a seperate thread as this may run in parallel
// with the interrupt driven "alsa out" function which can "starve"
// the main application of CPU cycles

void * funct_input_codec2 (void * c_globaldatain) {

// global data
c_globaldatastr * p_c_global;
s_globaldatastr * p_s_global;
g_globaldatastr * p_g_global;

p_c_global=(c_globaldatastr *) c_globaldatain;

p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;


// local vars
int retval, retval2;
int running, endoffile;
unsigned char inbuffer[7]; // 40 ms @ 1.4 bps = 56 bits = 7 octets
unsigned char outbuffer[24]; // 40 ms @ 4.8 bps = 192 bits = 24 octets
char retmsg[ISALRETMSGSIZE]; // return message from SAL function

// check for correct format
if (p_g_global->format != 20) {
	fprintf(stderr,"Error: wrong format in input_codec2: expected format 20, got %d\n",p_g_global->format);
}; // end if

// init vars
retval=0; retval2=0;

// init outbuffer:
// first 3 chars (sync pattern) are "1110 0111  0000 1000  0101 1100"
// rest is 3 times codec2 frame @ 1400 bps
outbuffer[0]=0xe7; outbuffer[1]=0x08; outbuffer[2]=0x5c;


// endless loop, in case of endless data input
running=1;

// init input source abstraction layer
input_sal(ISAL_CMD_INIT,c_globaldatain,0,&retval2,retmsg);

while (running) {
	// open
	retval=input_sal(ISAL_CMD_OPEN, NULL, 0, &retval2, retmsg);

	if (retval != ISAL_RET_SUCCESS) {
		// open failed
		fprintf(stderr,"%s",retmsg);
		running=0;
		break;
	}; // end if

	// read from input until end of file
	endoffile=0;

	// file has successfully opened.

	// - send sync pattern 128 bit "01010" (twice as what the specifications demand)
	// - send frame syncronisation pattern "0000 0101 0110 0111"

	// everything is bit-inverted

	bufferfill_bits(codec2_startsync_pattern, 1, sizeof(codec2_startsync_pattern)*8, p_s_global);

	// send version (0x11) 3 times, NON inverted
	unsigned char version=0x11;
	bufferfill_bits(&version, 0, 8, p_s_global);
	bufferfill_bits(&version, 0, 8, p_s_global);
	bufferfill_bits(&version, 0, 8, p_s_global);


	while (!(endoffile)) {
		// note, we expect everything to be read in one go
		retval=input_sal(ISAL_CMD_READ,inbuffer,7,&retval2,retmsg);

		if (retval == ISAL_RET_FAIL) {
			// failure
			fprintf(stderr,"%s",retmsg);
			break;
		}; // end if


		if (retval == ISAL_RET_READ_EOF) {
			// set "eof" marker but continue;
			// this will then drop out the while look after writing data
			endoffile=1;
		}; // end if

		// did we receive all data?
		if (retval2 < 7) {
			// set "eof" marker
			endoffile=1;

			// fill up rest of packet with "0x00"

			// as index of "buffer" starts at 0, buffer[retval2] is actually first octet
			// AFTER all data that has been received
			memset(&inbuffer[retval2],0x00,(7-retval2));
		}; // end if

		// success, copy codec2 frame 3 times
		memcpy(&outbuffer[3],inbuffer,7);
		memcpy(&outbuffer[10],inbuffer,7);
		memcpy(&outbuffer[17],inbuffer,7);


		// if end-of-file, change marker
		if (endoffile) {
			outbuffer[0]=0x7e; outbuffer[1]=0x80; outbuffer[2]=0xc5;
		}; // end if

		// write 192 bits (= 24 octets), inverted
		bufferfill_bits(outbuffer, 0, 192,p_s_global);

	}; // end while

	// file done, write two extra end-frame
	bufferfill_bits(outbuffer, 0, 192,p_s_global);
	bufferfill_bits(outbuffer, 0, 192,p_s_global);


	// close input file
	retval=input_sal(ISAL_CMD_CLOSE,NULL,0,&retval2,retmsg);

	// error in close?
	if (retval != ISAL_RET_SUCCESS) {
		fprintf(stderr,"%s",retmsg);
		running=0;
		break;
	}; // end if	

	// close was successfull,
	// if we cannot re-open stop here
	if (retval2 == ISAL_INPUT_REOPENNO) {
		running=0;
		break;
	}; // end if

}; // end while


// wait for "bits" queue to end
bufferwaitdone_bits(p_s_global);

// end program when all buffers empty
bufferwaitdone_audio(p_s_global);

exit(0);

}; // end function input
