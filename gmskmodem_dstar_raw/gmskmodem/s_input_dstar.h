/* INPUT AND PROCESS: DVTOOL format */


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

// This version processes both the "DVtool file" format and the 
// D-STAR STREAM format

// Difference is as follows:
// - DVtool file-format begins with a 10 octet header, containing a 6 octet signature and
// 	a 4 octet length indication
// - Every frame in the DVtool file-format begins with a 2 octet length indication

void * funct_input_dstar (void * c_globaldatain) {

// global data
c_globaldatastr * p_c_global;
s_globaldatastr * p_s_global;
g_globaldatastr * p_g_global;

p_c_global=(c_globaldatastr *) c_globaldatain;

p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;


// local vars
int retval, retval2;
int running;
char retmsg[ISALRETMSGSIZE]; // return message from SAL function
int dvtfile;
int inputopen;
int stop;


// DVtool data structures
str_dvtoolheader dvtoolheader;
str_configframe configframe;
str_dvframe dvframe;

// vars for header encode
char *p;
char c;
int bitcounter;
int loop;
int buffer[660], buffer2[660];
unsigned char buffer_c[83];

// vars for reading data frames
int totalframecounter;
int framecounter;
int endsend;

// check for correct format
if ((p_g_global->format != 1) && (p_g_global->format != 2)) {
	fprintf(stderr,"Error: wrong format in input_dvt: expected format 1 or 2, got %d\n",p_g_global->format);
}; // end if


if (p_g_global->format == 1) {
	dvtfile=1;
} else {
	dvtfile=0;
}; // end if

// init vars
retval=0; retval2=0;

// endless loop, in case of endless data input
running=1;

// init input source abstraction layer
retval=input_sal(ISAL_CMD_INIT,c_globaldatain,0,&retval2,retmsg);

if (retval != ISAL_RET_SUCCESS) {
	fprintf(stderr,"%s",retmsg);
	return(NULL);
}; // end if



while (running) {
	// init vars
	inputopen=0;


	// fake do-while loop, just to allow easy use of "break" to jump out of sequence
	do {

		// OPEN FILE
		retval=input_sal(ISAL_CMD_OPEN, NULL, 0, &retval2, retmsg);

		if (retval != ISAL_RET_SUCCESS) {
			// open failed
			fprintf(stderr,"%s",retmsg);
			break;
		}; // end if
		// set "inputopen" to 1, so it can be closed afterwards
		inputopen=1;



		// Part1: process header

		// only for DVTOOL file format

		if (dvtfile) {

			// read header
			retval=input_sal_readall((void *)&dvtoolheader,10,&retval2,retmsg);

			if (retval == ISAL_RET_FAIL) {
				// failure
				fprintf(stderr,"%s",retmsg);
				break;
			}; // end if

			if (retval == ISAL_RET_READ_EOF) {
				// End-of-File
				fprintf(stderr,"Error: unexpected end-of-file while reading DVTOOL signature!\n");
				break;
			}; // end if


			// check signature 

			retval=memcmp(&dvtoolheader.headertext,"DVTOOL",6);

			if (retval) {
				fprintf(stderr,"Error: did not find DVTOOL signature in input file!\n");
				break;
			}; // end if

		}; // end if (DVTOOL-file format)


		// At this point, we are pretty sure we have a valid dvtool file.
		// start creating the syncronisation pattern and send it

		// 128 bit pattern containing "1010... " (which is twice what
		// the specification demands, as that only requires 64 bits), followed
		// by the frame syncronisation pattern "1110110 01010000"

		// send with bit-inversion
		// NOTE, bufferfill_bits length is in BITS, not octets
		bufferfill_bits(dstar_startsync_pattern, 1, sizeof(dstar_startsync_pattern)*8, p_s_global);

		// wait for bits buffer empty
		bufferwaitdone_bits(p_s_global);


		// Part 2: configuration frame

		// read dvtool header configuration frame

		// read frame
		// DVTOOL-file format: read 58 octets
		// DSTAR stream format: read 56 octets (no header length)
		if (dvtfile) {
			retval=input_sal_readall(&configframe,58,&retval2,retmsg);
		} else {
			retval=input_sal_readall(&(configframe.headertext),56,&retval2,retmsg);
		}; // end else - if

		if (retval == ISAL_RET_FAIL) {
			// failure
			fprintf(stderr,"%s",retmsg);
			break;
		}; // end if
		
		if (retval == ISAL_RET_READ_EOF) {
			// End-of-File
			fprintf(stderr,"Error: unexpected end-of-file while reading DVTOOL configuration frame!\n");
			break;
		}; // end if


		// check length field, only for DVTool-file format
		if ((dvtfile) && (configframe.length != 56)) {
			fprintf(stderr,"Error: incorrect dvframe length for config frame got %d expected 56\n",configframe.length);
		}; // end if

		// check header signature
		retval=memcmp(&configframe.headertext,"DSVT",4);

		if (retval) {
			fprintf(stderr,"Error: did not find correct signature in configuration frame!\n");
			break;
		}; // end if


		// check config-or-data
		if (configframe.configordata != 0x10) {
			fprintf(stderr,"Error: incorrect Config-or-Data field in DVTOOL configuration frame in input file. Got %02X, expected 0x10\n",configframe.configordata);
			break;
		}; // end if

		// check data-or-voice
		if (configframe.dataorvoice != 0x20) {
			fprintf(stderr,"Error: DVTOOL configuration frame in input file not marked as voice. Got %02X, expected 0x20\n",configframe.dataorvoice);
			break;
		}; // end if

		// check frame counter
		if (configframe.framecounter != 0x80) {
			fprintf(stderr,"Error: DVTOOL configuration frame in input file has incorrect framecounter. Got %02X, expected 0x80\n",configframe.framecounter);
			break;
		}; // end if

		// header is OK, rewrite it if requested
		// rewrite header
//		rewriteheader(&configframe);

		// header is now good, encode it

		// convert octets into bits
		// init vars
		p=(char *) &configframe.flags[0];
		c=*p;
		bitcounter=0;

		for (loop=0;loop<328;loop++) {
			if (c & bit2octet[bitcounter]) {
				buffer[loop]=1;
			} else {
				buffer[loop]=0;
			}; // end else - if
			bitcounter++;

			if (bitcounter >= 8) {
				p++;
				c=*p;
				bitcounter=0;
			}; // end if
		}; // end for

		// 2 last bits are zero
		buffer[328]=0; buffer[329]=0;


		// FEC encoding
		retval=FECencoder(buffer,buffer2);

		// interleaving
		interleave(buffer2,buffer);

		// scrambling
		scramble_andconvertooctets(buffer,buffer_c);

		// send encoded header
		// note: bufferfill_bits length = BITS
		bufferfill_bits(buffer_c, 1, 660, p_s_global); 


		// part 3: read rest of dvtool file, frame per frame

		// init vars
		totalframecounter=0;
		framecounter=0;
		endsend=0;

		// endless loop, break out with "break"
		while (FOREVER) {
			// init vars
			stop=0;

			// read next frame
			if (dvtfile) {
				retval=input_sal_readall(&dvframe,29,&retval2,retmsg);
			} else {
				retval=input_sal_readall(&(dvframe.headertext),27,&retval2,retmsg);
			}; // end else - if

			if (retval == ISAL_RET_FAIL) {
				// failure
				fprintf(stderr,"%s",retmsg);

				endsend=0;			// endsend=0: still need to send frame with first part
										// of endmarker, followed by frame with 2nd marker
				break;
			};


			// check frame
			// check length, only for DVtool-file format
			if ((dvtfile) && (dvframe.length != 27)) {
				fprintf(stderr,"Error: incorrect dvframe length for frame %d, got %d expected 27\n",totalframecounter,dvframe.length);
				endsend=0;		// endsend=0: still need to send frame with first part
									// of endmarker, followed by frame with 2nd marker
				break;
			}; // end if
        
			// check signature
			if (memcmp(&dvframe.headertext,"DSVT",4)) {
				fprintf(stderr,"Error: incorrect dvframe signature for frame %d\n",totalframecounter);

				endsend=0;		// endsend=0: still need to send frame with first part
									// of endmarker, followed by frame with 2nd marker
				break;
			}; // end if

			// config or data
			if (dvframe.configordata != 0x20) {
				fprintf(stderr,"Error: Config-or-data of dvframe %d not correct. Got %02X, expected 0x20\n",totalframecounter,dvframe.configordata);

				endsend=0;		// endsend=0: still need to send frame with first part
									// of endmarker, followed by frame with 2nd marker
				break;
			}; // end if

			// check framecounter
			if ((dvframe.framecounter & 0x1f) != framecounter) {
				fprintf(stderr,"Warning: Framecounter of dvframe %d not correct. Got %d, expected %d\n",totalframecounter,(dvframe.framecounter & 0x1f),framecounter); 
				// correct framecounter
				framecounter=dvframe.framecounter & 0x1f;
			}; // end if

			if (framecounter == 0) {
				// frame 0: slow-data should contain sync-pattern (0x554d16) and no scrambling
				if (p_s_global->sync) {
				// if "resync" option is set, overwrite sync pattern
					dvframe.slowspeeddata[0] = 0x55;
					dvframe.slowspeeddata[1] = 0x4d;
					dvframe.slowspeeddata[2] = 0x16;
				}; // end if
			} else {
				// not frame 0
				if (p_s_global->zap) {
					// zap: clear slow-data part
					// rewrite with 0x000000, but also apply scrambling
					dvframe.slowspeeddata[0] = 0x70;
					dvframe.slowspeeddata[1] = 0x4f;
					dvframe.slowspeeddata[2] = 0x93;
				} else {
					// apply scrambling
					dvframe.slowspeeddata[0] ^= 0x70;
					dvframe.slowspeeddata[1] ^= 0x4f;
					dvframe.slowspeeddata[2] ^= 0x93;
				}; // end else - if
			}; // end if

			// is it the last frame (6th bit of framecounter is set)
			if (dvframe.framecounter & 0x40) {
				// mark "stop", but do not break out yet
				stop=1;

				// endsend=1 -> frame with first part of end marker send
				// need to send one frame with 2nd part 
				endsend=1;

				// replace slowspeeddata with "end" marker
				dvframe.slowspeeddata[0] = 0xfe;
				dvframe.slowspeeddata[1] = 0xaa;
				dvframe.slowspeeddata[2] = 0xaa;
			}; 

			// modulate and send, no reverse (as the .dvtool file is already reversed)
			// note: bufferfill_bits length = bits -> so 96 bits
			bufferfill_bits((unsigned char *) &dvframe.ambe[0], 0, 96, p_s_global);

			// if stop (last frame) break out of loop here;
			if (stop) {
				break;
			}; // end if

			totalframecounter++;

			framecounter++;

			// reset framecounter every 21 frames
			if (framecounter > 20) {
				framecounter=0;
			}; // end if
        
		}; // end loop forever reading frames


		// part 4: send "end" markers

		// check "endsend"

		// case 1: endsend = 0
		if (endsend < 1) {
			// endsend = 0 -> first frame with first part of endmarker
			// send fake frame

			// dummy frame
			dvframe.ambe[0]=0xfe; dvframe.ambe[1]=0xaa; dvframe.ambe[2]=0xaa; 
			dvframe.ambe[3]=0xfe; dvframe.ambe[4]=0xaa; dvframe.ambe[5]=0xaa;
			dvframe.ambe[6]=0xfe; dvframe.ambe[7]=0xaa; dvframe.ambe[8]=0xaa; 

			// first part of end marker: slowdata contains 0xfeaaaa
			dvframe.slowspeeddata[0] = 0xfe;
			dvframe.slowspeeddata[1] = 0xaa;
			dvframe.slowspeeddata[2] = 0xaa;


			// modulate and send, no reverse (as the .dvtool file is already reversed)
			// note: bufferfill_bits length = bits -> so 96 bits
			bufferfill_bits((unsigned char *) &dvframe.ambe[0], 0, 96, p_s_global);
		};

		// case 2: endsend = 1
		if (endsend <= 1) {
			// send part of end marker: ambe[0] up to ambe[2] contain 0xaa135e
			dvframe.ambe[0]=0xaa; dvframe.ambe[1]=0x13; dvframe.ambe[2]=0x5e; 

			// rest of frame (not specified in formal specs) contain 0xaa
			dvframe.ambe[3]=0xaa; dvframe.ambe[4]=0xaa; dvframe.ambe[5]=0xaa; 
			dvframe.ambe[6]=0xaa; dvframe.ambe[7]=0xaa; dvframe.ambe[8]=0xaa; 

			bufferfill_bits((unsigned char *) &dvframe.ambe[0], 0, 96, p_s_global);
		}

	} while (0); // fake do-while loop, just to allow easy use of "break" to jump out of sequence


	if (inputopen)  {
		// close input file
		retval=input_sal(ISAL_CMD_CLOSE,NULL,0,&retval2,retmsg);

		// error in close?
		if (retval != ISAL_RET_SUCCESS) {
			fprintf(stderr,"%s",retmsg);
			running=0;
			break;
		}; // end if
	}; // end if

	// if reopen is not possible, stop here
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
