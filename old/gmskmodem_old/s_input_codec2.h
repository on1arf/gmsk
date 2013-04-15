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
unsigned char outbuffer[24]; // 40 ms @ 4.8 bps = 192 bits = 24 octets
char retmsg[ISALRETMSGSIZE]; // return message from SAL function
int framecount=0;
int idfilecount=0;
int idinfile; // file

int status; // 0 = wait for START marker, 1 = receiving data

c2encap_str *c2_marker, *c2_voice; // containers for codec2 encapsulated frames

// init vars
retval=0; retval2=0;

// init outbuffer:
// first 3 chars (sync pattern) are "1110 0111  0000 1000  0101 1100"
// rest is 3 times codec2 frame @ 1400 bps
outbuffer[0]=0xe7; outbuffer[1]=0x08; outbuffer[2]=0x5c;


// allocate memory for received c2encap frame
c2_marker=malloc(sizeof(c2encap_str));

if (!c2_marker) {
	fprintf(stderr,"Error: could not allocate memory for receiving C2ENCAP frames!\n");
	exit(-1);
}; 

// c2_marker and c2_voice all point to same data
c2_voice = c2_marker;


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


	// init status to 0 (wait for "BEGIN" marker)
	status=0;

	while (!(endoffile)) {

		int loop;
		unsigned char *c1, *c2;
		int c2encapreceived;
		int typeofdata=0;
		int noread;


		// read data, first 3 octets must be "0x36f162", the signature of
		// a c2encap frame
		
		c2encapreceived=0;

		noread=0;

		while (!c2encapreceived) {

			// do note read if "noread" set (trying to resync)

			if (! noread) {
				// read 4 octets: signature + type of data
				retval=input_sal(ISAL_CMD_READ,c2_marker,4,&retval2,retmsg);

				// break out on failure
				if (retval == ISAL_RET_FAIL) {
					fprintf(stderr,"%s",retmsg);
					break;
				}; // end id

				// break out when reaching end of file
				if ((retval == ISAL_RET_READ_EOF) || (retval2 < 4)) {
					break;
				}; // end if
			} else {
				noread=0;
			}; // end if


			if (!memcmp(c2_marker,C2ENCAP_HEAD,3)) {
				// header received, get rest of data
				c2encapreceived=1;

				// if data@1400 bps, read 7 more octets
				// if marker, read 3 more octets
				typeofdata=c2_marker->header[3];
				if ((typeofdata == C2ENCAP_MARK_BEGIN) || (typeofdata == C2ENCAP_MARK_END)) {
					retval=input_sal(ISAL_CMD_READ,&c2_marker->c2data,3,&retval2,retmsg);

					// break out on error or EOF
					if (retval == ISAL_RET_FAIL) {
						// failure
						fprintf(stderr,"%s",retmsg);
						break;
					}; // end id

					// break out when reaching end of file
					if ((retval == ISAL_RET_READ_EOF) || (retval2 < 3)) {
						break;
					}; // end if

				} else if (typeofdata == C2ENCAP_DATA_VOICE1400) {
					retval=input_sal(ISAL_CMD_READ,&c2_voice->c2data,7,&retval2,retmsg);

					// break out on error or EOF
					if (retval == ISAL_RET_FAIL) {
						// failure
						fprintf(stderr,"%s",retmsg);
						break;
					}; // end id

					// break out when reaching end of file
					if ((retval == ISAL_RET_READ_EOF) || (retval2 < 7)) {
						break;
					}; // end if

				} else {
					if (p_g_global->verboselevel >= 3) {
						fprintf(stderr,"Warning: C2ENCAP frame received of unknown datatype: %X \n",typeofdata);
					};
				}; // end else - elsif - if

			} else {
				// signature does not match, move up 1 character and try again
				unsigned char *p1, *p2;

				if (p_g_global->verboselevel >= 3) {
					fprintf(stderr,"Warning: C2ENCAP frame received with invalid signature: %X%X%X \n",c2_marker->header[0],c2_marker->header[1],c2_marker->header[2]);
				}; // end if

				p1=&c2_marker->header[0]; p2=p1+1;
				*p1=*p2; p1++; p2++;
				*p1=*p2; p1++; p2++;
				*p1=*p2; 

				// read one more byte
				retval=input_sal(ISAL_CMD_READ,p2,1,&retval2,retmsg);

				// break out on error or EOF
				if (retval == ISAL_RET_FAIL) {
					// failure
					fprintf(stderr,"%s",retmsg);
					break;
				}; // end id

				// break out when reaching end of file
				if ((retval == ISAL_RET_READ_EOF) || (retval2 < 1)) {
					break;
				}; // end if

				// no need to read data on next loop
				noread=1;

			}; // end else - if

		}; // end while (c2encapreceived)


		// we have received a full C2ENC frame, or something went wrong.
		// first some error checking

		/// Error case 1: something weird
		// completely break out in case of error
		if (retval == ISAL_RET_FAIL) {
			break;
		}; // end if

		// Error case 2: c2encapreceived not set, so this means we reached a "EOF" situation
		if (!c2encapreceived) {
			if (retval == ISAL_RET_READ_EOF) {
				endoffile=1;
				if (p_g_global->verboselevel >= 2) {
					fprintf(stderr,"s_input_codec2: REACHED END OF INPUT FILE! \n");
				}; // end if
			} else {
				endoffile=1;
				if (p_g_global->verboselevel >= 2) {
					fprintf(stderr,"s_input_codec2: INPUT FILE TO SHORT \n");
				}; // end if
			}; // end else - if

			// in both cases, overwrite input frame with "MARK END" frame
			c2_marker->header[3]=C2ENCAP_MARK_END;
			memcpy(c2_marker->c2data.c2data_text3,"END",3);
			
		}; // end if


		// OK, we have a valid c2enc frame, process it.
		// What to do now depends on the statemachine

		// status: 0 WAIT FOR "BEGIN" marker
		if (status == 0) {
			// move to status 1, if receiving valid "BEGIN" marker
			if ((typeofdata == C2ENCAP_MARK_BEGIN) &&  (!memcmp(c2_marker->c2data.c2data_text3,"BEG",3))) {
				if (p_g_global->verboselevel >= 2) {
					fprintf(stderr,"s_input_codec2: BEGINNING STREAM! \n");
				}; // end if

				// marker is "begin" marker
				status=1;
			
				// send sync pattern
				// everything is bit-inverted

				bufferfill_bits(codec2_startsync_pattern, 1, sizeof(codec2_startsync_pattern)*8, p_s_global);

				// send version (0x11) 3 times, NON inverted
				unsigned char version=0x11;
				bufferfill_bits(&version, 0, 8, p_s_global);
				bufferfill_bits(&version, 0, 8, p_s_global);
				bufferfill_bits(&version, 0, 8, p_s_global);

			} else {
				if (p_g_global->verboselevel >= 2) {
					fprintf(stderr,"Warning: received C2ENC frame (type %X) of type in status=0.\n",c2_marker->header[3]);
				}; // end if
			}; // end if


		} else if (status == 1) {
		// status: 1 RECEIVING VOICE PACKET
			int frametosent=0;

			// determine if we have data to sent

			if (typeofdata == C2ENCAP_DATA_VOICE1400) {

				frametosent=1;
			} else if (typeofdata == C2ENCAP_MARK_END) {
				// end of file is reached
				endoffile=1;
				frametosent=1;
			} else {
				if (p_g_global->verboselevel >= 2) {
					fprintf(stderr,"Warning: received C2ENC frame (type %X) of type in status=1.\n",c2_marker->header[3]);
				}; // end if
			}; // end else 
			

			if (frametosent) {
				// prepare frame
				memcpy(&outbuffer[3],&c2_voice->c2data,7);

				// copy with interleaving
				for (loop=0; loop<=6; loop++) {
					outbuffer[interl2[loop]]=c2_voice->c2data.c2data_data7[loop];
				}; // end for

				// copy with interleaving
				for (loop=0; loop<=6; loop++) {
					outbuffer[interl3[loop]]=c2_voice->c2data.c2data_data7[loop];
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
					if (p_g_global->verboselevel >= 2) {
						fprintf(stderr,"s_input_codec2: END SYNC set! \n");
					}; // end if
					outbuffer[0]=0x7e; outbuffer[1]=0x80; outbuffer[2]=0xc5;

					// reinit counter "frame line"
					framecount=0;
				}; // end if (endoffile)

			}; // end if (frametosent)


			// OK, now sent frame
			// write 192 bits (= 24 octets), inverted
				bufferfill_bits(outbuffer, 0, 192,p_s_global);

		}; // end (status = 1)

	}; // end while (!endoffile)



	// end of file has been reached

	// repeat last final frame 2 more time
	// only if it ever was send (status ever reached value 1)
	if (status) {
		// file done, write two extra end-frame
		bufferfill_bits(outbuffer, 0, 192,p_s_global);
		bufferfill_bits(outbuffer, 0, 192,p_s_global);
	}; // end if

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

	// reinit outbuffer:
	// first 3 chars (sync pattern) are "1110 0111  0000 1000  0101 1100"
	// rest is 3 times codec2 frame @ 1400 bps
	outbuffer[0]=0xe7; outbuffer[1]=0x08; outbuffer[2]=0x5c;
}; // end while running


// wait for "bits" queue to end
bufferwaitdone_bits(p_s_global);

// end program when all buffers empty
bufferwaitdone_audio(p_s_global);

exit(0);

}; // end function input
