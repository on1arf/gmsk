/* processaudio.h */

// High level functions to process audio, received from an ALSA device
// The actual audio is received from the ringbuffer, fed by the "capture" function


// version 20111107: initial release
// version 20111110: read from file, dump bits, dump amplitude
// version 20111112: support for stereo files
// version 20111130: end stream on signal drop. do not process noise data,
//                   accept bits at startsync and descrable slow data
// version 20120105: raw mode audio output
// version 20120529: moved dstar related code to seperate file

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


void * funct_r_processaudio_dstar (void * c_globaldatain ) {


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

int bitcount;
int octetcount;
int bitposition=0;
int octetposition=0;
int countnoiseframe=0; // number of frames not processed due to average audio level > noise

int filesize;

int hadtowait;

int syncmask;

uint16_t last2octets;
int syncreceived, syncFFreceived;
uint32_t last4octets=0;
int radioheaderreceived=0;
int radioheaderbuffer[660];
int radioheaderbuffer2[660];
unsigned char radioheader[41];

int loop;
int retval, retval2=0;
char retmsg[ISALRETMSGSIZE];
int outputopen=0;

int endfound;
int bitmatch;

// flags that indicate certainty of good stream
int syncAA;
int crcerror;
int missedsync;



// time related
struct timeval now;
struct timezone tz;

int framecount=0;

uint16_t FCSinheader;
uint16_t FCScalculated;

uint16_t streamid;

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


// dvtool files
str_dvtoolheader dvtoolheader;
str_configframe configframe;
str_dvframe dvframe;


// init vars
state=0;
syncreceived=0; syncFFreceived=0;
last2octets=0;

syncAA=0; crcerror=0; missedsync=0;

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

if ((thisformat != 1) && (thisformat != 2)) {
	fprintf(stderr,"Error in processaudio_dstar, invalid format. Expected 1 or 2, got %d\n",thisformat);
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
	fprintf(stderr,"Error in processaudio, synsize should be between 1 and 16, got %d. Exiting!\n",p_r_global->syncsize);
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




// fill in dvtool header
memcpy(&dvtoolheader.headertext,"DVTOOL",6);

dvtoolheader.filesize=0;

// init var
filesize=0;
countnoiseframe=0;
hadtowait=0;


// fill in configframe structure, as much as possible
configframe.length=0x38;
memcpy(&configframe.headertext,"DSVT",4);
configframe.configordata=0x10;
configframe.unknown1[0]=0x00; configframe.unknown1[1]=0x00; configframe.unknown1[2]=0x00; 
configframe.dataorvoice=0x20;
configframe.unknown2[0]=0x00; configframe.unknown2[1]=0x01; configframe.unknown2[2]=0x01; 
configframe.framecounter=0x80;

// fill in digital voice frame, as much as possible
dvframe.length=0x1b;
memcpy(&dvframe.headertext,"DSVT",4);
dvframe.configordata=0x20;
dvframe.unknown1[0]=0x00; dvframe.unknown1[1]=0x00; dvframe.unknown1[2]=0x00; 
dvframe.dataorvoice=0x20;
dvframe.unknown2[0]=0x00; dvframe.unknown2[1]=0x01; dvframe.unknown2[2]=0x01; 


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


	// at this point, we have a valid frame without noise

	// reset countnoiseframe
	countnoiseframe=0;

	// reset "had to wait" flag
	hadtowait=0;


	// when receiving header (state is 1), add up audio-level data to calculate
	// maxaudiolevel when header completely received
	if (state == 1) {
		maxaudiolevelvalid_total += p_r_global->audioaverage[thisbuffer];
		maxaudiolevelvalid_numbersample++;
	}; // end if

	// when receiving data frame (state is 2), just count number of consequent audiolevels
	if (state == 2) {
		if ((maxaudiolevelvalid) && (p_r_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
			framesnoise++;
		} else {
			framesnoise=0;
		}; // end else - if
	}; // end if



	// PART 2: 
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
		// state 1: receiving D-STAR radioheader (660 bits)
		// state 2: receiving D-STAR main part of bitstream

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
					syncAA=0;
					bitmatch=countdiff16_fromlsb(last2octets,syncmask,p_r_global->syncsize,
						p_r_global->syncpattern,BITERRORSSTARTSYN);
				} else {
					syncAA=1;
					bitmatch=countdiff16_fromlsb(last2octets,syncmask,p_r_global->syncsize,
						p_r_global->syncpattern,0);
				}; // end if



				if (bitmatch) {
					int p;
					// OK, we have a valid frame sync, go to state 1 (dvtool file-format or D-STAR UDP stream)

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


					state=1;

					radioheaderreceived=0;

					// reset print-bit to beginning of stream
					if (p_r_global->dumpstream >= 2) {
						putchar(0x0a); // ASCII 0x0a = linefeed
					}; // end if
					printbit(-1,0,0,0);

					// store first data of audiolevel to calculate average
					maxaudiolevelvalid_total=p_r_global->audioaverage[thisbuffer];
					maxaudiolevelvalid_numbersample=1;

					// go to next bit
					continue;
				}; // end if
			}; 

			// end of checking, 
			// go to next frame
			continue;
		}; // end if (state 0)


		// state 1: D-STAR radioheader
		if (state == 1) {
			// read up to 660 characters
		
			radioheaderbuffer[radioheaderreceived++]=bit;

			if (radioheaderreceived >= 660) {
				int len;

				// OK, we have received all 660 bits of the header
				// calculate average audio level
				// if we receive a frame with an average value more then 40% over
				// that, it is concidered noise
				maxaudiolevelvalid = (MAXAUDIOLEVEL_MARGIN * maxaudiolevelvalid_total) / maxaudiolevelvalid_numbersample;


				scramble(radioheaderbuffer,radioheaderbuffer2);
				deinterleave(radioheaderbuffer2,radioheaderbuffer);
				len=FECdecoder(radioheaderbuffer,radioheaderbuffer2);

				if (len != 330) {
					fprintf(stderr,"FECdecoder did not return 330 bits. Something very strange happened!\n");

					// re-init
					state=0;
					syncreceived=0;
					last2octets=0;
					continue;
				}; // end if

				// convert bits into bytes

				// clear radioheader
				memset(radioheader,0,41);

				// note we receive 330 bits, but we only use 328 of them (41 octets)
				// bits 329 and 330 are unused
				octetcount=0; bitcount=0;
				for (loop=0; loop<328;loop++) {
					if (radioheaderbuffer2[loop]) {
						radioheader[octetcount] |= bit2octet[bitcount];
					}; 
					bitcount++;

					// increase octetcounter and reset bitcounter every 8 bits
					if (bitcount >= 8) {
						octetcount++;
						bitcount=0;
					}; // end if
				}; // end for


				// print header
				if (p_g_global->verboselevel >= 1) {
					printf("HEADER DUMP!!! \n");
					printf("-------------- \n");

					printf("FLAG1: %02X  - FLAG2: %02X  - FLAG3: %02X\n",radioheader[0],radioheader[1],radioheader[2]);

					printf("RPT 2: %c%c%c%c%c%c%c%c \n",radioheader[3],radioheader[4],radioheader[5],radioheader[6],radioheader[7],radioheader[8],radioheader[9],radioheader[10]);
					printf("RPT 1: %c%c%c%c%c%c%c%c \n",radioheader[11],radioheader[12],radioheader[13],radioheader[14],radioheader[15],radioheader[16],radioheader[17],radioheader[18]);
					printf("YOUR : %c%c%c%c%c%c%c%c \n",radioheader[19],radioheader[20],radioheader[21],radioheader[22],radioheader[23],radioheader[24],radioheader[25],radioheader[26]);
					printf("MY   : %c%c%c%c%c%c%c%c/%c%c%c%c \n",radioheader[27],radioheader[28],radioheader[29],radioheader[30],radioheader[31],radioheader[32],radioheader[33],radioheader[34], radioheader[35], radioheader[36], radioheader[37], radioheader[38]);
		
					FCSinheader=((radioheader[39] << 8) | radioheader[40] ) & 0xFFFF; 	
					FCScalculated=calc_fcs((unsigned char *) radioheader,39);
					printf("Check sum = %04X ",FCSinheader);

					if (FCSinheader == FCScalculated) {
						printf("(OK)\n");
						crcerror=0;
					} else {
						printf("(NOT OK!!! Calculated FCS = %04X)\n",FCScalculated);
						crcerror=1;
					}; // end else - if
				}; // end if (verboselevel)

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


				if ((outputopen) && (p_r_global->sendmarker)) {
					// send marker for begin DSTAR stream
					retval=output_dal(IDAL_CMD_WRITE,"DSTARSTREAMBEGIN.\n",16,&retval2,retmsg);

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if

				}; // end if
				
				// write DVtool header (initialised above)
				// note, the 10 octets written above are only used in files, not in UDP streams
				if ((thisformat == 1) && (outputopen)) {
					retval=output_dal(IDAL_CMD_WRITE,&dvtoolheader,10,&retval2,retmsg);

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if

				}; // end if


				// init vars
				// streamid a 2 byte random number
				gettimeofday(&now,&tz);
				srand(now.tv_usec);
				streamid=rand() & 0xffff;

				configframe.streamid=streamid;
				dvframe.streamid=streamid;

				// framecounter, goes from 0 to 20
				framecount=0;

				// fill in remaining part of config header

				// copy radioheader, from FLAGS to checksum to configframe
				// note, we do NOT recalculate the checksum, if it received with an
				// invalid value, that incorrect value will be copied to the file
				memcpy(&configframe.flags,radioheader,sizeof(radioheader));


				if (outputopen) {
					if (thisformat == 1) {
					// DVtool file format: 58 octets (header + 2 octets length)
						retval=output_dal(IDAL_CMD_WRITE,&configframe,58,&retval2,retmsg);
					} else if (thisformat == 2) {
					// UDP stream format: 56 octets (header, no length indication)
						retval=output_dal(IDAL_CMD_WRITE,&configframe.headertext,56,&retval2,retmsg);
					}; // end if

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if

				}; // end if


				state=2;

				// reinit some vars
				last4octets=0;

				bitposition=0;
				octetposition=0;

				framesnoise=0;

				missedsync=0;

			}; // end if (all 660 bits read, processing header)

		continue;
		}; // end if (state 1)

		// state 2: D-STAR main part of stream
		if (state == 2) {
			char marker=0;

			bitposition++;
			if (bitposition > 96) {
				bitposition=1;
			}; // end if

			last4octets <<= 1;
			if (bit) {
				last4octets |= 0x01;
			}; // end if

			// SYNC marker: 24 bits: 0101010101 1101000 1101000 
			// only look for it at the last position of every 21 frames
			if ((framecount == 0) && (bitposition == 96)) {
				if ((last4octets & 0x00FFFFFF) == 0x00AAB468) {
					marker='S';

					// we have a valid sync, reset "missed sync" counter
					missedsync=0;
				} else {
					// SYNC marker missed
					marker='M';

					// missed sync
					missedsync++;
				}; // end else - if
			}; // end if

			// solve bit slips: SYNC TO EARLY
			if (((framecount == 0) && (bitposition == 95)) && ((last4octets & 0x00FFFFFF) == 0x00AAB468)) {
				// error: found a SYNC marker but one position to early:

				// invalidate frame
				memset(&dvframe.ambe[0],0,9); // zap data
				dvframe.slowspeeddata[0]=0xAA;
				dvframe.slowspeeddata[1]=0xB4;
				dvframe.slowspeeddata[2]=0x68;

				// move up bitposition
				bitposition=96;

				// set "error" marker
				marker='E';
			}; // end if


			// solve bit slips: SYNC TO LATE
			if (((framecount == 1) && (bitposition == 1)) &&
					((last4octets & 0x00FFFFFF) == 0x00AAB468)) {

				// correct bitposition. Note: normally, bitposition goes from 1 to 96.
				// By using 0, we create a special condition
				bitposition=0;
				
				// set "error" marker
				marker='E';
			}; // end if


			if (p_r_global->dumpstream >= 1) {
				printbit(bit,6,2,marker);
			}; // end if


			// store bits into 12 octet structure
			if ((bitposition) && ((bitposition & 0x07) == 0x00)) {
				// bitposition is multiple of 8

				// note, the bits in the .dvtool are left-right inverted
 
				// octets 0 to 8 are DV
				// octets 9 to 11 are SlowData

				// a small trick: in the dvframe structure, ambe only contains 9
				// octets followed by "slowdata" structure of 3 octets
				// By letting the index go from 0 to 11, we fill up both parts with
				// just one index

				dvframe.ambe[octetposition]=(invertbits[(char)last4octets & 0xff]);
					
				octetposition++;
			}; // end if 


			// if octetposition is 12, this means we have received all 12 octets of
			// a complete frame.
			// we will use that further down to determine if we need to write a DVframe
			// or not

			// First, look for end-of-stream marker. If found, framecounter bit 6 is set to 1
			endfound=0;

			// END marker: accroding to the specifications: 48 bits: 32 bit "sync" + "00010011 01011110"
			// however, it has been noticed that some transceivers do not send out the full pattern
			// but only the 24 bit sync ("10") pattern at the end of the frame

			// additional check, if two or more consecutive frames with high average audio level 
			// (i.e. noise), also stop 

			if (bitposition == 96) {
				if ((last4octets & 0x00FFFFFF) == 0xAAAAAA){

					if (p_g_global->verboselevel >= 1) {
						fprintf(stderr,"END.\n\n\n");
					}; // end if

					// re-init
					syncreceived=0; syncFFreceived=0;
					last2octets=0;
					state=0;

					// the end-marker is just a fake pattern and does not contain any
					// data -> Zap slow data part
					dvframe.slowspeeddata[0]=0xAA;
					dvframe.slowspeeddata[1]=0xB4;
					dvframe.slowspeeddata[2]=0x68;

					// reset counters of printbit function
					printbit(-1,0,0,0);

					endfound=1;
				} else if (framesnoise >= 3) {
					if (p_g_global->verboselevel >= 1) {
						printf("CARRIER DROPPED!\n\n\n");
					}; // end if

					// re-init
					syncreceived=0; syncFFreceived=0;
					last2octets=0;
					state=0;

					// the end-marker is just a fake pattern and does not contain any
					// data -> Zap slow data part
					memset(&dvframe.ambe[0],0,9); // zap data
					dvframe.slowspeeddata[0]=0xAA;
					dvframe.slowspeeddata[1]=0xB4;
					dvframe.slowspeeddata[2]=0x68;

					// reset counters of printbit function
					printbit(-1,0,0,0);

					endfound=1;
				} else if (((missedsync >= 5) && (crcerror) && (syncAA)) || 
					(missedsync >= 30)) {

					// Breakout on Missed sync
					// If we have to many "missed sync", we concider this a end of stream
					// (perhaps the "noise level" system did not work)
					// if the stream have been started by "all-1" (instead of "all 0101"),
					// and the CRC was not correct; this was probably a bad stream to start
					// with; so break out after only 5 "missed sync" events (+- 2 seconds)
					// else, break out after 30 "missed sync" events

					printf("TO MANY MISSEDSYNC!\n\n\n");

					// re-init
					last2octets=0;
					state=0;
					syncreceived=0; syncFFreceived=0;

					// the end-marker is just a fake pattern and does not contain any
					// data -> Zap slow data part
					memset(&dvframe.ambe[0],0,9); // zap data
					dvframe.slowspeeddata[0]=0xAA;
					dvframe.slowspeeddata[1]=0xB4;
					dvframe.slowspeeddata[2]=0x68;

					// reset counters of printbit function
					printbit(-1,0,0,0);

					endfound=1;
				} else {

					// not END, scramble slow-speed data, except for frames number 0 (i.e. first
					// frame of stream + every 20 frames after that) as these frames contain
					// sync frames instead of data
					if (framecount != 0) {
						dvframe.slowspeeddata[0] ^= 0x70;
						dvframe.slowspeeddata[1] ^= 0x4f;
						dvframe.slowspeeddata[2] ^= 0x93;
					}; // end if

				}; // end else - elsif - if

			}; // end if (bitposition 96)


			// all checks done.
			// do we need to save a frame to file?

			// two senarions:
			// - end marker found
			// - complete audio-frame received (octetposition=12)

			if (endfound) {
				// last frame of stream

				// set bit 6 of octetposition (marks end of stream)
				framecount |= 0x40;

				dvframe.framecounter=framecount;

				// reset some vars
				octetposition=0; bitposition=0;


				// output final frame
				if (outputopen) {
					if (thisformat == 1) {
					// DVtool file format: 58 octets (header + 2 octets length)
						retval=output_dal(IDAL_CMD_WRITE,&dvframe,29,&retval2,retmsg);
					} else if (thisformat == 2) {
					// UDP stream format: 56 octets (header, no length indication)
						retval=output_dal(IDAL_CMD_WRITE,&dvframe.headertext,27,&retval2,retmsg);
					}; // end if

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if

				}; // end if


				if ((outputopen) && (p_r_global->sendmarker)) {
					retval=output_dal(IDAL_CMD_WRITE,"DSTARSTREAMEND.",15,&retval2,retmsg);
					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if
				}; // end if


				// close output
				// rewrind to beginning of file and write length (only for file output and dvtool file format)
				// this is done based on the "len" parameter
				if (outputopen){
					retval=output_dal(IDAL_CMD_CLOSE,NULL,filesize,&retval2,retmsg);
					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					} else {
						// success
						outputopen=0;
					}; // end if
				}; // end if

				// reset filesize
				filesize=0;

				continue;
			} else if (octetposition == 12) {

				// reset some vars
				octetposition=0; bitposition=0;

				// set framecounter
				dvframe.framecounter=framecount;

				// output frame
				if (outputopen) {
					if (thisformat == 1) {
					// DVtool file format: 58 octets (header + 2 octets length)
						retval=output_dal(IDAL_CMD_WRITE,&dvframe,29,&retval2,retmsg);
					} else if (thisformat == 2) {
					// UDP stream format: 56 octets (header, no length indication)
						retval=output_dal(IDAL_CMD_WRITE,&dvframe.headertext,27,&retval2,retmsg);
					}; // end if

					if (retval != IDAL_RET_SUCCESS) {
						// open failed
						fprintf(stderr,"%s",retmsg);
					}; // end if
				}; // end if

				filesize++;

				// increase framecounter for next frame
				framecount++;
				if (framecount > 20) {
					framecount=0;
				}; // end if

				continue;
			}; // end else - if (end found or frame is full)

		
			continue;
		}; // end if (state 2)


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


