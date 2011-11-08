/* processaudio.h */

// High level functions to process audio, received from an ALSA device
// The actual audio is received from the ringbuffer, fed by the "capture" function


// version 20111107: initial release

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

int sampleloop;
int16_t * audiobuffer;

int bit, state;

int bitcount, octetcount;
int bitposition;
int octetposition;

int filesize;

uint16_t last2octets;
int syncreceived;
uint64_t last8octets;
int radioheaderreceived;
int radioheaderbuffer[660];
int radioheaderbuffer2[660];
char radioheader[41];

int loop;
int maxdiff;
int ret;

int endfound;

// time related
struct timeval now;
struct timezone tz;

int filecount;
int framecount;
char fnamefull[160];

uint16_t FCSinheader;
uint16_t FCScalculated;

FILE  * fileout=NULL;

uint16_t streamid;


globaldatastr * p_global;
p_global=(globaldatastr *) globaldatain;

// dvtool files
str_dvtoolheader dvtoolheader;
str_configframe configframe;
str_dvframe dvframe;


// tempory buffer for slowdata
// to be added later. (slowdata needs to be descrambled)
//char slowdatain[3];

// init vars
state=0;
syncreceived=0;
last2octets=0;

// set to -1, will be increase before first use
filecount=-1;

if (!(p_global->fnameout)) {
	fprintf(stderr,"Error: no output filename!\n");
	exit(-1);
}; // end if

// fill in dvtool header
memcpy(&dvtoolheader.headertext,"DVTOOL",6);

dvtoolheader.filesize=0;

// init var
filesize=0;


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


// endless loop;
while (1) {

	// is there something to process?

	// check the next buffer slot. If the "capture" process is using it, we do not have any
	// audio available. -> wait and try again
	nextbuffer=(p_global->pntr_process + 1) & 0xFF;

	if (p_global->pntr_capture == nextbuffer) {
		// nothing to process: sleep for 1/4 ms
		usleep((useconds_t) 250);
		continue;
	};

	// OK, we have a frame with audio-information, process it, sample
	// per sample
	thisbuffer=nextbuffer;
	audiobuffer= (int16_t *) p_global->buffer[thisbuffer];
	thisbuffersize=p_global->buffersize[thisbuffer];


	for (sampleloop=0; sampleloop <thisbuffersize; sampleloop++) {
		audioin=audiobuffer[sampleloop];

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
		// state 1: receiving radioheader (660 bits)
		// state 2: receiving main part of bitstream

		// state 0
		if (state == 0) {
			// keep track of last 16 bits
			last2octets<<=1;
			if (bit) {
				last2octets |= 0x01;
			}; // end if

			if (syncreceived > 20) {
			// start looking for frame sync if we have received sufficient bitsync
				if ((last2octets & 0x7FFF) == 0x7650) {
					// OK, we have a valid frame sync, go to state 1
					state=1;
					radioheaderreceived=0;

					// go to next frame
					continue;
				}; // end if
			}; 

			if (last2octets == 0x5555) {
				syncreceived += 3;
			} else {
				if (syncreceived > 0) {
					syncreceived -= 1;
				}; // end if
			}; 

			// end of checking, 

			// go to next frame
			continue;
		}; // end if (state 0)


		// state 1: radioheader
		if (state == 1) {
			// read up to 660 characters
		
			radioheaderbuffer[radioheaderreceived++]=bit;

			if (radioheaderreceived >= 660) {
				int len;

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
		
				FCSinheader=((radioheader[39] << 8) | radioheader[40]) & 0xFFFF; 	
				FCScalculated=calc_fcs((unsigned char *) radioheader,39);
				printf("Check sum = %04X ",FCSinheader);

				if (FCSinheader == FCScalculated) {
					printf("(OK)\n");
				} else {
					printf("(NOT OK!!! Calculated FCS = %04X)\n",FCScalculated);
				}; // end else - if

				filecount++;



				// open file for output
				snprintf(fnamefull,160,"%s-%04d.dvtool",p_global->fnameout,filecount);

				fileout=fopen(fnamefull, "w");

				if (!fileout) {
					fprintf(stderr,"Error: output file %s could not be opened!\n",fnamefull);
					exit(-1);
				}; // end if



				// write DVtool header (initialised above)
				ret=fwrite(&dvtoolheader,10,1,fileout);
				if (ret != 1) {
					fprintf(stderr,"Error: cannot write DVtool header to file %s\n",fnamefull);
					exit(-1);
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



				// write DVtool header (initialised above)
				ret=fwrite(&configframe,58,1,fileout);
				if (ret != 1) {
					fprintf(stderr,"Error: cannot write configframe to file %s\n",fnamefull);
					exit(-1);
				}; // end if
				

				state=2;
				last8octets=0;

				bitposition=0;
				octetposition=0;

			}; // end if

		continue;
		}; // end if (state 1)

		// state 2: main part of stream
		if (state == 2) {
			char marker=0;
			int bitmatch;

			bitposition++;
			if (bitposition > 96) {
				bitposition=1;
			}; // end if

			last8octets <<= 1;
			if (bit) {
				last8octets |= 0x01;
			}; // end if

			// SYNC marker: 24 bits: 0101010101 1101000 1101000 
			if ((last8octets & 0x00FFFFFF) == 0x00AAB468) {
				marker='S';
			}; // end if

			printbit(bit,6,2,marker);


			// store bits into 12 octet structure
			if ((bitposition & 0x07) == 0x00) {
				// bitposition is multiple of 8

				// note, the bits in the .dvtool are left-right inverted
 
				// octets 0 to 8 are DV

//				if (octetposition <= 8) {
					dvframe.ambe[octetposition]=(invertbits[(char)last8octets & 0xff]);
//				} else {
//					// octets 9 to 11 are data.
//					// store these in tempory buffer ("slowdatain") as this data may need to be descrambled first
//					slowdatain[octetposition-9]=(char)(last8octets & 0xff);
//				}; // end else - if
					
				octetposition++;

			}; // end if 


			// if octetposition is 12, this means we have received all 12 octets of
			// a complete frame.
			// we will use that further down to determine if we need to write a DVframe
			// or not

			// First, look for end-of-stream marker. If found, framecounter bit 6 is set to 1
			endfound=0;

			// END marker: 48 bits: 32 bit "sync" + "00010011 01011110"
			// less stringent rules:
			// instead of doing a "=", we do an "exor" and count the number of differences.
			// This 48 bit pattern is found in the data-part of the last frame + 24 bits after that

			// so we do this check at position 24, or two bits before or after that

			if ((bitposition >= 22) && (bitposition <= 26)) {
				if ((bitposition == 26) || (bitposition == 22)) {
					maxdiff=0; // 2 * 2
				} else if ((bitposition == 25) || (bitposition == 23)) {
					maxdiff=2; // 1 * 2
				} else {
					// bit position 24
					maxdiff=6;
				}; // else - elseif - if

				bitmatch=countdiff64(last8octets,0xFFFFFFFFFFFFLL,48,0xAAAAAAAA135ELL,maxdiff);

				if (bitmatch) {
				// we have a match
					printf("END.\n\n\n");

					// re-init
					syncreceived=0;
					last2octets=0;
					state=0;

					// the end-marker is just a fake pattern and does not contain any
					// real audio -> Zap frame
					memset(dvframe.ambe,0,9);
					dvframe.slowspeeddata[0]=0xAA;
					dvframe.slowspeeddata[1]=0xB4;
					dvframe.slowspeeddata[2]=0x68;

					// reset counters of printbit function
					printbit(-1,0,0,0);

					endfound=1;
				}; // end if
			}; // end if (bitposition between 22 and 26)


			// additional "END" checks:
			// data-field containing all "0" or all "1"
			// data-field containing all "01" or "10"
			if (bitposition == 96) {
				uint32_t datapart; // datapart is 3 octets

				datapart=(last8octets & 0xFFFFFF);

				if ((datapart == 0) || (datapart == 0xFFFFFF) || (datapart == 0x555555) ) {
					printf("EXIT.\n\n\n");

					// re-init
					syncreceived=0;
					last2octets=0;
					state=0;

					// reset counters of printbit function
					printbit(-1,0,0,0);

					// the end-marker is just a fake pattern and does not contain any
					// real audio ->  zap contents of frame
					memset(dvframe.ambe,0,9);
					dvframe.slowspeeddata[0]=0xAA;
					dvframe.slowspeeddata[1]=0xB4;
					dvframe.slowspeeddata[2]=0x68;

					endfound=1;
				}; // end if
			}; // end if

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


				// write frame to file
				ret=fwrite(&dvframe,29,1,fileout);
				if (ret != 1) {
					fprintf(stderr,"Error: cannot write dvframe %d (last frame of stream) to file %s\n",framecount,fnamefull);
					exit(-1);
				}; // end if
				filesize++;


				// now move back to the beginning of the dvtool file.
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

				// close file
				fclose(fileout);

				continue;
			} else if (octetposition == 12) {

				// reset some vars
				octetposition=0; bitposition=0;

				// set framecounter
				dvframe.framecounter=framecount;

				// write frame to file
				ret=fwrite(&dvframe,29,1,fileout);
				if (ret != 1) {
					fprintf(stderr,"Error: cannot write dvframe %d to file %s\n",framecount,fnamefull);
					exit(-1);
				}; // end if
				filesize ++;


				// increase framecounter for next frame
				framecount++;
				if (framecount > 20) {
					framecount=0;
				}; // end if

				continue;
			}; // end else - if

		
			continue;
		}; // end if (state 2)

	}; // end for (sampleloop)

	p_global->pntr_process = nextbuffer;


}; // end while (endless loop)


/// end main program
return(0);
}; // end main program


