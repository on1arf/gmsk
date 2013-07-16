//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// codec versions 4800/0 and 2400/15



/*
 *      Copyright (C) 2013 by Kristoff Bonne, ON1ARF
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


// API test - demodulate
// test application for demodulation chain

// Release information
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library
// version 20130614: merge 2400 and 4800 bps modem, added support for aux-data and raw gmsk 



// API TEST APPLICATION:
// start demodulation process
// take raw audio input from file
// send audio to modulation chain, print out all messages received in msgchain
// save to file


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include <c2gmsk.h>

int main (int argc, char ** argv) {

// API structures
struct c2gmsk_session * sessid=NULL;
struct c2gmsk_msgchain * chain=NULL,  ** pchain; // pointer to pointer of message chain
struct c2gmsk_param param;
struct c2gmsk_msg * msg;

// buffers for data
char txtline[203]; // text line for "printbit". Should normally by up to 192, add 3 for "space + marker" and terminating null, add 8 for headroom for PLL syncing-errors
unsigned char c2buff[8]; // buffer is 8 octets: to accomodate all bitrates (1200 and 2400 bps c2 = 8 octets/frame, 1200 bps c2 = 7 octets/frame)
int bitrate, outputformat;


// reuse input buffer for pcm and for gmsk
union {
	int16_t pcmbuffer[1920]; // 40 ms audio @ 48000 = 1920 samples = 3840 octets
	unsigned char gmskbuffer[24]; // 40 ms gmsk @ 4800 bps
} buffer_u;

int fout, fin; // file out and file in 
char * infilename, * outfilename; // filename input and output files

// some local vars
int framecount, msgcount;
int ret,ret2;
int sizetoread;

// vars used to extract messages from message queue
int tod; // type of data
int datasize; // datasize
int data[4]; // used for numeric data


// bitrate to  text
char * bitrate2text[] = { "2400", "4800"};


// check parameters, we need at least 4 parameters: infile and outfile
if (argc < 5) {
	fprintf(stderr,"Error: at least 4 parameters needed.\n");
	fprintf(stderr,"Usage: %s modaudiofile.raw outfile.c2 bitrate outputformat\n",argv[0]);
	exit(-1);
};

infilename=argv[1];
outfilename=argv[2];

bitrate=atoi(argv[3]);

if ((bitrate != 2400) && (bitrate != 4800)) {
	fprintf(stderr,"Error: bitrate should be 2400 or 4800. Got %d.\n",bitrate);
	exit(-1);
}; // end if

if ((argv[4][0] == 'a') || (argv[4][0] == 'A')) {
	outputformat=C2GMSK_OUTPUTFORMAT_AUDIO;
} else if ((argv[4][0] == 'g') || (argv[4][0] == 'G')) {
	outputformat=C2GMSK_OUTPUTFORMAT_GMSK;
} else {
	fprintf(stderr,"Error: output format should be audio or GMSK, got %c.\n",argv[4][0]);
	exit(-1);
}; // end elsif - if

// open out file
fout=open(outfilename,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

if (fout < 0) {
	fprintf(stderr,"Error: could not open file for output: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if


// open input file
fin=open(infilename,O_RDONLY);

if (fin < 0) {
	fprintf(stderr,"Error: could not open file for input: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if

// pchain points to location of chain
pchain=&chain;


// before creating the session, parameters need to be created
ret=c2gmsk_param_init(&param); // no stream related parameters
if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: Could not initialise parameter structures! \n");
	exit(-1);
}; // end if

// fill in parameters
param.expected_apiversion=20130614;
if (bitrate == 2400) {
	param.d_bitrate=C2GMSK_MODEMBITRATE_2400;
} else {
	param.d_bitrate=C2GMSK_MODEMBITRATE_4800;
}; // end else - if

// disactivate modulation
param.m_disabled=C2GMSK_DISABLED;

// set expected outputformat
param.outputformat=outputformat;

// create gmsk session
sessid=c2gmsk_sess_new(&param,&ret, pchain);

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",ret);
	exit(-1);
}; // end if

// c2gmsk_sess_new returns list of supported versions/modes
chain=*pchain;

// loop throu all messages
// message chain is in "chain"
// we are only interested in CAPABILITIES messages
tod=C2GMSK_MSG_CAPABILITIES;

while ((msg = c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, chain, &tod, &datasize, NULL))) {
	ret=c2gmsk_msgdecode_numeric (msg, data);

	if (ret < 0) {
		printf("Error: c2gmsk_msgdecode_numeric got error %d: %s\n",-ret,c2gmsk_printstr_ret(-ret));
		continue;
	}; // end if

	// data[0] = versionid
	// data[1] = bitrate

	if (ret == 2) {
		// bitrate should be 1 or 2
		if ((data[1] >= 1 ) && (data[1] <= 2)) {
			printf("Supported Capabilities of API: Versionid %d, bitrate %s \n",data[0],bitrate2text[data[1]-1]);
		} else {
			printf("Supported Capabilities of API: Versionid %d, bitrate %d \n",data[0],data[1]);
		}; // end else - if
	} else {
		printf("Error: Capabilities should return a message with 2 dataelements. Got %d. Skipping message \n",ret);
	}; 

}; // end while

// add line
printf("\n");

//////////////////////////////////////////
// session is created, let's get started

if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
	sizetoread=1920 * sizeof(int16_t);
} else {
	// GMSK

	if (bitrate == 2400) {
		sizetoread=12; // 40 ms @ 2400 bps = 96 bits = 12 octets
	} else {
		sizetoread=24; // 40 ms @ 4800 bps + 192 bits = 24 octets
	}; // end if
}; // end if


ret=read(fin, &buffer_u, sizetoread);

framecount=0;

while (ret == sizetoread) {

	framecount++;

	if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
		ret2=c2gmsk_demodpcm(sessid,buffer_u.pcmbuffer,pchain);
	} else {
		ret2=c2gmsk_demodgmsk(sessid,buffer_u.gmskbuffer,pchain);
	}; // end if

	chain=*pchain;

	if (ret2 != C2GMSK_RET_OK) {
		fprintf(stderr,"Error: c2gmsk_dmod failed: %d (%s) \n",ret2,c2gmsk_printstr_ret(ret2));
		exit(-1);
	}; // end if


	msgcount=0;

	// loop as long as we find messages in the queue
	while ((msg = c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, chain, &tod, &datasize, NULL))) {

		// add \n before 1st message
		if (msgcount) {
			printf("\nProcessing message %d.%d\n",framecount,msgcount);
		} else {
			printf("Processing message %d.%d\n",framecount,msgcount);
		}; // end else - if

		msgcount++;

	
		if (tod == C2GMSK_PRINTBIT_MOD) {
			printf("PRINTBIT DEMODULATED DATA:\n%s \n",c2gmsk_msgdecode_printbit (msg,txtline,1));
			continue;
		};

		if (tod == C2GMSK_PRINTBIT_ALL) {
			printf("PRINTBIT ALL:\n%s \n",c2gmsk_msgdecode_printbit (msg,txtline,1));
			continue;
		};

		// is it a call that return one or more numeric vales?
		ret2=c2gmsk_msgdecode_numeric (msg, data);
		if (ret2 < 0) {
			printf("Error in msgdecode - numeric for tod %d. Error %d: %s\n",tod,-ret2,c2gmsk_printstr_ret(-ret2));
			continue;
		}; // end if


		if (ret2 > 0) {
			if (ret2 > 1) {
				printf("type-of-data %d (%s): %d data fields:\n",tod, c2gmsk_printstr_msg(tod),ret2);
			} else {
				printf("type-of-data %d (%s): 1 data field:\n", tod, c2gmsk_printstr_msg(tod));
			}; 


			if (ret2 <= 4) {
				// print all data
				int l;

				if (tod == C2GMSK_MSG_VERSIONID) {
					printf("%08X",data[0]);

					for (l=2; l<=ret2; l++) {
						printf(", %08X",data[l-1]);
					}; // end for
					printf("\n");
				} else {
					printf("%d",data[0]);
					for (l=2; l<=ret2; l++) {
						printf(", %d",data[l-1]);
					}; // end for
					printf("\n");
				}; // end else - if
			} else {
				fprintf(stderr,"Error: number of vars returned by msgdecode - numeric not supported!\n");
			}; // end switch

			continue;
		}; // end else - if


		// codec2?
		if (tod == C2GMSK_MSG_CODEC2) {
			ret2=c2gmsk_msgdecode_c2(msg, c2buff);

			if (ret2 > 0) {
				// codec2, check version

				if (ret2 != C2GMSK_CODEC2_1400) {
					fprintf(stderr,"Error: unsupported codec2 bitrate %d\n",ret2);
					continue;
				};

				printf("Writing codec2 frame \n");
				// write data in file
				ret2=write(fout,c2buff,C2GMSK_CODEC2_FRAMESIZE_1400);

				if (ret2 < 0) {
					// write should return number of octets written
					fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
				}; // end if

				continue;
			} else if (ret2 < 0) {
				printf("Error: c2gmsk_msgdecode codec2 for tod %d got error %d: %s\n",tod,-ret2,c2gmsk_printstr_ret(-ret2));
				continue;
			}; // end if
		}; ; // end if


		// end-of-stream notifications 
		if ((tod >= 0x50) && (tod <= 0x5f)) {
			printf("End of stream notification: %s \n",c2gmsk_printstr_msg(tod));
			continue;			
		}; // end if


		// other things, not processed
		printf("********* Type-of-data: %02X (%s) not processed!!!!  \n",tod,c2gmsk_printstr_msg(tod));

		continue;


	}; // end while all messages in queue

	ret=read(fin, &buffer_u, sizetoread);
}; // end while



close(fout);
return(0);
}; // end if
