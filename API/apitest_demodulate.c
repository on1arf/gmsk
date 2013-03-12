//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x111111): 4800 bps, 1/3 repetition code FEC


// The low-level DSP functions are largly taken from the
// pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


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

#include "gmskmodemapi.h"

int main () {

// API structures
session * sessid=NULL;
msgchain * chain=NULL; msgchain ** pchain; // pointer to pointer of message chain
c2gmsk_params *params;
c2gmsk_msg * msg;

// buffers for data
char txtline[203]; // text line for "printbit". Should normally by up to 192, add 3 for "space + marker" and terminating null, add 8 for headroom for PLL syncing-errors
unsigned char c2buff[8]; // buffer is 8 octets: to accomodate all bitrates (1200 and 2400 bps c2 = 8 octets/frame, 1200 bps c2 = 7 octets/frame)
int16_t pcmbuffer[1920]; // 40 ms audio @ 48000 = 1920 samples = 3840 octets

int fout, fin; // file out and file in 

// some local vars
int framecount, msgcount;
int ret;



// open out file
fout=open("c2out.c2",O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

if (fout < 0) {
	fprintf(stderr,"Error: could not open file for output: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if


// open input file
fin=open("test.raw",O_RDONLY);

if (fin < 0) {
	fprintf(stderr,"Error: could not open file for input: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if

// pchain points to location of chain
pchain=&chain;


// before creating the session, parameters need to be created
params=c2gmsk_params_init();
if (!params) {
	fprintf(stderr,"Error: Could not create parameters structure! \n");
	exit(-1);
}; // end if

// fill in parameters
params->d_disableaudiolevelcheck=1;


// create gmsk session
sessid=c2gmsk_sess_new(params,&ret);

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",ret);
	exit(-1);
}; // end if


//////////////////////////////////////////
// session is created, let's get started

ret=read(fin, pcmbuffer, sizeof(pcmbuffer));

framecount=0;

while (ret == sizeof(pcmbuffer)) {
// msg

int tod; // type of data
int datasize; // datasize

	framecount++;

	ret=c2gmsk_demod(sessid,pcmbuffer,pchain);
	chain=*pchain;

	if (ret != C2GMSK_RET_OK) {
		fprintf(stderr,"Error: c2gmsk_dmod failed: %d (%s) \n",ret,c2gmsk_printstr_ret(ret));
		exit(-1);
	}; // end if


	msgcount=0;

	// loop as long as we find messages in the queue
	while ((msg = c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, chain, &tod, &datasize, NULL))) {
		int ret;
		int data[4];

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
		ret=c2gmsk_msgdecode_numeric (msg, data);

		if (ret > 0) {
			if (ret > 1) {
				printf("type-of-data %d (%s): %d data fields:\n",tod, c2gmsk_printstr_msg(tod),ret);
			} else {
				printf("type-of-data %d (%s): 1 data field:\n", tod, c2gmsk_printstr_msg(tod));
			}; 


			if (ret <= 4) {
				// print all data
				int l;

				// 1st data element
				printf("%d",data[0]);

				// 2nd up to 4th data element
				for (l=2; l<=ret; l++) {
					printf(", %d",data[l-1]);
				}; // end for
				printf("\n");
			} else {
				fprintf(stderr,"Error: number of vars returned by msgdecode - numeric not supported!\n");
			}; // end switch

			continue;
		} else if (ret < 0) {
			fprintf(stderr,"Error: message-decode numeric fails on sanity check!\n");
			continue;
		}; // end else - if


		// codec2?
		ret=c2gmsk_msgdecode_c2(msg, c2buff);

		if (ret > 0) {
			// codec2, check version

			if (ret != C2GMSK_CODEC2_1400) {
				fprintf(stderr,"Error: unsupported codec2 bitrate %d\n",ret);
				continue;
			};

			printf("Writing codec2 frame \n");
			// write data in file
			ret=write(fout,c2buff,C2GMSK_CODEC2_FRAMESIZE_1400);

			if (ret < 0) {
				// write should return number of octets written
				fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
			}; // end if

			continue;
		} else if (ret < 0) {
			fprintf(stderr,"Error: message-decode codec2 fails on sanity check!\n");
			continue;
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

	ret=read(fin, pcmbuffer, sizeof(pcmbuffer));
}; // end while



close(fout);
return(0);
}; // end if
