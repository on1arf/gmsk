/* processaudio.h */

// High level functions to process audio, received from an ALSA device
// The actual audio is received from the ringbuffer, fed by the "capture" function


// version 20111107: initial release
// version 20111110: read from file, dump bits, dump amplitude
// version 20111112: support for stereo files
// version 20111130: end stream on signal drop. do not process noise data,
//                   accept bits at startsync and descrable slow data
// version 20120105: raw mode audio output

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


void * funct_processaudio (void * globaldatain ) {


// vars

int16_t audioin;

int nextbuffer;
int thisbuffer;
int thisbuffersize;
int thisfileend;

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

int rawsyncmask;

uint16_t last2octets;
int syncreceived;
uint32_t last4octets=0;
int radioheaderreceived=0;
int radioheaderbuffer[660];
int radioheaderbuffer2[660];
unsigned char radioheader[41];

int loop;
int ret;

int endfound;
int bitmatch;

// time related
struct timeval now;
struct timezone tz;

int filecount;
int framecount=0;
char fnamefull[160];

uint16_t FCSinheader;
uint16_t FCScalculated;

FILE  * fileout=NULL;

uint16_t streamid;

int16_t *samplepointer;
int channel;
int buffermask;

int udpsocket=0;

// vars to calculate "maximum audio level" for a valid stream (i.e. no noise)
int maxaudiolevelvalid=0;
int framesnoise=0;

// raw buffer
unsigned char rawbuffer[12]; // 96 bits = 12 octets = 20 ms of audio
unsigned char *p_rawbuffer; 
int rawbuffer_octet, rawbuffer_bit; // rawbuffer octet and bit counter

// temp values to calculate maximum audio level
int64_t maxaudiolevelvalid_total=0;
int maxaudiolevelvalid_numbersample=0;

globaldatastr * p_global;
p_global=(globaldatastr *) globaldatain;

// dvtool files
str_dvtoolheader dvtoolheader;
str_configframe configframe;
str_dvframe dvframe;


// init vars
state=0;
syncreceived=0;
last2octets=0;

// rawsync mask: depends on rawsyncsize
// should contain valid between 1 and 16.
// check just to be sure
if ((p_global->rawsyncsize < 1) || (p_global->rawsyncsize > 16)) {
	fprintf(stderr,"Error in processaudio, rawsynsize should be between 1 and 16, got %d. Exiting!\n",p_global->rawsyncsize);
	exit(-1);
}; // end if
rawsyncmask=size2mask[p_global->rawsyncsize-1];


// precalculate values to avoid having to do the calculation every
// itteration
if (p_global->stereo) {
	channel=2;
	buffermask=0x7f;
} else {
	channel=1;
	buffermask=0xff;
}; // end if



if (p_global->udpsockaddr) {
	// create UDP socket
	udpsocket=socket(AF_INET6,SOCK_DGRAM,0);

	if (udpsocket < 0) {
		fprintf(stderr,"Error: could not create UDP socket. Shouldn't happen!\n");
		exit(-1);
	}; // end if
}; // end if


// set to -1, will be increase before first use
filecount=-1;

if ((!(p_global->fnameout)) && (!(p_global->udpsockaddr))){
	fprintf(stderr,"Error: no output filename or udphost:udpport!\n");
	exit(-1);
}; // end if


// fill in dvtool header
memcpy(&dvtoolheader.headertext,"DVTOOL",6);

dvtoolheader.filesize=0;

// init var
filesize=0;
countnoiseframe=0;
hadtowait=0;

if (!(p_global->raw)) {
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
}; // end if (not raw)


// init thisfileend. For ALSA capturing, there is no file ending; so we
// just thread this as a endless file
thisfileend=0;

// endless loop;
while (!(thisfileend)) {

	// is there something to process?

	// check the next buffer slot. If the "capture" process is using it, we do not have any
	// audio available. -> wait and try again
	nextbuffer=(p_global->pntr_process + 1) & buffermask;

	if (p_global->pntr_capture == nextbuffer) {
		// nothing to process: sleep for 1/4 of ms (= 5 ms)
		usleep((useconds_t) 250);

		// set "had to wait" marker
		hadtowait=1;
		continue;
	};

	// OK, we have a frame with audio-information, process it, sample
	// per sample
	thisbuffer=nextbuffer;
	audiobuffer= (int16_t *) p_global->buffer[thisbuffer];
	thisbuffersize=p_global->buffersize[thisbuffer];

	if (p_global->fileorcapture) {
		thisfileend=p_global->fileend[thisbuffer];
		p_global->pntr_process = nextbuffer;
	}; // end if



	// things to do at the beginning of every block of data

	// dump average if requested
	if (p_global->dumpaverage) {
		printf("(%04X)",p_global->audioaverage[thisbuffer]);
	}; // end if


	// do we process the audio ?
	// if state is 0 (waiting for sync), and input audio-level is to high
	// (i.e. received signal is noise), we skip the complete frame
	if ((state == 0) && (maxaudiolevelvalid) && (p_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
		// skip audio frame

		// how many frames did we already skip?
		countnoiseframe++;

		// if to much (more then 900 seconds), clear "maxaudiolevel", just to be sure
		if (countnoiseframe > MAXNOISEREJECT) {
			maxaudiolevelvalid=0;
		}; // end if

		// sleep for 10 ms (half of one audiosample), only when reading from ALSA device
		// and we have he "had to wait" marker set

		if ((hadtowait) && (p_global->fileorcapture == 0)) {	
			usleep((useconds_t) 10000);
		}; // end if

		// reset "had to wait"
		hadtowait=0;
	
		// go to next audio frame
		p_global->pntr_process = nextbuffer;
		continue;
	};


	// if state 3 (raw data receive) and audio-level is to high (i.e. we receive noise), this is seen
	// as a end-of-stream situation
	if ((state == 3) && (maxaudiolevelvalid) && (p_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {

		// if raw buffer already started, fill in rest of remaining raw buffer with all 0
		// and send out
		if ((rawbuffer_octet) || (rawbuffer_bit)) {
			// fill in rest of current octet

			// raw invert = 1 -> first bit is on left of octet (MSB)
			// raw invert = 0 -> first bit is on right of octet (LSB)
			if (p_global->rawinvert) {
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

			// now send and/or write raw buffer

			// output to file?
			if (p_global->fnameout) {

				// write to file
				ret=fwrite(&rawbuffer, 12,1,fileout);
				if (ret < 0) {
					// error
					fprintf(stderr,"Warning: Error writing final raw frame to file %s!\n",p_global->fnameout); 
				}; // end if

				// if writing to stdout -> write end marker
				if ((p_global->outtostdout) && (p_global->sendmarker)) {
					// send marker for END raw stream
					fprintf(fileout,"RAWSTREAMEND.\n");
				}; // end if

				// flush file
				fflush(fileout);

			};

			// send UDP packets
			if (p_global->udpsockaddr) {
				// send UDP frame
				ret=sendto(udpsocket,rawbuffer,12,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

				if (ret < 0) {
					// error
					fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
				}; // end if
			}; // end if (send UDP)


		}; // end if (still data in buffer, send it)

		// re-init vars
		state=0;
	}; // end if


	// reset countnoiseframe
	countnoiseframe=0;

	// reset "had to wait" flag
	hadtowait=0;


	// when receiving header (state is 1), add up audio-level data to calculate
	// maxaudiolevel when header completely received

	if (state == 1) {
		maxaudiolevelvalid_total += p_global->audioaverage[thisbuffer];
		maxaudiolevelvalid_numbersample++;
	}; // end if

	// when receiving data frame (state is 2), just count number of consequent audiolevels
	if (state == 2) {
		if ((maxaudiolevelvalid) && (p_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
			framesnoise++;
		} else {
			framesnoise=0;
		}; // end else - if
	}; // end if

	// raw data (state == 3), calculate noiselevel based on first 8 audio groups
	// (= 160 ms, slightly less then size of header for D-STAR and easier to calculate
	if (state == 3) {
		if (maxaudiolevelvalid_numbersample < 8) {
			maxaudiolevelvalid_total += p_global->audioaverage[thisbuffer];
			maxaudiolevelvalid_numbersample++;

			// did we now receive sufficient samples ?
			if (maxaudiolevelvalid_numbersample > 8) {
				// shift right 3 = divide by 8
				maxaudiolevelvalid = (maxaudiolevelvalid_total >> 3) * MAXAUDIOLEVEL_MARGIN;
			}; // end if
			
		} else {
			if ((maxaudiolevelvalid) && (p_global->audioaverage[thisbuffer] > maxaudiolevelvalid)) {
				framesnoise++;
			} else {
				framesnoise=0;
			}; // end else - if
		}; // end else - if
	}; // end if



	// process buffer

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
		// state 3: receiving RAW

		// state 0
		if (state == 0) {
			if (p_global->dumpstream >= 2) {
				printbit(bit,6,2,0);
			}; // end if

			// keep track of last 16 bits
			last2octets<<=1;
			if (bit) {
				last2octets |= 0x01;
			}; // end if

			// according the specifications, the syncronisation pattern is 64 times "0101", however there also
			// seams to be cases where the sequence "1111" is send.
			if ((last2octets == 0x5555) || (last2octets == 0xffff)) {
				syncreceived += 3;
			} else {
				if (syncreceived > 0) {
					syncreceived--;
				}; // end if
			}; 

			if (syncreceived > 20) {
			// start looking for frame sync if we have received sufficient bitsync

				// we accept up to "BITERROR START SYN" penality points for errors
				// rawsyncmask is initialised above, based on rawsyncsize
				bitmatch=countdiff16(last2octets,rawsyncmask,p_global->rawsyncsize,p_global->rawsync,BITERRORSSTARTSYN);

				if (bitmatch) {
					// OK, we have a valid frame sync, go to state 1 (D-STAR) or 3 (raw)
					if (p_global->raw) {
						state=3;

						// init buffervars
						rawbuffer_octet=0;
						rawbuffer_bit=0;

						p_rawbuffer=rawbuffer;
					} else {
						state=1;
					}; // end else - if

					radioheaderreceived=0;

					// reset print-bit to beginning of stream
					if (p_global->dumpstream >= 2) {
						putchar(0x0a); // ASCII 0x0a = linefeed
					}; // end if
					printbit(-1,0,0,0);

					// store first data of audiolevel to calculate average
					maxaudiolevelvalid_total=p_global->audioaverage[thisbuffer];
					maxaudiolevelvalid_numbersample=1;

					// if raw, write to file or open UDP 
					if (p_global->raw) {
						// write sync pattern + sync frame to file or UDP
						// problem, we do not have the sync pattern, so we will recreate it
						// based on 128 times "01" plus the frame sync pattern

						// 128 times "01" = 32 octets + maximum 16 bit frame-sync						
						unsigned char syncbuffer[34];
						uint16_t mask;
						uint16_t syncpattern;

						// the end of the "0101"-pattern should match exactly the beginning of the
						// frame sync. So the pattern of the first 32 octets depends on the size
						// of the sync frame, either "10101010" if size of synpattern is even
						// or "01010101" it if odd. In that case, an additional 0 is added in front

						if (p_global->rawsyncsize & 0x01) {
							// odd
							if (p_global->rawinvert) {
								// invert bit order
								memset(syncbuffer,0xaa,32);
							} else {
								// no invert
								memset(syncbuffer,0x55,32);
							}; // end else - if

							// do not invert mask (will be done later)
							mask=0x5555 & ~size2mask[(p_global->rawsyncsize) -1];
						} else {
							// even
							if (p_global->rawinvert) {
								// invert bit order
								memset(syncbuffer,0x55,32);
							} else {
								// no invert
								memset(syncbuffer,0xaa,32);
							}; // end else - if

							// do not invert mask (will be done later)
							mask=0xaaaa & ~size2mask[(p_global->rawsyncsize) -1];
						}; // end else - if (odd or even)

						// create last 2 octets, always non-inverted at this stage
						syncpattern=p_global->rawsync & size2mask[(p_global->rawsyncsize) -1];
						syncpattern |= mask;

						// copy of last 2 octets of buffer
						if (p_global->rawinvert) {
						// invert
							syncbuffer[32]=(unsigned char) (invertbits[(syncpattern >> 8) & 0xFF]);
							syncbuffer[33]=(unsigned char) (invertbits[syncpattern & 0xFF]);
						} else {
						// no invert
							syncbuffer[32]=(unsigned char) (syncpattern >> 8) & 0xFF;
							syncbuffer[33]=(unsigned char) syncpattern & 0xFF;
						}; // end else - if

						// output to file?
						if (p_global->fnameout) {

							if (p_global->outtostdout) {
								// open to standard out
								// fileout becomes standard-out
								fileout=stdout;

								// send marker for begin raw stream
								if (p_global->sendmarker) {
									fprintf(fileout,"RAWSTREAMBEGIN:\n");
									fflush(fileout);
								}; // end if

							} else {
								// open to real file
								fileout=fopen(p_global->fnameout,"w");

								if (!fileout) {
									fprintf(stderr,"Error: raw output file %s could not be opened!\n",p_global->fnameout);
									exit(-1);
								}; // end if
							}; // end if

							// write to file
							ret=fwrite(&syncbuffer, 34,1,fileout);
							if (ret < 0) {
								// error
								fprintf(stderr,"Warning: Error in write of raw header to file %s!\n",p_global->fnameout); 
							}; // end if

							// flush file
							fflush(fileout);
						};

						// send UDP packets
						if (p_global->udpsockaddr) {
							// send UDP frame
							// start at "headertext" (i.e. do not send 2 "length" indication octets)
							ret=sendto(udpsocket,&syncbuffer,34,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

							if (ret < 0) {
								// error
								fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
							}; // end if
						}; // end if (send UDP)


					}; // end if (raw)

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
				} else {
					printf("(NOT OK!!! Calculated FCS = %04X)\n",FCScalculated);
				}; // end else - if

				filecount++;


				// do we output to files ???
				if (p_global->fnameout) {

					// write to file or standard out?
					if (p_global->outtostdout) {
						fileout=stdout;

						// write marker for start DSTAR stream
						if (p_global->sendmarker) {
							fprintf(fileout,"DSTARSTREAMBEGIN:\n");
							fflush(fileout);
						}; // end if

						// set fullname
						memcpy(fnamefull,"-",2); // copy 2 octets: "-" + \000
					} else {
						// open file for output
						snprintf(fnamefull,160,"%s-%04d.dvtool",p_global->fnameout,filecount);

						fileout=fopen(fnamefull, "w");

						if (!fileout) {
							fprintf(stderr,"Error: output file %s could not be opened!\n",fnamefull);
							exit(-1);
						}; // end if
					}; // end else - if

					
					// write DVtool header (initialised above)
					ret=fwrite(&dvtoolheader,10,1,fileout);
					if (ret != 1) {
						fprintf(stderr,"Error: cannot write DVtool header to file %s\n",fnamefull);
						exit(-1);
					}; // end if
					fflush(fileout);
				}; // end if

				// note, the 10 octets written above are only used in files, not in UDP streams

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



				// do we output to files ?
				if (p_global->fnameout) {
					// write DVtool header (initialised above)
					ret=fwrite(&configframe,58,1,fileout);
					if (ret != 1) {
						fprintf(stderr,"Error: cannot write configframe to file %s\n",fnamefull);
						exit(-1);
					}; // end if
					fflush(fileout);
				};

				if (p_global->udpsockaddr) {
				// send UDP frame
				// start at "headertext" (i.e. do not send 2 "length" indication octets)
					ret=sendto(udpsocket,&configframe.headertext,56,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

					if (ret < 0) {
						// error
						fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
					}; // end if
				}; // end send UDP

				state=2;

				// reinit some vars
				last4octets=0;

				bitposition=0;
				octetposition=0;

				framesnoise=0;

			}; // end if

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
				} else {
					// SYNC marker missed
					marker='M';
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


			if (p_global->dumpstream >= 1) {
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

					printf("END.\n\n\n");

					// re-init
					syncreceived=0;
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
				} else if (framesnoise >= 1) {
					printf("CARRIER DROPPED!\n\n\n");

					// re-init
					syncreceived=0;
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


				// do we output to files ?
				if (p_global->fnameout) {

					// write frame to file
					ret=fwrite(&dvframe,29,1,fileout);
					if (ret != 1) {
						fprintf(stderr,"Error: cannot write dvframe %d (last frame of stream) to file %s\n",framecount,fnamefull);
						exit(-1);
					}; // end if
					fflush(fileout);

					filesize++;


					if (p_global->outtostdout) {
						// done, write endstream" marker
						if (p_global->sendmarker) {
							fprintf(fileout,"DSTARSTREAMEND\n");
							fflush(fileout);
						}; // end if 
					} else {
						// move back to the beginning of the dvtool file.
						// rewrite header + filesize

						ret=fseek(fileout,0,SEEK_SET);
						if (ret) {
							fprintf(stderr,"Error: cannot rewind to position 0 in file %s\n",fnamefull);
							exit(-1);
						}; // end if


						// write size
						// write DVtool header (initialised above)
						dvtoolheader.filesize=filesize;

						ret=fwrite(&dvtoolheader,10,1,fileout);
						if (ret != 1) {
							fprintf(stderr,"Error: cannot rewrite DVtool header to file %s\n",fnamefull);
							exit(-1);
						}; // end if

						// close file, also does fflush at the same time
						fclose(fileout);
					}; // end if
				}; // end if 
				
				if (p_global->udpsockaddr) {
				// send UDP frame
				// start at "headertext" (i.e. do not send 2 "length" indication octets)
					ret=sendto(udpsocket,&dvframe.headertext,27,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

					if (ret < 0) {
						// error
						fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
					}; // end if
				}; // end if (send UDP)

				// reset filesize
				filesize=0;

				continue;
			} else if (octetposition == 12) {

				// reset some vars
				octetposition=0; bitposition=0;

				// set framecounter
				dvframe.framecounter=framecount;

				// do we output to files ?
				if (p_global->fnameout) {
					// write frame to file
					ret=fwrite(&dvframe,29,1,fileout);
					if (ret != 1) {
						fprintf(stderr,"Error: cannot write dvframe %d to file %s\n",framecount,fnamefull);
						exit(-1);
					}; // end if
					fflush(fileout);
					filesize++;
				}; // end if


				if (p_global->udpsockaddr) {
				// send UDP frame
				// start at "headertext" (i.e. do not send 2 "length" indication octets)
					ret=sendto(udpsocket,&dvframe.headertext,27,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

					if (ret < 0) {
						// error
						fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
					}; // end if
				}; // end if (send UDP)


				// increase framecounter for next frame
				framecount++;
				if (framecount > 20) {
					framecount=0;
				}; // end if

				continue;
			}; // end else - if

		
			continue;
		}; // end if (state 2)


		// state 3: RAW stream
		if (state == 3) {
			// raw invert = 1 -> first bit is on left of octet (MSB)
			// raw invert = 0 -> first bit is on right of octet (LSB)
			if (p_global->rawinvert) {
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

				// output to file?
				if (p_global->fnameout) {

					// write to file
					ret=fwrite(&rawbuffer, 12,1,fileout);
					if (ret < 0) {
						// error
						fprintf(stderr,"Warning: Error writing raw frame to file %s!\n",p_global->fnameout); 
					}; // end if

					// flush file
					fflush(fileout);
				};

				// send UDP packets
				if (p_global->udpsockaddr) {
					// send UDP frame
					ret=sendto(udpsocket,rawbuffer,12,0, (struct sockaddr *) p_global->udpsockaddr, sizeof(struct sockaddr_in6));

					if (ret < 0) {
						// error
						fprintf(stderr,"Warning: udp packet could not be send %d (%s)!\n",errno,strerror(errno)); 
					}; // end if
				}; // end if (send UDP)


				// reinit vals
				rawbuffer_octet=0; rawbuffer_bit=0; p_rawbuffer=rawbuffer;
			}; // end if

		}; // end if

	}; // end for (sampleloop)

	p_global->pntr_process = nextbuffer;


}; // end while (for capture: endless loop; for file: EOF)



/// end program
exit(0);
}; // end thread "process audio"


