/* INPUT AND PROCESS: CODEC2 format */


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
int framecount=0;
int startsent=0;
int idfilecount=0;
int idinfile; // file

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

	// input has successfully opened.

	// mark "start sent" as 0
	// actually sending the header has been moved down after we have received the first valid data from input
	startsent=0;


	while (!(endoffile)) {
		int loop;
		unsigned char *c1, *c2;
		// note, we expect everything to be read in one go
		retval=input_sal(ISAL_CMD_READ,inbuffer,7,&retval2,retmsg);

		if (retval == ISAL_RET_FAIL) {
			// failure
			fprintf(stderr,"%s",retmsg);
			break;
		}; // end if



		// before sending, check if we already send a "sync pattern"

		if (! startsent) {
		
			// - send sync pattern 128 bit "01010" (twice as what the specifications demand)
			// - send frame syncronisation pattern "0000 0101 0110 0111"

			// everything is bit-inverted

			bufferfill_bits(codec2_startsync_pattern, 1, sizeof(codec2_startsync_pattern)*8, p_s_global);

			// send version (0x11) 3 times, NON inverted
			unsigned char version=0x11;
			bufferfill_bits(&version, 0, 8, p_s_global);
			bufferfill_bits(&version, 0, 8, p_s_global);
			bufferfill_bits(&version, 0, 8, p_s_global);

			// set start sent flag
			startsent=1;
		}; // end start sent?

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

		// success, copy codec2 
		memcpy(&outbuffer[3],inbuffer,7);

		// copy with interleaving
		for (loop=0; loop<=6; loop++) {
			outbuffer[interl2[loop]]=inbuffer[loop];
		}; // end for

		// copy with interleaving
		for (loop=0; loop<=6; loop++) {
			outbuffer[interl3[loop]]=inbuffer[loop];
		}; // end for



		// apply scrambling to make stream more random

		// we have up to 8 known exor-patterns
		framecount &= 0x7;

		c1=&scramble_exor[framecount][0];
		// scrambling chars 3 up to 24 (starting at 0)
		c2=&outbuffer[3];

		// scramble 21 chars
		for (loop=0; loop < 21; loop++) {
			*c2 ^= *c1;
			c2++; c1++;
		}; // end for

		framecount++; // no need to do range-checking, is done above


		// if end-of-file, change marker
		if (endoffile) {
			outbuffer[0]=0x7e; outbuffer[1]=0x80; outbuffer[2]=0xc5;

			// reinit counter "frame line"
			framecount=0;
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

	// play out signature audio-file
	if (p_s_global->idfile) {
		// wait for "bits" queue to end
		bufferwaitdone_bits(p_s_global);

		// only play out idfile every "idfreq" times
		if (idfilecount == 0) {
			int filesize=0;
			ssize_t sizeread;

			unsigned char filebuffer[160];


			idinfile=open(p_s_global->idfile,O_RDONLY);

			if (idinfile) {
				filesize = 0;

				// maximum 3 seconds (300 times 10 ms)
				while (filesize < 300) {
				// read 10 ms (8000 Khz, 16 bit size, mono) = 160 octets
					sizeread=read(idinfile,filebuffer,160);

					if (sizeread < 160) {
					// stop if no audio anymore
						break;
					}; // end if

					// write 80 samples, repeat every sample 6 times (8Khz -> 48 Khz samplerate)
					bufferfill_audio((int16_t *)filebuffer,80,6,p_s_global);

				}; // end while
			}; // end if

		}; // end if

		idfilecount++;



		if (idfilecount >= p_s_global->idfreq) {
			idfilecount=0;
		}; // end if

	}; // signature audio file

	// close was successfull,
	// if we cannot re-open stop here 
	if (retval2 == ISAL_INPUT_REOPENNO) {
		running=0;
		break;
	}; // end if

}; // end while running


// wait for "bits" queue to end
bufferwaitdone_bits(p_s_global);

// end program when all buffers empty
bufferwaitdone_audio(p_s_global);

exit(0);

}; // end function input
