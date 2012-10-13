/* processaudio.h */

// High level functions to process audio, received from an ALSA device
// The actual audio is received from the ringbuffer, fed by the "capture" function


// version 20111107: initial release
// version 20111110: read from file, dump bits, dump amplitude
// version 20111112: support for stereo files
// version 20111130: end stream on signal drop. do not process noise data,
//                   accept bits at startsync and descrable slow data
// version 20120105: raw mode audio output
// version 20120529: moved raw code to seperate file

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


void * funct_r_processaudio_raw (void * c_globaldatain ) {


// vars

int16_t audioin;

int nextbuffer;
int thisbuffer;
int thisbuffersize;
int thisfileend;
int thisaudioaverage;

int sampleloop;
int16_t * audiobuffer;

int bit, state;

int countnoiseframe=0; // number of frames not processed due to average audio level > noise

int hadtowait;

int syncmask;

uint16_t last2octets;
int syncreceived, syncFFreceived;

int loop;
int retval, retval2=0;
char retmsg[ISALRETMSGSIZE];
int outputopen=0;

int bitmatch;


int16_t *samplepointer;
int channel;
int buffermask;


// vars to calculate "maximum audio level" for a valid stream (i.e. no noise)
int maxaudiolevelvalid=0;
int framesnoise=0;
// ringbuffer to store last 32 values of average audio level
// used to detect false-positive start-of-stream for raw streams
int averageleveltable[32];
int averagelevelindex;
int averagevalid;
uint64_t averagetotal;
int maxlevel;


// raw buffer
unsigned char rawbuffer[12]; // 96 bits = 12 octets = 20 ms of audio
unsigned char *p_rawbuffer=NULL; 
int rawbuffer_octet=0, rawbuffer_bit=0; // rawbuffer octet and bit counter
int rawcarrierdrop;

// temp values to calculate maximum audio level
int64_t maxaudiolevelvalid_total=0;
int maxaudiolevelvalid_numbersample=0;

// output format:
int thisformat;

c_globaldatastr * p_c_global;
r_globaldatastr * p_r_global;
g_globaldatastr * p_g_global;

p_c_global=(c_globaldatastr *) c_globaldatain;

p_r_global=p_c_global->p_r_global;
p_g_global=p_c_global->p_g_global;



// init vars
state=0;
syncreceived=0; syncFFreceived=0;
last2octets=0;

rawcarrierdrop=0;

for (loop=0;loop<32;loop++) {
	averageleveltable[loop]=0;
}; // end if
averagelevelindex=0;




// thisformat: 1: dvtool-file format, 2: DSTAR stream, 10: raw, 20: codec2
if (p_r_global->recformat) {
	thisformat=p_r_global->recformat;
} else {
	thisformat=p_g_global->format;
}; // end if


if (thisformat != 10) {
	fprintf(stderr,"Error in processaudio_dstar, invalid format. Expected 10, got %d\n",thisformat);
	exit(-1);
}; // end if



// init OUTPUT DESTINATION ABSTRACTION LAYER
retval=output_dal(IDAL_CMD_INIT,p_c_global,0,&retval2,retmsg);

if (retval != IDAL_RET_SUCCESS) {
	// init failed
	fprintf(stderr,"%s",retmsg);
	exit(-1);
}; // end if


// sync mask: depends on syncsize
// should contain valid between 1 and 16.
// check just to be sure
if ((p_r_global->syncsize < 1) || (p_r_global->syncsize > 16)) {
	fprintf(stderr,"Error in processaudio, rawsynsize should be between 1 and 16, got %d. Exiting!\n",p_r_global->syncsize);
	exit(-1);
}; // end if
syncmask=size2mask[p_r_global->syncsize-1];


// precalculate values to avoid having to do the calculation every
// itteration
if (p_r_global->stereo) {
	channel=2;
	buffermask=0x7f;
} else {
	channel=1;
	buffermask=0xff;
}; // end if




// init var
countnoiseframe=0;
hadtowait=0;



// init thisfileend. For ALSA capturing, there is no file ending; so we
// just thread this as a endless file
thisfileend=0;


// endless loop;
while (!(thisfileend)) {

	// is there something to process?

	// check the next buffer slot. If the "capture" process is using it, we do not have any
	// audio available. -> wait and try again
	nextbuffer=(p_r_global->pntr_process + 1) & buffermask;

	if (p_r_global->pntr_capture == nextbuffer) {
		// nothing to process: sleep for 1/4 of ms (= 5 ms)
		usleep((useconds_t) 250);

		// set "had to wait" marker
		hadtowait=1;
		continue;
	};

	// OK, we have a frame with audio-information, process it, sample
	// per sample
	thisbuffer=nextbuffer;
	audiobuffer= (int16_t *) p_r_global->buffer[thisbuffer];
	thisbuffersize=p_r_global->buffersize[thisbuffer];
	thisaudioaverage=p_r_global->audioaverage[thisbuffer];

	#ifdef _USEALSA
		if (p_r_global->fileorcapture) {
			thisfileend=p_r_global->fileend[thisbuffer];
		}; // end if
	#else
		thisfileend=p_r_global->fileend[thisbuffer];
	#endif


	// PART1:
	// things to do at the beginning of every block of data

	// dump average if requested
	if (p_r_global->dumpaverage) {
		printf("(%04X)",thisaudioaverage);
	}; // end if


	// sample was received when modem was transmitting. Ignore it
	if (p_r_global->sending[thisbuffer]) {
		// sleep for 10 ms (half of one audiosample), only when reading from ALSA device
		// and we have he "had to wait" marker set

		#ifdef _USEALSA
		if ((hadtowait) && (p_r_global->fileorcapture == 0)) {	
			usleep((useconds_t) 10000);
		}; // end if
		#endif

		// reset "had to wait"
		hadtowait=0;
	
		// go to next audio frame
		p_r_global->pntr_process = nextbuffer;
		continue;
	}; // end if


	// store average audio-level values in  ringbuffer; to be used
	// later to detect false-positive start-of-stream
	averagelevelindex++;
	averagelevelindex &= 0x1f; // wrap from 0 to 31
	averageleveltable[averagelevelindex]=thisaudioaverage;
	

	// do we process the audio ?
	// if state is 0 (waiting for sync), and input audio-level is to high
	// (i.e. received signal is noise), we skip the complete frame
	if ((state == 0) && (maxaudiolevelvalid) && (p_r_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
		// skip audio frame

		// how many frames did we already skip?
		countnoiseframe++;

		// if to much (more then 900 seconds), clear "maxaudiolevel", just to be sure
		if (countnoiseframe > MAXNOISEREJECT) {
			maxaudiolevelvalid=0;
		}; // end if

		// sleep for 10 ms (half of one audiosample), only when reading from ALSA device
		// and we have he "had to wait" marker set

		#ifdef _USEALSA
		if ((hadtowait) && (p_r_global->fileorcapture == 0)) {	
			usleep((useconds_t) 10000);
		}; // end if
		#endif

		// reset "had to wait"
		hadtowait=0;
	
		// go to next audio frame
		p_r_global->pntr_process = nextbuffer;
		continue;
	};


	// if state 3 (raw data receive) and audio-level is to high (i.e. we receive noise), this is seen
	// as a end-of-stream situation
	if (maxaudiolevelvalid) {
		if (thisaudioaverage> maxaudiolevelvalid) {
			rawcarrierdrop++;

			if (rawcarrierdrop >= 3) {

				if (p_g_global->verboselevel >= 1) {
					fprintf(stderr,"RAW STREAM END.\n\n\n");
				}; // end if

				// if raw buffer already started, fill in rest of remaining raw buffer with all 0
				// and send out
				if ((rawbuffer_octet) || (rawbuffer_bit)) {
					// fill in rest of current octet

					// raw invert = 1 -> first bit is on left of octet (MSB)
					// raw invert = 0 -> first bit is on right of octet (LSB)
					if (p_g_global->rawinvert) {
						while (rawbuffer_bit < 8) {
							*p_rawbuffer<<=1;
							rawbuffer_bit++;
						}; // end while

						// move up pointer
						rawbuffer_octet++;
						p_rawbuffer++;
					} else {
						while (rawbuffer_bit < 8) {
							*p_rawbuffer>>=1;
							rawbuffer_bit++;
						}; // end if

						rawbuffer_octet++;
						p_rawbuffer++;
					}; // end if (rawinvert?)

					// fill in rest of octets with all zero
					while (rawbuffer_octet < 12) {
						*p_rawbuffer=0;
						p_rawbuffer++;
						rawbuffer_octet++;
					}; // end while
				}; // end if (blank rest of buffer)


				// now send and/or write raw buffer
				if (outputopen) {
					retval=output_dal(IDAL_CMD_WRITE,&rawbuffer,12,&retval2, retmsg);
					if (retval != IDAL_RET_SUCCESS) {
						// write failed
						fprintf(stderr,"%s",retmsg);
						break;
					}; // end if
				}; // end if


				// write raw-endstream marker
				if ((outputopen) && (p_r_global->sendmarker)) {
					retval=output_dal(IDAL_CMD_WRITE,"RAWSTREAMEND.\n",15,&retval2,retmsg);

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
						break;
					}; // end if
				}; // end if

				// close output
				if (outputopen) {
					retval=output_dal(IDAL_CMD_CLOSE,NULL,0,&retval2, retmsg);
					if (retval != IDAL_RET_SUCCESS) {
						// close failed
						fprintf(stderr,"%s",retmsg);
					}; // end if
					outputopen=0;
				}; // end if

				// re-init vars
				state=0;

				// go to next audio frame
				p_r_global->pntr_process = nextbuffer;
				continue;
			}; // end if (carrierdrop >= 3)
		} else {
			// no carrier drop. Reset counter
			rawcarrierdrop=0;
		};
	}; // end if (maxaudiolevelvalid)

	// at this point, we have a valid frame without noise

	// reset countnoiseframe
	countnoiseframe=0;

	// reset "had to wait" flag
	hadtowait=0;


	// calculate noiselevel based on first 8 audio groups
	// (= 160 ms, slightly less then size of header for D-STAR and easier to calculate
	// without needing floats

	if (maxaudiolevelvalid_numbersample < 8) {
		maxaudiolevelvalid_total += p_r_global->audioaverage[thisbuffer];
		maxaudiolevelvalid_numbersample++;

		// did we now receive sufficient samples ?
		if (maxaudiolevelvalid_numbersample >= 8) {
			// shift right 3 = divide by 8
			maxaudiolevelvalid = (maxaudiolevelvalid_total >> 3) * MAXAUDIOLEVEL_MARGIN;
		}; // end if
		
	} else {
		if ((maxaudiolevelvalid) && (p_r_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
			framesnoise++;
		} else {
			framesnoise=0;
		}; // end else - if
	}; // end else - if



	// process individual samples in buffer

	samplepointer=audiobuffer;
	for (sampleloop=0; sampleloop <thisbuffersize; sampleloop++) {
		// read audio:

		// For mono: read mono channel
		// For stereo: read left channel
		audioin=*samplepointer;

		//move up pointer one (mono) or two (stereo). 
		samplepointer += channel;


		bit=demodulate(audioin);

		// the demodulate function returns three possible values:
		// -1: not a valid bit
		// 0 or 1, valid bit

		if (bit < 0) {
			// not a valid bit
			continue;
		}; // end if

		// "State" variable:
		// state 0: waiting for sync "10101010 1010101" (bit sync) or
		// 	"1110110 01010000" (frame sync)
		// state 10: receiving RAW

		// state 0
		if (state == 0) {
			if (p_r_global->dumpstream >= 2) {
				printbit(bit,6,2,0);
			}; // end if

			// keep track of last 16 bits
			last2octets<<=1;
			if (bit) {
				last2octets |= 0x01;
			}; // end if

			// according the specifications, the syncronisation pattern is 64 times "0101", however there also
			// seams to be cases where the sequence "1111" is send.
			if (last2octets == 0xffff) {
				syncFFreceived += 3;
			} else {
				if (syncFFreceived > 0) {
					syncFFreceived--;
				}; // end if
			}; 

			if ((last2octets == 0x5555) || (last2octets == 0xaaaa)) {
				syncreceived += 3;
			} else {
				if (syncreceived > 0) {
					syncreceived--;
				}; // end if
			}; 

			if ((syncreceived > 20)  || (syncFFreceived > 20)) {
			// start looking for frame sync if we have received sufficient bitsync

				// if we have received a pattern with all "1010", we accept up to
				// "BITERROR START SYN" penality points for errors

				// if we have received a pattern with all "1111", we are more strict
				// and only accept a full match
				// syncmask is initialised above, based on syncsize
				if (syncreceived > 20) {
					bitmatch=countdiff16_fromlsb(last2octets,syncmask,p_r_global->syncsize,
						p_r_global->syncpattern,BITERRORSSTARTSYN);
				} else {
					bitmatch=countdiff16_fromlsb(last2octets,syncmask,p_r_global->syncsize,
						p_r_global->syncpattern,0);
				}; // end if



				if (bitmatch) {
					int p;
					// OK, we have a valid frame sync, go to state 1 (dvtool file-format or D-STAR UDP stream) or 10 (raw)

					// Additional check: compair average audio level with average of 16 up to 31 samples
					// ago (when the signal should contain noise). The current audio-level should be at
					// least 18.75 % (13/16) below that "noise level"

					// init vars
					averagevalid=1;
					averagetotal=0;

					// the averagelevel table is a 32 wide ringbuffer
					// the index points (N) to the last value added in the buffer; so the value N+1
					// is actually the value N-31
					p=averagelevelindex;

					for (loop=0; loop<16;loop++) {
						int thisaverage;
						p++;
						p &= 0x1f;

						thisaverage=averageleveltable[p];

						// average value = 0? Not valid
						if (!thisaverage) {
							averagevalid=0;
						} else {
							averagetotal += thisaverage;
						}; // end if
					}; // end for

					
					if (averagevalid) {
						maxlevel = (averagetotal >> 8) * 13; // >>8=/256: divide by 16 (for 16 samples) and
																		// then a 2nd time for get 13/16 of average total

						if (thisaudioaverage < maxlevel) {
							if (p_g_global->verboselevel >= 1) {
								fprintf(stderr,"START STREAM: Average audio level: %04X, max: %04X\n",thisaudioaverage,maxlevel);
							}; // end if
						} else {
							if (p_g_global->verboselevel >= 1) {
								fprintf(stderr,"START STREAM CANCELED - noiselevel %04X to high (max %04X)\n",thisaudioaverage, maxlevel);
								continue;
							}; // end if
						}; // end if
					} else {
						if (p_g_global->verboselevel >= 1) {
							fprintf(stderr,"START STREAM CANCELED - not yet enough data for noiselevel test\n");
							continue;
						}; // end if
					}; // end if


					// go to new state: 10 (raw)
					state=10;

					// init buffervars
					rawbuffer_octet=0;
					rawbuffer_bit=0;

					p_rawbuffer=rawbuffer;


					// reset print-bit to beginning of stream
					if (p_r_global->dumpstream >= 2) {
						putchar(0x0a); // ASCII 0x0a = linefeed
					}; // end if
					printbit(-1,0,0,0);

					// store first data of audiolevel to calculate average
					maxaudiolevelvalid_total=p_r_global->audioaverage[thisbuffer];
					maxaudiolevelvalid_numbersample=1;




					// raw: open Destination Abstraction Layer and WRITE

					// write sync pattern + sync frame 
					// problem, we do not have the sync pattern, so we will recreate it
					// based on 128 times "01" plus the frame sync pattern

					// 256 times "01" = 64 octets + maximum 16 bit frame-sync						
					unsigned char syncbuffer[130];
					uint16_t mask;
					uint16_t syncpattern;

					// the end of the "0101"-pattern should match exactly the beginning of the
					// frame sync. So the pattern of the first 64 octets depends on the size
					// of the sync frame, either "10101010" if size of synpattern is even
					// or "01010101" it if odd. In that case, an additional 0 is added in front

					if (p_r_global->syncsize & 0x01) {
						// odd
						if (!p_g_global->rawinvert) {
							// invert bit order
							memset(syncbuffer,0xaa,128);
						} else {
							// no invert
							memset(syncbuffer,0x55,128);
						}; // end else - if

						// do not invert mask (will be done later)
						mask=0x5555 & ~size2mask[(p_r_global->syncsize) -1];
					} else {
						// even
						if (!p_g_global->rawinvert) {
							// invert bit order
							memset(syncbuffer,0x55,128);
						} else {
							// no invert
							memset(syncbuffer,0xaa,128);
						}; // end else - if

						// do not invert mask (will be done later)
						mask=0xaaaa & ~size2mask[(p_r_global->syncsize) -1];
					}; // end else - if (odd or even)

					// create last 2 octets, always non-inverted at this stage
					syncpattern=p_r_global->syncpattern & size2mask[(p_r_global->syncsize) -1];
					syncpattern |= mask;

					// copy of last 2 octets of buffer
					if (!p_g_global->rawinvert) {
					// invert
						syncbuffer[128]=(unsigned char) (invertbits[(syncpattern >> 8) & 0xFF]);
						syncbuffer[129]=(unsigned char) (invertbits[syncpattern & 0xFF]);
					} else {
					// no invert
						syncbuffer[128]=(unsigned char) (syncpattern >> 8) & 0xFF;
						syncbuffer[129]=(unsigned char) syncpattern & 0xFF;
					}; // end else - if


					// open Output Destination Abstraction layer
					if (!(outputopen)) {
						retval=output_dal(IDAL_CMD_OPEN,NULL,0,&retval2,retmsg);

						if (retval == IDAL_RET_SUCCESS) {
							outputopen=1;
						} else {
							// open failed
							fprintf(stderr,"%s",retmsg);
					}; // end if
					}; // end if

					// send marker for begin raw stream
					if ((outputopen) && (p_r_global->sendmarker)) {
						retval=output_dal(IDAL_CMD_WRITE,"RAWSTREAMBEGIN.\n",16,&retval2,retmsg);

						if (retval != IDAL_RET_SUCCESS) {
							// open failed
							fprintf(stderr,"%s",retmsg);
						}; // end if

					}; // end if

					if (outputopen) {
						retval=output_dal(IDAL_CMD_WRITE,&syncbuffer, 130,&retval2,retmsg);

						if (retval != IDAL_RET_SUCCESS) {
							// open failed
							fprintf(stderr,"%s",retmsg);
						}; // end if

					}; // end if

					// go to next bit
					continue;
				}; // end if
			}; 

			// end of checking, 
			// go to next frame
			continue;
		}; // end if (state 0)


		if (state == 10) {
			// raw invert = 1 -> first bit is on left of octet (MSB)
			// raw invert = 0 -> first bit is on right of octet (LSB)
			if (p_g_global->rawinvert) {
				*p_rawbuffer<<=1;
				if (bit) {
					*p_rawbuffer |= 1;
				}; // end if
			} else {
				*p_rawbuffer>>=1;
				if (bit) {
					*p_rawbuffer |= 0x80;
				}; // end if
			}; // end else - if

			rawbuffer_bit++;

			if (rawbuffer_bit >= 8) {
				// move up octet counter
				rawbuffer_octet++;
				p_rawbuffer++;
				
				rawbuffer_bit=0;
			}; // end if

			// complete frame? (12 octets)
			if (rawbuffer_octet >= 12) {

				// send or write raw frame

				if (outputopen) {
					retval=output_dal(IDAL_CMD_WRITE,&rawbuffer,12,&retval2, retmsg);
					if (retval != IDAL_RET_SUCCESS) {
						// write failed
						fprintf(stderr,"%s",retmsg);
						break;
					}; // end if
				}; // end if

				// reinit vals
				rawbuffer_octet=0; rawbuffer_bit=0; p_rawbuffer=rawbuffer;
			}; // end if

		}; // end if (state = 10 -> raw)

	}; // end for (sampleloop)

	p_r_global->pntr_process = nextbuffer;

}; // end while (for capture: endless loop; for file: EOF)


#ifdef _USEALSA
if (p_r_global->fileorcapture == 0) {
	// capture
	fprintf(stderr,"Error: CAPTURE-THREAD TERMINATED for capture. Should not happen! \n");
}; // end if
#endif

/// end program
exit(0);
}; // end thread "process audio"


